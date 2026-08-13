#pragma once

#include <stdint.h>

extern int spry_debugDraw;

typedef struct spry_Raster {
    int offsetX;
    int offsetY;
    int sizeX;
    int sizeY;
    int stride;
    uint32_t* bytes;
} spry_Raster;

typedef struct spry_Font {
    int glyphSizeY;
    int start;
    int stepX;
    int stepY;
    // These are interpreted differently for fonts.
    spry_Raster raster;
} spry_Font;

void spry_drawText(
    spry_Raster *restrict raster,
    const spry_Font *restrict font,
    const char *restrict text,
    int x,
    int y,
    uint8_t color
);
