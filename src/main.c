#include "pico_hdmi/hstx_data_island_queue.h"
#include "pico_hdmi/hstx_packet.h"
#include "pico_hdmi/video_output.h"

#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "hardware/clocks.h"
#include "hardware/vreg.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
// ============================================================================
// Configuration
// ============================================================================

#ifdef VIDEO_MODE_1280x720
#define FRAME_WIDTH 1280
#define FRAME_HEIGHT 720
#define BOX_SIZE 64
#else
#define FRAME_WIDTH 640
#define FRAME_HEIGHT 480
#define BOX_SIZE 32
#endif
#define BG_COLOR 0x0010  // Dark blue (RGB565)
#define BOX_COLOR 0xFFE0 // Yellow (RGB565)

// Audio configuration
#define AUDIO_SAMPLE_RATE 48000
#define TONE_AMPLITUDE 6000

// ============================================================================
// Animation State
// ============================================================================

static volatile int box_x = 50, box_y = 50;
#ifdef VIDEO_MODE_1280x720
static int box_dx = 4, box_dy = 2;
#else
static int box_dx = 2, box_dy = 1;
#endif

// ============================================================================
// Audio State - Für Elise
// ============================================================================

#define SINE_TABLE_SIZE 256
static int16_t sine_table[SINE_TABLE_SIZE];
static uint32_t audio_phase = 0;
static uint32_t phase_increment = 0;
static int audio_frame_counter = 0;

// Use Korobeiniki for the demo (Für Elise kept for reference)

static int current_melody_length = KOROBEINIKI_LENGTH;

static int melody_index = 0;
static int note_frames_remaining = 0;

static void init_sine_table(void)
{
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        float angle = (float)i * 2.0F * 3.14159265F / SINE_TABLE_SIZE;
        sine_table[i] = (int16_t)(sinf(angle) * TONE_AMPLITUDE);
    }
}

static void advance_melody(void)
{
    if (--note_frames_remaining <= 0) {
        melody_index = (melody_index + 1) % current_melody_length;

        note_frames_remaining = current_melody[melody_index].duration;
        uint16_t freq = current_melody[melody_index].freq;
        if (freq > 0) {
            phase_increment = (uint32_t)(((uint64_t)freq << 32) / AUDIO_SAMPLE_RATE);
        } else {
            phase_increment = 0; // Rest
        }
    }
}

static inline int16_t get_sine_sample(void)
{
    if (phase_increment == 0)
        return 0; // Rest
    int16_t s = sine_table[(audio_phase >> 24) & 0xFF];
    audio_phase += phase_increment;
    return s;
}

static void generate_audio(void)
{
    // Keep the audio queue fed
    while (hstx_di_queue_get_level() < 200) {
        audio_sample_t samples[4];
        for (int i = 0; i < 4; i++) {
            int16_t s = get_sine_sample();
            samples[i].left = s;
            samples[i].right = s;
        }

        hstx_packet_t packet;
        audio_frame_counter = hstx_packet_set_audio_samples(&packet, samples, 4, audio_frame_counter);

        hstx_data_island_t island;
        hstx_encode_data_island(&island, &packet, false, DI_HSYNC_ACTIVE);
        hstx_di_queue_push(&island);
    }
}

// ============================================================================
// Scanline Callback (runs on Core 1)
// ============================================================================

#define spry_bufferWidth 320
#define spry_bufferHeight 180

uint8_t spry_buffer[spry_bufferHeight][spry_bufferWidth] = {0};

static float lineCompute = 0;
static float linePeriod = 0;

#define spry_timeLines 0

static void __scratch_x("") scanline_callback(
    uint32_t v_scanline, uint32_t active_line, uint32_t *dst
) {
    (void)v_scanline;
#if spry_timeLines
    // Line period.
    static absolute_time_t lastStart = 0;
    if (!lastStart) {
        lastStart = get_absolute_time();
    }
    static absolute_time_t start;
    float retention = 0.9;
    start = get_absolute_time();
    float currentPeriod = absolute_time_diff_us(lastStart, start);
    lastStart = start;
    linePeriod = retention * linePeriod + (1.0 - retention) * currentPeriod;
#endif
    // Scale vertical and check bounds.
    active_line /= 4;
    if (active_line < 0 || active_line >= spry_bufferHeight) {
        // TODO Does this ever happen?
        return;
    }
    // Scale and translate current scan line.
    // TODO Save each translation for 4 lines?
    uint8_t* line = spry_buffer[active_line];
    for (int j = 0; j < spry_bufferWidth; j += 1) {
        // Convert rgb332 to rgb565.
        uint8_t rgb332 = line[j];
        uint8_t r3 = (rgb332 >> 5) & 0x07;
        uint8_t g3 = (rgb332 >> 2) & 0x07;
        uint8_t b2 = rgb332 & 0x03;
        uint16_t r5 = (r3 << 2) | (r3 >> 1);
        uint16_t g6 = (g3 << 3) | g3;
        uint16_t b5 = (b2 << 3) | (b2 << 1) | (b2 >> 1);
        uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;
        // Repeat for two pixels, then repeat for 4 total.
        uint32_t rgb565Rep = (rgb565 << 16) | rgb565;
        *(dst++) = rgb565Rep;
        *(dst++) = rgb565Rep;
    }
#if spry_timeLines
    // Compute time.
    absolute_time_t end = get_absolute_time();
    float currentCompute = absolute_time_diff_us(start, end);
    lineCompute = retention * lineCompute + (1.0 - retention) * currentCompute;
#endif
}

// ============================================================================
// Main (Core 0)
// ============================================================================

static void update_box(void)
{
    int x = box_x + box_dx;
    int y = box_y + box_dy;

    if (x <= 0 || x + BOX_SIZE >= FRAME_WIDTH) {
        box_dx = -box_dx;
        x = box_x + box_dx;
    }
    if (y <= 0 || y + BOX_SIZE >= FRAME_HEIGHT) {
        box_dy = -box_dy;
        y = box_y + box_dy;
    }

    box_x = x;
    box_y = y;
}

static void __attribute__((optimize("O0"))) spini(int max, int step) {
    for (int i = 0; i < max; i += step) {
        // pass
    }
}

extern char __data_start__[];
extern char __data_end__[];
extern char __bss_start__[];
extern char __bss_end__[];
extern char __ram_text_start__[];
extern char __ram_text_end__[];
extern char __end__[];
extern char __HeapLimit[];
extern char __StackLimit[];
extern char __StackTop[];

int main(void)
{
#ifdef VIDEO_MODE_1280x720
    // 720p60: 372 MHz at 1.3V. Closest achievable to 371.25 MHz with 12 MHz XOSC
    // (0.2% high -> 74.4 MHz pixel clock, within HDMI tolerance for 720p60).
    vreg_set_voltage(VREG_VOLTAGE_1_30);
    sleep_ms(10);
    set_sys_clock_khz(372000, true);
#else
    // 480p60: set system clock to 126 MHz for HSTX timing (25.2 MHz pixel clock).
    set_sys_clock_khz(126000, true);
#endif

    stdio_init_all();

    // Initialize LED for heartbeat
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    sleep_ms(1000);

#ifndef BOUNCING_BOX_DVI_ONLY
    // Initialize audio
    init_sine_table();
    note_frames_remaining = current_melody[0].duration;
    phase_increment = (uint32_t)(((uint64_t)current_melody[0].freq << 32) / AUDIO_SAMPLE_RATE);
#endif

    // Initialize HDMI output
    hstx_di_queue_init();
    video_output_init(FRAME_WIDTH, FRAME_HEIGHT);
#ifdef BOUNCING_BOX_DVI_ONLY
    video_output_set_dvi_mode(true);
#endif

    // Register scanline callback
    video_output_set_scanline_callback(scanline_callback);

#ifndef BOUNCING_BOX_DVI_ONLY
    // Pre-fill audio buffer
    generate_audio();
#endif

    // Launch Core 1 for HSTX output
    multicore_launch_core1(video_output_core1_run);
    sleep_ms(100);

    // Main loop - animation + audio
    uint32_t last_frame = 0;
    bool led_state = false;
    int blinkCount = 0;

    for (int i = 0; i < spry_bufferHeight; i += 1) {
        for (int j = 0; j < spry_bufferWidth; j += 1) {
            spry_buffer[i][j] = i == j ? 0xb6 : 0x49;
        }
    }

    while (1) {
#ifndef BOUNCING_BOX_DVI_ONLY
        // Keep audio buffer fed
        generate_audio();
#endif

        while (video_frame_count == last_frame) {
#ifndef BOUNCING_BOX_DVI_ONLY
            generate_audio();
#endif
            tight_loop_contents();
        }
        last_frame = video_frame_count;

        // Update animation
        update_box();

#ifndef BOUNCING_BOX_DVI_ONLY
        // Advance melody (one note step per frame)
        advance_melody();
#endif

        // LED heartbeat
        if ((video_frame_count % 30) == 0) {
            led_state = !led_state;
            gpio_put(PICO_DEFAULT_LED_PIN, led_state);
            if (led_state) {
                uint32_t ramStart = 0x20000000;
                uint32_t static_ram_used = (uint32_t)__end__ - ramStart;
                uint32_t ramText =
                    (uint32_t)(__ram_text_end__ - __ram_text_start__);
                uint32_t ramData = (uint32_t)(__data_end__ - __data_start__);
                uint32_t ramBss = (uint32_t)(__bss_end__ - __bss_start__);
                printf(
                    "Blink %d at %d: %f / %f, %d vs %d\n",
                    blinkCount,
                    video_frame_count,
                    lineCompute,
                    linePeriod,
                    static_ram_used,
                    ramText + ramData + ramBss
                );
                blinkCount += 1;
            }
        }

        // Input.
        int c = stdio_getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            printf("Key: %d %zu %d\n", c, sizeof(spry_buffer), spry_buffer[c][50]);
            absolute_time_t start = get_absolute_time();
            spini(10000000, 1);
            int64_t diff = absolute_time_diff_us(start, get_absolute_time());
            printf("Time: %f\n", diff * 1e-6);
        }
    }

    return 0;
}
