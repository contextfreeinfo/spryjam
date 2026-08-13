#include <stdio.h>
#include "draw.h"

int spry_debugDraw = 0;

static void spry_drawCharLine(
    uint32_t* dst,
    // The following two are actually just 8 bits each.
    uint32_t line,
    uint32_t color
) {
    // First half.
    // TODO Something other than always 8-bit wide fonts.
    uint32_t chunk = *dst;
    chunk |= (line & 0x80) ? (color << 24) : (chunk & 0xff000000);
    chunk |= (line & 0x40) ? (color << 16) : (chunk & 0x00ff0000);
    chunk |= (line & 0x20) ? (color << 8) : (chunk & 0x0000ff00);
    chunk |= (line & 0x10) ? color : (chunk & 0x000000ff);
    *dst = chunk;
    // Second half.
    chunk = *(++dst);
    chunk |= (line & 0x08) ? (color << 24) : (chunk & 0xff000000);
    chunk |= (line & 0x04) ? (color << 16) : (chunk & 0x00ff0000);
    chunk |= (line & 0x02) ? (color << 8) : (chunk & 0x0000ff00);
    chunk |= (line & 0x01) ? color : (chunk & 0x000000ff);
    *dst = chunk;
}

void spry_drawChar(
    spry_Raster *restrict raster,
    const spry_Font *restrict font,
    char c,
    int x,
    int y,
    uint8_t color
) {
    int cIndex = (c - font->start) * font->raster.stride;
    if (c < 0 || c >= font->raster.sizeY * font->raster.stride) {
        return;
    }
    uint32_t* dst = raster->bytes + y * raster->stride + x;
    uint32_t* src = font->raster.bytes + cIndex;
    uint32_t* srcEnd = src + font->raster.stride;
    if (spry_debugDraw) {
        printf("%c: %p %p %p\n", c, dst, src, srcEnd);
    }
    return;
    // TODO Adjust offset and size for boundaries before loop.
    while (src < srcEnd) {
        uint32_t lines = *src;
        // TODO Line sizes other than 8 bits.
        spry_drawCharLine(dst, lines & 0xff, color);
        dst += raster->stride;
        spry_drawCharLine(dst, (lines >> 8) & 0xff, color);
        dst += raster->stride;
        spry_drawCharLine(dst, (lines >> 16) & 0xff, color);
        dst += raster->stride;
        spry_drawCharLine(dst, lines >> 24, color);
    }
}

void spry_drawText(
    spry_Raster *restrict raster,
    const spry_Font *restrict font,
    const char *restrict text,
    int x,
    int y,
    uint8_t color
) {
    if (spry_debugDraw) {
        printf("%s: %d\n", text, raster->stride);
    }
    for (const char* c = text; *c; c += 1) {
        spry_drawChar(raster, font, *c, x, y, color);
        x += font->stepX;
    }
}
