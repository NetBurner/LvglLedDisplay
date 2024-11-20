#include "Screen.h"
#include "Display.h"
#include <string.h>

uint32_t Screen::GetCoreColor(uint32_t color, uint32_t intensityLevel)
{
    if (color == 0) return 0;
    uint32_t result = 0;
    uint32_t colorRed = color & 0xFF;
    uint32_t colorGreen = (color >> 8) & 0xFF;
    uint32_t colorBlue = (color >> 16) & 0xFF;
    colorRed /= (256 / 8);
    colorGreen /= (256 / 8);
    colorBlue /= (256 / 8);

    if (Display::kColorIntensityPattern[colorRed][intensityLevel]) result |= 1;
    if (Display::kColorIntensityPattern[colorGreen][intensityLevel]) result |= 2;
    if (Display::kColorIntensityPattern[colorBlue][intensityLevel]) result |= 4;

    return result;
}

uint32_t Screen::GetColorValueStatic(uint32_t upperPixel, uint32_t lowerPixel, uint32_t intensityLevel)
{
    return (GetCoreColor(upperPixel, intensityLevel) | (GetCoreColor(lowerPixel, intensityLevel) << 3)) << 17;
}

Screen::Screen() = default;

Screen::~Screen() = default;

void Screen::Initialize(int numColumns, int numRows)
{
    cols = numColumns;
    rows = numRows;
    Clear();
}

void Screen::SetPixel(int col, int row, uint32_t color)
{
    if (col < 0 || col >= cols || row < 0 || row >= rows)
    {
        return;
    }
    pixels[row * cols + col] = color;
}

uint32_t Screen::GetPixel(int col, int row) const
{
    if (col < 0 || col >= cols || row < 0 || row >= rows)
    {
        return 0;
    }
    return pixels[row * cols + col];
}

void Screen::Clear()
{
    memset(pixels, 0, cols * rows * sizeof(uint32_t));
}
