# LED Matrix Display with LVGL and NetBurner

> See the companion blog post for this project at: https://www.netburner.com/learn/external-libraries-lvgl-led-display/

This guide will describe how to create a project in [NBEclipse](https://www.netburner.com/netburner-software/nbeclipse-ide/) that can build an application for a NetBurner device. The application will rely on LVGL being built as a static library, which is automatically built as part of the same NBEclipse project.

This guide was written for NNDK 3.5.2. It should be applicable for all 3.X NNDK releases. 

[LVGL](https://lvgl.io/) is a small graphics library for driving displays. The main project source will exist in the "src" folder, while LVGL will exist in the "lvgl" folder at the root of the project. When building, the application will first build LVGL based on a custom makefile. Then, it will build the application. In further builds, LVGL will only be re-built if its source or dependencies change.

## Prototyping with SOMRT1061

NetBurner's new SOMRT1061 based on the NXP i.MX RT106x / RT1060 platform (similar to the RT1062) has been designed for maximum power and flexibility in a small power-saving footprint. It comes in a 1-inch surface mount pick-and-place-compatible board or presoldered to a carrier board with 0.1-inch header pins, and the development kit includes the SOM on the carrier board for easy programming and prototyping.

One goal of this Display project is to demonstrate a typical workflow for customers developing a proof-of-concept and minimum viable product, then moving on to low-volume alpha and beta testing with limited manufacturing resources, and then finally to high-volume production. We've included KiCad design files and a schematic so you can see how easy it is to deliver results with NetBurner modules.

## Install

- NBEclipse from the NetBurner NNDK 3.5.2 or higher, if not already
- EEZ Studio from https://www.envox.eu/studio/studio-introduction/
- For EEZ Studio, you may need to install some Python 3 dependencies: `pip3 install pypng lz4`

## Configure Environment

1. Review the makefile that we've added to the `lvgl` subfolder. This makefile will contain the source files that will need to be built to generate the lvgl static library, instead of using lvgl's default cmake routine. This also includes special NetBurner rules to simplify the build. The makefile should reference all the source files necessary to build the project, but not unneeded things like the lvgl `draw/nxp`, `drivers` or `others` folders:
```
# Generated static library file name
NAME := liblvgl

# Source files to build
C_SRC += \
		$(sort $(wildcard src/*.c)) \
			$(sort $(wildcard src/core/*.c)) \
			$(sort $(wildcard src/display/*.c)) \
			$(sort $(wildcard src/draw/*.c)) \
			$(sort $(wildcard src/draw/sw/*.c)) \
			$(sort $(wildcard src/draw/sw/blend/*.c)) \
			$(sort $(wildcard src/font/*.c)) \
			$(sort $(wildcard src/indev/*.c)) \
			$(sort $(wildcard src/layouts/*.c)) \
			$(sort $(wildcard src/layouts/flex/*.c)) \
			$(sort $(wildcard src/layouts/grid/*.c)) \
			$(sort $(wildcard src/libs/barcode/*.c)) \
			$(sort $(wildcard src/libs/bin_decoder/*.c)) \
			$(sort $(wildcard src/libs/bmp/*.c)) \
			$(sort $(wildcard src/libs/ffmpeg/*.c)) \
			$(sort $(wildcard src/libs/freetype/*.c)) \
			$(sort $(wildcard src/libs/fsdrv/*.c)) \
			$(sort $(wildcard src/libs/gif/*.c)) \
			$(sort $(wildcard src/libs/libjpeg_turbo/*.c)) \
			$(sort $(wildcard src/libs/libpng/*.c)) \
			$(sort $(wildcard src/libs/lodepng/*.c)) \
			$(sort $(wildcard src/libs/lz4/*.c)) \
			$(sort $(wildcard src/libs/qrcode/*.c)) \
			$(sort $(wildcard src/libs/rle/*.c)) \
			$(sort $(wildcard src/libs/rlottie/*.c)) \
			$(sort $(wildcard src/libs/thorvg/*.c)) \
			$(sort $(wildcard src/libs/tiny_ttf/*.c)) \
			$(sort $(wildcard src/libs/tjpg/*.c)) \
			$(sort $(wildcard src/misc/*.c)) \
			$(sort $(wildcard src/misc/cache/*.c)) \
			$(sort $(wildcard src/osal/*.c)) \
			$(sort $(wildcard src/stdlib/*.c)) \
			$(sort $(wildcard src/stdlib/builtin/*.c)) \
			$(sort $(wildcard src/themes/*.c)) \
			$(sort $(wildcard src/themes/default/*.c)) \
			$(sort $(wildcard src/themes/mono/*.c)) \
			$(sort $(wildcard src/themes/simple/*.c)) \
			$(sort $(wildcard src/tick/*.c)) \
			$(sort $(wildcard src/widgets/*.c)) \
			$(sort $(wildcard src/widgets/animimage/*.c)) \
			$(sort $(wildcard src/widgets/arc/*.c)) \
			$(sort $(wildcard src/widgets/bar/*.c)) \
			$(sort $(wildcard src/widgets/button/*.c)) \
			$(sort $(wildcard src/widgets/buttonmatrix/*.c)) \
			$(sort $(wildcard src/widgets/calendar/*.c)) \
			$(sort $(wildcard src/widgets/canvas/*.c)) \
			$(sort $(wildcard src/widgets/chart/*.c)) \
			$(sort $(wildcard src/widgets/checkbox/*.c)) \
			$(sort $(wildcard src/widgets/dropdown/*.c)) \
			$(sort $(wildcard src/widgets/image/*.c)) \
			$(sort $(wildcard src/widgets/imagebutton/*.c)) \
			$(sort $(wildcard src/widgets/keyboard/*.c)) \
			$(sort $(wildcard src/widgets/label/*.c)) \
			$(sort $(wildcard src/widgets/led/*.c)) \
			$(sort $(wildcard src/widgets/line/*.c)) \
			$(sort $(wildcard src/widgets/list/*.c)) \
			$(sort $(wildcard src/widgets/lottie/*.c)) \
			$(sort $(wildcard src/widgets/menu/*.c)) \
			$(sort $(wildcard src/widgets/msgbox/*.c)) \
			$(sort $(wildcard src/widgets/objx_templ/*.c)) \
			$(sort $(wildcard src/widgets/property/*.c)) \
			$(sort $(wildcard src/widgets/roller/*.c)) \
			$(sort $(wildcard src/widgets/scale/*.c)) \
			$(sort $(wildcard src/widgets/slider/*.c)) \
			$(sort $(wildcard src/widgets/span/*.c)) \
			$(sort $(wildcard src/widgets/spinbox/*.c)) \
			$(sort $(wildcard src/widgets/spinner/*.c)) \
			$(sort $(wildcard src/widgets/switch/*.c)) \
			$(sort $(wildcard src/widgets/table/*.c)) \
			$(sort $(wildcard src/widgets/tabview/*.c)) \
			$(sort $(wildcard src/widgets/textarea/*.c)) \
			$(sort $(wildcard src/widgets/tileview/*.c)) \
			$(sort $(wildcard src/widgets/win/*.c))


# Generate a static library in the makefile folder
TARGET_TYPE := lib

# Boilerplate.mk contains all rules to build makefiles for NetBurner devices
include $(NNDK_ROOT)/make/boilerplate.mk
```
2. There's also a makefile in the root of the project that includes all C and CPP files in `src` and `src/ui`. If you add any new source files, especially new LVGL images/fonts, add them here.
3. If you change which fonts are being used, especially builtin fonts like Montserrat, edit `lvgl/src/lv_conf.h` to set their constant to 1

### Make (CLI)

Building on the CLI is straightforward, though you do need to build lvgl manually:

```
cd lvgl
make -j -e PLATFORM=SOMRT1061
cd ..
make -j load -e DEVIP=192.168.10.150 -e PLATFORM=SOMRT1061
```

### NBEclipse

1. Create a new NBEclipse project. Use the New Project wizard to generate a NetBurner C++ executable project. In this guide, the project name DisplayApplication will be used. 
2. Copy and paste the src and lvgl folders from the file system directly into the NBEclipse project root. The project layout in the project explorer should now look like:
```
⌄ DisplayApplication
	> Build Targets
	> Includes
	⌄ src
	    > ui
	> html
	⌄ lvgl
		> src
	> overload
	DisplayApplication.eez-project
	makefile.defs
	makefile.targets
	map.png
	README.md
```
4. Add a pre-build step to the project. This step will run the LVGL makefile before building the application binary. Note that the makefile in the root of the project is not used by NBEclipse: it's there for CLI-based workflows.
	1. Right click on project and select `Properties`
	2. Click on `C/C++ Build -> Settings`
	3. Select the tab `Build Steps`
	4. Add the command to run the makefile `make -C ../lvgl`
5. Add the static library and header folders to the project
	1. Right click on project and select `Properties`
	2. Click on `C/C++ Build -> Settings`
	3. Select the tab `Tool Settings`
	4. Under both `GNU C Compiler` and then `GNU C++ Compiler`, select Includes.
		1. Under the `Include Paths (-I)` pane, add the root folder that the static library uses as its base directory when `#include` statements are used in source. For LVGL, we use `"${workspace_loc:/${ProjName}/lvgl/src}"`.
	5. Under `GNU C/C++ Linker`, select `Libraries`
		1. Add `lvgl` to the `Libraries (-l)` pane. You may think to add `liblvgl.a` here, but that would be incorrect. The GNU toolchain always prepends lib and appends the file extension automatically.
		2. Add `../lvgl/obj/release` to the `Library search path (-L)` pane. This is the folder that will contain the `liblvgl.a` static library that is built by the library makefile.

With these changes in place, the application project will now properly build both the static library and the project binary, and appropriately detect and rebuild changes.

## Running

Assuming an Internet connection via an Ethernet cable that can access `https://api.wheretheiss.at/v1/satellites/25544` and get time from `pool.ntp.org`, the display will load the GUI from LVGL and update it with the last 75 known positions of the International Space Station, once per minute. It will also show the current date and time.

You can change the time zone and other settings by browsing to your NetBurner device's Config page, i.e. `http://192.168.10.150:20034`, and clicking the Expand All button followed by your changes and the red Update Record button.

## Develop

- Open the `.eez-project` file in the project root in EEZ Studio.
- Save and Build your EEZ project when done.
- Edit `main.cpp` accordingly if you want to modify GUI elements programmatically: the `objects` and `objects_t` struct is defined by EEZ Studio in `ui/screens.h` and contains any named UI elements. You can see how we make them functional in the body of `UserMain()`.

## EEZ Bugs

Be aware that EEZ Studio has a couple rough edges:

- Setting the chart dot size `lv_obj_set_style_size(chart, 2, 2, LV_PART_INDICATOR)` isn't supported inside EEZ Studio, so we do this in `main.cpp` instead.
- When changing a bitmap or other externally-referenced file, its source may need to be re-selected from the filesystem: these files are imported inside the EEZ project and won't automatically update themselves.
- When setting bitmaps in EEZ Studio, it may erroneously display the same bitmap across all bitmaps in the project. This should be ignorable and is a UI bug. 

## Assembly

Hardware and assembly instructions coming soon, in the meantime the 3D printed `DisplayFrame.stl` will hold two of these LED matrix panels side by side with eight M3x10 screws: https://www.adafruit.com/product/5362