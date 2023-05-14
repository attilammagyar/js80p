/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023  Attila M. Magyar
 *
 * JS80P is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * JS80P is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cstring>

#include "plugin/fst/plugin.hpp"

#include "midi.hpp"
#include "serializer.hpp"


namespace JS80P
{

static const std::string BANK_PROG_SEPARATOR_START{"[Prg_"};
static const std::string BANK_PROG_SEPARATOR_END{"]\r\n"};

static const std::string WHITESPACE{"\t\n\v\f\r "};
void trim(std::string& str) {
    str.erase(str.find_last_not_of(WHITESPACE) + 1); // right trim
    str.erase(0, str.find_first_not_of(WHITESPACE)); // left trim
}

static constexpr int FST_OP_CODE_NAMES_LEN = 255;

static constexpr char const* FST_OP_CODE_NAMES[FST_OP_CODE_NAMES_LEN] = {
    "Open", // 0
    "Close", // 1
    "SetProgram", // 2
    "GetProgram", // 3
    "SetProgramName", // 4
    "GetProgramName", // 5
    "GetParamLabel", // 6
    "GetParamDisplay", // 7
    "GetParamName", // 8
    "UNKNOWN",
    "SetSampleRate", // 10
    "SetBlockSize", // 11
    "MainsChanged", // 12
    "EditGetRect", // 13
    "EditOpen", // 14
    "EditClose", // 15
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "EditIdle", // 19
    "UNKNOWN",
    "UNKNOWN",
    "Identify", // 22
    "GetChunk", // 23
    "SetChunk", // 24
    "ProcessEvents", // 25
    "CanBeAutomated", // 26
    "String2Parameter", // 27
    "UNKNOWN",
    "GetProgramNameIndexed", // 29
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "GetInputProperties", // 33
    "GetOutputProperties", // 34
    "GetPlugCategory", // 35
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "SetSpeakerArrangement", // 42
    "UNKNOWN",
    "UNKNOWN",
    "GetEffectName", // 45
    "UNKNOWN",
    "GetVendorString", // 47
    "GetProductString", // 48
    "GetVendorVersion", // 49
    "VendorSpecific", // 50
    "CanDo", // 51
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "GetVstVersion", // 58
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "GetCurrentMidiProgram", // 63
    "UNKNOWN",
    "UNKNOWN",
    "GetMidiNoteName", // 66
    "UNKNOWN",
    "UNKNOWN",
    "GetSpeakerArrangement", // 69
    "ShellGetNextPlugin", // 70
    "StartProcess", // 71
    "StopProcess", // 72
    "SetTotalSampleToProcess", // 73
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "SetProcessPrecision", // 77
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
    "UNKNOWN",
};

const FstPlugin::float_param_infos_t FstPlugin::float_param_infos {
    // Synth - Global
      FloatParamInfo{"MIX Oh"}
    , FloatParamInfo{"PM Oh", 100.0 / Constants::PM_MAX}
    , FloatParamInfo{"FM Oh", 100.0 / Constants::FM_MAX}
    , FloatParamInfo{"AM Oh my god, will", 100.0 / Constants::AM_MAX}
    // Synth - Modulator (Oscillator 1)
    , FloatParamInfo{"MAMP"}
    , FloatParamInfo{"MVS"}
    , FloatParamInfo{"MFLD", 100.0 / Constants::FOLD_MAX}
    , FloatParamInfo{"MPRT", 1.0, "%.3f", "s"}
    , FloatParamInfo{"MPRD", 1.0, "%.1f", "c"}
    , FloatParamInfo{"MDTN", Constants::DETUNE_SCALE, "%.f", "st"}
    , FloatParamInfo{"MFIN", 1.0, "%.1f", "c"}
    , FloatParamInfo{"MWID"}
    , FloatParamInfo{"MPAN"}
    , FloatParamInfo{"MVOL"}
    // Synth - Modulator Custom Waveform Harmonics 1 - 10
    , FloatParamInfo{"MC1"}
    , FloatParamInfo{"MC2"}
    , FloatParamInfo{"MC3"}
    , FloatParamInfo{"MC4"}
    , FloatParamInfo{"MC5"}
    , FloatParamInfo{"MC6"}
    , FloatParamInfo{"MC7"}
    , FloatParamInfo{"MC8"}
    , FloatParamInfo{"MC9"}
    , FloatParamInfo{"MC10"}
    // Synth - Modulator Filters 1 & 2
    , FloatParamInfo{"MF1FRQ", 1.0, "%.1f", "Hz"}
    , FloatParamInfo{"MF1Q", 1.0, "%.2f", ""}
    , FloatParamInfo{"MF1G", 1.0, "%.2f", "dB"}
    , FloatParamInfo{"MF2FRQ", 1.0, "%.1f", "Hz"}
    , FloatParamInfo{"MF2Q", 1.0, "%.2f", ""}
    , FloatParamInfo{"MF2G", 1.0, "%.2f", "dB"}

    // Synth - Carrier Oscillator 1
    , FloatParamInfo{"CAMP"}
    , FloatParamInfo{"CVS"}
    , FloatParamInfo{"CFLD", 100.0 / Constants::FOLD_MAX}
    , FloatParamInfo{"CPRT", 1.0, "%.3f", "s"}
    , FloatParamInfo{"CPRD", 1.0, "%.1f", "c"}
    , FloatParamInfo{"CDTN", 0.01, "%.f", "st"}
    , FloatParamInfo{"CFIN", 1.0, "%.1f", "c"}
    , FloatParamInfo{"CWID"}
    , FloatParamInfo{"CPAN"}
    , FloatParamInfo{"CVOL"}
    // Synth - Carrier Custom Waveform Harmonics 1 - 10
    , FloatParamInfo{"CC1"}
    , FloatParamInfo{"CC2"}
    , FloatParamInfo{"CC3"}
    , FloatParamInfo{"CC4"}
    , FloatParamInfo{"CC5"}
    , FloatParamInfo{"CC6"}
    , FloatParamInfo{"CC7"}
    , FloatParamInfo{"CC8"}
    , FloatParamInfo{"CC9"}
    , FloatParamInfo{"CC10"}
    // Synth - Carrier Filters 1 & 2
    , FloatParamInfo{"CF1FRQ", 1.0, "%.1f", "Hz"}
    , FloatParamInfo{"CF1Q", 1.0, "%.2f", ""}
    , FloatParamInfo{"CF1G", 1.0, "%.2f", "dB"}
    , FloatParamInfo{"CF2FRQ", 1.0, "%.1f", "Hz"}
    , FloatParamInfo{"CF2Q", 1.0, "%.2f", ""}
    , FloatParamInfo{"CF2G", 1.0, "%.2f", "dB"}

    // Effects - Global
    , FloatParamInfo{"EOG"}
    , FloatParamInfo{"EDG"}
    // Effects - Filters 1 & 2
    , FloatParamInfo{"EF1FRQ", 1.0, "%.1f", "Hz"}
    , FloatParamInfo{"EF1Q", 1.0, "%.2f", ""}
    , FloatParamInfo{"EF1G", 1.0, "%.2f", "dB"}
    , FloatParamInfo{"EF2FRQ", 1.0, "%.1f", "Hz"}
    , FloatParamInfo{"EF2Q", 1.0, "%.2f", ""}
    , FloatParamInfo{"EF2G", 1.0, "%.2f", "dB"}
    // Effects - Echo
    , FloatParamInfo{"EEDEL", 1.0, "%.3f", "s"}
    , FloatParamInfo{"EEFB"}
    , FloatParamInfo{"EEDF", 1.0, "%.1f", "Hz"}
    , FloatParamInfo{"EEDG", 1.0, "%.2f", "dB"}
    , FloatParamInfo{"EEWID"}
    , FloatParamInfo{"EEHPF", 1.0, "%.1f", "Hz"}
    , FloatParamInfo{"EEWET"}
    , FloatParamInfo{"EEDRY"}
    // Effects - Reverb
    , FloatParamInfo{"ERRS"}
    , FloatParamInfo{"ERDF", 1.0, "%.1f", "Hz"}
    , FloatParamInfo{"ERDG", 1.0, "%.2f", "Db"}
    , FloatParamInfo{"ERWID"}
    , FloatParamInfo{"ERHPF", 1.0, "%.1f", "Hz"}
    , FloatParamInfo{"ERWET"}
    , FloatParamInfo{"ERDRY"}

    // Controllers - Flexible Controllers 1 - 10
    , FloatParamInfo{"F1IN"}
    , FloatParamInfo{"F1MIN"}
    , FloatParamInfo{"F1MAX"}
    , FloatParamInfo{"F1AMT"}
    , FloatParamInfo{"F1DST"}
    , FloatParamInfo{"F1RND"}
    , FloatParamInfo{"F2IN"}
    , FloatParamInfo{"F2MIN"}
    , FloatParamInfo{"F2MAX"}
    , FloatParamInfo{"F2AMT"}
    , FloatParamInfo{"F2DST"}
    , FloatParamInfo{"F2RND"}
    , FloatParamInfo{"F3IN"}
    , FloatParamInfo{"F3MIN"}
    , FloatParamInfo{"F3MAX"}
    , FloatParamInfo{"F3AMT"}
    , FloatParamInfo{"F3DST"}
    , FloatParamInfo{"F3RND"}
    , FloatParamInfo{"F4IN"}
    , FloatParamInfo{"F4MIN"}
    , FloatParamInfo{"F4MAX"}
    , FloatParamInfo{"F4AMT"}
    , FloatParamInfo{"F4DST"}
    , FloatParamInfo{"F4RND"}
    , FloatParamInfo{"F5IN"}
    , FloatParamInfo{"F5MIN"}
    , FloatParamInfo{"F5MAX"}
    , FloatParamInfo{"F5AMT"}
    , FloatParamInfo{"F5DST"}
    , FloatParamInfo{"F5RND"}
    , FloatParamInfo{"F6IN"}
    , FloatParamInfo{"F6MIN"}
    , FloatParamInfo{"F6MAX"}
    , FloatParamInfo{"F6AMT"}
    , FloatParamInfo{"F6DST"}
    , FloatParamInfo{"F6RND"}
    , FloatParamInfo{"F7IN"}
    , FloatParamInfo{"F7MIN"}
    , FloatParamInfo{"F7MAX"}
    , FloatParamInfo{"F7AMT"}
    , FloatParamInfo{"F7DST"}
    , FloatParamInfo{"F7RND"}
    , FloatParamInfo{"F8IN"}
    , FloatParamInfo{"F8MIN"}
    , FloatParamInfo{"F8MAX"}
    , FloatParamInfo{"F8AMT"}
    , FloatParamInfo{"F8DST"}
    , FloatParamInfo{"F8RND"}
    , FloatParamInfo{"F9IN"}
    , FloatParamInfo{"F9MIN"}
    , FloatParamInfo{"F9MAX"}
    , FloatParamInfo{"F9AMT"}
    , FloatParamInfo{"F9DST"}
    , FloatParamInfo{"F9RND"}
    , FloatParamInfo{"F10IN"}
    , FloatParamInfo{"F10MIN"}
    , FloatParamInfo{"F10MAX"}
    , FloatParamInfo{"F10AMT"}
    , FloatParamInfo{"F10DST"}
    , FloatParamInfo{"F10RND"}

    // Envelopes 1 - 6
    , FloatParamInfo{"N1AMT"}
    , FloatParamInfo{"N1INI"}
    , FloatParamInfo{"N1DEL", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N1ATK", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N1PK"}
    , FloatParamInfo{"N1HLD", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N1DEC", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N1SUS"}
    , FloatParamInfo{"N1REL", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N1FIN"}
    , FloatParamInfo{"N2AMT"}
    , FloatParamInfo{"N2INI"}
    , FloatParamInfo{"N2DEL", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N2ATK", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N2PK"}
    , FloatParamInfo{"N2HLD", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N2DEC", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N2SUS"}
    , FloatParamInfo{"N2REL", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N2FIN"}
    , FloatParamInfo{"N3AMT"}
    , FloatParamInfo{"N3INI"}
    , FloatParamInfo{"N3DEL", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N3ATK", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N3PK"}
    , FloatParamInfo{"N3HLD", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N3DEC", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N3SUS"}
    , FloatParamInfo{"N3REL", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N3FIN"}
    , FloatParamInfo{"N4AMT"}
    , FloatParamInfo{"N4INI"}
    , FloatParamInfo{"N4DEL", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N4ATK", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N4PK"}
    , FloatParamInfo{"N4HLD", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N4DEC", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N4SUS"}
    , FloatParamInfo{"N4REL", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N4FIN"}
    , FloatParamInfo{"N5AMT"}
    , FloatParamInfo{"N5INI"}
    , FloatParamInfo{"N5DEL", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N5ATK", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N5PK"}
    , FloatParamInfo{"N5HLD", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N5DEC", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N5SUS"}
    , FloatParamInfo{"N5REL", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N5FIN"}
    , FloatParamInfo{"N6AMT"}
    , FloatParamInfo{"N6INI"}
    , FloatParamInfo{"N6DEL", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N6ATK", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N6PK"}
    , FloatParamInfo{"N6HLD", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N6DEC", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N6SUS"}
    , FloatParamInfo{"N6REL", 1.0, "%.3f", "s"}
    , FloatParamInfo{"N6FIN"}

    // LFOs 1 - 8
    , FloatParamInfo{"L1FRQ", 1.0, "%.2f", "Hz"}
    , FloatParamInfo{"L1PHS", 360.0, "%.1f", "deg"}
    , FloatParamInfo{"L1MIN"}
    , FloatParamInfo{"L1MAX"}
    , FloatParamInfo{"L1AMT", 200.0}
    , FloatParamInfo{"L1DST"}
    , FloatParamInfo{"L1RND"}
    , FloatParamInfo{"L2FRQ", 1.0, "%.2f", "Hz"}
    , FloatParamInfo{"L2PHS", 360.0, "%.1f", "deg"}
    , FloatParamInfo{"L2MIN"}
    , FloatParamInfo{"L2MAX"}
    , FloatParamInfo{"L2AMT", 200.0}
    , FloatParamInfo{"L2DST"}
    , FloatParamInfo{"L2RND"}
    , FloatParamInfo{"L3FRQ", 1.0, "%.2f", "Hz"}
    , FloatParamInfo{"L3PHS", 360.0, "%.1f", "deg"}
    , FloatParamInfo{"L3MIN"}
    , FloatParamInfo{"L3MAX"}
    , FloatParamInfo{"L3AMT", 200.0}
    , FloatParamInfo{"L3DST"}
    , FloatParamInfo{"L3RND"}
    , FloatParamInfo{"L4FRQ", 1.0, "%.2f", "Hz"}
    , FloatParamInfo{"L4PHS", 360.0, "%.1f", "deg"}
    , FloatParamInfo{"L4MIN"}
    , FloatParamInfo{"L4MAX"}
    , FloatParamInfo{"L4AMT", 200.0}
    , FloatParamInfo{"L4DST"}
    , FloatParamInfo{"L4RND"}
    , FloatParamInfo{"L5FRQ", 1.0, "%.2f", "Hz"}
    , FloatParamInfo{"L5PHS", 360.0, "%.1f", "deg"}
    , FloatParamInfo{"L5MIN"}
    , FloatParamInfo{"L5MAX"}
    , FloatParamInfo{"L5AMT", 200.0}
    , FloatParamInfo{"L5DST"}
    , FloatParamInfo{"L5RND"}
    , FloatParamInfo{"L6FRQ", 1.0, "%.2f", "Hz"}
    , FloatParamInfo{"L6PHS", 360.0, "%.1f", "deg"}
    , FloatParamInfo{"L6MIN"}
    , FloatParamInfo{"L6MAX"}
    , FloatParamInfo{"L6AMT", 200.0}
    , FloatParamInfo{"L6DST"}
    , FloatParamInfo{"L6RND"}
    , FloatParamInfo{"L7FRQ", 1.0, "%.2f", "Hz"}
    , FloatParamInfo{"L7PHS", 360.0, "%.1f", "deg"}
    , FloatParamInfo{"L7MIN"}
    , FloatParamInfo{"L7MAX"}
    , FloatParamInfo{"L7AMT", 200.0}
    , FloatParamInfo{"L7DST"}
    , FloatParamInfo{"L7RND"}
    , FloatParamInfo{"L8FRQ", 1.0, "%.2f", "Hz"}
    , FloatParamInfo{"L8PHS", 360.0, "%.1f", "deg"}
    , FloatParamInfo{"L8MIN"}
    , FloatParamInfo{"L8MAX"}
    , FloatParamInfo{"L8AMT", 200.0}
    , FloatParamInfo{"L8DST"}
    , FloatParamInfo{"L8RND"}
};

#ifdef N_T_C
const FstPlugin::options_t modes {
    "Mix&Mod"
    , "Split C3"
    , "Split Db3"
    , "Split D3"
    , "Split Eb3"
    , "Split E3"
    , "Split F3"
    , "Split Gb3"
    , "Split G3"
    , "Split Ab3"
    , "Split A3"
    , "Split Bb3"
    , "Split B3"
    , "Split C4"
};
const FstPlugin::options_t waveforms {
    "Sine"
    , "Saw"
    , "Soft Sw"
    , "Inv Saw"
    , "Soft I S"
    , "Triangle"
    , "Soft Tri"
    , "Square"
    , "Soft Sqr"
    , "Custom"

};
const FstPlugin::options_t biquad_filter_types {
    "LP"
    , "HP"
    , "BP"
    , "Notch"
    , "Bell"
    , "LS"
    , "HS"
};

const FstPlugin::int_param_infos_t FstPlugin::int_param_infos {
    // Synth - Global - Mode
      IntParamInfo{"MODE", &FstPlugin::modes}
    // Synth - Modulator (Oscillator 1) Waveform
    , IntParamInfo{"MWAV", &FstPlugin::waveforms}
    // Synth - Carrier (Oscillator 2) Waveform
    , IntParamInfo{"CWAV", &FstPlugin::waveforms}
    // Synth - Modulator Filters 1 & 2 Types
    , IntParamInfo{"MF1TYP", &FstPlugin::biquad_filter_types}
    , IntParamInfo{"MF2TYP", &FstPlugin::biquad_filter_types}
    // Synth - Carrier Filters 1 & 2 Types
    , IntParamInfo{"CF1TYP", &FstPlugin::biquad_filter_types}
    , IntParamInfo{"CF2TYP", &FstPlugin::biquad_filter_types}
    // Effects - Filters 1 & 2 Types
    , IntParamInfo{"EF1TYP", &FstPlugin::biquad_filter_types}
    , IntParamInfo{"EF2TYP", &FstPlugin::biquad_filter_types}
    // LFOs - LFO 1 - 8 Waveforms
    , IntParamInfo{"L1WAV", &FstPlugin::waveforms}
    , IntParamInfo{"L2WAV", &FstPlugin::waveforms}
    , IntParamInfo{"L3WAV", &FstPlugin::waveforms}
    , IntParamInfo{"L4WAV", &FstPlugin::waveforms}
    , IntParamInfo{"L5WAV", &FstPlugin::waveforms}
    , IntParamInfo{"L6WAV", &FstPlugin::waveforms}
    , IntParamInfo{"L7WAV", &FstPlugin::waveforms}
    , IntParamInfo{"L8WAV", &FstPlugin::waveforms}
};

#else   // #ifdef N_T_C
const FstPlugin::int_param_infos_t FstPlugin::int_param_infos {
    // Synth - Global - Mode
      IntParamInfo{"MODE", JS80P::GUI::MODES, JS80P::GUI::MODES_COUNT}
    // Synth - Modulator (Oscillator 1) Waveform
    , IntParamInfo{"MWAV", JS80P::GUI::WAVEFORMS, JS80P::GUI::WAVEFORMS_COUNT}
    // Synth - Carrier (Oscillator 2) Waveform
    , IntParamInfo{"CWAV", JS80P::GUI::WAVEFORMS, JS80P::GUI::WAVEFORMS_COUNT}
    // Synth - Modulator Filters 1 & 2 Types
    , IntParamInfo{"MF1TYP", JS80P::GUI::BIQUAD_FILTER_TYPES, JS80P::GUI::BIQUAD_FILTER_TYPES_COUNT}
    , IntParamInfo{"MF2TYP", JS80P::GUI::BIQUAD_FILTER_TYPES, JS80P::GUI::BIQUAD_FILTER_TYPES_COUNT}
    // Synth - Carrier Filters 1 & 2 Types
    , IntParamInfo{"CF1TYP", JS80P::GUI::BIQUAD_FILTER_TYPES, JS80P::GUI::BIQUAD_FILTER_TYPES_COUNT}
    , IntParamInfo{"CF2TYP", JS80P::GUI::BIQUAD_FILTER_TYPES, JS80P::GUI::BIQUAD_FILTER_TYPES_COUNT}
    // Effects - Filters 1 & 2 Types
    , IntParamInfo{"EF1TYP", JS80P::GUI::BIQUAD_FILTER_TYPES, JS80P::GUI::BIQUAD_FILTER_TYPES_COUNT}
    , IntParamInfo{"EF2TYP", JS80P::GUI::BIQUAD_FILTER_TYPES, JS80P::GUI::BIQUAD_FILTER_TYPES_COUNT}
    // LFOs - LFO 1 - 8 Waveforms
    , IntParamInfo{"L1WAV", JS80P::GUI::WAVEFORMS, JS80P::GUI::WAVEFORMS_COUNT}
    , IntParamInfo{"L2WAV", JS80P::GUI::WAVEFORMS, JS80P::GUI::WAVEFORMS_COUNT}
    , IntParamInfo{"L3WAV", JS80P::GUI::WAVEFORMS, JS80P::GUI::WAVEFORMS_COUNT}
    , IntParamInfo{"L4WAV", JS80P::GUI::WAVEFORMS, JS80P::GUI::WAVEFORMS_COUNT}
    , IntParamInfo{"L5WAV", JS80P::GUI::WAVEFORMS, JS80P::GUI::WAVEFORMS_COUNT}
    , IntParamInfo{"L6WAV", JS80P::GUI::WAVEFORMS, JS80P::GUI::WAVEFORMS_COUNT}
    , IntParamInfo{"L7WAV", JS80P::GUI::WAVEFORMS, JS80P::GUI::WAVEFORMS_COUNT}
    , IntParamInfo{"L8WAV", JS80P::GUI::WAVEFORMS, JS80P::GUI::WAVEFORMS_COUNT}
};
#endif  // #ifdef N_T_C

AEffect* FstPlugin::create_instance(
        audioMasterCallback const host_callback,
        GUI::PlatformData const platform_data
) noexcept {
    AEffect* effect = new AEffect();

    FstPlugin* fst_plugin = new FstPlugin(effect, host_callback, platform_data);

    memset(effect, 0, sizeof(AEffect));

    effect->dispatcher = &dispatch;
    effect->flags = (
        effFlagsHasEditor
        | effFlagsIsSynth
        | effFlagsCanReplacing
        | effFlagsCanDoubleReplacing
        | effFlagsProgramChunks
    );
    effect->magic = kEffectMagic;
    effect->numInputs = 0;
    effect->numOutputs = (t_fstInt32)FstPlugin::OUT_CHANNELS;
    effect->numPrograms = (t_fstInt32)FstPlugin::NO_OF_PROGRAMS;
    effect->numParams = (t_fstInt32)FstPlugin::NO_OF_PARAMS;
    effect->object = (void*)fst_plugin;
    effect->process = &process_accumulating;
    effect->processReplacing = &process_replacing;
    effect->processDoubleReplacing = &process_double_replacing;
    effect->getParameter = &get_parameter;
    effect->setParameter = &set_parameter;
    effect->uniqueID = CCONST('a', 'm', 'j', '8');
    effect->version = FstPlugin::VERSION;

    return effect;
}


VstIntPtr VSTCALLBACK FstPlugin::dispatch(
        AEffect* effect,
        VstInt32 op_code,
        VstInt32 index,
        VstIntPtr ivalue,
        void* pointer,
        float fvalue
) {
    JS80P::FstPlugin* fst_plugin = (JS80P::FstPlugin*)effect->object;

    // if (
            // true
            // && op_code != effEditIdle
            // && op_code != effProcessEvents
            // && op_code != 53
            // && op_code != effGetProgram
            // && op_code != effEditGetRect
    // ) {
        // fprintf(
            // stderr,
            // "op_code=%d, op_code_name=%s, index=%d, ivalue=%d, fvalue=%f\n",
            // (int)op_code,
            // ((op_code < FST_OP_CODE_NAMES_LEN) ? FST_OP_CODE_NAMES[op_code] : "???"),
            // (int)index,
            // (int)ivalue,
            // fvalue
        // );
    // }

    switch (op_code) {
        case effProcessEvents:
            fst_plugin->process_events((VstEvents*)pointer);
            return 1;

        case effClose:
            delete fst_plugin;
            return 0;

        case effSetProgram:
            fst_plugin->set_program((size_t)ivalue);
            return 0;

        case effGetProgram:
            return fst_plugin->get_program();

        case effSetProgramName:
            fst_plugin->set_program_name((const char*)pointer);
            return 0;

        case effGetProgramName:
            fst_plugin->get_program_name((char*)pointer);
            return 0;

        case effGetProgramNameIndexed:
            return (VstIntPtr)fst_plugin->get_program_name_indexed((char*)pointer, (size_t)index);

        case effGetParamLabel:
            fst_plugin->get_parameter_label((size_t)index, (char*)pointer);
            return 0;

        case effGetParamDisplay:
            fst_plugin->get_parameter_display((size_t)index, (char*)pointer);
            return 0;

        case effGetParamName:
            fst_plugin->get_parameter_name((size_t)index, (char*)pointer);
            return 0;

        case effCanBeAutomated:
            return (VstIntPtr)fst_plugin->can_parameter_be_automated((size_t)index);

        case effSetSampleRate:
            fst_plugin->set_sample_rate(fvalue);
            return 0;

        case effSetBlockSize:
            fst_plugin->set_block_size(ivalue);
            return 0;

        case effMainsChanged:
            if (ivalue) {
                fst_plugin->resume();
            } else {
                fst_plugin->suspend();
            }
            return 0;

        case effEditGetRect:
            *((ERect**)pointer) = &fst_plugin->window_rect;
            return (VstIntPtr)pointer;

        case effEditOpen:
            fst_plugin->open_gui((GUI::PlatformWidget)pointer);
            return 1;

        case effEditIdle:
            fst_plugin->gui_idle();
            return 0;

        case effEditClose:
            fst_plugin->close_gui();
            return 0;

        case effGetChunk:
            return fst_plugin->get_chunk((void**)pointer, index ? true : false);

        case effSetChunk:
            fst_plugin->set_chunk((void const*)pointer, ivalue, index ? true : false);
            return 0;

        case effGetPlugCategory:
            return kPlugCategSynth;

        case effGetEffectName:
        case effGetProductString:
            strncpy((char*)pointer, Constants::PLUGIN_NAME, 8);
            return 1;

        case effGetVendorString:
            strncpy((char*)pointer, Constants::COMPANY_NAME, 24);
            return 1;

        case effGetVendorVersion:
            return FstPlugin::VERSION;

        case effGetVstVersion:
            return kVstVersion;

        case effIdentify:
            return CCONST('N', 'v', 'E', 'f');

        case effCanDo:
            if (strcmp("receiveVstMidiEvent", (char const*)pointer) == 0) {
                return 1;
            }

            // JS80P_DEBUG(
                // "op_code=%d, op_code_name=%s, index=%d, ivalue=%d, fvalue=%f, pointer=%s",
                // (int)op_code,
                // ((op_code < FST_OP_CODE_NAMES_LEN) ? FST_OP_CODE_NAMES[op_code] : "???"),
                // (int)index,
                // (int)ivalue,
                // fvalue,
                // (char*)pointer
            // );
            return 0;

        default:
            return 0;
    }

    return 0;
}


void VSTCALLBACK FstPlugin::process_accumulating(
        AEffect* effect,
        float** indata,
        float** outdata,
        VstInt32 frames
) {
    JS80P::FstPlugin* fst_plugin = (JS80P::FstPlugin*)effect->object;

    fst_plugin->generate_and_add_samples(frames, outdata);
}


void VSTCALLBACK FstPlugin::process_replacing(
        AEffect* effect,
        float** indata,
        float** outdata,
        VstInt32 frames
) {
    JS80P::FstPlugin* fst_plugin = (JS80P::FstPlugin*)effect->object;

    fst_plugin->generate_samples<float>(frames, outdata);
}


void VSTCALLBACK FstPlugin::process_double_replacing(
        AEffect* effect,
        double** indata,
        double** outdata,
        VstInt32 frames
) {
    JS80P::FstPlugin* fst_plugin = (JS80P::FstPlugin*)effect->object;

    fst_plugin->generate_samples<double>(frames, outdata);
}

float VSTCALLBACK FstPlugin::get_parameter(
        AEffect* effect,
        VstInt32 index
) {
    JS80P::FstPlugin* fst_plugin = static_cast<JS80P::FstPlugin*>(effect->object);

    return fst_plugin->get_parameter(static_cast<size_t>(index));
}

void VSTCALLBACK FstPlugin::set_parameter(
        AEffect* effect,
        VstInt32 index,
        float fvalue
) {
    JS80P::FstPlugin* fst_plugin = static_cast<JS80P::FstPlugin*>(effect->object);

    fst_plugin->set_parameter(static_cast<size_t>(index), fvalue);
}



FstPlugin::FstPlugin(
        AEffect* const effect,
        audioMasterCallback const host_callback,
        GUI::PlatformData const platform_data
) noexcept
    : synth(),
    effect(effect),
    host_callback(host_callback),
    platform_data(platform_data),
    round(0),
    gui(NULL)
{
    window_rect.top = 0;
    window_rect.left = 0;
    window_rect.bottom = GUI::HEIGHT;
    window_rect.right = GUI::WIDTH;

    std::string program{Serializer::serialize(&synth)};
    serialized_programs.fill(program);
}


FstPlugin::~FstPlugin()
{
    close_gui();
}


void FstPlugin::set_sample_rate(float const new_sample_rate) noexcept
{
    synth.set_sample_rate((Frequency)new_sample_rate);
}


void FstPlugin::set_block_size(VstIntPtr const new_block_size) noexcept
{
    synth.set_block_size((Integer)new_block_size);
}


void FstPlugin::suspend() noexcept
{
    synth.suspend();
}


void FstPlugin::resume() noexcept
{
    synth.resume();
    host_callback(effect, audioMasterWantMidi, 0, 1, NULL, 0.0f);
}


void FstPlugin::process_events(VstEvents const* const events) noexcept
{
    VstEvent* event = NULL;

    for (VstInt32 i = 0; i < events->numEvents; ++i) {
        event = events->events[i];

        if (event->type == kVstMidiType) {
            process_midi_event((VstMidiEvent*)event);
        }
    }
}


void FstPlugin::process_midi_event(VstMidiEvent const* const event) noexcept
{
    Seconds const time_offset = (
        synth.sample_count_to_time_offset((Integer)event->deltaFrames)
    );

    Midi::Dispatcher::dispatch<Synth>(
        synth, time_offset, (Midi::Byte*)event->midiData
    );
}


template<typename NumberType>
void FstPlugin::generate_samples(
        VstInt32 const sample_count,
        NumberType** samples
) noexcept {
    if (sample_count < 1) {
        return;
    }

    Sample const* const* buffer = render_next_round(sample_count);

    for (Integer c = 0; c != Synth::OUT_CHANNELS; ++c) {
        for (VstInt32 i = 0; i != sample_count; ++i) {
            samples[c][i] = (NumberType)buffer[c][i];
        }
    }
}


Sample const* const* FstPlugin::render_next_round(VstInt32 sample_count) noexcept
{
    round = (round + 1) & ROUND_MASK;

    return synth.generate_samples(round, (Integer)sample_count);
}


void FstPlugin::generate_and_add_samples(
        VstInt32 const sample_count,
        float** samples
) noexcept {
    if (sample_count < 1) {
        return;
    }

    Sample const* const* buffer = render_next_round(sample_count);

    for (Integer c = 0; c != Synth::OUT_CHANNELS; ++c) {
        for (VstInt32 i = 0; i != sample_count; ++i) {
            samples[c][i] += (float)buffer[c][i];
        }
    }
}


void FstPlugin::import_serialized_program(const std::string& serialized_program) noexcept
{
    synth.process_messages();
    Serializer::import(&synth, serialized_program);
    synth.process_messages();
}


VstIntPtr FstPlugin::get_chunk(void** chunk, bool isPreset) noexcept
{
    serialized_programs[current_program_index] = Serializer::serialize(&synth); // This is not only important for 'isPreset',
                                                                                // but also for whole bank state, because for the
                                                                                // current program, modifications as well as e.g.
                                                                                // a patch load via js80p GUI could have happend!
    if (isPreset) {
        *chunk = (void*)serialized_programs[current_program_index].c_str();
        return (VstIntPtr)serialized_programs[current_program_index].size();
    } else {
        serialized_bank.clear();
        for (size_t i{0}; i < NO_OF_PROGRAMS; ++i) {
            serialized_bank += BANK_PROG_SEPARATOR_START + std::to_string(i) + BANK_PROG_SEPARATOR_END;
            serialized_bank += serialized_programs[i];
        }
        *chunk = (void*)serialized_bank.c_str();
        return (VstIntPtr)serialized_bank.size();
    }
}


void FstPlugin::set_chunk(void const* chunk, VstIntPtr const size, bool isPreset) noexcept
{
    std::string serialized((char const*)chunk, (std::string::size_type)size);
    store_state_of_previous_program_in_set_program = false;
    if (isPreset) {
        serialized_program_names[current_program_index] = "";       // It will be 'calculated' on demand!
        serialized_programs[current_program_index] = serialized;
        import_serialized_program(serialized);
    } else {
        serialized_program_names.fill("");                          // They will be 'calculated' on demand!
        std::string::size_type searchStart(0);
        for (size_t i{0}; i < NO_OF_PROGRAMS; ++i) {
            const std::string searchString1{BANK_PROG_SEPARATOR_START + std::to_string(i) + BANK_PROG_SEPARATOR_END};
            auto indexStart{serialized.find(searchString1, searchStart)};
            if (std::string::npos == indexStart) {
                return;     	// Something is seriously wrong here!
            }
            indexStart += searchString1.length();
            const std::string searchString2{BANK_PROG_SEPARATOR_START + std::to_string(i + 1) + BANK_PROG_SEPARATOR_END};
            const auto indexEnd{serialized.find(searchString2, indexStart)};
            if (std::string::npos == indexEnd) {
                if (i != NO_OF_PROGRAMS - 1) {
                    return;     // Something is seriously wrong here!
                }
            }
            const std::string serialized_program{serialized.substr(indexStart, indexEnd - indexStart)};
            serialized_programs[i] = serialized_program;
            if (i == current_program_index) {
                import_serialized_program(serialized_program);
            }
            searchStart = indexEnd;
        }
    }
}


VstIntPtr FstPlugin::get_program() const noexcept
{
    return static_cast<VstIntPtr>(current_program_index);
}


void FstPlugin::set_program(size_t index) noexcept
{
    if (index < NO_OF_PROGRAMS && index != current_program_index) {
        if (store_state_of_previous_program_in_set_program) {
            // Store  state of current program (which soon will be previous program!)
            serialized_programs[current_program_index] = Serializer::serialize(&synth);
        } else {
            store_state_of_previous_program_in_set_program = true;
        }
        synth.process_messages();
        Serializer::import(&synth, serialized_programs[index]);
        synth.process_messages();
        current_program_index = index;
    }
}


void FstPlugin::get_program_name_short(char* name, size_t index) noexcept
{
    std::string program_name{serialized_program_names[index].substr(0, kVstMaxProgNameLen - 1)};
    trim(program_name);
    size_t i{0};
    for (; i < program_name.length(); ++i) {
        name[i] = program_name.data()[i];
    }
    name[i] = '\0';
}


bool FstPlugin::get_program_name_indexed(char* name, size_t index) noexcept
{
    if (index < NO_OF_PROGRAMS) {
        if (serialized_program_names[index].empty()) {
            static const std::string searchString(Serializer::PROG_NAME_LINE_TAG);
            auto startIndex{serialized_programs[index].find(searchString)};
            if (std::string::npos != startIndex) {
                startIndex += searchString.length();
                auto endIndex{serialized_programs[index].find("\r\n", startIndex)};
                if (std::string::npos != endIndex) {
                    serialized_program_names[index] = serialized_programs[index].substr(startIndex, endIndex - startIndex);
                }
            }
            if (serialized_program_names[index].empty()) {
                serialized_program_names[index] = "???";
            }
        }
        get_program_name_short(name, index);
        return true;
    }
    return false;
}


void FstPlugin::get_program_name(char* name) noexcept
{
    serialized_program_names[current_program_index] = synth.get_program_name(); // Get current name from synth.
    // 'get_program_name_short' will only work correctly with properly updated 'serialized_program_names[index]'.
    get_program_name_short(name, current_program_index);
}


void FstPlugin::set_program_name(const char* name)
{
    serialized_program_names[current_program_index] = name;
    trim(serialized_program_names[current_program_index]);
    synth.set_program_name(serialized_program_names[current_program_index]);
}


float FstPlugin::get_parameter(size_t index) const noexcept
{
    return synth.get_param_ratio_atomic(static_cast<Synth::ParamId>(index));
}


void FstPlugin::set_parameter(size_t index, float fvalue) noexcept
{
    synth.push_message(Synth::MessageType::SET_PARAM, static_cast<Synth::ParamId>(index), fvalue, 0);
}


void FstPlugin::get_parameter_label(size_t index, char* label) const noexcept
{
    // Use only 'label' to display something like e.g. 'Sine', 'Soft Tri' for 'MWAV'.
    // Use 'label' together with 'display' to display something like '2400.00 Hz' where 'Hz' would be the 'label'.
    Synth::ParamId ParamId{static_cast<Synth::ParamId>(index)};
    if (ParamId < Synth::FLOAT_PARAMS) {
        snprintf(label, 9, "%s", float_param_infos[index].label.c_str());
    } else {
        label[0] = '\0';
        //snprintf(label, 9, "%s", "Hmmm");
    }
}


void FstPlugin::get_parameter_display(size_t index, char* display) const noexcept
{
    Synth::ParamId ParamId{static_cast<Synth::ParamId>(index)};
    Number ratio{synth.get_param_ratio_atomic(ParamId)};
    if (ParamId < Synth::FLOAT_PARAMS) {
        const auto& param_info{float_param_infos[index]};
        snprintf(display, 9, param_info.format.c_str(), synth.float_param_ratio_to_display_value(ParamId, ratio) * param_info.scale);
        //snprintf(display, 9, "%.3f", synth.float_param_ratio_to_display_value(ParamId, ratio));
    } else {
        const auto& param_info{int_param_infos[index - Synth::FLOAT_PARAMS]};
        auto value{synth.int_param_ratio_to_display_value(ParamId, ratio)};
#ifdef N_T_C
        if (value < param_info.options->size()) {
            snprintf(display, 9, "%s", param_info.options->at(value).c_str());
#else
        if (value < param_info.number_of_options) {
            snprintf(display, 9, "%s", param_info.options[value]);
#endif
        } else {
            strncpy(display, "???", 4);
        }

        //snprintf(display, 9, "%d", synth.int_param_ratio_to_display_value(ParamId, ratio));
        //display[0] = '\0';
    }
}


void FstPlugin::get_parameter_name(size_t index, char* name) const noexcept
{
#ifdef N_T_C
    const std::string param_name{synth.get_param_name(static_cast<Synth::ParamId>(index)).substr(0, Constants::PARAM_NAME_MAX_LENGTH)};
#else
    Synth::ParamId ParamId{static_cast<Synth::ParamId>(index)};
    std::string param_name;
    if (ParamId < Synth::FLOAT_PARAMS) {
        param_name = float_param_infos[index].name.substr(0, 15);
    } else {
        param_name = synth.get_param_name(static_cast<Synth::ParamId>(index)).substr(0, Constants::PARAM_NAME_MAX_LENGTH);
    }
#endif
    size_t i{0};
    for (; i < param_name.length(); ++i) {
        name[i] = param_name.data()[i];
    }
    name[i] = '\0';
}


bool FstPlugin::can_parameter_be_automated(size_t index) const noexcept
{
    return true;
}


void FstPlugin::open_gui(GUI::PlatformWidget parent_window)
{
    close_gui();
    gui = new GUI(platform_data, parent_window, &synth, false);
    gui->show();
}


void FstPlugin::gui_idle()
{
    /*
    Some hosts (e.g. Ardour 5.12.0) send an effEditIdle message before sending
    the first effEditOpen.
    */
    if (gui == NULL) {
        return;
    }

    gui->idle();
}


void FstPlugin::close_gui()
{
    if (gui == NULL) {
        return;
    }

    delete gui;

    gui = NULL;
}

}
