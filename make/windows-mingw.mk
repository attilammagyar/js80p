TARGET_PLATFORM_CXXFLAGS = \
	-mwindows \
	-D UNICODE \
	-D _UNICODE

TARGET_PLATFORM_CXXINCS =

RM = del /q
MKDIR = mkdir

EXE = .exe

CPP_DEV_PLATFORM = $(ROOT_DIR)/bin/g++.exe
CPP_TARGET_PLATFORM = $(CPP_DEV_PLATFORM)
DOXYGEN = "C:/Program Files/doxygen/bin/doxygen.exe"
WINDRES = $(ROOT_DIR)/bin/windres.exe

include make/mingw.mk
