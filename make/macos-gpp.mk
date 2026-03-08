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
SUFFIX ?=

INSTRUCTION_SET ?= native

RM ?= rm -f
MKDIR ?= mkdir
COPY ?= cp -v
SED ?= sed

CPP_DEV_PLATFORM ?= /usr/bin/g++ \
	-stdlib=libc++ \
	-isysroot $(shell xcrun --show-sdk-path) \
	-mmacos-version-min=10.12
CPP_TARGET_PLATFORM ?= $(CPP_DEV_PLATFORM)

CPPCHECK ?= echo Skipping Cppcheck:
DOXYGEN ?= echo Skipping Doxygen

CHECK_MEMORY ?= /bin/bash scripts/mac_check_leaks.sh

TARGET_PLATFORM_LFLAGS = -Wall -framework Cocoa -framework UniformTypeIdentifiers
LINK_DEV_EXE = $(CPP_DEV_PLATFORM) $(TARGET_PLATFORM_LFLAGS)
LINK_FST = $(CPP_TARGET_PLATFORM) -bundle
LINK_VST3 = $(CPP_TARGET_PLATFORM) -bundle

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
GUI_PLAYGROUND_APP_IMG_DIR = $(GUI_PLAYGROUND_APP_RES_DIR)/img

GUI_PLAYGROUND_APP_DIRS = \
	$(GUI_PLAYGROUND_APP_CONTENTS_DIR) \
	$(GUI_PLAYGROUND_APP_BIN_DIR) \
	$(GUI_PLAYGROUND_APP_RES_DIR) \
	$(GUI_PLAYGROUND_APP_IMG_DIR)

$(GUI_PLAYGROUND_APP_ROOT_DIR): | $(BUILD_DIR)
	$(MKDIR) $@

$(GUI_PLAYGROUND_APP_CONTENTS_DIR): | $(GUI_PLAYGROUND_APP_ROOT_DIR)
	$(MKDIR) $@

$(GUI_PLAYGROUND_APP_BIN_DIR) $(GUI_PLAYGROUND_APP_RES_DIR): \
		| $(GUI_PLAYGROUND_APP_CONTENTS_DIR)
	$(MKDIR) $@

$(GUI_PLAYGROUND_APP_IMG_DIR): | $(GUI_PLAYGROUND_APP_RES_DIR)
	$(MKDIR) $@

GUI_PLAYGROUND = $(GUI_PLAYGROUND_APP_BIN_DIR)/gui-playground
GUI_PLAYGROUND_SOURCES = src/gui/macos-playground.cpp
GUI_TARGET_PLATFORM_HEADERS = src/gui/macos.hpp
GUI_TARGET_PLATFORM_SOURCES = src/gui/macos.cpp

OBJ_TARGET_GUI_EXTRA = $(BUILD_DIR)/gui-macos.o

GUI_PLAYGROUND_EXTRA = \
	$(BUILD_DIR)/macos-playground.o \
	$(foreach GUI_IMAGE,$(GUI_IMAGES),$(GUI_PLAYGROUND_APP_IMG_DIR)/$(GUI_IMAGE).png)

OBJECTIVE_CPP = -fobjc-arc -x objective-c++

LINK_GUI_PLAYGROUND = $(LINK_DEV_EXE) $(BUILD_DIR)/macos-playground.o

$(BUILD_DIR)/macos-playground.o: src/gui/macos-playground.mm | $(BUILD_DIR)
	$(COMPILE_TARGET) $(OBJECTIVE_CPP) -c -o $@ $<

$(GUI_PLAYGROUND_APP_IMG_DIR)/%.png: gui/img/%.png | $(GUI_PLAYGROUND_APP_IMG_DIR)
	$(COPY) $< $@

FST_ROOT_DIR = $(FST_DIR)/js80p.vst
FST_CONTENTS_DIR = $(FST_ROOT_DIR)/Contents
FST_BIN_DIR = $(FST_CONTENTS_DIR)/MacOS
FST_RES_DIR = $(FST_CONTENTS_DIR)/Resources
FST_IMG_DIR = $(FST_RES_DIR)/img

FST_DIRS = \
	$(FST_CONTENTS_DIR) \
	$(FST_BIN_DIR) \
	$(FST_RES_DIR) \
	$(FST_IMG_DIR)

$(FST_ROOT_DIR): | $(FST_DIR)
	$(MKDIR) $@

$(FST_CONTENTS_DIR): | $(FST_ROOT_DIR)
	$(MKDIR) $@

$(FST_BIN_DIR) $(FST_RES_DIR): | $(FST_CONTENTS_DIR)
	$(MKDIR) $@

$(FST_IMG_DIR): | $(FST_RES_DIR)
	$(MKDIR) $@

$(FST_CONTENTS_DIR)/Info.plist: \
		src/plugin/fst/Info.plist.tpl | $(FST_CONTENTS_DIR)
	$(SED) 's/{YEAR}/'$(shell date '+%Y')'/g; s/{VERSION}/'$(VERSION_DOT)'/g' $< >$@

FST = $(FST_BIN_DIR)/js80p
FST_MAIN_SOURCES = src/plugin/fst/mach-o.cpp
FST_EXTRA =

$(FST_IMG_DIR)/%.png: gui/img/%.png | $(FST_IMG_DIR)
	$(COPY) $< $@

FST_GUI_IMAGES = \
	$(foreach GUI_IMAGE,$(filter-out vst_logo,$(GUI_IMAGES)),$(FST_IMG_DIR)/$(GUI_IMAGE).png)

VST3_ROOT_DIR = $(VST3_DIR)/js80p.vst3
VST3_CONTENTS_DIR = $(VST3_ROOT_DIR)/Contents
VST3_BIN_DIR = $(VST3_CONTENTS_DIR)/MacOS
VST3_RES_DIR = $(VST3_CONTENTS_DIR)/Resources
VST3_IMG_DIR = $(VST3_RES_DIR)/img

VST3_DIRS = \
	$(VST3_CONTENTS_DIR) \
	$(VST3_BIN_DIR) \
	$(VST3_RES_DIR) \
	$(VST3_IMG_DIR)

$(VST3_ROOT_DIR): | $(VST3_DIR)
	$(MKDIR) $@

$(VST3_CONTENTS_DIR): | $(VST3_ROOT_DIR)
	$(MKDIR) $@

$(VST3_BIN_DIR) $(VST3_RES_DIR): | $(VST3_CONTENTS_DIR)
	$(MKDIR) $@

$(VST3_IMG_DIR): | $(VST3_RES_DIR)
	$(MKDIR) $@

$(VST3_CONTENTS_DIR)/Info.plist: \
		src/plugin/vst3/Info.plist.tpl | $(VST3_CONTENTS_DIR)
	$(SED) 's/{YEAR}/'$(shell date '+%Y')'/g; s/{VERSION}/'$(VERSION_DOT)'/g' $< >$@

VST3 = $(VST3_BIN_DIR)/js80p
VST3_MAIN_SOURCES = src/plugin/vst3/mach-o.cpp
VST3_GUI_PLATFORM = kPlatformTypeNSView
VST3_EXTRA =
VST3_PLUGIN_SOURCES = \
	src/plugin/vst3/plugin.cpp \
	src/plugin/vst3/plugin-macos.cpp
VST3_TARGET_PLATFORM_CXXFLAGS = $(OBJECTIVE_CPP)

$(VST3_IMG_DIR)/%.png: gui/img/%.png | $(VST3_IMG_DIR)
	$(COPY) $< $@

VST3_GUI_IMAGES = \
	$(foreach GUI_IMAGE,$(GUI_IMAGES),$(VST3_IMG_DIR)/$(GUI_IMAGE).png)

# TODO: untangle GUI resources and dirs for various bundle types from gui-macos.o
$(BUILD_DIR)/gui-macos.o: \
		src/gui/macos.mm \
		$(BUILD_DIR)/macos-playground.o \
		$(GUI_PLAYGROUND_EXTRA) \
		$(FST_GUI_IMAGES) \
		$(FST_CONTENTS_DIR)/Info.plist \
		$(VST3_GUI_IMAGES) \
		$(VST3_CONTENTS_DIR)/Info.plist \
		| $(BUILD_DIR) \
		$(GUI_PLAYGROUND_APP_DIRS) \
		$(FST_DIRS) \
		$(VST3_DIRS)
	$(COMPILE_TARGET) $(OBJECTIVE_CPP) -c -o $@ $<

VST3_MODULE_INFO_TOOL = $(BUILD_DIR)$(DIR_SEP)vst3_module_info_tool
VST3_MODULE_INFO_LFLAGS = -pthread -ldl

DEV_PLATFORM_CLEAN = $(VST3_MODULE_INFO_TOOL)

.PHONY: vst3moduleinfo

vst3moduleinfo: $(VST3_MODULE_INFO_TOOL)

$(VST3_MODULE_INFO_TOOL): src/plugin/vst3/moduleinfo.cpp | $(BUILD_DIR)
	$(CPP_TARGET_PLATFORM) \
		$(OBJECTIVE_CPP) -framework AppKit \
		$(VST3_CXXINCS) $(VST3_CXXFLAGS) \
		$(VST3_MODULE_INFO_LFLAGS) \
		$< -o $@
