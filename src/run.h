#pragma once

#include <stdlib.h>
#include "rio/src/parse.h"

// If each name averages 7 bytes, that's 8 with null char, meaning bytes / 4
// still is likely to be half empty.
#define rio_nameStartsSize (rio_namesSize / 4)

typedef struct spry_RunState {
    uint8_t codeBytes[rio_codeSize];
    uint8_t dataBytes[rio_dataSize];
    uint8_t namesBytes[rio_namesSize];
    rio_UInt16 nameStarts[rio_nameStartsSize];
    rio_Def defs[rio_defsSize];
} spry_RunState;

int spry_run(const char* text, size_t textSize);
