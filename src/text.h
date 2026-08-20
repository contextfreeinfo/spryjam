#pragma once

// Under 4.3 ms to move all.
// Under 4.2 ms to draw a full screen at 4x7 font size.
// Under 8.5 ms worst case so far. Leaves 8 ms of frame budget.
#define spry_textSize (32 << 10)

typedef struct spry_TextEditor {
    int cursor[2];
    uint8_t text[spry_textSize];
} spry_TextEditor;
