###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023, 2024  Attila M. Magyar
# Copyright (C) 2023  Patrik Ehringer
#
# JS80P is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# JS80P is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
###############################################################################

ROOT_DIR ?= C:/mingw64
DIR_SEP = \\

TARGET_PLATFORM_CXXFLAGS = \
	-mwindows \
	$(MINGW_CXXFLAGS) \
	-D UNICODE \
	-D _UNICODE

TARGET_PLATFORM_CXXINCS =

RM ?= del /q
MKDIR ?= mkdir

EXE = .exe
DEV_EXE = .exe

CPP_DEV_PLATFORM ?= $(ROOT_DIR)/bin/g++.exe
CPP_TARGET_PLATFORM ?= $(CPP_DEV_PLATFORM)
CPPCHECK ?= echo Skipping
DOXYGEN ?= "C:/Program Files/doxygen/bin/doxygen.exe"
VALGRIND ?=
WINDRES ?= $(ROOT_DIR)/bin/windres.exe

CPPCHECK_FLAGS =
VALGRIND_FLAGS =

LINK_DEV_EXE = $(CPP_DEV_PLATFORM) -Wall -static

include make/mingw.mk
