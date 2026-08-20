#include "pico_hdmi/hstx_data_island_queue.h"
#include "pico_hdmi/hstx_packet.h"
#include "pico_hdmi/video_output.h"

#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/vreg.h"

#include "pio_usb.h"
#include "tusb.h"
// #include "usb-host/fwUSBHost.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "draw.h"
#include "font.h"
#include "input.h"
#include "mem.h"
#include "sdcard.h"

#include <wili8jam/src/psram.h>

// ============================================================================
// Configuration
// ============================================================================

// Screen blanks out in this mode. TODO Why???
// #define BOUNCING_BOX_DVI_ONLY

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
#define TONE_AMPLITUDE 30

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

static void init_sine_table(void) {
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

bool spry_mute = true;

static void generate_audio(void)
{
    // Keep the audio queue fed
    while (hstx_di_queue_get_level() < 200) {
        audio_sample_t samples[4];
        for (int i = 0; i < 4; i++) {
            int16_t s = get_sine_sample();
            if (spry_mute) {
                samples[i].left = 0;
                samples[i].right = 0;
            } else {
                samples[i].left = s;
                samples[i].right = s;
            }
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

// Inset by 5% each side because overscan? 216 also scales into 1080.
// But SNES is 224.
// #define spry_bufferWidth 288
// #define spry_bufferHeight 216

#define spry_bufferWidth 320
#define spry_bufferHeight 240

uint8_t spry_buffer[spry_bufferHeight][spry_bufferWidth]
    __attribute__((aligned(4))) = {0};
uint8_t spry_bufferVid[spry_bufferHeight][spry_bufferWidth]
    __attribute__((aligned(4))) = {0};

uint16_t spry_palette[256] = {0};

int spry_bufferDmaChan;

void __not_in_flash_func(spry_vsyncCallback)(void) {
    static bool needed = false;
    // TODO Just run at 30 Hz?
    needed = !needed;
    // if (!needed) return;
    dma_channel_set_read_addr(spry_bufferDmaChan, spry_buffer, false);
    dma_channel_set_write_addr(spry_bufferDmaChan, spry_bufferVid, true);
    // TODO Signal when write finished.
}

void spry_initCopyBuffer(void) {
    spry_bufferDmaChan = dma_claim_unused_channel(true);
    dma_channel_config_t conf =
        dma_channel_get_default_config(spry_bufferDmaChan);
    channel_config_set_transfer_data_size(&conf, DMA_SIZE_32);
    channel_config_set_read_increment(&conf, true);
    channel_config_set_write_increment(&conf, true);
    channel_config_set_dreq(&conf, DREQ_FORCE);
    dma_channel_configure(
        spry_bufferDmaChan,
        &conf,
        spry_bufferVid,
        spry_buffer,
        sizeof(spry_buffer) / 4,
        false
    );
}

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
    int line = active_line / 2;
    line -= (240 - spry_bufferHeight) / 2;
    dst += (320 - spry_bufferWidth) / 2;
    if (line < 0 || line >= spry_bufferHeight) {
        // memset(dst, 0, spry_bufferWidth * 2);
        uint32_t* src = (uint32_t*)spry_bufferVid[0];
        uint32_t* lineEnd = src + (spry_bufferWidth / 4);
        while (src < lineEnd) {
            dst[0] = 0;
            dst[1] = 0;
            dst[2] = 0;
            dst[3] = 0;
            src += 1;
            dst += 4;
        }
        return;
    }
    // Scale and translate current scan line.
    // TODO Save each translation for 4 lines?
    // int i = 0;
    uint32_t* src = (uint32_t*)spry_bufferVid[line];
    uint32_t* lineEnd = src + (spry_bufferWidth / 4);
    while (src < lineEnd) {
        uint32_t bytes = *(src++);
        uint8_t byte0 = bytes & 0xff;
        uint8_t byte1 = (bytes >> 8) & 0xff;
        uint8_t byte2 = (bytes >> 16) & 0xff;
        uint8_t byte3 = bytes >> 24;
        uint16_t color0 = spry_palette[byte0];
        uint16_t color1 = spry_palette[byte1];
        uint16_t color2 = spry_palette[byte2];
        uint16_t color3 = spry_palette[byte3];
        dst[0] = (color0 << 16) | color0;
        dst[1] = (color1 << 16) | color1;
        dst[2] = (color2 << 16) | color2;
        dst[3] = (color3 << 16) | color3;
        dst += 4;
    }
#if spry_timeLines
    // Compute time.
    absolute_time_t end = get_absolute_time();
    float currentCompute = absolute_time_diff_us(start, end);
    lineCompute = retention * lineCompute + (1.0 - retention) * currentCompute;
#endif
}

// Init palette to rgb332 colors.
static void spry_initPalette(void) {
    for (int j = 0; j < 256; j += 1) {
        // Convert rgb332 to rgb565.
        uint8_t rgb332 = j;
        uint8_t r3 = (rgb332 >> 5) & 0x07;
        uint8_t g3 = (rgb332 >> 2) & 0x07;
        uint8_t b2 = rgb332 & 0x03;
        uint16_t r5 = (r3 << 2) | (r3 >> 1);
        uint16_t g6 = (g3 << 3) | g3;
        uint16_t b5 = (b2 << 3) | (b2 << 1) | (b2 >> 1);
        uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;
        spry_palette[j] = rgb565;
    }
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

void spry_prepClock(void) {
#ifdef VIDEO_MODE_1280x720
    // 720p60: 372 MHz at 1.3V. Closest achievable to 371.25 MHz with 12 MHz XOSC
    // (0.2% high -> 74.4 MHz pixel clock, within HDMI tolerance for 720p60).
    vreg_set_voltage(VREG_VOLTAGE_1_30);
    sleep_ms(10);
    set_sys_clock_khz(372000, true);
#else
    // 480p60: set system clock to 126 MHz for HSTX timing (25.2 MHz pixel clock).
    // Except double-clock for speed, and set MODE_HSTX_CLK_DIV=2 in build.
    set_sys_clock_khz(252000, true);
#endif

    // Enable 5V VBUS power to USB-A host port (GPIO 11 controls power switch)
    gpio_init(11);
    gpio_set_dir(11, GPIO_OUT);
    gpio_put(11, 1);

    // Configure PIO-USB host on port 1 (D+ = GPIO1, D- = GPIO2).
    pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
    pio_cfg.pin_dp = 1;
    pio_cfg.tx_ch = 9;
    tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
}

int spry_textIndex = 0;
uint8_t spry_userSram[256 << 10] = {0};

int main(void) {
    spry_prepClock();
    stdio_init_all();
    spry_initInput();

    // Initialize LED for heartbeat
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // Maybe because of clock hz change above vs usb programming?
    // sleep_ms(200);

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

    size_t psramSize = setup_psram();
    memset(spry_exMem->text, 0, sizeof(spry_exMem->text));
    uint8_t* spry_text = spry_exMem->text;

    // Register scanline callback
    spry_initPalette();
    video_output_set_scanline_callback(scanline_callback);

    spry_initCopyBuffer();
    video_output_set_vsync_callback(spry_vsyncCallback);

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
    spry_Raster raster = {
        .sizeX = spry_bufferWidth,
        .sizeY = spry_bufferHeight,
        .stride = spry_bufferWidth,
        .bytes = (uint8_t*)spry_buffer,
    };
    spry_Font* font = &spry_font8;

    if (false) {
        // Test chars for a filled screen.
        char loopC = 'M';
        for (int i = 0, ci = 0; i < 30; i += 1) {
            for (int j = 0; j < 64; j += 1, ci += 1) {
                spry_text[ci] = loopC;
            }
            spry_text[ci++] = '\n';
        }
        loopC += 1;
        if (loopC > 126) {
            loopC = ' ';
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
            input_update();
            tight_loop_contents();
        }
        last_frame = video_frame_count;

#ifndef BOUNCING_BOX_DVI_ONLY
        // Advance melody (one note step per frame)
        advance_melody();
#endif

        int c = input_getchar();
        int modifiers = input_modifiers();
        uint8_t ctrl =
            KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL;
        if (modifiers & ctrl) {
            if (input_key(HID_KEY_MINUS)) {
                font = &spry_font7;
            } else if (input_key(HID_KEY_EQUAL)) {
                font = &spry_font8;
            } else if (input_key(HID_KEY_P)) {
                // This whole mute thing isn't working great.
                // TODO Probably need events even with ctrl.
                spry_mute = !spry_mute;
            }
        } else if (
            (c >= 32 || c == '\n' || c == '\t') &&
            spry_textIndex < spry_textSize - 1
        ) {
            spry_text[spry_textIndex++] = (char)c;
        } else if (c == 8 && spry_textIndex > 0) {
            spry_text[--spry_textIndex] = 0;
        } else if (c != -1) {
            printf("c: %d\n", c);
        }

        // TODO Move to draw function. Call only after framebuffer copy.
        // Animate at 30 fps.
        // TODO Measure drawing time.
        int drawTime = 0;
        if ((video_frame_count % 1) == 0) {
            float offsetX = 0;
            float offsetY = 0;
            if (false) {
                // Test animation for fun.
                float t = video_frame_count * 0.05;
                float mag = 20.0;
                float offsetX = cos(t) * mag;
                float offsetY = sin(t) * mag;
            }
            absolute_time_t start = get_absolute_time();
            int offset = video_frame_count % spry_bufferWidth;
            spry_userSram[0] += 1;
            memset(spry_buffer, 0x49, sizeof(spry_buffer));
            // for (int y = 0; y < spry_bufferHeight; y += 1) {
            //     int x = (y + spry_bufferWidth + offset) % spry_bufferWidth;
            //     spry_buffer[y][x] = 0xb6;
            // }
            // uint8_t* pixel = spry_buffer[0];
            // for (int i = 0; i < spry_bufferHeight; i += 1) {
            //     for (int j = 0; j < spry_bufferWidth; j += 1, pixel += 1) {
            //         int offsetJ =
            //             (j + spry_bufferWidth - offset) % spry_bufferWidth;
            //         *pixel = i == offsetJ ? 0xb6 : 0x49;
            //     }
            // }
            const char* messages[] = {
                "the five boxing wizards jump quickly. 0!1@2#3$4%5^6&7*8(9)+<=>,?",
                "THE FIVE BOXING WIZARDS JUMP QUICKLY. 0!1@2#3$4%5^6&7*8(9)+<=>,?",
            };
            int stepColor = 1;
            int row = 0;
            char* message = spry_text;
            int y = 0;
            while (true) {
                char* lineEnd = strchr(message, '\n');
                bool done = !lineEnd;
                if (done) lineEnd = message + strlen(message);
                int lineLength = lineEnd - message;
                int drawX = offsetX;
                int drawY = y + offsetY;
                spry_drawText(
                    &raster, font, message, lineLength, drawX, drawY, 0xb6
                );
                if (done) break;
                message = lineEnd + 1;
                y += font->stepY;
            }
            // for (int y = 0; y < spry_bufferHeight; y += font->stepY) {
            //     // const char* message = messages[row % 2];
            //     const char* message = spry_text;
            //     int stepX = font->stepX * (strlen(message) + 1);
            //     for (int x = 0; x < spry_bufferWidth; x += stepX) {
            //         uint8_t color = stepColor + video_frame_count / 60;
            //         spry_drawText(&raster, font, message, strlen(message), x, y, color);
            //         stepColor += 1;
            //     }
            //     row += 1;
            // }
            drawTime = absolute_time_diff_us(start, get_absolute_time());
            if (spry_debugDraw > 0) {
                spry_debugDraw -= 1;
            }
        }

        // LED heartbeat
        if ((video_frame_count % 30) == 0) {
            led_state = !led_state;
            gpio_put(PICO_DEFAULT_LED_PIN, led_state);
            if (led_state) {
                uint32_t ramStart = 0x20000000;
                uint32_t static_ram_used = (uint32_t)__end__ - ramStart;
                uint32_t ramText = 0;
                // (uint32_t)(__ram_text_end__ - __ram_text_start__);
                uint32_t ramData = (uint32_t)(__data_end__ - __data_start__);
                uint32_t ramBss = (uint32_t)(__bss_end__ - __bss_start__);
                printf(
                    "Blink %d at %d: %f / %f, %d vs %d, %zu, %d\n",
                    blinkCount,
                    video_frame_count,
                    lineCompute,
                    linePeriod,
                    static_ram_used,
                    ramText + ramData + ramBss,
                    psramSize,
                    drawTime
                );
                blinkCount += 1;
            }
        }

        // Input.
        int ct = stdio_getchar_timeout_us(0);
        if (ct != PICO_ERROR_TIMEOUT) {
            spry_debugDraw = 4;
            printf(
                "Key: %d %zu %p\n",
                ct, sizeof(spry_buffer), spry_buffer
            );
            absolute_time_t start = get_absolute_time();
            switch (ct) {
                case 'f':
                    spry_sdListRoot();
                    break;
                case 'm':
                    memmove(spry_text, spry_text + 1, spry_textSize - 1);
                    if (spry_textIndex) {
                        spry_textIndex -= 1;
                    }
                    break;
                case 's':
                    spini(10000000, 1);
                    break;
            }
            int64_t diff = absolute_time_diff_us(start, get_absolute_time());
            printf("Time: %f\n", diff * 1e-6);
        }
    }

    return 0;
}
