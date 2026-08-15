#include <draw.h>
#include <font.h>
#include <stdio.h>
#include <string.h>

#define rasterSizeX 24
#define rasterSizeY 20

uint8_t rasterBytes[rasterSizeY][rasterSizeX] = {0};

spry_Raster raster = {
    .sizeX = rasterSizeX,
    .sizeY = rasterSizeY,
    .stride = rasterSizeX,
    .bytes = (uint8_t*)rasterBytes,
};

void printRaster(void) {
    for (int i = 0; i < rasterSizeY; i += 1) {
        for (int j = 0; j < rasterSizeX; j += 1) {
            printf(" %02x", rasterBytes[i][j]);
        }
        printf("\n");
    }
}

int main(void) {
    spry_debugDraw = 1;
    printf("rasterBytes: %p\n", rasterBytes);
    const char* text = "Hello!";
    spry_drawText(
        &raster,
        &spry_font7,
        text,
        strlen(text),
        -3,
        -1,
        0xff
    );
    spry_drawText(
        &raster,
        &spry_font8,
        text,
        strlen(text),
        -3,
        7,
        0xff
    );
    spry_drawText(
        &raster,
        &spry_font8,
        text,
        strlen(text),
        -3,
        16,
        0xff
    );
    printRaster();
    return 0;
}
