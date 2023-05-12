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
    effect->object = (void*)fst_plugin;
    effect->process = &process_accumulating;
    effect->processReplacing = &process_replacing;
    effect->processDoubleReplacing = &process_double_replacing;
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
