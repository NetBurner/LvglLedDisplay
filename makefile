#NB_REVISION#

#NB_COPYRIGHT#


NAME    = DisplayApplication

NBINCLUDE += -I"lvgl/src"

CPP_SRC     += \
			src/main.cpp \
			src/Display.cpp \
			src/Screen.cpp \
			src/dma.cpp
C_SRC		+= \
			src/ui/images.c \
			src/ui/screens.c \
			src/ui/styles.c \
			src/ui/ui.c \
			src/ui/ui_image_map.c

XTRALIB		+= $(OBJDIR)/release/liblvgl.a

CREATEDTARGS += obj/htmldata.cpp
CPP_SRC += obj/htmldata.cpp
obj/htmldata.cpp : $(wildcard html/*.*)
	comphtml html -oobj/htmldata.cpp

include $(NNDK_ROOT)/make/boilerplate.mk
