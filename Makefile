###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023, 2024  Attila M. Magyar
# Copyright (C) 2023  @aimixsaka (https://github.com/aimixsaka/)
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

DEV_OS ?= linux
TARGET_PLATFORM ?= x86_64-gpp
VERSION_STR ?= dev
VERSION_INT ?= 999000
VERSION_AS_FILE_NAME ?= dev

BUILD_DIR_BASE ?= build
BUILD_DIR = $(BUILD_DIR_BASE)$(DIR_SEP)$(TARGET_PLATFORM)-$(SUFFIX)-$(INSTRUCTION_SET)
DEV_DIR = $(BUILD_DIR_BASE)$(DIR_SEP)dev-$(DEV_OS)-$(SUFFIX)-$(INSTRUCTION_SET)
DIST_DIR_BASE ?= dist
DIST_DIR_PREFIX ?= $(DIST_DIR_BASE)$(DIR_SEP)js80p-$(VERSION_AS_FILE_NAME)-$(TARGET_OS)-$(SUFFIX)-$(INSTRUCTION_SET)
API_DOC_DIR ?= doc/api

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
	-D JS80P_TARGET_PLATFORM=$(TARGET_PLATFORM) \
	-D JS80P_INSTRUCTION_SET=$(INSTRUCTION_SET) \
	-Wall \
	-Werror \
	-Wno-error=maybe-uninitialized \
	-ffast-math \
	-O3 \
	-std=c++17
# maybe-uninitialized: see PannedDelay(input, stereo_mode, tempo_sync) in src/dsp/delay.cpp

ifneq ($(INSTRUCTION_SET),none)
JS80P_CXXFLAGS += -m$(INSTRUCTION_SET)
endif

ifeq ($(DEBUG_LOG),)
DEBUG_LOG_CXXFLAGS =
else
DEBUG_LOG_CXXFLAGS = -D JS80P_DEBUG_LOG=$(DEBUG_LOG)
endif

FST_DIR = $(DIST_DIR_PREFIX)-fst
VST3_DIR = $(DIST_DIR_PREFIX)-vst3_single_file

VSTXML = $(DIST_DIR_BASE)/js80p.vstxml
VSTXMLGEN = $(DEV_DIR)/vstxmlgen$(DEV_EXE)

UPGRADE_PATCH = $(DEV_DIR)/upgrade-patch$(DEV_EXE)

.PHONY: \
	all \
	check \
	check_basic \
	check_dsp \
	check_param \
	check_synth \
	clean \
	dirs \
	docs \
	fst \
	gui_playground \
	log_tables_error_tsv \
	perf \
	show_fst_dir \
	show_versions \
	show_vst3_dir \
	static_analysis \
	test_example \
	upgrade_patch \
	vst3

all: dirs fst vst3

include make/$(DEV_OS)-$(TARGET_PLATFORM).mk

OBJ_TARGET_FST_MAIN = $(BUILD_DIR)/fst-main.o
OBJ_TARGET_FST_PLUGIN = $(BUILD_DIR)/fst-plugin.o
OBJ_TARGET_VST3_MAIN = $(BUILD_DIR)/vst3-main.o
OBJ_TARGET_VST3_PLUGIN = $(BUILD_DIR)/vst3-plugin.o
OBJ_TARGET_BANK = $(BUILD_DIR)/bank.o
OBJ_TARGET_SERIALIZER = $(BUILD_DIR)/serializer.o
OBJ_TARGET_SYNTH = $(BUILD_DIR)/synth.o
OBJ_TARGET_GUI = $(BUILD_DIR)/gui.o
OBJ_TARGET_MTS_ESP = $(BUILD_DIR)/mts-esp.o

OBJ_TARGET_GUI_PLAYGROUND = $(BUILD_DIR)/gui-playground.o

OBJ_DEV_FST_PLUGIN = $(DEV_DIR)/fst-plugin.o
OBJ_DEV_BANK = $(DEV_DIR)/bank.o
OBJ_DEV_GUI_STUB = $(DEV_DIR)/gui-stub.o
OBJ_DEV_MTS_ESP = $(DEV_DIR)/mts-esp.o
OBJ_DEV_SERIALIZER = $(DEV_DIR)/serializer.o
OBJ_DEV_SYNTH = $(DEV_DIR)/synth.o
OBJ_DEV_UPGRADE_PATCH = $(DEV_DIR)/upgrade-patch.o
OBJ_DEV_VSTXMLGEN = $(DEV_DIR)/vstxmlgen.o

OBJ_DEV_TEST_BANK = $(DEV_DIR)/test_bank.o
OBJ_DEV_TEST_GUI = $(DEV_DIR)/test_gui.o
OBJ_DEV_TEST_SERIALIZER = $(DEV_DIR)/test_serializer.o

TEST_OBJS = \
	$(OBJ_DEV_BANK) \
	$(OBJ_DEV_GUI_STUB) \
	$(OBJ_DEV_SERIALIZER) \
	$(OBJ_DEV_SYNTH) \
	$(OBJ_DEV_TEST_BANK) \
	$(OBJ_DEV_TEST_GUI) \
	$(OBJ_DEV_TEST_SERIALIZER)

FST_OBJS = \
	$(OBJ_TARGET_GUI_EXTRA) \
	$(OBJ_TARGET_FST_MAIN) \
	$(OBJ_TARGET_FST_PLUGIN) \
	$(OBJ_TARGET_GUI) \
	$(OBJ_TARGET_BANK) \
	$(OBJ_TARGET_SERIALIZER) \
	$(OBJ_TARGET_SYNTH) \
	$(OBJ_TARGET_MTS_ESP)

VST3_OBJS = \
	$(OBJ_TARGET_GUI_EXTRA) \
	$(OBJ_TARGET_VST3_MAIN) \
	$(OBJ_TARGET_VST3_PLUGIN) \
	$(OBJ_TARGET_GUI) \
	$(OBJ_TARGET_BANK) \
	$(OBJ_TARGET_SERIALIZER) \
	$(OBJ_TARGET_SYNTH) \
	$(OBJ_TARGET_MTS_ESP)

GUI_PLAYGROUND_OBJS = \
	$(OBJ_TARGET_GUI_EXTRA) \
	$(OBJ_TARGET_GUI) \
	$(OBJ_TARGET_GUI_PLAYGROUND) \
	$(OBJ_TARGET_SERIALIZER) \
	$(OBJ_TARGET_SYNTH)

UPGRADE_PATCH_OBJS = \
	$(OBJ_DEV_UPGRADE_PATCH) \
	$(OBJ_DEV_SYNTH) \
	$(OBJ_DEV_SERIALIZER)

VSTXMLGEN_OBJS = \
	$(OBJ_DEV_SYNTH) \
	$(OBJ_DEV_SERIALIZER) \
	$(OBJ_DEV_BANK) \
	$(OBJ_DEV_GUI_STUB) \
	$(OBJ_DEV_MTS_ESP) \
	$(OBJ_DEV_FST_PLUGIN) \
	$(OBJ_DEV_VSTXMLGEN)

PARAM_COMPONENTS = \
	dsp/envelope \
	dsp/lfo \
	dsp/lfo_envelope_list \
	dsp/macro \
	dsp/math \
	dsp/midi_controller \
	dsp/oscillator \
	dsp/param \
	dsp/queue \
	dsp/signal_producer

SYNTH_COMPONENTS = \
	synth \
	note_stack \
	spscqueue \
	voice \
	$(PARAM_COMPONENTS) \
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
	dsp/peak_tracker \
	dsp/reverb \
	dsp/side_chain_compressable_effect \
	dsp/wavefolder \
	dsp/wavetable

TESTS_BASIC = \
	test_math \
	test_queue \
	test_signal_producer

TESTS_PARAM = \
	test_envelope \
	test_lfo \
	test_lfo_envelope_list \
	test_macro \
	test_midi_controller \
	test_oscillator \
	test_param

TESTS_DSP = \
	test_biquad_filter \
	test_biquad_filter_slow \
	test_delay \
	test_distortion \
	test_gain \
	test_mixer \
	test_param_slow \
	test_peak_tracker \
	test_wavefolder

TESTS_SYNTH = \
	test_note_stack \
	test_renderer \
	test_spscqueue \
	test_synth \
	test_voice

TESTS = \
	$(TESTS_BASIC) \
	$(TESTS_DSP) \
	$(TESTS_PARAM) \
	$(TESTS_SYNTH) \
	test_gui \
	test_bank \
	test_midi \
	test_serializer

PERF_TESTS = \
	chord \
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

SERIALIZER_HEADERS = src/serializer.hpp

SERIALIZER_SOURCES = src/serializer.cpp

BANK_HEADERS = \
	src/bank.hpp \
	$(SERIALIZER_HEADERS)

BANK_SOURCES = \
	src/bank.cpp \
	src/programs.cpp

JS80P_HEADERS = \
	src/gui/gui.hpp \
	src/gui/widgets.hpp \
	src/mtsesp.hpp \
	src/renderer.hpp \
	$(BANK_HEADERS) \
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

VSTXMLGEN_SOURCES = src/plugin/fst/vstxmlgen.cpp

VST3_HEADERS = \
	$(JS80P_HEADERS) \
	src/plugin/vst3/plugin.hpp

VST3_SOURCES = \
	src/plugin/vst3/plugin.cpp \
	$(VST3_MAIN_SOURCES) \
	$(JS80P_SOURCES)

GUI_COMMON_HEADERS = $(JS80P_HEADERS)

GUI_COMMON_SOURCES = \
	src/gui/widgets.cpp \
	src/gui/gui.cpp

GUI_HEADERS = \
	$(GUI_TARGET_PLATFORM_HEADERS) \
	$(JS80P_HEADERS)

GUI_SOURCES = \
	$(GUI_TARGET_PLATFORM_SOURCES) \
	$(GUI_COMMON_SOURCES)

GUI_STUB_HEADERS = \
	$(GUI_COMMON_HEADERS)

GUI_STUB_SOURCES = \
	src/gui/stub.cpp \
	$(GUI_COMMON_SOURCES)

UPGRADE_PATCH_SOURCES = src/upgrade_patch.cpp

MTS_ESP_SOURCES = lib/mtsesp/Client/libMTSClient.cpp
MTS_ESP_HEADERS = lib/mtsesp/Client/libMTSClient.h

CPPCHECK_DONE = $(BUILD_DIR)/cppcheck-done.txt

TEST_LIBS = \
	tests/test.cpp \
	tests/utils.hpp \
	tests/utils.cpp

TEST_CPPS = $(foreach TEST,$(TESTS),tests/$(TEST).cpp)
TEST_BINS = $(foreach TEST,$(TESTS),$(DEV_DIR)/$(TEST)$(DEV_EXE))
TEST_BASIC_BINS = $(foreach TEST,$(TESTS_BASIC),$(DEV_DIR)/$(TEST)$(DEV_EXE))
TEST_DSP_BINS = $(foreach TEST,$(TESTS_DSP),$(DEV_DIR)/$(TEST)$(DEV_EXE))
TEST_PARAM_BINS = $(foreach TEST,$(TESTS_PARAM),$(DEV_DIR)/$(TEST)$(DEV_EXE))
TEST_SYNTH_BINS = $(foreach TEST,$(TESTS_SYNTH),$(DEV_DIR)/$(TEST)$(DEV_EXE))
PERF_TEST_BINS = $(foreach TEST,$(PERF_TESTS),$(DEV_DIR)/$(TEST)$(DEV_EXE))

TEST_CXXFLAGS = \
	-D TEST_MAX_ARRAY_PRINT=$(TEST_MAX_ARRAY_PRINT) \
	-D JS80P_ASSERTIONS=1 \
	-I./tests \
	-g

MTS_ESP_CXXFLAGS = \
	-Wno-array-bounds \
	-Wno-char-subscripts

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
	-Wno-unknown-pragmas \
	-Wno-unused-value

COMPILE_TARGET = \
	$(CPP_TARGET_PLATFORM) \
		$(TARGET_PLATFORM_CXXINCS) \
		$(JS80P_CXXINCS) $(JS80P_CXXFLAGS) \
		$(TARGET_PLATFORM_CXXFLAGS) \
		$(DEBUG_LOG_CXXFLAGS)

COMPILE_FST = \
	$(CPP_TARGET_PLATFORM) \
		$(TARGET_PLATFORM_CXXINCS) \
		$(FST_CXXINCS) $(FST_CXXFLAGS) \
		$(TARGET_PLATFORM_CXXFLAGS) \
		$(DEBUG_LOG_CXXFLAGS)

COMPILE_VST3 = \
	$(CPP_TARGET_PLATFORM) \
		$(TARGET_PLATFORM_CXXINCS) \
		$(VST3_CXXINCS) $(VST3_CXXFLAGS) \
		$(TARGET_PLATFORM_CXXFLAGS) \
		$(DEBUG_LOG_CXXFLAGS)

COMPILE_DEV = \
	$(CPP_DEV_PLATFORM) \
		$(JS80P_CXXINCS) $(JS80P_CXXFLAGS) \
		$(TEST_CXXFLAGS) \
		$(DEBUG_LOG_CXXFLAGS)

RUN_WITH_VALGRIND = $(VALGRIND) $(VALGRIND_FLAGS)

show_fst_dir:
	@echo $(FST_DIR)

show_vst3_dir:
	@echo $(VST3_DIR)

fst: $(FST) $(VSTXML)

vst3: $(VST3)

vstxml: $(VSTXML)

dirs: $(BUILD_DIR) $(DEV_DIR) $(API_DOC_DIR) $(FST_DIR) $(VST3_DIR)

$(BUILD_DIR) $(DEV_DIR): | $(BUILD_DIR_BASE)
	$(MKDIR) $@

$(BUILD_DIR_BASE) $(DIST_DIR_BASE) $(API_DOC_DIR):
	$(MKDIR) $@

clean:
	$(RM) \
		$(CPPCHECK_DONE) \
		$(DEV_DIR)/chord.o \
		$(DEV_PLATFORM_CLEAN) \
		$(FST) \
		$(FST_OBJS) \
		$(GUI_PLAYGROUND) \
		$(GUI_PLAYGROUND_OBJS) \
		$(PERF_TEST_BINS) \
		$(TEST_BINS) \
		$(TEST_OBJS) \
		$(UPGRADE_PATCH) \
		$(UPGRADE_PATCH_OBJS) \
		$(VST3) \
		$(VST3_OBJS) \
		$(VSTXML) \
		$(VSTXMLGEN) \
		$(VSTXMLGEN_OBJS)
	$(RM) $(API_DOC_DIR)/html/*.* $(API_DOC_DIR)/html/search/*.*

check: \
	perf \
	$(CPPCHECK_DONE) \
	upgrade_patch \
	$(TEST_LIBS) \
	$(TEST_BINS) \
	| $(DEV_DIR)

check_basic: perf $(TEST_LIBS) $(TEST_BASIC_BINS) | $(DEV_DIR)
check_dsp: perf $(TEST_LIBS) $(TEST_DSP_BINS) | $(DEV_DIR)
check_param: perf $(TEST_LIBS) $(TEST_PARAM_BINS) | $(DEV_DIR)
check_synth: perf $(TEST_LIBS) $(TEST_SYNTH_BINS) | $(DEV_DIR)

test_example: $(DEV_DIR)/test_example$(DEV_EXE) | $(DEV_DIR)

perf: $(PERF_TEST_BINS) | $(DEV_DIR)

log_tables_error_tsv: $(DEV_DIR)/log_tables_error_tsv$(DEV_EXE)

docs: Doxyfile $(API_DOC_DIR) $(API_DOC_DIR)/html/index.html

gui_playground: $(GUI_PLAYGROUND)

static_analysis: $(CPPCHECK_DONE)

$(CPPCHECK_DONE): \
		$(FST_HEADERS) \
		$(FST_SOURCES) \
		$(GUI_HEADERS) \
		$(GUI_PLAYGROUND_SOURCES) \
		$(GUI_SOURCES) \
		$(JS80P_HEADERS) \
		$(JS80P_SOURCES) \
		$(MTS_ESP_HEADERS) \
		$(MTS_ESP_SOURCES) \
		$(UPGRADE_PATCH_SOURCES) \
		$(VST3_HEADERS) \
		$(VST3_SOURCES) \
		$(VSTXMLGEN_SOURCES) \
		$(TEST_CPPS) \
		$(TEST_LIBS) \
		| $(BUILD_DIR) show_versions
	$(CPPCHECK) $(CPPCHECK_FLAGS) src/ tests/
	echo > $@

upgrade_patch: $(UPGRADE_PATCH)

$(API_DOC_DIR)/html/index.html: \
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
		| $(API_DOC_DIR)
	$(DOXYGEN)

$(FST): $(FST_EXTRA) $(FST_OBJS) | $(FST_DIR) show_versions
	$(LINK_FST) $^ -o $@ $(TARGET_PLATFORM_LFLAGS)

$(VST3): $(VST3_EXTRA) $(VST3_OBJS) | $(VST3_DIR) show_versions
	$(LINK_VST3) $^ -o $@ $(TARGET_PLATFORM_LFLAGS)

$(FST_DIR) $(VST3_DIR): | $(DIST_DIR_BASE)
	$(MKDIR) $@

$(GUI_PLAYGROUND): $(GUI_PLAYGROUND_OBJS) | $(BUILD_DIR) show_versions
	$(LINK_TARGET_EXE) $^ -o $@ $(TARGET_PLATFORM_LFLAGS)

$(OBJ_TARGET_GUI_PLAYGROUND): \
		$(GUI_PLAYGROUND_SOURCES) \
		$(GUI_SOURCES) $(GUI_HEADERS) \
		| $(BUILD_DIR)
	$(COMPILE_TARGET) -c -o $@ $<

$(UPGRADE_PATCH): $(UPGRADE_PATCH_OBJS) | $(DEV_DIR) show_versions
	$(LINK_DEV_EXE) $^ -o $@

$(OBJ_DEV_UPGRADE_PATCH): $(UPGRADE_PATCH_SOURCES) | $(DEV_DIR)
	$(COMPILE_DEV) -c -o $@ $<

$(OBJ_TARGET_SYNTH): $(SYNTH_SOURCES) $(SYNTH_HEADERS) | $(BUILD_DIR)
	$(COMPILE_TARGET) -c -o $@ $<

$(OBJ_DEV_SYNTH): $(SYNTH_SOURCES) $(SYNTH_HEADERS) | $(DEV_DIR)
	$(COMPILE_DEV) -c -o $@ $<

$(OBJ_TARGET_BANK): \
		$(BANK_SOURCES) $(BANK_HEADERS) $(SYNTH_HEADERS) | $(BUILD_DIR)
	$(COMPILE_TARGET) -c -o $@ $<

$(OBJ_DEV_BANK): $(BANK_SOURCES) $(BANK_HEADERS) $(SYNTH_HEADERS) | $(DEV_DIR)
	$(COMPILE_DEV) -c -o $@ $<

$(OBJ_TARGET_SERIALIZER): \
		$(SERIALIZER_SOURCES) $(SERIALIZER_HEADERS) $(SYNTH_HEADERS) \
		| $(BUILD_DIR)
	$(COMPILE_TARGET) -c -o $@ $<

$(OBJ_DEV_SERIALIZER): \
		$(SERIALIZER_SOURCES) $(SERIALIZER_HEADERS) $(SYNTH_HEADERS) \
		| $(DEV_DIR)
	$(COMPILE_DEV) -c -o $@ $<

$(OBJ_TARGET_GUI): $(GUI_SOURCES) $(GUI_HEADERS) | $(BUILD_DIR)
	$(COMPILE_TARGET) -c -o $@ $<

$(OBJ_DEV_GUI_STUB): $(GUI_STUB_SOURCES) $(GUI_STUB_HEADERS) | $(DEV_DIR)
	$(COMPILE_DEV) -c -o $@ $<

$(OBJ_TARGET_MTS_ESP) : $(MTS_ESP_SOURCES) $(MTS_ESP_HEADERS) | $(BUILD_DIR)
	$(COMPILE_TARGET) $(MTS_ESP_CXXFLAGS) -c -o $@ $<

$(OBJ_DEV_MTS_ESP) : $(MTS_ESP_SOURCES) $(MTS_ESP_HEADERS) | $(DEV_DIR)
	$(COMPILE_DEV) $(MTS_ESP_CXXFLAGS) -c -o $@ $<

$(OBJ_TARGET_FST_PLUGIN): src/plugin/fst/plugin.cpp $(FST_HEADERS) | $(BUILD_DIR)
	$(COMPILE_FST) -c -o $@ $<

$(OBJ_DEV_FST_PLUGIN): src/plugin/fst/plugin.cpp $(FST_HEADERS) | $(DEV_DIR)
	$(CPP_DEV_PLATFORM) \
		$(FST_CXXINCS) $(FST_CXXFLAGS) $(DEBUG_LOG_CXXFLAGS) -c -o $@ $<

$(OBJ_TARGET_FST_MAIN): $(FST_MAIN_SOURCES) $(FST_HEADERS) | $(BUILD_DIR)
	$(COMPILE_FST) -c -o $@ $<

$(OBJ_TARGET_VST3_PLUGIN): $(VST3_PLUGIN_SOURCES) $(VST3_HEADERS) | $(BUILD_DIR)
	$(COMPILE_VST3) -c -o $@ $<

$(OBJ_TARGET_VST3_MAIN): \
		$(VST3_MAIN_SOURCES) \
		src/plugin/vst3/vst3.cpp \
		$(VST3_HEADERS) \
		| $(BUILD_DIR)
	$(COMPILE_VST3) -c -o $@ $<

$(VSTXML): $(VSTXMLGEN) | $(DIST_DIR_BASE)
	$(VSTXMLGEN) $(VSTXML)

$(VSTXMLGEN): $(VSTXMLGEN_OBJS) | $(DEV_DIR) show_versions
	$(LINK_DEV_EXE) $^ -o $@

$(OBJ_DEV_VSTXMLGEN): $(VSTXMLGEN_SOURCES) $(FST_HEADERS) | $(DEV_DIR)
	$(COMPILE_DEV) \
		$(FST_CXXINCS) $(FST_CXXFLAGS) $(MTS_ESP_CXXFLAGS) \
		-c -o $@ $<

$(DEV_DIR)/chord$(DEV_EXE): \
		$(DEV_DIR)/chord.o \
		$(OBJ_DEV_SYNTH) $(OBJ_DEV_SERIALIZER) $(OBJ_DEV_BANK) \
		| $(DEV_DIR) show_versions
	$(LINK_DEV_EXE) $^ -o $@

$(DEV_DIR)/chord.o: \
		tests/performance/chord.cpp $(JS80P_HEADERS) | $(DEV_DIR)
	$(COMPILE_DEV) -c -o $@ $<

$(DEV_DIR)/perf_math$(DEV_EXE): \
		tests/performance/perf_math.cpp \
		src/dsp/math.hpp src/dsp/math.cpp \
		src/js80p.hpp \
		| $(DEV_DIR) show_versions
	$(COMPILE_DEV) -o $@ $<

$(DEV_DIR)/log_tables_error_tsv$(DEV_EXE): \
		scripts/log_tables_error_tsv.cpp \
		src/dsp/math.cpp src/dsp/math.hpp \
		src/js80p.hpp \
		| $(DEV_DIR) show_versions
	$(COMPILE_DEV) -o $@ $<

$(DEV_DIR)/test_example$(DEV_EXE): \
		tests/test_example.cpp $(TEST_LIBS) | $(DEV_DIR) show_versions
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_bank$(DEV_EXE): \
		$(OBJ_DEV_BANK) \
		$(OBJ_DEV_SERIALIZER) \
		$(OBJ_DEV_SYNTH) \
		$(OBJ_DEV_TEST_BANK) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS) $(TEST_DSP_BINS) $(TEST_PARAM_BINS) $(TEST_SYNTH_BINS)
	$(LINK_DEV_EXE) $^ -o $@
	$(RUN_WITH_VALGRIND) $@

$(OBJ_DEV_TEST_BANK): \
		tests/test_bank.cpp \
		$(BANK_HEADERS) $(SYNTH_HEADERS) \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions
	$(COMPILE_DEV) -c -o $@ $<

$(DEV_DIR)/test_biquad_filter$(DEV_EXE): \
		tests/test_biquad_filter.cpp \
		src/dsp/biquad_filter.cpp src/dsp/biquad_filter.hpp \
		src/dsp/filter.cpp src/dsp/filter.hpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS) $(TEST_PARAM_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_biquad_filter_slow$(DEV_EXE): \
		tests/test_biquad_filter_slow.cpp \
		src/dsp/biquad_filter.cpp src/dsp/biquad_filter.hpp \
		src/dsp/filter.cpp src/dsp/filter.hpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS) $(TEST_PARAM_BINS)
	$(COMPILE_DEV) -o $@ $<
	$@

$(DEV_DIR)/test_delay$(DEV_EXE): \
		tests/test_delay.cpp \
		src/dsp/delay.cpp src/dsp/delay.hpp \
		src/dsp/filter.cpp src/dsp/filter.hpp \
		src/dsp/biquad_filter.cpp src/dsp/biquad_filter.hpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS) $(TEST_PARAM_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_distortion$(DEV_EXE): \
		tests/test_distortion.cpp \
		src/dsp/distortion.cpp src/dsp/distortion.hpp \
		src/dsp/filter.cpp src/dsp/filter.hpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS) $(TEST_PARAM_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_envelope$(DEV_EXE): \
		tests/test_envelope.cpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_gain$(DEV_EXE): \
		tests/test_gain.cpp \
		src/dsp/gain.cpp src/dsp/gain.hpp \
		src/dsp/filter.cpp src/dsp/filter.hpp \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS) $(TEST_PARAM_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_gui$(DEV_EXE): \
		$(OBJ_DEV_GUI_STUB) \
		$(OBJ_DEV_SERIALIZER) \
		$(OBJ_DEV_SYNTH) \
		$(OBJ_DEV_TEST_GUI) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS) $(TEST_DSP_BINS) $(TEST_PARAM_BINS) $(TEST_SYNTH_BINS)
	$(LINK_DEV_EXE) $^ -o $@
	$(RUN_WITH_VALGRIND) $@

$(OBJ_DEV_TEST_GUI): \
		tests/test_gui.cpp $(GUI_COMMON_HEADERS) $(TEST_LIBS) | $(DEV_DIR)
	$(COMPILE_DEV) -c -o $@ $<

$(DEV_DIR)/test_lfo$(DEV_EXE): \
		tests/test_lfo.cpp \
		src/dsp/wavetable.cpp src/dsp/wavetable.hpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_lfo_envelope_list$(DEV_EXE): \
		tests/test_lfo_envelope_list.cpp \
		src/dsp/lfo_envelope_list.cpp src/dsp/lfo_envelope_list.hpp \
		src/js80p.hpp \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_macro$(DEV_EXE): \
		tests/test_macro.cpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_math$(DEV_EXE): \
		tests/test_math.cpp \
		src/dsp/math.cpp src/dsp/math.hpp \
		src/js80p.hpp \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions
	$(COMPILE_DEV) -o $@ $<
	$@

$(DEV_DIR)/test_midi$(DEV_EXE): \
		tests/test_midi.cpp src/midi.hpp src/js80p.hpp \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_midi_controller$(DEV_EXE): \
		tests/test_midi_controller.cpp \
		src/dsp/midi_controller.cpp src/dsp/midi_controller.hpp \
		src/dsp/queue.cpp src/dsp/queue.hpp \
		src/js80p.hpp \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_mixer$(DEV_EXE): \
		tests/test_mixer.cpp \
		src/dsp/mixer.cpp src/dsp/mixer.hpp \
		src/dsp/signal_producer.cpp src/dsp/signal_producer.hpp \
		src/js80p.hpp \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS) $(TEST_PARAM_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_note_stack$(DEV_EXE): \
		tests/test_note_stack.cpp \
		src/note_stack.hpp src/note_stack.cpp \
		src/js80p.hpp \
		src/midi.hpp \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_oscillator$(DEV_EXE): \
		tests/test_oscillator.cpp \
		src/dsp/wavetable.cpp src/dsp/wavetable.hpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_param$(DEV_EXE): \
		tests/test_param.cpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_param_slow$(DEV_EXE): \
		tests/test_param_slow.cpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS) $(TEST_PARAM_BINS)
	$(COMPILE_DEV) -o $@ $<
	$@

$(DEV_DIR)/test_peak_tracker$(DEV_EXE): \
		tests/test_peak_tracker.cpp \
		src/dsp/peak_tracker.cpp src/dsp/peak_tracker.hpp \
		src/js80p.hpp \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_queue$(DEV_EXE): \
		tests/test_queue.cpp \
		src/dsp/queue.cpp src/dsp/queue.hpp \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_renderer$(DEV_EXE): \
		tests/test_renderer.cpp \
		src/renderer.hpp \
		$(TEST_LIBS) \
		$(SYNTH_HEADERS) \
		$(SYNTH_SOURCES) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS) $(TEST_DSP_BINS) $(TEST_PARAM_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_serializer$(DEV_EXE): \
		$(OBJ_DEV_SERIALIZER) \
		$(OBJ_DEV_SYNTH) \
		$(OBJ_DEV_TEST_SERIALIZER) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS) $(TEST_DSP_BINS) $(TEST_PARAM_BINS) $(TEST_SYNTH_BINS)
	$(LINK_DEV_EXE) $^ -o $@
	$(RUN_WITH_VALGRIND) $@

$(OBJ_DEV_TEST_SERIALIZER): \
		tests/test_serializer.cpp \
		$(SERIALIZER_HEADERS) $(SYNTH_HEADERS) \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions
	$(COMPILE_DEV) -c -o $@ $<

$(DEV_DIR)/test_signal_producer$(DEV_EXE): \
		tests/test_signal_producer.cpp \
		src/dsp/queue.cpp src/dsp/queue.hpp \
		src/dsp/signal_producer.cpp src/dsp/signal_producer.hpp \
		src/js80p.hpp \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_spscqueue$(DEV_EXE): \
		tests/test_spscqueue.cpp \
		src/spscqueue.hpp src/spscqueue.cpp \
		src/js80p.hpp \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_synth$(DEV_EXE): \
		tests/test_synth.cpp \
		$(TEST_LIBS) \
		$(SYNTH_HEADERS) \
		$(SYNTH_SOURCES) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS) $(TEST_DSP_BINS) $(TEST_PARAM_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_voice$(DEV_EXE): \
		tests/test_voice.cpp \
		src/voice.cpp src/voice.hpp \
		src/midi.hpp \
		src/dsp/biquad_filter.cpp src/dsp/biquad_filter.hpp \
		src/dsp/distortion.cpp src/dsp/distortion.hpp \
		src/dsp/filter.cpp src/dsp/filter.hpp \
		src/dsp/wavefolder.cpp src/dsp/wavefolder.hpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS) $(TEST_DSP_BINS) $(TEST_PARAM_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@

$(DEV_DIR)/test_wavefolder$(DEV_EXE): \
		tests/test_wavefolder.cpp \
		src/dsp/filter.cpp src/dsp/filter.hpp \
		src/dsp/wavefolder.cpp src/dsp/wavefolder.hpp \
		$(PARAM_HEADERS) $(PARAM_SOURCES) \
		$(TEST_LIBS) \
		| $(DEV_DIR) show_versions \
		$(TEST_BASIC_BINS)
	$(COMPILE_DEV) -o $@ $<
	$(RUN_WITH_VALGRIND) $@
