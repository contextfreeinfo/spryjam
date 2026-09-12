#pragma once

#include <stdlib.h>
#include "rio/src/parse.h"

typedef struct spry_RunState {
    uint8_t codeBytes[rio_codeSize];
    uint8_t dataBytes[rio_dataSize];
} spry_RunState;

int spry_run(const char* text, size_t textSize);
