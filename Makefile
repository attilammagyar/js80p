DEV_OS ?= linux
TARGET_PLATFORM ?= x86_64-gpp
VERSION_STR ?= dev
VERSION_INT ?= 999000
VERSION_AS_FILE_NAME ?= dev

BUILD_DIR_BASE ?= build
BUILD_DIR = $(BUILD_DIR_BASE)$(DIR_SEP)$(TARGET_PLATFORM)
DIST_DIR_BASE ?= dist
DIST_DIR_PREFIX ?= $(DIST_DIR_BASE)$(DIR_SEP)js80p-$(VERSION_AS_FILE_NAME)-$(TARGET_OS)-$(SUFFIX)
DOC_DIR ?= doc

TEST_MAX_ARRAY_PRINT ?= 20

JS80P_CXXINCS = \
	-I./lib \
	-I./src

FST_CXXINCS = $(JS80P_CXXINCS)

VST3_CXXINCS = \
	-I./lib/vst3sdk \
	$(JS80P_CXXINCS)

JS80P_CXXFLAGS = \
	-D JS80P_VERSION_STR=$(VERSION_STR) \
	-D JS80P_VERSION_INT=$(VERSION_INT) \
	-Wall \
	-Werror \
	-msse2 \
	-ffast-math \
	-O3

# DEBUG_LOG ?= -D JS80P_DEBUG_LOG=$(DEBUG_LOG_FILE)
DEBUG_LOG ?=

FST_DIR = $(DIST_DIR_PREFIX)-fst
VST3_DIR = $(DIST_DIR_PREFIX)-vst3_single_file

OBJ_GUI_PLAYGROUND = $(BUILD_DIR)/gui-playground-$(SUFFIX).o

OBJ_UPGRADE_PATCH = $(BUILD_DIR)/upgrade-patch-$(SUFFIX).o

.PHONY: \
	all \
	check \
	clean \
	dirs \
	docs \
	fst \
	gui_playground \
	perf \
	show_fst_dir \
	show_vst3_dir \
	upgrade_patch \
	vst3

all: dirs fst vst3

include make/$(DEV_OS)-$(TARGET_PLATFORM).mk

OBJ_FST_MAIN = $(BUILD_DIR)/fst-main-$(SUFFIX).o
OBJ_FST_PLUGIN = $(BUILD_DIR)/fst-plugin-$(SUFFIX).o
OBJ_VST3_MAIN = $(BUILD_DIR)/vst3-main-$(SUFFIX).o
OBJ_VST3_PLUGIN = $(BUILD_DIR)/vst3-plugin-$(SUFFIX).o
OBJ_BANK = $(BUILD_DIR)/bank-$(SUFFIX).o
OBJ_SERIALIZER = $(BUILD_DIR)/serializer-$(SUFFIX).o
OBJ_SYNTH = $(BUILD_DIR)/synth-$(SUFFIX).o
OBJ_GUI = $(BUILD_DIR)/gui-$(SUFFIX).o

FST_OBJS = \
	$(OBJ_GUI_EXTRA) \
	$(OBJ_FST_MAIN) \
	$(OBJ_FST_PLUGIN) \
	$(OBJ_GUI) \
	$(OBJ_BANK) \
	$(OBJ_SERIALIZER) \
	$(OBJ_SYNTH)

VST3_OBJS = \
	$(OBJ_GUI_EXTRA) \
	$(OBJ_VST3_MAIN) \
	$(OBJ_VST3_PLUGIN) \
	$(OBJ_GUI) \
	$(OBJ_BANK) \
	$(OBJ_SERIALIZER) \
	$(OBJ_SYNTH)

GUI_PLAYGROUND_OBJS = \
	$(OBJ_GUI_EXTRA) \
	$(OBJ_GUI) \
	$(OBJ_GUI_PLAYGROUND) \
	$(OBJ_SERIALIZER) \
	$(OBJ_SYNTH)

UPGRADE_PATCH_OBJS = \
	$(OBJ_SERIALIZER) \
	$(OBJ_SYNTH) \
	$(OBJ_UPGRADE_PATCH)

PARAM_COMPONENTS = \
	dsp/envelope \
	dsp/flexible_controller \
	dsp/lfo \
	dsp/math \
	dsp/midi_controller \
	dsp/oscillator \
	dsp/param \
	dsp/queue \
	dsp/signal_producer

SYNTH_COMPONENTS = \
	$(PARAM_COMPONENTS) \
	synth \
	dsp/biquad_filter \
	dsp/chorus \
	dsp/delay \
	dsp/distortion \
	dsp/echo \
	dsp/effect \
	dsp/effects \
	dsp/filter \
	dsp/gain \
	dsp/mixer \
	dsp/reverb \
	dsp/voice \
	dsp/wavefolder \
	dsp/wavetable

TESTS = \
	test_example \
	test_biquad_filter \
	test_delay \
	test_distortion \
	test_envelope \
	test_flexible_controller \
	test_gain \
	test_gui \
	test_lfo \
	test_math \
	test_midi_controller \
	test_mixer \
	test_oscillator \
	test_param \
	test_bank \
	test_queue \
	test_renderer \
	test_serializer \
	test_signal_producer \
	test_synth \
	test_voice \
	test_wavefolder

PERF_TESTS = \
	perf_math

PARAM_HEADERS = \
	src/js80p.hpp \
	$(foreach COMPONENT,$(PARAM_COMPONENTS),src/$(COMPONENT).hpp)

PARAM_SOURCES = \
	$(foreach COMPONENT,$(PARAM_COMPONENTS),src/$(COMPONENT).cpp)

SYNTH_HEADERS = \
	src/debug.hpp \
	src/js80p.hpp \
	src/midi.hpp \
	$(foreach COMPONENT,$(SYNTH_COMPONENTS),src/$(COMPONENT).hpp)

SYNTH_SOURCES = \
	$(foreach COMPONENT,$(SYNTH_COMPONENTS),src/$(COMPONENT).cpp)

JS80P_HEADERS = \
	src/gui/gui.hpp \
	src/gui/widgets.hpp \
	src/bank.hpp \
	src/renderer.hpp \
	src/serializer.hpp \
	$(SYNTH_HEADERS)

JS80P_SOURCES = \
	src/gui/gui.cpp \
	src/gui/widgets.cpp \
	src/bank.cpp \
	src/programs.cpp \
	src/serializer.cpp \
	$(SYNTH_SOURCES)

FST_HEADERS = \
	$(JS80P_HEADERS) \
	src/plugin/fst/plugin.hpp

FST_SOURCES = \
	src/plugin/fst/plugin.cpp \
	$(FST_MAIN_SOURCES) \
	$(JS80P_SOURCES)

VST3_HEADERS = \
	$(JS80P_HEADERS) \
	src/plugin/vst3/plugin.hpp

VST3_SOURCES = \
	src/plugin/vst3/plugin.cpp \
	$(VST3_MAIN_SOURCES) \
	$(JS80P_SOURCES)

GUI_HEADERS = \
	$(GUI_TARGET_PLATFORM_HEADERS) \
	$(JS80P_HEADERS)

GUI_SOURCES = \
	$(GUI_TARGET_PLATFORM_SOURCES) \
	src/gui/widgets.cpp \
	src/gui/gui.cpp

UPGRADE_PATCH_SOURCES = src/upgrade_patch.cpp

TEST_LIBS = \
	tests/test.cpp \
	tests/utils.cpp

TEST_CPPS = $(foreach TEST,$(TESTS),tests/$(TEST).cpp)
TEST_BINS = $(foreach TEST,$(TESTS),$(BUILD_DIR)/$(TEST)$(EXE))
PERF_TEST_BINS = $(foreach TEST,$(PERF_TESTS),$(BUILD_DIR)/$(TEST)$(EXE))

TEST_CXXFLAGS = \
	-D TEST_MAX_ARRAY_PRINT=$(TEST_MAX_ARRAY_PRINT) \
	-D JS80P_ASSERTIONS=1 \
	-I./tests \
	-g

FST_CXXFLAGS = \
	-DFST_DONT_DEPRECATE_UNKNOWN \
	$(JS80P_CXXFLAGS)

VST3_CXXFLAGS = \
	-DRELEASE \
	-DJS80P_VST3_GUI_PLATFORM=$(VST3_GUI_PLATFORM) \
	$(JS80P_CXXFLAGS) \
	-Wno-class-memaccess \
	-Wno-format \
	-Wno-multichar \
	-Wno-parentheses \
	-Wno-pragmas \
	-Wno-unknown-pragmas

show_fst_dir:
	@echo $(FST_DIR)

show_vst3_dir:
	@echo $(VST3_DIR)

fst: $(FST)

vst3: $(VST3)

dirs: $(BUILD_DIR) $(DOC_DIR) $(FST_DIR) $(VST3_DIR)

$(BUILD_DIR): | $(BUILD_DIR_BASE)
	$(MKDIR) $@

$(BUILD_DIR_BASE):
	$(MKDIR) $@

$(DIST_DIR_BASE):
	$(MKDIR) $@

$(DOC_DIR):
	$(MKDIR) $@

clean:
	$(RM) \
		$(DEV_PLATFORM_CLEAN) \
		$(FST) \
		$(FST_OBJS) \
		$(GUI_PLAYGROUND) \
		$(GUI_PLAYGROUND_OBJS) \
		$(PERF_TEST_BINS) \
		$(TEST_BINS) \
		$(UPGRADE_PATCH) \
		$(VST3) \
		$(VST3_OBJS)
	$(RM) $(DOC_DIR)/html/*.* $(DOC_DIR)/html/search/*.*

check: perf $(TEST_LIBS) $(TEST_BINS) | $(BUILD_DIR)
	$(BUILD_DIR)/test_math$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_queue$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_signal_producer$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_midi_controller$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_flexible_controller$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_param$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_envelope$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_oscillator$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_lfo$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_biquad_filter$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_delay$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_distortion$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_wavefolder$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_gain$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_voice$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_mixer$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_synth$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_renderer$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_serializer$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_bank$(EXE)
	$(VALGRIND) $(BUILD_DIR)/test_gui$(EXE)
# 	$(VALGRIND) $(BUILD_DIR)/test_example$(EXE)

perf: $(BUILD_DIR) $(PERF_TEST_BINS)

docs: Doxyfile $(DOC_DIR) $(DOC_DIR)/html/index.html

gui_playground: $(GUI_PLAYGROUND)

upgrade_patch: $(UPGRADE_PATCH)

$(DOC_DIR)/html/index.html: \
		Doxyfile \
		$(JS80P_HEADERS) \
		$(JS80P_SOURCES) \
		$(TEST_LIBS) \
		$(GUI_HEADERS) \
		$(GUI_SOURCES) \
		$(FST_HEADERS) \
		$(FST_SOURCES) \
		$(VST3_HEADERS) \
		$(VST3_SOURCES) \
		| $(DOC_DIR)
	$(DOXYGEN)

$(FST): $(FST_PLATFORM_OBJS) $(FST_OBJS) | $(FST_DIR)
	$(LINK_FST) \
		$(FST_PLATFORM_OBJS) $(FST_OBJS) \
		-o $@ $(TARGET_PLATFORM_LFLAGS)

$(FST_DIR): | $(DIST_DIR_BASE)
	$(MKDIR) $@

$(VST3): $(VST3_PLATFORM_OBJS) $(VST3_OBJS) | $(VST3_DIR)
	$(LINK_VST3) \
		$(VST3_PLATFORM_OBJS) $(VST3_OBJS) \
		-o $@ $(TARGET_PLATFORM_LFLAGS)

$(VST3_DIR): | $(DIST_DIR_BASE)
	$(MKDIR) $@

$(GUI_PLAYGROUND): $(GUI_PLAYGROUND_OBJS) | $(BUILD_DIR)
	$(LINK_GUI_PLAYGROUND) $(GUI_PLAYGROUND_OBJS) -o $@ $(TARGET_PLATFORM_LFLAGS)

$(OBJ_GUI_PLAYGROUND): \
		$(GUI_PLAYGROUND_SOURCES) \
		$(GUI_SOURCES) $(GUI_HEADERS) \
		| $(BUILD_DIR)
	$(CPP_TARGET_PLATFORM) \
		$(TARGET_PLATFORM_CXXINCS) $(JS80P_CXXINCS) $(JS80P_CXXFLAGS) $(TARGET_PLATFORM_CXXFLAGS) \
		$(DEBUG_LOG) -c $< -o $@

$(UPGRADE_PATCH): $(UPGRADE_PATCH_OBJS) | $(BUILD_DIR)
	$(LINK_UPGRADE_PATCH) $(UPGRADE_PATCH_OBJS) -o $@

$(OBJ_UPGRADE_PATCH): $(UPGRADE_PATCH_SOURCES) | $(BUILD_DIR)
	$(CPP_TARGET_PLATFORM) \
		$(TARGET_PLATFORM_CXXINCS) $(JS80P_CXXINCS) $(JS80P_CXXFLAGS) $(TARGET_PLATFORM_CXXFLAGS) \
		$(DEBUG_LOG) -c $< -o $@

$(OBJ_SYNTH): $(SYNTH_HEADERS) $(SYNTH_SOURCES) | $(BUILD_DIR)
	$(CPP_TARGET_PLATFORM) \
		$(TARGET_PLATFORM_CXXINCS) $(JS80P_CXXINCS) $(JS80P_CXXFLAGS) $(TARGET_PLATFORM_CXXFLAGS) \
		$(DEBUG_LOG) -c src/synth.cpp -o $@

$(OBJ_BANK): \
		src/bank.cpp src/bank.hpp \
		src/serializer.cpp src/serializer.hpp \
		$(SYNTH_HEADERS) \
		| $(BUILD_DIR)
	$(CPP_TARGET_PLATFORM) \
		$(TARGET_PLATFORM_CXXINCS) $(JS80P_CXXINCS) $(JS80P_CXXFLAGS) $(TARGET_PLATFORM_CXXFLAGS) \
		$(DEBUG_LOG) -c $< -o $@

$(OBJ_SERIALIZER): \
		src/serializer.cpp src/serializer.hpp \
		$(SYNTH_HEADERS) \
		| $(BUILD_DIR)
	$(CPP_TARGET_PLATFORM) \
		$(TARGET_PLATFORM_CXXINCS) $(JS80P_CXXINCS) $(JS80P_CXXFLAGS) $(TARGET_PLATFORM_CXXFLAGS) \
		$(DEBUG_LOG) -c $< -o $@

$(OBJ_GUI) : \
		$(GUI_SOURCES) $(GUI_HEADERS) \
		| $(BUILD_DIR)
	$(CPP_TARGET_PLATFORM) \
		$(TARGET_PLATFORM_CXXINCS) $(JS80P_CXXINCS) $(JS80P_CXXFLAGS) $(TARGET_PLATFORM_CXXFLAGS) \
		$(DEBUG_LOG) -c $< -o $@

$(OBJ_FST_PLUGIN): \
		src/plugin/fst/plugin.cpp \
		$(FST_HEADERS) \
		| $(BUILD_DIR)
	$(CPP_TARGET_PLATFORM) \
		$(TARGET_PLATFORM_CXXINCS) $(FST_CXXINCS) $(FST_CXXFLAGS) $(TARGET_PLATFORM_CXXFLAGS) \
		$(DEBUG_LOG) -c $< -o $@

$(OBJ_FST_MAIN): \
		$(FST_MAIN_SOURCES) \
		$(FST_HEADERS) \
		| $(BUILD_DIR)
	$(CPP_TARGET_PLATFORM) \
		$(TARGET_PLATFORM_CXXINCS) $(FST_CXXINCS) $(FST_CXXFLAGS) $(TARGET_PLATFORM_CXXFLAGS) \
		$(DEBUG_LOG) -c $< -o $@

$(OBJ_VST3_PLUGIN): \
		$(VST3_PLUGIN_SOURCES) \
		$(VST3_HEADERS) \
		| $(BUILD_DIR)
	$(CPP_TARGET_PLATFORM) \
		$(TARGET_PLATFORM_CXXINCS) $(VST3_CXXINCS) $(VST3_CXXFLAGS) $(TARGET_PLATFORM_CXXFLAGS) \
		$(DEBUG_LOG) -c $< -o $@

$(OBJ_VST3_MAIN): \
		$(VST3_MAIN_SOURCES) \
		$(VST3_HEADERS) \
		| $(BUILD_DIR)
	$(CPP_TARGET_PLATFORM) \
		$(TARGET_PLATFORM_CXXINCS) $(VST3_CXXINCS) $(VST3_CXXFLAGS) $(TARGET_PLATFORM_CXXFLAGS) \
		$(DEBUG_LOG) -c $< -o $@

$(BUILD_DIR)/perf_math$(EXE): \
		tests/performance/perf_math.cpp \
		src/dsp/math.hpp src/dsp/math.cpp \
		src/js80p.hpp \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_example$(EXE): \
	tests/test_example.cpp \
	$(TEST_LIBS) \
	| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_bank$(EXE): \
		tests/test_bank.cpp \
		src/bank.cpp src/bank.hpp \
		src/serializer.cpp src/serializer.hpp \
		$(TEST_LIBS) \
		$(SYNTH_HEADERS) \
		$(SYNTH_SOURCES) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_biquad_filter$(EXE): \
		tests/test_biquad_filter.cpp \
		src/dsp/biquad_filter.cpp src/dsp/biquad_filter.hpp \
		src/dsp/filter.cpp src/dsp/filter.hpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_delay$(EXE): \
		tests/test_delay.cpp \
		src/dsp/delay.cpp src/dsp/delay.hpp \
		src/dsp/filter.cpp src/dsp/filter.hpp \
		src/dsp/biquad_filter.cpp src/dsp/biquad_filter.hpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) \
		$(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) \
		-Wno-maybe-uninitialized \
		-o $@ $<

$(BUILD_DIR)/test_distortion$(EXE): \
		tests/test_distortion.cpp \
		src/dsp/distortion.cpp src/dsp/distortion.hpp \
		src/dsp/filter.cpp src/dsp/filter.hpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_envelope$(EXE): \
		tests/test_envelope.cpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_flexible_controller$(EXE): \
		tests/test_flexible_controller.cpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_gain$(EXE): \
		tests/test_gain.cpp \
		src/dsp/gain.cpp src/dsp/gain.hpp \
		src/dsp/filter.cpp src/dsp/filter.hpp \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_gui$(EXE): \
		tests/test_gui.cpp \
		tests/gui_stubs.cpp \
		$(TEST_LIBS) \
		$(JS80P_HEADERS) \
		$(JS80P_SOURCES) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_lfo$(EXE): \
		tests/test_lfo.cpp \
		src/dsp/wavetable.cpp src/dsp/wavetable.hpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_math$(EXE): \
		tests/test_math.cpp \
		src/dsp/math.cpp src/dsp/math.hpp \
		src/js80p.hpp \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_midi_controller$(EXE): \
		tests/test_midi_controller.cpp \
		src/dsp/midi_controller.cpp src/dsp/midi_controller.hpp \
		src/dsp/queue.cpp src/dsp/queue.hpp \
		src/js80p.hpp \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_mixer$(EXE): \
		tests/test_mixer.cpp \
		src/dsp/mixer.cpp src/dsp/mixer.hpp \
		src/dsp/signal_producer.cpp src/dsp/signal_producer.hpp \
		src/js80p.hpp \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_oscillator$(EXE): \
		tests/test_oscillator.cpp \
		src/dsp/wavetable.cpp src/dsp/wavetable.hpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_param$(EXE): \
		tests/test_param.cpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_queue$(EXE): \
		tests/test_queue.cpp \
		src/dsp/queue.cpp src/dsp/queue.hpp \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_renderer$(EXE): \
		tests/test_renderer.cpp \
		src/renderer.hpp \
		$(TEST_LIBS) \
		$(SYNTH_HEADERS) \
		$(SYNTH_SOURCES) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_serializer$(EXE): \
		tests/test_serializer.cpp \
		src/serializer.cpp src/serializer.hpp \
		$(TEST_LIBS) \
		$(SYNTH_HEADERS) \
		$(SYNTH_SOURCES) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_signal_producer$(EXE): \
		tests/test_signal_producer.cpp \
		src/dsp/queue.cpp src/dsp/queue.hpp \
		src/dsp/signal_producer.cpp src/dsp/signal_producer.hpp \
		src/js80p.hpp \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_synth$(EXE): \
		tests/test_synth.cpp \
		$(TEST_LIBS) \
		$(SYNTH_HEADERS) \
		$(SYNTH_SOURCES) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_voice$(EXE): \
		tests/test_voice.cpp \
		$(TEST_LIBS) \
		$(SYNTH_SOURCES) \
		$(SYNTH_HEADERS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_wavefolder$(EXE): \
		tests/test_wavefolder.cpp \
		src/dsp/filter.cpp src/dsp/filter.hpp \
		src/dsp/wavefolder.cpp src/dsp/wavefolder.hpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(BUILD_DIR)
	$(CPP_DEV_PLATFORM) $(JS80P_CXXINCS) $(TEST_CXXFLAGS) $(JS80P_CXXFLAGS) -o $@ $<
