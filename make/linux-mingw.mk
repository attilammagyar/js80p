TARGET_OS = windows

FST = $(FST_DIR)/js80p.dll
FST_MAIN_SOURCES = src/plugin/fst/dll.cpp

VST3 = $(VST3_DIR)/js80p.vst3
VST3_DLL = $(BUILD_DIR)/js80p.dll
VST3_MAIN_SOURCES = src/plugin/vst3/dll.cpp
VST3_GUI_PLATFORM = kPlatformTypeHWND

GUI_PLAYGROUND = $(BUILD_DIR)/gui-playground$(SUFFIX).exe
GUI_PLAYGROUND_SOURCES = src/gui/win32-playground.cpp
GUI_PLATFORM_HEADERS = src/gui/win32.hpp
GUI_PLATFORM_SOURCES = src/gui/win32.cpp

# DEBUG_LOG_FILE ?= C:\\\\debug.txt

RM = rm -f
MKDIR = mkdir

EXE =

CPP = /usr/bin/g++
CPP_PLATFORM = /usr/bin/$(PLATFORM)-g++
DOXYGEN = /usr/bin/doxygen
WINDRES = /usr/bin/$(PLATFORM)-windres
VALGRIND ?= /usr/bin/valgrind --error-exitcode=99 --track-origins=yes --quiet

LINK_DLL = $(CPP_PLATFORM) -Wall -s -shared -static
LINK_EXE = $(CPP_PLATFORM) -Wall -s -static

PLATFORM_LFLAGS = -lgdi32 -luser32 -lkernel32 -municode -lcomdlg32 -lole32

LINK_FST = $(LINK_DLL)
LINK_VST3 = $(LINK_DLL)
LINK_GUI_PLAYGROUND = $(LINK_EXE)

PLATFORM_CXXFLAGS =
PLATFORM_CXXINCS =
