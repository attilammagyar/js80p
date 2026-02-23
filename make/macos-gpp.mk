###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2026  Attila M. Magyar
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

TARGET_OS = macos
DIR_SEP = /

RM ?= rm -f
MKDIR ?= mkdir
COPY ?= cp -v

CPP_TARGET_PLATFORM ?= $(CPP_DEV_PLATFORM)

CPPCHECK ?= echo Skipping Cppcheck:
DOXYGEN ?= echo Skipping Doxygen

CHECK_MEMORY ?= leaks -quiet -atExit --

LINK_DEV_EXE = $(CPP_DEV_PLATFORM) -Wall

DEV_EXE =

CPPCHECK_FLAGS =

TARGET_PLATFORM_CXXFLAGS = \
	-fPIC \
	-fvisibility=hidden

TARGET_PLATFORM_CXXINCS =

GUI_PLAYGROUND_APP_ROOT_DIR = $(BUILD_DIR)/gui-playground-$(SUFFIX).app
GUI_PLAYGROUND_APP_CONTENTS_DIR = $(GUI_PLAYGROUND_APP_ROOT_DIR)/Contents
GUI_PLAYGROUND_APP_BIN_DIR = $(GUI_PLAYGROUND_APP_CONTENTS_DIR)/MacOS
GUI_PLAYGROUND_APP_RES_DIR = $(GUI_PLAYGROUND_APP_CONTENTS_DIR)/Resources

GUI_PLAYGROUND_APP_DIRS = \
	$(GUI_PLAYGROUND_APP_CONTENTS_DIR) \
	$(GUI_PLAYGROUND_APP_BIN_DIR) \
	$(GUI_PLAYGROUND_APP_RES_DIR)

$(GUI_PLAYGROUND_APP_ROOT_DIR): | $(BUILD_DIR)
	$(MKDIR) $@

$(GUI_PLAYGROUND_APP_CONTENTS_DIR): | $(GUI_PLAYGROUND_APP_ROOT_DIR)
	$(MKDIR) $@

$(GUI_PLAYGROUND_APP_BIN_DIR) $(GUI_PLAYGROUND_APP_RES_DIR): \
		| $(GUI_PLAYGROUND_APP_CONTENTS_DIR)
	$(MKDIR) $@

GUI_PLAYGROUND = $(GUI_PLAYGROUND_APP_BIN_DIR)/gui-playground
GUI_PLAYGROUND_SOURCES = src/gui/macos-playground.cpp
GUI_TARGET_PLATFORM_HEADERS = src/gui/macos.hpp
GUI_TARGET_PLATFORM_SOURCES = src/gui/macos.cpp

OBJ_TARGET_GUI_EXTRA = $(BUILD_DIR)/gui-macos.o

GUI_PLAYGROUND_EXTRA = \
	$(BUILD_DIR)/macos-playground.o \
	$(foreach GUI_IMAGE,$(GUI_IMAGES),$(GUI_PLAYGROUND_APP_RES_DIR)/$(GUI_IMAGE).png)

OBJECTIVE_CPP = -fobjc-arc -x objective-c++

LINK_GUI_PLAYGROUND = \
	$(CPP_DEV_PLATFORM) -Wall -framework Cocoa $(BUILD_DIR)/macos-playground.o

$(BUILD_DIR)/gui-macos.o: \
		src/gui/macos.mm \
		$(BUILD_DIR)/macos-playground.o \
		$(GUI_PLAYGROUND_EXTRA) \
		| $(GUI_PLAYGROUND_APP_DIRS) $(BUILD_DIR)
	$(COMPILE_TARGET) $(OBJECTIVE_CPP) -c -o $@ $<

$(BUILD_DIR)/macos-playground.o: src/gui/macos-playground.mm | $(BUILD_DIR)
	$(COMPILE_TARGET) $(OBJECTIVE_CPP) -c -o $@ $<

$(GUI_PLAYGROUND_APP_RES_DIR)/%.png: gui/img/%.png | $(GUI_PLAYGROUND_APP_RES_DIR)
	$(COPY) $< $@
