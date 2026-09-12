#include <string.h>
#include "mem.h"

spry_Mem spry_mem;

void spry_initAllMem(void) {
    memset(&spry_exMem->textEditor, 0, sizeof(spry_ExMem));
    spry_exMem->settings = (spry_Settings){
        .font = &spry_font8,
    };
}
