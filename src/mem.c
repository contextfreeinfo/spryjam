#include <string.h>
#include "mem.h"

spry_Mem spry_mem;

void spry_initAllMem(void) {
    spry_exMem->settings = (spry_Settings){
        .font = &spry_font8,
    };
    memset(&spry_exMem->textEditor, 0, sizeof(spry_TextEditor));
}
