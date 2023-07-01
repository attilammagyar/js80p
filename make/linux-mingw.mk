TARGET_PLATFORM_CXXFLAGS = $(MINGW_CXXFLAGS)
TARGET_PLATFORM_CXXINCS =
DIR_SEP = /

RM = rm -f
MKDIR = mkdir

EXE =

CPP_DEV_PLATFORM = /usr/bin/g++
CPP_TARGET_PLATFORM = /usr/bin/$(TARGET_PLATFORM)-g++
DOXYGEN = /usr/bin/doxygen
WINDRES = /usr/bin/$(TARGET_PLATFORM)-windres
VALGRIND ?= /usr/bin/valgrind --error-exitcode=99 --track-origins=yes --quiet

include make/mingw.mk
