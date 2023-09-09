TARGET_OS = windows

# DEBUG_LOG_FILE ?= STDERR
# DEBUG_LOG_FILE ?= C:\\\\debug.txt

FST = $(FST_DIR)/js80p.dll
FST_MAIN_SOURCES = src/plugin/fst/dll.cpp
FST_PLATFORM_OBJS = src/plugin/fst/js80p.def

VST3 = $(VST3_DIR)/js80p.vst3
VST3_MAIN_SOURCES = src/plugin/vst3/dll.cpp
VST3_GUI_PLATFORM = kPlatformTypeHWND
VST3_PLATFORM_OBJS = src/plugin/vst3/js80p.def
VST3_PLUGIN_SOURCES = \
    src/plugin/vst3/plugin.cpp \
    src/plugin/vst3/plugin-win32.cpp

DEV_PLATFORM_CLEAN =

GUI_PLAYGROUND = $(BUILD_DIR)/gui-playground-$(SUFFIX).exe
GUI_PLAYGROUND_SOURCES = src/gui/win32-playground.cpp
GUI_TARGET_PLATFORM_HEADERS = src/gui/win32.hpp
GUI_TARGET_PLATFORM_SOURCES = src/gui/win32.cpp

GUI_IMAGES = \
	gui/img/about.bmp \
	gui/img/controllers1.bmp \
	gui/img/controllers2.bmp \
	gui/img/effects.bmp \
	gui/img/envelopes.bmp \
	gui/img/knob_states-controlled.bmp \
	gui/img/knob_states-free.bmp \
	gui/img/knob_states-none.bmp \
	gui/img/lfos.bmp \
	gui/img/synth.bmp \
	gui/img/vst_logo.bmp

OBJ_GUI_EXTRA = $(BUILD_DIR)/js80p-$(SUFFIX).res

$(OBJ_GUI_EXTRA): src/gui/gui.rc $(GUI_IMAGES) | $(BUILD_DIR)
	$(WINDRES) -i $< --input-format=rc -o $@ -O coff

UPGRADE_PATCH = $(BUILD_DIR)/upgrade-patch-$(SUFFIX).exe

VALGRIND ?=

MINGW_CXXFLAGS = -D OEMRESOURCE

TARGET_PLATFORM_LFLAGS = -lgdi32 -luser32 -lkernel32 -municode -lcomdlg32 -lole32

LINK_DLL = $(CPP_TARGET_PLATFORM) -Wall -shared -static
LINK_EXE = $(CPP_TARGET_PLATFORM) -Wall -static

LINK_FST = $(LINK_DLL)
LINK_VST3 = $(LINK_DLL)
LINK_GUI_PLAYGROUND = $(LINK_EXE)
LINK_UPGRADE_PATCH = $(LINK_EXE)
