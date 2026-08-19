#pragma once

#define spry_psramBase 0x11000000
#define spry_psramSize (8 << 20)

#define spry_textSize (32 << 10) // 5.5 ms to move all

// TODO typedef struct spry_Mem

typedef struct spry_ExMem {
    // TODO Unions for different modes?
    // TODO Dev other apps, like sprite editor, within spry?
    uint8_t text[spry_textSize];
    // TODO Built code region.
} spry_ExMem;

#define spry_exMem ((spry_ExMem*)spry_psramBase)
