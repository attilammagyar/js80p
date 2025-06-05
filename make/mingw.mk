###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023, 2024, 2025  Attila M. Magyar
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

TARGET_OS = windows

# DEBUG_LOG ?= STDERR
# DEBUG_LOG ?= C:\\\\debug.txt
DEBUG_LOG ?=

FST = $(FST_DIR)/js80p.dll
FST_MAIN_SOURCES = src/plugin/fst/dll.cpp
FST_EXTRA = src/plugin/fst/js80p.def

VST3 = $(VST3_DIR)/js80p.vst3
VST3_MAIN_SOURCES = src/plugin/vst3/dll.cpp
VST3_GUI_PLATFORM = kPlatformTypeHWND
VST3_EXTRA = src/plugin/vst3/js80p.def
VST3_PLUGIN_SOURCES = \
	src/plugin/vst3/plugin.cpp \
	src/plugin/vst3/plugin-win32.cpp

GUI_PLAYGROUND = $(BUILD_DIR)/gui-playground-$(SUFFIX).exe
GUI_PLAYGROUND_SOURCES = src/gui/win32-playground.cpp
GUI_TARGET_PLATFORM_HEADERS = src/gui/win32.hpp
GUI_TARGET_PLATFORM_SOURCES = src/gui/win32.cpp

GUI_IMAGES = \
	gui/img/about.bmp \
	gui/img/effects.bmp \
	gui/img/envelopes1.bmp \
	gui/img/envelopes2.bmp \
	gui/img/env_shapes-01.bmp \
	gui/img/env_shapes-10.bmp \
	gui/img/knob_states-controlled.bmp \
	gui/img/knob_states-free.bmp \
	gui/img/knob_states-none.bmp \
	gui/img/knob_states-red.bmp \
	gui/img/lfos.bmp \
	gui/img/macro_distortions.bmp \
	gui/img/macro_midpoint_states.bmp \
	gui/img/macros1.bmp \
	gui/img/macros2.bmp \
	gui/img/macros3.bmp \
	gui/img/reversed.bmp \
	gui/img/screw_states.bmp \
	gui/img/screw_states_synced.bmp \
	gui/img/synth.bmp \
	gui/img/vst_logo.bmp

OBJ_TARGET_GUI_EXTRA = $(BUILD_DIR)/gui-$(SUFFIX).res

$(OBJ_TARGET_GUI_EXTRA): src/gui/gui.rc $(GUI_IMAGES) | $(BUILD_DIR)
	$(WINDRES) -i $< --input-format=rc -o $@ -O coff

MINGW_CXXFLAGS = -D OEMRESOURCE

TARGET_PLATFORM_LFLAGS = -lgdi32 -luser32 -lkernel32 -municode -lcomdlg32 -lole32

LINK_DLL = $(CPP_TARGET_PLATFORM) -Wall -shared -static
LINK_TARGET_EXE = $(CPP_TARGET_PLATFORM) -Wall -static

LINK_FST = $(LINK_DLL)
LINK_VST3 = $(LINK_DLL)

show_versions:
	@echo ___ Target platform compiler:
	$(CPP_TARGET_PLATFORM) --version
	@echo ___ Dev platform compiler:
	$(CPP_DEV_PLATFORM) --version
	@echo ___ Cppcheck:
	$(CPPCHECK) --version
