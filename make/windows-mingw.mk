TARGET_OS = windows

FST = $(FST_DIR)/js80p.dll
FST_MAIN_SOURCES = src/plugin/fst/dll.cpp

VST3 = $(VST3_DIR)/js80p.vst3
VST3_MAIN_SOURCES = src/plugin/vst3/dll.cpp
VST3_GUI_PLATFORM = kPlatformTypeHWND

GUI_PLAYGROUND = $(BUILD_DIR)/gui-playground$(SUFFIX).exe
GUI_PLAYGROUND_SOURCES = src/gui/win32-playground.cpp
GUI_PLATFORM_HEADERS = src/gui/win32.hpp
GUI_PLATFORM_SOURCES = src/gui/win32.cpp

# DEBUG_LOG_FILE ?= C:\\\\debug.txt

PLATFORM_CXXFLAGS = \
	-mwindows \
	-D UNICODE \
	-D _UNICODE

RM = del /q
MKDIR = mkdir

EXE = .exe

ROOT_DIR = C:/mingw64

CPP = $(ROOT_DIR)/bin/g++.exe
CPP_PLATFORM = $(CPP)
DOXYGEN = "C:/Program Files/doxygen/bin/doxygen.exe"
WINDRES = $(ROOT_DIR)/bin/windres.exe
VALGRIND ?=

LINK_DLL = $(CPP_PLATFORM) -Wall -s -shared -static
LINK_EXE = $(CPP_PLATFORM) -Wall -s -static

PLATFORM_LFLAGS = -lgdi32 -luser32 -lkernel32 -municode -lcomdlg32 -lole32

LINK_FST = $(LINK_DLL)
LINK_VST3 = $(LINK_DLL)
LINK_GUI_PLAYGROUND = $(LINK_EXE)

PLATFORM_CXXINCS = \
	-I$(ROOT_DIR)/lib/gcc/x86_64-w64-mingw32/12.2.0/include \
	-I$(ROOT_DIR)/include/c++/12.2.0/backward \
	-I$(ROOT_DIR)/include/c++/12.2.0/x86_64-w64-mingw32 \
	-I$(ROOT_DIR)/include/c++/12.2.0 \
	-I$(ROOT_DIR)/include
