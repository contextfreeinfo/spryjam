#include <no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src/include/hw_config.h>
#include <no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src/include/f_util.h>
#include "sdcard.h"

static spi_t spi = {
    .hw_inst = spi0,
    .miso_gpio = 36,
    .mosi_gpio = 35,
    .sck_gpio = 34,
    .baud_rate = 12500000, // 12.5 MHz
};

static sd_spi_if_t spi_if = {
    .spi = &spi,
    .ss_gpio = 39,
};

static sd_card_t sdcard = {
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if,
    .use_card_detect = true,
    .card_detect_gpio = 34,
    .card_detect_use_pull = true,
    .card_detect_pull_hi = true,
    // TODO Also?: bool (*sd_test_com)(sd_card_t *sd_card_p);
};

size_t sd_get_num() {
    return 1;
}

sd_card_t *sd_get_by_num(size_t num) {
    return num == 0 ? &sdcard : NULL;
}

void spry_listDir(const char* path) {
    printf("Listing: %s\n", path);
    DIR dir;
    FRESULT fr = f_opendir(&dir, path);
    if (fr) {
        printf("Failed to open dir (%d): %s\n", fr, FRESULT_str(fr));
        return;
    }
    FILINFO info;
    while (!f_readdir(&dir, &info) && info.fname[0]) {
        if (info.fattrib & AM_DIR) {
            printf("%s/\n", info.fname);
        } else {
            printf("%s (%lu bytes)\n", info.fname, info.fsize);
        }
    }
    f_closedir(&dir);
}

void spry_sdListRoot(void) {
    // Mount.
    FATFS fs;
    FRESULT fr = f_mount(&fs, "0:", 1);
    if (fr) {
        printf("Failed to mount SD card (%d) %s\n", fr, FRESULT_str(fr));
        return;
    }
    printf("Mounted sdcard.\n");
    // List root.
    spry_listDir("/");
    spry_listDir("/CARTS");
    spry_listDir("/surely-missing");
    // Unmount.
    fr = f_unmount("0:");
    if (fr) {
        printf("Failed to unmount SD card (%d): %s\n", fr, FRESULT_str(fr));
        return;
    }
    printf("Umounted sdcard.\n");
}
