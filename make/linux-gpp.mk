TARGET_OS = linux
DIR_SEP = /

FST = $(FST_DIR)/js80p.so
FST_MAIN_SOURCES = src/plugin/fst/so.cpp

VST3 = $(VST3_DIR)/js80p.vst3
VST3_MAIN_SOURCES = src/plugin/vst3/so.cpp
VST3_GUI_PLATFORM = kPlatformTypeX11EmbedWindowID
VST3_PLATFORM_OBJS =
VST3_PLUGIN_SOURCES = \
	src/plugin/vst3/plugin.cpp \
	src/plugin/vst3/plugin-xcb.cpp

VST3_MODULE_INFO_TOOL = $(BUILD_DIR)$(DIR_SEP)vst3_module_info_tool
VST3_MODULE_INFO_LFLAGS = -pthread -Wl,--no-as-needed -ldl

DEV_PLATFORM_CLEAN = $(VST3_MODULE_INFO_TOOL)

.PHONY: vst3moduleinfo

vst3moduleinfo: $(VST3_MODULE_INFO_TOOL)

$(VST3_MODULE_INFO_TOOL): src/plugin/vst3/moduleinfo.cpp | $(BUILD_DIR)
	$(CPP_TARGET_PLATFORM) \
		$(JS80P_CXXINCS) $(VST3_CXXINCS) $(VST3_CXXFLAGS) $(JS80P_CXXFLAGS) \
		$(VST3_MODULE_INFO_LFLAGS) \
		$< -o $@

GUI_PLAYGROUND = $(BUILD_DIR)/gui-playground-$(SUFFIX)
GUI_PLAYGROUND_SOURCES = src/gui/xcb-playground.cpp
GUI_TARGET_PLATFORM_HEADERS = src/gui/xcb.hpp
GUI_TARGET_PLATFORM_SOURCES = src/gui/xcb.cpp

OBJ_GUI_EXTRA = \
	$(LIB_PATH)/libcairo.so \
	$(LIB_PATH)/libxcb.so \
	$(LIB_PATH)/libxcb-render.so \
	$(BUILD_DIR)/img_about.o \
	$(BUILD_DIR)/img_controllers1.o \
	$(BUILD_DIR)/img_controllers2.o \
	$(BUILD_DIR)/img_effects.o \
	$(BUILD_DIR)/img_envelopes.o \
	$(BUILD_DIR)/img_knob_states-controlled.o \
	$(BUILD_DIR)/img_knob_states-free.o \
	$(BUILD_DIR)/img_knob_states-none.o \
	$(BUILD_DIR)/img_lfos.o \
	$(BUILD_DIR)/img_synth.o \
	$(BUILD_DIR)/img_vst_logo.o

UPGRADE_PATCH = $(BUILD_DIR)/upgrade-patch-$(SUFFIX)

$(LIB_PATH): | $(BUILD_DIR)
	$(MKDIR) $@

$(LIB_PATH)/libcairo.so: $(SYS_LIB_PATH)/libcairo.so.2 | $(LIB_PATH)
	ln -s $< $@

$(LIB_PATH)/libxcb.so: $(SYS_LIB_PATH)/libxcb.so.1 | $(LIB_PATH)
	ln -s $< $@

$(LIB_PATH)/libxcb-render.so: $(SYS_LIB_PATH)/libxcb-render.so.0 | $(LIB_PATH)
	ln -s $< $@

$(BUILD_DIR)/img_about.o: gui/img/about.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_controllers1.o: gui/img/controllers1.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_controllers2.o: gui/img/controllers2.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_effects.o: gui/img/effects.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_envelopes.o: gui/img/envelopes.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_knob_states-controlled.o: gui/img/knob_states-controlled.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_knob_states-free.o: gui/img/knob_states-free.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_knob_states-none.o: gui/img/knob_states-none.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_lfos.o: gui/img/lfos.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_synth.o: gui/img/synth.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

$(BUILD_DIR)/img_vst_logo.o: gui/img/vst_logo.png | $(BUILD_DIR)
	$(OBJCOPY) $< $@

# DEBUG_LOG_FILE ?= STDERR
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

LINK_SO = $(CPP_TARGET_PLATFORM) -Wall -shared
LINK_EXE = $(CPP_TARGET_PLATFORM) -Wall

TARGET_PLATFORM_LFLAGS = \
    $(ARCH_LFLAGS) \
    -lcairo \
    -lxcb \
    -lxcb-render

LINK_FST = $(LINK_SO)
LINK_VST3 = $(LINK_SO)
LINK_GUI_PLAYGROUND = $(LINK_EXE)
LINK_UPGRADE_PATCH = $(LINK_EXE)

TARGET_PLATFORM_CXXFLAGS = \
    $(ARCH_CXXFLAGS) \
    -Wno-strict-aliasing \
    -Wno-int-to-pointer-cast \
    -Wno-cpp \
    -fPIC \
    -fvisibility=hidden

TARGET_PLATFORM_CXXINCS = -I/usr/include
