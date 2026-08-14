#include <stdio.h>
#include "draw.h"

int spry_debugDraw = 0;

static void spry_drawCharLine(
    uint8_t* dst,
    // The following two are actually just 8 bits each.
    uint8_t line,
    uint8_t color
) {
    dst[0] = (line & 0x01) ? color : dst[0];
    dst[1] = (line & 0x02) ? color : dst[1];
    dst[2] = (line & 0x04) ? color : dst[2];
    dst[3] = (line & 0x08) ? color : dst[3];
    dst[4] = (line & 0x10) ? color : dst[4];
    dst[5] = (line & 0x20) ? color : dst[5];
    dst[6] = (line & 0x40) ? color : dst[6];
    dst[7] = (line & 0x80) ? color : dst[7];
    // // printf("line: 0x%02x into %p\n", line, dst);
    // uint32_t* dstStart = dst;
    // // First half.
    // // TODO Something other than always 8-bit wide fonts.
    // uint32_t chunk = *dst;
    // chunk |= (line & 0x80) ? (color << 24) : (chunk & 0xff000000);
    // chunk |= (line & 0x40) ? (color << 16) : (chunk & 0x00ff0000);
    // chunk |= (line & 0x20) ? (color << 8) : (chunk & 0x0000ff00);
    // chunk |= (line & 0x10) ? color : (chunk & 0x000000ff);
    // // printf("chunk: 0x%08x\n", chunk);
    // *dst = chunk;
    // // Second half.
    // chunk = *(++dst);
    // chunk |= (line & 0x08) ? (color << 24) : (chunk & 0xff000000);
    // chunk |= (line & 0x04) ? (color << 16) : (chunk & 0x00ff0000);
    // chunk |= (line & 0x02) ? (color << 8) : (chunk & 0x0000ff00);
    // chunk |= (line & 0x01) ? color : (chunk & 0x000000ff);
    // // printf("chunk: 0x%08x\n", chunk);
    // *dst = chunk;
    // // printf("dst: 0x%08x%08x\n", *dstStart, *dst);
}

void spry_drawChar(
    spry_Raster */*restrict*/ raster,
    const spry_Font */*restrict*/ font,
    char c,
    int x,
    int y,
    uint8_t color
) {
    // if (spry_debugDraw) {
    //     printf("%c %d: %d %d\n", c, c - font->start, x, y);
    // }
    // Prevent memory overflows.
    // TODO Edge draws.
    if (x < 0) return;
    if (x + 8 >= raster->sizeX) return;
    // if (y + font->glyphSizeY >= raster->sizeY) return;
    // Draw.
    int fontStride = font->raster.stride * font->glyphSizeY;
    c -= font->start;
    if (c < 0 || c >= font->raster.sizeY) return;
    int cIndex = c * fontStride;
    uint8_t* dst = raster->bytes + y * raster->stride + x;
    uint8_t* src = font->raster.bytes + cIndex;
    uint8_t* srcEnd = src + fontStride;
    // if (spry_debugDraw) {
    //     printf(
    //         "%c %d: %d %d, %p %p %p\n",
    //         c + font->start, c, x, y, dst, src, srcEnd
    //     );
    // }
    // TODO Adjust offset and size for boundaries before loop.
    int offset = 8 - font->raster.sizeX - font->raster.offsetX;
    while (src < srcEnd) {
        if (y < 0) continue;
        if (y >= raster->sizeY) break;
        // TODO What if byte width > 1?
        uint8_t lines = *src;
        lines >>= offset;
        // TODO Line sizes other than 8 bits.
        // if (spry_debugDraw) {
        //     printf("lines: 0x%08x\n", lines);
        // }
        spry_drawCharLine(dst, lines, color);
        dst += raster->stride;
        // spry_drawCharLine(dst, (lines >> 8) & 0xff, color);
        // dst += raster->stride;
        // spry_drawCharLine(dst, (lines >> 16) & 0xff, color);
        // dst += raster->stride;
        // spry_drawCharLine(dst, lines >> 24, color);
        // dst += raster->stride;
        src += 1;
        y += 1;
    }
}

void spry_drawText(
    spry_Raster */*restrict*/ raster,
    const spry_Font */*restrict*/ font,
    const char */*restrict*/ text,
    int x,
    int y,
    uint8_t color
) {
    // if (spry_debugDraw) {
    //     printf("%s: %d\n", text, raster->stride);
    // }
    for (const char* c = text; *c; c += 1) {
        spry_drawChar(raster, font, *c, x, y, color);
        x += font->stepX;
    }
}
