#pragma once

// Under 4.3 ms to move 32k.
// But much faster in sram. Like under 1.7 ms for 64k.
// Under 4.2 ms to draw a full screen at 4x7 font size.
// Under 3.6 ms for 5x8 because fewer.
// Doesn't much change on draw time for sram.
// Under 13.6 ms for 4x 5x8 to simulate 640x480.
// Under 15.7 ms for 4x 4x7, and things fall apart.
// If we do 80x40 with 7x11 font for 649x480, that might work.
// Under 8.5 ms worst case so far. Leaves 8 ms of frame budget.
#define spry_textSize (64 << 10)

typedef struct spry_TextEditor {
    int cursor[2];
    uint8_t text[spry_textSize];
} spry_TextEditor;
