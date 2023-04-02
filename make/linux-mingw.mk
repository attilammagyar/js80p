FST_MAIN = $(FST_MAIN_DIR)/js80p.dll
FST_MAIN_OS = windows
FST_MAIN_SOURCES = src/fst/dll.cpp

GUI_PLAYGROUND = $(BUILD_DIR)/gui-playground$(SUFFIX).exe
GUI_PLAYGROUND_SOURCES = src/gui/win32-playground.cpp
GUI_PLATFORM_HEADERS = src/gui/win32.hpp
GUI_PLATFORM_SOURCES = src/gui/win32.cpp

DEBUG_LOG_FILE ?= C:\\\\debug.txt

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

PLATFORM_LFLAGS = -lgdi32 -luser32 -lkernel32 -municode -lcomdlg32

LINK_FST_MAIN = $(LINK_DLL)
LINK_GUI_PLAYGROUND = $(LINK_EXE)

PLATFORM_CXXFLAGS =
PLATFORM_CXXINCS =
