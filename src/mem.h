#pragma once

#define spry_psramBase 0x11000000
#define spry_psramSize (8 << 20)

#define spry_textSize (0xFFFF)

typedef struct spry_ExMem {
    uint8_t text[spry_textSize];
} spry_ExMem;

#define spry_exMem ((spry_ExMem*)spry_psramBase)
