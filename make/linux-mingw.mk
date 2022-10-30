RM = rm -f
MKDIR = mkdir

EXE =

CPP = /usr/bin/g++
CPPW = /usr/bin/$(PLATFORM)-g++
CCW = /usr/bin/$(PLATFORM)-gcc
DOXYGEN = /usr/bin/doxygen
WINDRES = /usr/bin/$(PLATFORM)-windres
VALGRIND ?= /usr/bin/valgrind --error-exitcode=99 --track-origins=yes --quiet

LINK_DLL = $(CPPW) $(WIN_DLL_LFLAGS)
LINK_WIN_EXE = $(CPPW) $(WIN_EXE_LFLAGS)

PLATFORM_CXXFLAGS =
PLATFORM_CXXINCS =
