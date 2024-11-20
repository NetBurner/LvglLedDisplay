#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

class Screen
{
   public:
    Screen();
    ~Screen();

    void Initialize(int numColumns, int numRows);

    int cols;
    int rows;

    void SetPixel(int col, int row, uint32_t color);
    uint32_t GetPixel(int col, int row) const;
    void Clear();

    static uint32_t GetColorValueStatic(uint32_t upperPixel, uint32_t lowerPixel, uint32_t intensityLevel);

   private:
    uint32_t pixels[64 * 64];

    static uint32_t GetCoreColor(uint32_t color, uint32_t intensityLevel);
};

#endif  // SCREEN_H
