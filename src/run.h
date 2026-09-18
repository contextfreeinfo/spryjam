#pragma once

#include <stdlib.h>
#include "rio/src/parse.h"

#define spry_rio_codeSize (64 << 10)
#define spry_rio_dataSize (32 << 10)
#define spry_rio_defsSize (2 << 10)
#define spry_rio_namesSize (8 << 10)
#define spry_rio_typesSize (32 << 10)
#define spry_rio_zeroSize (96 << 10)

// If each name averages 7 bytes, that's 8 with null char, meaning bytes / 4
// still is likely to be half empty.
#define spry_rio_nameStartsSize (spry_rio_namesSize / 4)

typedef struct spry_RunState {
    uint8_t codeBytes[spry_rio_codeSize];
    uint8_t dataBytes[spry_rio_dataSize];
    union {
        // Memory space for zero-init globals, including arrays.
        // We don't need to use this space during compiling.
        uint8_t zeroBytes[spry_rio_zeroSize];
        // So we can reuse the same space for buffers needed by the compiler.
        struct {
            uint8_t namesBytes[spry_rio_namesSize];
            rio_UInt16 nameStarts[spry_rio_nameStartsSize];
            rio_Def defs[spry_rio_defsSize];
            uint8_t types[spry_rio_typesSize];
        } work;
    };
} spry_RunState;

int spry_run(const char* text, size_t textSize);
