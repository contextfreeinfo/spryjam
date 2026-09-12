#include <stdio.h>
#include "run.h"
#include "mem.h"

int spry_run(const char* text, size_t textSize) {
    spry_RunState* runState = &spry_exMem->runState;
    printf("runState %p size %zu\n", runState, sizeof(spry_RunState));
    return 0;
}
