#include "Display.h"
#include <string.h>
#include <stdio.h>

const int Display::kGrayCodeSequence[32] = {0,  1,  3,  2,  6,  7,  5,  4,  12, 13, 15, 14, 10, 11, 9,  8,
                                            24, 25, 27, 26, 30, 31, 29, 28, 20, 21, 23, 22, 18, 19, 17, 16};

const bool Display::kColorIntensityPattern[8][8] = {
    {false, false, false, false, false, false, false, false}, {false, false, false, true, false, false, false, false},
    {false, false, true, false, false, true, false, false},   {false, true, false, true, false, true, false, false},
    {false, true, false, true, false, true, false, true},     {true, true, false, true, true, false, true, true},
    {true, false, true, false, true, false, true, true},      {true, true, true, true, true, true, true, true}};


Display::Display(int numScreens_, int numColumnsPerScreen, int numRowsPerScreen) : numScreens(numScreens_)
{
    if (numScreens > kMaxScreens) numScreens = kMaxScreens;

    totalCols = numColumnsPerScreen * numScreens;
    totalRows = numRowsPerScreen;

    // Initialize screens
    for (int i = 0; i < numScreens; ++i)
    {
        screens[i].Initialize(numColumnsPerScreen, numRowsPerScreen);
    }

    Clear();
}

Display::~Display() = default;

void Display::SetPixel(int col, int row, uint32_t color)
{
    if (col < 0 || col >= totalCols || row < 0 || row >= totalRows)
    {
        return;
    }

    int screenIndex = 0;
    int localCol = col;
    int localRow = row;

    MapToScreen(localCol, localRow, screenIndex);

    screens[screenIndex].SetPixel(localCol, localRow, color);

    UpdateRowData(col, row);

    // int otherRow = row ^ 32;
    // if (otherRow >= 0 && otherRow < totalRows)
    // {
    //     UpdateRowData(col, otherRow);
    // }
}

uint32_t Display::GetPixel(int col, int row) const
{
    if (col < 0 || col >= totalCols || row < 0 || row >= totalRows)
    {
        return 0;
    }

    int screenIndex = 0;
    int localCol = col;
    int localRow = row;

    MapToScreen(localCol, localRow, screenIndex);

    return screens[screenIndex].GetPixel(localCol, localRow);
}

void Display::Clear()
{
    for (int i = 0; i < numScreens; ++i)
    {
        screens[i].Clear();
    }

    memset(rowData, 0, sizeof(rowData));

    UpdateRowData();
}

void Display::Print()
{
    for (int y = 0; y < totalRows; y++)
    {
        for (int x = 0; x < totalCols; x++)
        {
            printf("%c", GetPixel(x, y) ? '#' : ' ');
        }
        printf("\r\n");
    }
}

void Display::UpdateRowData()
{
    for (int hardwareRow = 0; hardwareRow < kMaxHardwareRows; ++hardwareRow)
    {
        for (int col = 0; col < totalCols; ++col)
        {
            UpdateRowData(col, hardwareRow);
        }
    }
}

void Display::UpdateRowData(int col, int row)
{
    int hardwareRow = row & 0x1F;
    int xPos = col * 2;

    uint32_t upperPixel = GetPixel(col, hardwareRow);
    uint32_t lowerPixel = GetPixel(col, hardwareRow + 32);

    for (uint32_t colorCycle = 0; colorCycle < kColorCycle; ++colorCycle)
    {
        uint32_t colorValue = Screen::GetColorValueStatic(upperPixel, lowerPixel, colorCycle);
        rowData[colorCycle][hardwareRow][xPos] = colorValue;
        rowData[colorCycle][hardwareRow][xPos + 1] = colorValue | (64 << 17);
    }
}

const uint32_t *Display::GetRowData(int colorCycle, int hardwareRow) const
{
    return rowData[colorCycle][hardwareRow];
}

void Display::MapToScreen(int &col, int &row, int &screenIndex) const
{
    int colsPerScreen = totalCols / numScreens;
    screenIndex = col / colsPerScreen;
    col = col % colsPerScreen;
}
