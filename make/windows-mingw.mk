ROOT_DIR = C:/mingw64

TARGET_PLATFORM_CXXFLAGS = \
	-mwindows \
	-D UNICODE \
	-D _UNICODE

TARGET_PLATFORM_CXXINCS = \
	-I$(ROOT_DIR)/lib/gcc/x86_64-w64-mingw32/12.2.0/include \
	-I$(ROOT_DIR)/include/c++/12.2.0/backward \
	-I$(ROOT_DIR)/include/c++/12.2.0/x86_64-w64-mingw32 \
	-I$(ROOT_DIR)/include/c++/12.2.0 \
	-I$(ROOT_DIR)/include

RM = del /q
MKDIR = mkdir

EXE = .exe

CPP_DEV_PLATFORM = $(ROOT_DIR)/bin/g++.exe
CPP_TARGET_PLATFORM = $(CPP_DEV_PLATFORM)
DOXYGEN = "C:/Program Files/doxygen/bin/doxygen.exe"
WINDRES = $(ROOT_DIR)/bin/windres.exe

include make/mingw.mk
