###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023, 2024  Attila M. Magyar
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
