# SUFFIX = 32bit
# OBJ_GUI_BFDNAME=elf32-i386
# OBJ_GUI_BFDARCH=i386

SUFFIX = 64bit
OBJ_GUI_BFDNAME=elf64-x86-64
OBJ_GUI_BFDARCH=i386:x86-64

TARGET_OS = linux

FST = $(FST_DIR)/js80p.so
FST_MAIN_SOURCES = src/plugin/fst/so.cpp

VST3 = $(VST3_DIR)/js80p.vst3
VST3_BIN = $(BUILD_DIR)/js80p.so
VST3_MAIN_SOURCES = src/plugin/vst3/so.cpp
VST3_GUI_PLATFORM = kPlatformTypeX11EmbedWindowID
VST3_PLATFORM_OBJS =

GUI_PLAYGROUND = $(BUILD_DIR)/gui-playground$(SUFFIX)
GUI_PLAYGROUND_SOURCES = src/gui/xcb-playground.cpp
GUI_TARGET_PLATFORM_HEADERS = src/gui/xcb.hpp
GUI_TARGET_PLATFORM_SOURCES = src/gui/xcb.cpp

OBJ_GUI_DATA = \
	$(BUILD_DIR)/img_about.o \
	$(BUILD_DIR)/img_controllers.o \
	$(BUILD_DIR)/img_effects.o \
	$(BUILD_DIR)/img_envelopes.o \
	$(BUILD_DIR)/img_knob_states.o \
	$(BUILD_DIR)/img_knob_states_inactive.o \
	$(BUILD_DIR)/img_lfos.o \
	$(BUILD_DIR)/img_synth.o \
	$(BUILD_DIR)/img_vst_logo.o

$(BUILD_DIR)/img_about.o: gui/img/about.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_controllers.o: gui/img/controllers.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_effects.o: gui/img/effects.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_envelopes.o: gui/img/envelopes.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_knob_states.o: gui/img/knob_states.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_knob_states_inactive.o: gui/img/knob_states_inactive.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_lfos.o: gui/img/lfos.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_synth.o: gui/img/synth.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_vst_logo.o: gui/img/vst_logo.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

# DEBUG_LOG_FILE ?= /tmp/debug.txt

RM = rm -f
MKDIR = mkdir

EXE =

CPP_DEV_PLATFORM = /usr/bin/g++
CPP_TARGET_PLATFORM = /usr/bin/g++
DOXYGEN = /usr/bin/doxygen
VALGRIND ?= /usr/bin/valgrind --error-exitcode=99 --track-origins=yes --quiet
OBJCOPY = /usr/bin/objcopy \
	--input binary \
	--output $(OBJ_GUI_BFDNAME) \
	--binary-architecture $(OBJ_GUI_BFDARCH)

LINK_SO = $(CPP_TARGET_PLATFORM) -Wall -s -shared
LINK_EXE = $(CPP_TARGET_PLATFORM) -Wall #-s

TARGET_PLATFORM_LFLAGS = -L /usr/X11R6/lib -lcairo -lxcb -lxcb-render

LINK_FST = $(LINK_SO)
LINK_VST3 = $(LINK_SO)
LINK_GUI_PLAYGROUND = $(LINK_EXE)

TARGET_PLATFORM_CXXFLAGS = \
    -Wno-strict-aliasing \
    -Wno-int-to-pointer-cast \
    -Wno-cpp

TARGET_PLATFORM_CXXINCS = -I/usr/include
