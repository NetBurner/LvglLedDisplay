#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <nbrtos.h>
#include <lvgl.h>
#include "Screen.h"

class Display
{
   public:
    static const int kMaxScreens = 2;   // Maximum number of screens supported

    Display(int numScreens, int numColumnsPerScreen, int numRowsPerScreen);
    ~Display();

    int totalCols;
    int totalRows;

    void SetPixel(int col, int row, uint32_t color);
    uint32_t GetPixel(int col, int row) const;
    void Clear();
    void Print();

    void UpdateRowData();
    void UpdateRowData(int col, int row);

    const uint32_t *GetRowData(int colorCycle, int hardwareRow) const;

    static const int kGrayCodeSequence[32];
    static const bool kColorIntensityPattern[8][8];

    static uint32_t RGB(int red, int green, int blue) {
        return (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16);
    };

   private:
    int numScreens;
    Screen screens[kMaxScreens];

    static constexpr int kColorCycle = 8;
    static constexpr int kMaxHardwareRows = 32;
    static constexpr int kMaxColumns = 128;
    uint32_t rowData[kColorCycle][kMaxHardwareRows][kMaxColumns * 2];

    // Helper functions
    void MapToScreen(int &col, int &row, int &screenIndex) const;

};

#endif   // DISPLAY_H
