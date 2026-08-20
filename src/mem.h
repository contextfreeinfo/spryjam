#pragma once

#define spry_psramBase 0x11000000
#define spry_psramSize (8 << 20)

// Under 4.3 ms to move all.
// Under 4.2 ms to draw a full screen at 4x7 font size.
// Under 8.5 ms worst case so far. Leaves 8 ms of frame budget.
#define spry_textSize (32 << 10)

// TODO typedef struct spry_Mem

typedef struct spry_ExMem {
    // TODO Unions for different modes?
    // TODO Dev other apps, like sprite editor, within spry?
    uint8_t text[spry_textSize];
    // TODO Built code region.
} spry_ExMem;

#define spry_exMem ((spry_ExMem*)spry_psramBase)
