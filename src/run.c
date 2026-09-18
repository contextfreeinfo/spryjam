#include <stdio.h>
#include <string.h>
#include "run.h"
#include "mem.h"

spry_RunState sramRunState;

// Make these global to reduce stack usage.
static rio_Engine engine;
static rio_Parser parser;

int spry_run(const char* text, size_t textSize) {
    spry_RunState* runState = &sramRunState;
    // spry_RunState* runState = &spry_exMem->runState;
    printf("runState %p size %zu\n", runState, sizeof(spry_RunState));
    engine = (rio_Engine){
        .code = {
            .span = {.size = spry_rio_codeSize, .items = runState->codeBytes},
            // Use up each first word so nil pointers aren't useful.
            .used = rio_ptrSize,
        },
        .data = {
            .span = {.size = spry_rio_dataSize, .items = runState->dataBytes},
            .used = rio_ptrSize,
        },
        .defs = {
            .span = {.size = spry_rio_defsSize, .items = runState->work.defs},
            .used = 1,
        },
    };
    engine.memory = engine.data;
    rio_Buffer_Byte file = {
        // Not actually going to write this.
        .span = { .items = (uint8_t*)text, .size = textSize },
        // Here `used` tracks our read position.
        .used = 0,
    };
    parser = (rio_Parser){
        .engine = &engine,
        .gen = {.code = &engine.code},
        .lexer = {.file = &file},
        .names = {
            .starts = {
                .size = spry_rio_nameStartsSize,
                .items = runState->work.nameStarts,
            },
            .strings = {
                .span = {
                    .size = spry_rio_namesSize,
                    .items = runState->work.namesBytes,
                },
                // This makes index 0 always be an invalid empty name.
                .used = 2,
            },
        },
    };
    // Prefill defs such as `log`.
    // And we know initial defs won't overflow.
    int32_t logName;
    rio_table(&parser.names, (rio_Byte*)"log", &logName);
    rio_pushDef(&engine.defs, (rio_Def){
        .name = logName,
        .constant = true,
        .ptrVal = (intptr_t)rio_runLog,
    });
    rio_Def logDef = engine.defs.span.items[engine.defs.used - 1];
    if (rio_verbosity) {
        printf("log fun %d at %p\n", logDef.name, (void*)logDef.ptrVal);
    }
    // Parse.
    rio_Err err = rio_parse(&parser);
    // Zero out zero space *after* compiling. This costs a few hundred ms.
    // TODO Track exactly how much to zero, which might be more or less?
    memset(runState->zeroBytes, 0, sizeof(runState->zeroBytes));
    if (engine.main) {
        void (*codeMain)(void) = (void (*)(void))engine.main;
        if (rio_verbosity) {
            printf("Wanting to call main at: %p\n", (void*)(intptr_t)codeMain);
        }
        codeMain();
    }
    printf(
        "size %zu, data %zu, zero %zu, work %zu\n",
        sizeof(spry_RunState),
        offsetof(spry_RunState, dataBytes),
        offsetof(spry_RunState, zeroBytes),
        offsetof(spry_RunState, work.namesBytes)
    );
    return err;
}

rio_Err rio_close(rio_File file) {
    rio_Buffer_Byte* bytes = file;
    bytes->used = bytes->span.size;
    return 0;
}

rio_Err rio_read(rio_File file, uint8_t* c) {
    rio_Buffer_Byte* bytes = file;
    if (bytes->used < bytes->span.size) {
        *c = bytes->span.items[bytes->used];
        bytes->used += 1;
        return 0;
    }
    return rio_Err_eof;
}
