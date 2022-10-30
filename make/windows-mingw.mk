RM = del /q
MKDIR = mkdir

EXE = .exe

ROOT_DIR = C:/mingw64

CPP = $(ROOT_DIR)/bin/g++.exe
CPPW = $(ROOT_DIR)/bin/g++.exe
CCW = $(ROOT_DIR)/bin/g++.exe
DOXYGEN = "C:/Program Files/doxygen/bin/doxygen.exe"
WINDRES = $(ROOT_DIR)/bin/windres.exe
VALGRIND ?=

LINK_DLL = $(CPPW) $(WIN_DLL_LFLAGS)
LINK_WIN_EXE = $(CPPW) $(WIN_EXE_LFLAGS)

PLATFORM_CXXFLAGS =

PLATFORM_CXXINCS = \
	-I$(ROOT_DIR)/lib/gcc/x86_64-w64-mingw32/12.2.0/include \
	-I$(ROOT_DIR)/include/c++/12.2.0/backward \
	-I$(ROOT_DIR)/include/c++/12.2.0/x86_64-w64-mingw32 \
	-I$(ROOT_DIR)/include/c++/12.2.0 \
	-I$(ROOT_DIR)/include
