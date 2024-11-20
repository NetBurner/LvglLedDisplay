/*NB_REVISION*/

/*NB_COPYRIGHT*/

#include <init.h>
#include <nbrtos.h>
#include <system.h>
#include <sim.h>
#include <remoteconsole.h>
#include <fdprintf.h>
#include <stdlib.h>
#include <nbrtos.h>
#include <pins.h>
#include <string.h>
#include <webclient/http_funcs.h>
#include <json_lexer.h>
#include <nbtime.h>
#include <nbstring.h>
#include <config_obj.h> 

#include "HiResDelay.h"
#include "IntervalTimer.h"
#include "Display.h"
#include "core/lv_obj_style_gen.h"
#include "dma.h"

#include <lvgl.h>
#include "display/lv_display.h"
#include "misc/lv_color.h"

#include "misc/lv_types.h"
#include "ui/ui.h"
#include "ui/screens.h"

const char *AppName = "Display";

// Assuming each screen is 64x64
const int numScreens = 2;
const int numColumnsPerScreen = 64;
const int numRowsPerScreen = 64;
#define ISS_INTERVAL 20 // seconds between updates
#define ISS_MAX_POINTS 225 // about 75 minutes of history vs 90 minute orbital period

#define NTP_SERVER_NAME "pool.ntp.org"
NtpClientServlet TheNtpClient;

// Pin Definitions
#define PIN_OE      Pins[12]
#define PIN_STB     Pins[13]
#define PIN_RED0    Pins[41]
#define PIN_GREEN0  Pins[42]
#define PIN_BLUE0   Pins[43]
#define PIN_RED1    Pins[44]
#define PIN_GREEN1  Pins[45]
#define PIN_BLUE1   Pins[46]
#define PIN_CLK     Pins[47]
#define PIN_ADDR_A  Pins[51]
#define PIN_ADDR_B  Pins[50]
#define PIN_ADDR_CA Pins[89]
#define PIN_ADDR_C  Pins[53]
#define PIN_ADDR_D  Pins[90]
#define PIN_ADDR_DA Pins[59]
#define PIN_ADDR_E  Pins[57]

Display display(numScreens, numColumnsPerScreen, numRowsPerScreen);
// Multiplied by 2 because we update 2 columns at a time. 32 rows are a limit of the 5 bit address design
uint32_t oneRowBuffer[numColumnsPerScreen * numScreens * 2] FAST_SYS_VAR;


class ClockSettings : public config_obj
{
public:
    config_chooser timezone{"Time Zone","PST","GMT,EST,CST,MST,PST","US time zone"};
    config_bool    show24hr{false,"24-hour Clock","Display 24-hour time (false for 12-hour)"};
    config_bool    showMap{1,"Map Display","Show ISS Map?"};
    config_bool    showAltitude{1,"Altitude Display","Show ISS Altitude?"};
    config_bool    showTime{1,"Time Display","Show time?"};
    config_bool    showDate{1,"Date Display","Show date?"};
    ConfigEndMarker;   // No new data members below this line

    explicit ClockSettings(const char *name, const char *desc) : config_obj(name, desc){};
    ClockSettings(config_obj &owner, const char *name, const char *desc) : config_obj(owner, name, desc){};
};
static ClockSettings CurConfig(appdata, "ClockSettings", "Display Settings");

const char tzsetFormatString[][120] = {
    {"EST5EDT4,M3.2.0/01:00:00,M11.1.0/02:00:00"},    // eastern
    {"CST6CDT5,M3.2.0/01:00:00,M11.1.0/02:00:00"},    // central
    {"MST7MDT6,M3.2.0/01:00:00,M11.1.0/02:00:00"},    // mountain
    {"PST8PDT7,M3.2.0/01:00:00,M11.1.0/02:00:00"},    // pacific
    {""}
};

void SetMyTZ()
{
    int index=0;
    if(CurConfig.timezone=="GMT")  {return; }
    else if(CurConfig.timezone=="EST") index=1;
    else if(CurConfig.timezone=="CST") index=2;
    else if(CurConfig.timezone=="MST") index=3;
    else if(CurConfig.timezone=="PST") index=4;
    tzsetchar((char*)tzsetFormatString[index-1]);
}

void SetAddress(int address)
{
    PIN_ADDR_A = (address >> 0) & 1;
    PIN_ADDR_B = (address >> 1) & 1;
    PIN_ADDR_C = (address >> 2) & 1;
    PIN_ADDR_D = (address >> 3) & 1;
    PIN_ADDR_E = (address >> 4) & 1;
    asm volatile("dsb");
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
}

void DisplayTask(void *param)
{
    PIN_OE.function(PinIO::PIN_FN_OUT);
    PIN_OE = 1;

    // Initialize color pins
    PIN_RED0.function(PinIO::PIN_FN_OUT);
    PIN_GREEN0.function(PinIO::PIN_FN_OUT);
    PIN_BLUE0.function(PinIO::PIN_FN_OUT);
    PIN_RED1.function(PinIO::PIN_FN_OUT);
    PIN_GREEN1.function(PinIO::PIN_FN_OUT);
    PIN_BLUE1.function(PinIO::PIN_FN_OUT);

    // Initialize address pins
    PIN_ADDR_A.function(PinIO::PIN_FN_OUT);
    PIN_ADDR_B.function(PinIO::PIN_FN_OUT);
    PIN_ADDR_C.function(PinIO::PIN_FN_OUT);
    PIN_ADDR_CA.function(PinIO::PIN_FN_IN);
    PIN_ADDR_D.function(PinIO::PIN_FN_OUT);
    PIN_ADDR_DA.function(PinIO::PIN_FN_IN);
    PIN_ADDR_E.function(PinIO::PIN_FN_OUT);

    // Initialize control pins
    PIN_CLK.function(PinIO::PIN_FN_OUT);
    PIN_STB.function(PinIO::PIN_FN_OUT);
    PIN_OE.function(PinIO::PIN_FN_OUT);
    PIN_OE = 1;

    // Set initial address pins to 0
    PIN_ADDR_A = 0;
    PIN_ADDR_B = 0;
    PIN_ADDR_C = 0;
    PIN_ADDR_D = 0;
    PIN_ADDR_E = 0;

    volatile GPIO_Type *gpio = (GPIO_Type *)GPIO1_BASE;
    uint32_t *gpioDataRegister = (uint32_t *)&(gpio->DR);

    DMA_Init();
    OS_SEM displaySemaphore;
    IntervalOSSem(&displaySemaphore, 32 * 60 * 8);
    const int kColorCycle = 8;

    while (true)
    {
        for (int colorIndex = 0; colorIndex < kColorCycle; ++colorIndex)
        {
            for (int rowIndex = 0; rowIndex < 32; ++rowIndex)
            {
                int grayCode = Display::kGrayCodeSequence[rowIndex];

                // Copy the rowData to oneRowBuffer for DMA transfer
                memcpy(oneRowBuffer, display.GetRowData(colorIndex, grayCode), display.totalCols * 2 * sizeof(uint32_t));

                size_t dataSize = display.totalCols * 2 * sizeof(uint32_t);
                PIN_CLK = 0;
                PIN_STB = 0;
                asm volatile("dsb");
                asm volatile("nop");
                asm volatile("nop");
                DMA_MemoryStream(gpioDataRegister, oneRowBuffer, dataSize);
                PIN_CLK.clr();
                PIN_OE = 1;
                SetAddress(grayCode);
                asm volatile("dsb");
                PIN_STB = 1;
                asm volatile("dsb");
                PIN_STB = 0;
                asm volatile("dsb");
                PIN_OE = 0;

                displaySemaphore.Pend(0);
                asm volatile("dsb");
                asm volatile("nop");
                asm volatile("nop");
                asm volatile("nop");
            }
        }
    }
}

void msCounterISR()
{
    lv_tick_inc(1);
}

void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, unsigned char *px_map)
{
    int32_t x_start = area->x1;
    int32_t y_start = area->y1;
    int32_t x_end = area->x2;
    int32_t y_end = area->y2;

    for (int32_t y = y_start; y <= y_end; y++)
    {
        for (int32_t x = x_start; x <= x_end; x++)
        {
            int red = *px_map++;     // Red (8 bits)
            int green = *px_map++;   // Green
            int blue = *px_map++;    // Blue

            uint32_t color = (red << 16) | (green << 8) | blue;

            display.SetPixel(x, y, color);
        }
    }

    // Inform LVGL that flushing is done
    lv_display_flush_ready(disp);
}


void lvgl_display_init()
{
    lv_display_t *disp = lv_display_create(display.totalCols, display.totalRows);

    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB888);

    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    // Set up the display buffers
    // Define the buffer size in bytes (e.g., for a partial buffer)
    const size_t buf_size_bytes = (display.totalCols * 10) * sizeof(lv_color32_t); // Buffer for 10 rows

    // Allocate the buffers
    lv_color32_t *buf1 = new lv_color32_t[display.totalCols * 10];

    lv_display_set_buffers(disp, buf1, NULL, buf_size_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void LVGLTask(void *param)
{
    while (true)
    {
        lv_timer_handler();
        OSTimeDly(1);
    }
}


void UserMain(void *pd)
{
    DCDC->REG3 = (DCDC->REG3 & (~DCDC_REG3_TRG_MASK)) | DCDC_REG3_TRG(0x13); // dan's overclocked core
    // SetOSProtWindow(((uint32_t)&OSTCBTbl[3])&~((1UL<<5)-1), 5); // dan's OS_Prot changes/trap

    init();
    EnableSystemDiagnostics();
    StartHttp();
    WaitForActiveNetwork(TICKS_PER_SECOND * 5);

    EnableRemoteConsole();

    iprintf("Web Application: %s\r\nNNDK Revision: %s\r\n", AppName, GetReleaseTag());

    iprintf("Starting LVGL\r\n");
    lv_init();
    lvgl_display_init();
    
    // Create a timer to increment the LVGL tick
    int timerNumer = IntervalInterruptCallback(&msCounterISR, 1000);
    if (timerNumer < 0)
    {
        iprintf("Error creating timer: %d\r\n", timerNumer);
    }
    // Create a high priority task to service LVGL
    OSSimpleTaskCreatewName(LVGLTask, 21, "LVGLTask");

    // Create a task that updates the display
    OSSimpleTaskCreatewName(DisplayTask, 20, "Display");

    DelayObject delay;

    ui_init();

    lv_obj_t *datelabel = objects.datelabel;
    lv_obj_t *timelabel = objects.timelabel;
    lv_obj_t *altitudearc = objects.altitudearc;
    lv_obj_t *altitudelabel = objects.altitudelabel;
    lv_obj_t *chart = objects.xychart;
    lv_chart_set_type(chart, LV_CHART_TYPE_SCATTER);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -90, 90); // latitude extents
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, -180, 180); // longitude extents
    lv_chart_set_point_count(chart, ISS_MAX_POINTS);
    lv_obj_set_style_size(chart, 2, 2, LV_PART_INDICATOR); // chart dots

    lv_chart_series_t * ser1 = lv_chart_add_series(chart, lv_color_hex(0x660000), LV_CHART_AXIS_PRIMARY_Y);
    bool isFirstPoint = true;
    float lastAlt = 0;
    int lastLongitude = 0;

    time_t timeVal;
    struct tm timeStruct;

    while (1)
    {
        if (TheNtpClient.TimeIsValid() && (CurConfig.showTime || CurConfig.showDate)) {
            // get time
            timeVal = time(NULL);
            SetMyTZ();
            if(CurConfig.timezone == "GMT")
                gmtime_r((const time_t *) &timeVal, &timeStruct);
            else
                localtime_r((const time_t *) &timeVal, &timeStruct);
            if (CurConfig.showTime) {
                NBString timeStr;
                // process time
                if (CurConfig.show24hr)
                {
                    timeStr.sprintf("%02d:%02d:%02d",timeStruct.tm_hour,timeStruct.tm_min,timeStruct.tm_sec);
                } else {
                    char ampm = 'p';
                    if (timeStruct.tm_hour>12) timeStruct.tm_hour-=12;
                    else if (timeStruct.tm_hour<1) timeStruct.tm_hour=12;
                    else ampm = 'a';
                    timeStr.sprintf("%d:%02d:%02d%c",timeStruct.tm_hour,timeStruct.tm_min,timeStruct.tm_sec,ampm);
                }
                // display time
                lv_label_set_text(timelabel,timeStr.c_str());
            } else {
                lv_label_set_text(timelabel,NULL);
            }
            if (CurConfig.showDate) {
                NBString dateStr;
                dateStr.sprintf("%d-%02d-%02d",timeStruct.tm_year+1900,timeStruct.tm_mon+1,timeStruct.tm_mday);
                // display date
                lv_label_set_text(datelabel,dateStr.c_str());
            } else {
                lv_label_set_text(datelabel,NULL);
            }
        } else {
            lv_label_set_text(timelabel,NULL);
            lv_label_set_text(datelabel,NULL);
        }

        // Check ISS position every 60 sec to avoid API overload
        if ( (isFirstPoint || Secs % ISS_INTERVAL == 0) && (CurConfig.showAltitude || CurConfig.showMap) ) {
            ParsedJsonDataSet json;
            bool result = DoGet("https://api.wheretheiss.at/v1/satellites/25544", json);

            if (
                result &&
                json("latitude").IsNumber() &&
                json("longitude").IsNumber() &&
                json("altitude").IsNumber()
            ) {
                int latitude = (int)json("latitude");
                int longitude = (int)json("longitude");
                float alt = (float)json("altitude");

                printf("\n%lu got lat %d long %d alt %02f", Secs, latitude, longitude, alt);

                if (CurConfig.showAltitude) {
                    // process the altitude
                    NBString altT;
                    altT.sprintf("%.02f", alt);
                    if (lastAlt != 0 && alt>lastAlt) {
                        altT.Append("+",1);
                    } else if (lastAlt != 0 && alt<lastAlt) {
                        altT.Append("-",1);
                    }
                    lastAlt = alt;

                    // populate the altitude indicator
                    lv_arc_set_value(altitudearc, (int)alt);
                    lv_label_set_text(altitudelabel, altT.c_str());

                    lv_obj_remove_flag(altitudearc, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_remove_flag(altitudelabel, LV_OBJ_FLAG_HIDDEN);
                } else {
                    lv_obj_add_flag(altitudearc, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(altitudelabel, LV_OBJ_FLAG_HIDDEN);
                }

                if (CurConfig.showMap) {
                    // process the coordinates
                    if (isFirstPoint) {
                        // start with two points to draw the initial line
                        lv_chart_set_next_value2(chart, ser1, longitude-2, latitude);
                        isFirstPoint = false;
                    }
                    if (lastLongitude != 0 && longitude < lastLongitude) {
                        // insert a blank point to break the line across the international date line
                        lv_chart_set_next_value(chart, ser1, LV_CHART_POINT_NONE);
                    }
                    lastLongitude = longitude;

                    // populate the map chart
                    lv_chart_set_next_value2(chart, ser1, longitude, latitude);

                    lv_obj_remove_flag(chart, LV_OBJ_FLAG_HIDDEN);
                } else {
                    lv_obj_add_flag(chart, LV_OBJ_FLAG_HIDDEN);
                }
            } else {
                printf("\nError getting ISS data from https://wheretheiss.at");
            }
        }

        // Loop every second for clock increment
        OSTimeDly(TICKS_PER_SECOND);
    }
}
