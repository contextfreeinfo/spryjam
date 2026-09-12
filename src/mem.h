#pragma once

#include "font.h"
#include "text.h"
#include "run.h"

#define spry_psramBase 0x11000000
#define spry_psramSize (8 << 20)

#define spry_screenWidth 320
#define spry_screenHeight 240
#define spry_userSpaceSize (256 << 10)

typedef struct spry_Mem {
    uint16_t palette[256];

    uint8_t screen[spry_screenHeight][spry_screenWidth]
        __attribute__((aligned(4)));

    uint8_t userSpace[spry_userSpaceSize]
        __attribute__((aligned(4)));
} spry_Mem;

// TODO Start using this.
extern spry_Mem spry_mem;

// For global settings that don't need fast access.
typedef struct spry_Settings {
    spry_Font* font;
} spry_Settings;

typedef struct spry_ExMem {
    // TODO 1 MB ram disk with settings inside. Also clipboard.
    // Make it a key/value store with per-app ownership and R/W permissions.
    spry_Settings settings;

    spry_RunState runState;

    // TODO Unions for different modes?
    // TODO Dev other apps, like sprite editor, within spry?
    // TODO Text editor needs to be just another app.
    spry_TextEditor textEditor;

    // TODO Built code region.
} spry_ExMem;

#define spry_exMem ((spry_ExMem*)spry_psramBase)

void spry_initAllMem(void);
