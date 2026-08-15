#include <stdio.h>
#include "draw.h"

int spry_debugDraw = 0;

static void spry_drawCharLine(
    uint8_t* dst,
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
}

static void spry_drawCharLineSlow(
    uint8_t* dst,
    uint8_t count,
    uint8_t line,
    uint8_t color
) {
    dst[0] = (line & 0x01) ? color : dst[0];
    if (count == 1) return;
    dst[1] = (line & 0x02) ? color : dst[1];
    if (count == 2) return;
    dst[2] = (line & 0x04) ? color : dst[2];
    if (count == 3) return;
    dst[3] = (line & 0x08) ? color : dst[3];
    if (count == 4) return;
    dst[4] = (line & 0x10) ? color : dst[4];
    if (count == 5) return;
    dst[5] = (line & 0x20) ? color : dst[5];
    if (count == 6) return;
    dst[6] = (line & 0x40) ? color : dst[6];
    if (count == 7) return;
    dst[7] = (line & 0x80) ? color : dst[7];
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
    if (x >= raster->sizeX) return;
    // if (x + 8 >= raster->sizeX) return;
    // if (y + font->glyphSizeY >= raster->sizeY) return;
    // Draw.
    int fontStride = font->raster.stride * font->glyphSizeY;
    c -= font->start;
    if (c < 0 || c >= font->raster.sizeY) return;
    int cIndex = c * fontStride;
    int underflowX = x < 0 ? -x : 0;
    // int overflowX = (x > raster->sizeX ? x - raster->sizeX : 0) + underflowX;
    uint8_t* dst = raster->bytes + y * raster->stride + x + underflowX;
    uint8_t* src = font->raster.bytes + cIndex;
    uint8_t* srcEnd = src + fontStride;
    // if (spry_debugDraw) {
    //     printf(
    //         "%c %d: %d %d, %p %p %p\n",
    //         c + font->start, c, x, y, dst, src, srcEnd
    //     );
    //     spry_debugDraw -= 1;
    // }
    // TODO Custom loop if no chance of overflow or underflow?
    int offset = 8 - font->raster.sizeX - font->raster.offsetX;
    for (; src < srcEnd; dst += raster->stride, ++src, ++y) {
        if (y < 0) continue;
        if (y >= raster->sizeY) break;
        int maxX = x + 8;
        int overflowX = maxX > raster->sizeX ? maxX - raster->sizeX : 0;
        // TODO What if byte width > 1?
        uint8_t lines = *src;
        lines >>= offset + underflowX;
        // TODO Line sizes other than 8 bits.
        // if (spry_debugDraw) {
        //     printf("lines: 0x%08x\n", lines);
        // }
        int count = 8 - (overflowX + underflowX);
        count = count > 0 ? count : 0;
        if (underflowX || overflowX) {
            spry_drawCharLineSlow(dst, count, lines, color);
        } else {
            spry_drawCharLine(dst, lines, color);
        }
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
    //     printf("%s: %p\n", text, font->raster.bytes);
    // }
    for (const char* c = text; *c; c += 1) {
        spry_drawChar(raster, font, *c, x, y, color);
        x += font->stepX;
    }
}
