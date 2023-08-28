/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023  Attila M. Magyar
 * Copyright (C) 2023  Patrik Ehringer
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

#include "serializer.hpp"


namespace JS80P
{

static constexpr int FST_OP_CODE_NAMES_LEN = 78;

static constexpr char const* FST_OP_CODE_NAMES[FST_OP_CODE_NAMES_LEN] = {
    "Open",                         /*    0 */
    "Close",                        /*    1 */
    "SetProgram",                   /*    2 */
    "GetProgram",                   /*    3 */
    "SetProgramName",               /*    4 */
    "GetProgramName",               /*    5 */
    "GetParamLabel",                /*    6 */
    "GetParamDisplay",              /*    7 */
    "GetParamName",                 /*    8 */
    "UNKNOWN-9",                    /*    9 */
    "SetSampleRate",                /*   10 */
    "SetBlockSize",                 /*   11 */
    "MainsChanged",                 /*   12 */
    "EditGetRect",                  /*   13 */
    "EditOpen",                     /*   14 */
    "EditClose",                    /*   15 */
    "UNKNOWN-16",                   /*   16 */
    "UNKNOWN-17",                   /*   17 */
    "UNKNOWN-18",                   /*   18 */
    "EditIdle",                     /*   19 */
    "UNKNOWN-20",                   /*   20 */
    "UNKNOWN-21",                   /*   21 */
    "Identify",                     /*   22 */
    "GetChunk",                     /*   23 */
    "SetChunk",                     /*   24 */
    "ProcessEvents",                /*   25 */
    "CanBeAutomated",               /*   26 */
    "String2Parameter",             /*   27 */
    "UNKNOWN-28",                   /*   28 */
    "GetProgramNameIndexed",        /*   29 */
    "UNKNOWN-30",                   /*   30 */
    "UNKNOWN-31",                   /*   31 */
    "UNKNOWN-32",                   /*   32 */
    "GetInputProperties",           /*   33 */
    "GetOutputProperties",          /*   34 */
    "GetPlugCategory",              /*   35 */
    "UNKNOWN-36",                   /*   36 */
    "UNKNOWN-37",                   /*   37 */
    "UNKNOWN-38",                   /*   38 */
    "UNKNOWN-39",                   /*   39 */
    "UNKNOWN-40",                   /*   40 */
    "UNKNOWN-41",                   /*   41 */
    "SetSpeakerArrangement",        /*   42 */
    "UNKNOWN-43",                   /*   43 */
    "UNKNOWN-44",                   /*   44 */
    "GetEffectName",                /*   45 */
    "UNKNOWN-46",                   /*   46 */
    "GetVendorString",              /*   47 */
    "GetProductString",             /*   48 */
    "GetVendorVersion",             /*   49 */
    "VendorSpecific",               /*   50 */
    "CanDo",                        /*   51 */
    "UNKNOWN-52",                   /*   52 */
    "UNKNOWN-53",                   /*   53 */
    "UNKNOWN-54",                   /*   54 */
    "UNKNOWN-55",                   /*   55 */
    "UNKNOWN-56",                   /*   56 */
    "UNKNOWN-57",                   /*   57 */
    "GetVstVersion",                /*   58 */
    "UNKNOWN-59",                   /*   59 */
    "UNKNOWN-60",                   /*   60 */
    "UNKNOWN-61",                   /*   61 */
    "UNKNOWN-62",                   /*   62 */
    "GetCurrentMidiProgram",        /*   63 */
    "UNKNOWN-64",                   /*   64 */
    "UNKNOWN-65",                   /*   65 */
    "GetMidiNoteName",              /*   66 */
    "UNKNOWN-67",                   /*   67 */
    "UNKNOWN-68",                   /*   68 */
    "GetSpeakerArrangement",        /*   69 */
    "ShellGetNextPlugin",           /*   70 */
    "StartProcess",                 /*   71 */
    "StopProcess",                  /*   72 */
    "SetTotalSampleToProcess",      /*   73 */
    "UNKNOWN-74",                   /*   74 */
    "UNKNOWN-75",                   /*   75 */
    "UNKNOWN-76",                   /*   76 */
    "SetProcessPrecision",          /*   77 */
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
    effect->numOutputs = (VstInt32)FstPlugin::OUT_CHANNELS;
    effect->numPrograms = (VstInt32)Bank::NUMBER_OF_PROGRAMS;
    effect->numParams = NUMBER_OF_PARAMETERS;
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
            // && (op_code != effProcessEvents || fst_plugin->prev_logged_op_code != effProcessEvents)
            // && op_code != 53
            // && op_code != effGetProgram
            // && op_code != effEditGetRect
            // && op_code != effGetProgramNameIndexed
    // ) {
        // fst_plugin->prev_logged_op_code = op_code;
        // JS80P_DEBUG(
            // "plugin=%p, op_code=%d, op_code_name=%s, index=%d, ivalue=%d, fvalue=%f",
            // effect->object,
            // (int)op_code,
            // ((op_code < FST_OP_CODE_NAMES_LEN) ? FST_OP_CODE_NAMES[op_code] : "???"),
            // (int)index,
            // (int)ivalue,
            // fvalue
        // );
    // }

    switch (op_code) {
        case effProcessEvents:
            fst_plugin->process_vst_events((VstEvents*)pointer);
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
            return fst_plugin->get_program_name((char*)pointer, (size_t)index);

        case effGetParamLabel:
            fst_plugin->get_param_label((size_t)index, (char*)pointer);
            return 0;

        case effGetParamDisplay:
            fst_plugin->get_param_display((size_t)index, (char*)pointer);
            return 0;

        case effGetParamName:
            fst_plugin->get_param_name((size_t)index, (char*)pointer);
            return 0;

        case effCanBeAutomated:
            return 1;

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


float VSTCALLBACK FstPlugin::get_parameter(AEffect* effect, VstInt32 index)
{
    JS80P::FstPlugin* fst_plugin = (JS80P::FstPlugin*)effect->object;

    return fst_plugin->get_parameter((size_t)index);
}


void VSTCALLBACK FstPlugin::set_parameter(
        AEffect* effect,
        VstInt32 index,
        float fvalue
) {
    JS80P::FstPlugin* fst_plugin = (JS80P::FstPlugin*)effect->object;

    fst_plugin->set_parameter((size_t)index, fvalue);
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
    gui(NULL),
    renderer(synth),
    serialized_bank(""),
    next_program(0),
    min_samples_before_next_cc_ui_update(8192),
    remaining_samples_before_next_cc_ui_update(0),
    prev_logged_op_code(-1),
    save_current_patch_before_changing_program(false),
    had_midi_cc_event(false)
{
    window_rect.top = 0;
    window_rect.left = 0;
    window_rect.bottom = GUI::HEIGHT;
    window_rect.right = GUI::WIDTH;

    parameters[0] = Parameter("Program", NULL, Synth::ControllerId::NONE);
    parameters[1] = create_midi_ctl_param(
        Synth::ControllerId::PITCH_WHEEL, &synth.pitch_wheel
    );
    parameters[2] = create_midi_ctl_param(
        Synth::ControllerId::CHANNEL_PRESSURE, &synth.channel_pressure_ctl
    );

    size_t index = 3;

    for (Integer cc = 0; cc != Synth::MIDI_CONTROLLERS; ++cc) {
        Midi::Controller const midi_controller = (Midi::Controller)cc;

        if (!Synth::is_supported_midi_controller(midi_controller)) {
            continue;
        }

        /*
        The sustain pedal was added in v1.9.0, but if it was put in the middle
        of the list of exported parameters, then it could break DAW projects
        that have automations for parameters which come after it. In order to
        avoid such backward-incompatibility, we need to put it at the end.
        */
        if (midi_controller == Midi::SUSTAIN_PEDAL) {
            continue;
        }

        parameters[index] = create_midi_ctl_param(
            (Synth::ControllerId)cc,
            synth.midi_controllers[cc]
        );
        ++index;
    }

    parameters[index++] = create_midi_ctl_param(
        Synth::ControllerId::SUSTAIN_PEDAL,
        synth.midi_controllers[Synth::ControllerId::SUSTAIN_PEDAL]
    );
}


FstPlugin::~FstPlugin()
{
    close_gui();
}


void FstPlugin::set_sample_rate(float const new_sample_rate) noexcept
{
    if (new_sample_rate > HOST_CC_UI_UPDATE_FREQUENCY) {
        min_samples_before_next_cc_ui_update = 1 + (Integer)(
            new_sample_rate * HOST_CC_UI_UPDATE_FREQUENCY_INV
        );
        remaining_samples_before_next_cc_ui_update = min_samples_before_next_cc_ui_update;
    }

    synth.set_sample_rate((Frequency)new_sample_rate);
}


void FstPlugin::set_block_size(VstIntPtr const new_block_size) noexcept
{
    synth.set_block_size((Integer)new_block_size);
    renderer.reset();
}


void FstPlugin::suspend() noexcept
{
    synth.suspend();
    renderer.reset();
}


void FstPlugin::resume() noexcept
{
    synth.resume();
    renderer.reset();
    host_callback(effect, audioMasterWantMidi, 0, 1, NULL, 0.0f);
}


void FstPlugin::process_vst_events(VstEvents const* const events) noexcept
{
    for (VstInt32 i = 0; i < events->numEvents; ++i) {
        VstEvent* event = events->events[i];

        if (event->type == kVstMidiType) {
            process_vst_midi_event((VstMidiEvent*)event);
        }
    }

    if (had_midi_cc_event && remaining_samples_before_next_cc_ui_update == 0) {
        had_midi_cc_event = false;
        remaining_samples_before_next_cc_ui_update = min_samples_before_next_cc_ui_update;
        update_host_display();
    }
}


void FstPlugin::process_vst_midi_event(VstMidiEvent const* const event) noexcept
{
    Seconds const time_offset = (
        synth.sample_count_to_time_offset((Integer)event->deltaFrames)
    );
    Midi::Byte const* const midi_bytes = (Midi::Byte const*)event->midiData;

    Midi::Dispatcher::dispatch<FstPlugin>(*this, time_offset, midi_bytes);
    Midi::Dispatcher::dispatch<Synth>(synth, time_offset, midi_bytes);
}


template<typename NumberType>
void FstPlugin::generate_samples(
        VstInt32 const sample_count,
        NumberType** samples
) noexcept {
    if (sample_count < 1) {
        return;
    }

    prepare_rendering(sample_count);
    renderer.write_next_round<NumberType>(sample_count, samples);

    /*
    It would be nice to notify the host about param changes that originate from
    the plugin, but since these parameters only ever change due to MIDI CC
    messages, we don't want the host to record them both as MIDI CC and as
    parameter automation.
    */
    // for (size_t i = 0; i != NUMBER_OF_PARAMETERS; ++i) {
        // if (UNLIKELY(parameters[i].needs_host_update())) {
            // host_callback(
                // effect,
                // audioMasterAutomate,
                // (VstInt32)i,
                // 0,
                // NULL,
                // parameters[i].get_value()
            // );
        // }
    // }
}


void FstPlugin::prepare_rendering(Integer const sample_count) noexcept
{
    handle_program_change();
    handle_parameter_changes();
    update_bpm();

    if (had_midi_cc_event) {
        if (remaining_samples_before_next_cc_ui_update >= sample_count) {
            remaining_samples_before_next_cc_ui_update -= sample_count;
        } else {
            remaining_samples_before_next_cc_ui_update = 0;
        }
    }
}


void FstPlugin::handle_program_change() noexcept
{
    if (parameters[0].is_dirty()) {
        this->next_program = Bank::normalized_parameter_value_to_program_index(
            (Number)parameters[0].get_last_set_value()
        );

        parameters[0].clear();
    }

    size_t const next_program = this->next_program;
    size_t const current_program = bank.get_current_program_index();

    if (next_program == current_program) {
        return;
    }

    if (save_current_patch_before_changing_program) {
        bank[current_program].import(Serializer::serialize(synth));
    } else {
        save_current_patch_before_changing_program = true;
    }

    bank.set_current_program_index(next_program);
    import_patch(bank[next_program].serialize());
}


void FstPlugin::handle_parameter_changes() noexcept
{
    for (size_t i = 1; i != NUMBER_OF_PARAMETERS; ++i) {
        Parameter& parameter = parameters[i];

        if (LIKELY(!parameter.is_dirty())) {
            continue;
        }

        Midi::Controller const controller_id = parameter.get_controller_id();

        parameter.clear();

        if (Synth::is_supported_midi_controller(controller_id)) {
            /*
            Some hosts (e.g. FL Studio 21) swallow most MIDI CC messages, and
            the only way to make physical knobs and faders on a MIDI keyboard
            work in the plugin is to export parameters to which those MIDI CC
            messages can be assigned in the host, and then interpret the
            changes of these parameters as if the corresponding MIDI CC message
            had been received.
            */
            synth.control_change(
                0.0,
                0,
                controller_id,
                float_to_midi_byte(parameter.get_last_set_value())
            );
        } else {
            MidiController* const midi_controller = parameter.get_midi_controller();

            if (LIKELY(midi_controller != NULL)) {
                midi_controller->change(0.0, (Number)parameter.get_last_set_value());
            }
        }
    }
}


Midi::Byte FstPlugin::float_to_midi_byte(float const value) const noexcept
{
    return std::min(
        (Midi::Byte)127,
        std::max((Midi::Byte)0, (Midi::Byte)std::round(value * 127.0f))
    );
}


void FstPlugin::update_bpm() noexcept
{
    VstTimeInfo const* time_info = (
        (VstTimeInfo const*)host_callback(
            effect, audioMasterGetTime, 0, kVstTempoValid, NULL, 0.0f
        )
    );

    if (time_info == NULL || (time_info->flags & kVstTempoValid) == 0) {
        return;
    }

    synth.set_bpm((Number)time_info->tempo);
}


void FstPlugin::update_host_display() noexcept
{
    host_callback(effect, audioMasterUpdateDisplay, 0, 0, NULL, 0.0f);
}


void FstPlugin::generate_and_add_samples(
        VstInt32 const sample_count,
        float** samples
) noexcept {
    if (sample_count < 1) {
        return;
    }

    prepare_rendering(sample_count);
    renderer.add_next_round<float>(sample_count, samples);
}


void FstPlugin::import_patch(const std::string& patch) noexcept
{
    synth.process_messages();
    Serializer::import(synth, patch);
    synth.process_messages();
    renderer.reset();
}


VstIntPtr FstPlugin::get_chunk(void** chunk, bool is_preset) noexcept
{
    size_t const current_program = bank.get_current_program_index();

    bank[current_program].import(Serializer::serialize(synth));

    if (is_preset) {
        std::string const& serialized = bank[current_program].serialize();
        *chunk = (void*)serialized.c_str();

        return (VstIntPtr)serialized.size();
    } else {
        serialized_bank = bank.serialize();
        *chunk = (void*)serialized_bank.c_str();

        return (VstIntPtr)serialized_bank.size();
    }
}


void FstPlugin::set_chunk(void const* chunk, VstIntPtr const size, bool is_preset) noexcept
{
    size_t const current_program = bank.get_current_program_index();
    std::string buffer((char const*)chunk, (std::string::size_type)size);

    save_current_patch_before_changing_program = false;

    if (is_preset) {
        bank[current_program].import(buffer);
    } else {
        bank.import(buffer);
    }

    import_patch(bank[current_program].serialize());

    parameters[0].set_value(
        (float)Bank::program_index_to_normalized_parameter_value(current_program)
    );
}


void FstPlugin::control_change(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Controller const controller,
        Midi::Byte const new_value
) noexcept {
    had_midi_cc_event = true;
}


void FstPlugin::program_change(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Byte const new_program
) noexcept {
    set_program((size_t)new_program);
    had_midi_cc_event = true;
}


void FstPlugin::channel_pressure(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Byte const pressure
) noexcept {
    had_midi_cc_event = true;
}


void FstPlugin::pitch_wheel_change(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Word const new_value
) noexcept {
    had_midi_cc_event = true;
}


VstIntPtr FstPlugin::get_program() const noexcept
{
    return next_program;
}


void FstPlugin::set_program(size_t index) noexcept
{
    if (index >= Bank::NUMBER_OF_PROGRAMS || index == next_program) {
        return;
    }

    next_program = index;
    parameters[0].set_value(
        (float)Bank::program_index_to_normalized_parameter_value(index)
    );
}


VstIntPtr FstPlugin::get_program_name(char* name, size_t index) noexcept
{
    if (index >= Bank::NUMBER_OF_PROGRAMS) {
        return 0;
    }

    strncpy(name, bank[index].get_name().c_str(), kVstMaxProgNameLen - 1);

    return 1;
}


void FstPlugin::get_program_name(char* name) noexcept
{
    strncpy(name, bank[next_program].get_name().c_str(), kVstMaxProgNameLen - 1);
}


void FstPlugin::set_program_name(const char* name)
{
    bank[next_program].set_name(name);
}


void FstPlugin::open_gui(GUI::PlatformWidget parent_window)
{
    close_gui();
    gui = new GUI(FST_H_VERSION, platform_data, parent_window, synth, false);
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


FstPlugin::Parameter FstPlugin::create_midi_ctl_param(
        Synth::ControllerId const controller_id,
        MidiController* midi_controller
) noexcept {
    return Parameter(
        GUI::get_controller(controller_id)->short_name,
        midi_controller != NULL
            ? midi_controller
            : synth.midi_controllers[controller_id],
        (Midi::Controller)controller_id
    );
}


FstPlugin::Parameter::Parameter()
    : midi_controller(NULL),
    name("unknown"),
    controller_id(0),
    // change_index(-1), /* See FstPlugin::generate_samples() */
    value(0.5f),
    is_dirty_(false)
{
}


FstPlugin::Parameter::Parameter(
        char const* name,
        MidiController* midi_controller,
        Midi::Controller const controller_id
) : midi_controller(midi_controller),
    name(name),
    controller_id(controller_id),
    // change_index(-1), /* See FstPlugin::generate_samples() */
    value(0.5f),
    is_dirty_(false)
{
}


char const* FstPlugin::Parameter::get_name() const noexcept
{
    return name;
}


MidiController* FstPlugin::Parameter::get_midi_controller() const noexcept
{
    return midi_controller;
}


Midi::Controller FstPlugin::Parameter::get_controller_id() const noexcept
{
    return controller_id;
}


/* See FstPlugin::generate_samples() */
// bool FstPlugin::Parameter::needs_host_update() const noexcept
// {
    // if (UNLIKELY(midi_controller == NULL)) {
        // return false;
    // }

    // return change_index != midi_controller->get_change_index();
// }


float FstPlugin::Parameter::get_value() noexcept
{
    if (UNLIKELY(midi_controller == NULL)) {
        return get_last_set_value();
    }

    float const value = (float)midi_controller->get_value();

    /* See FstPlugin::generate_samples() */
    // change_index=  midi_controller->get_change_index();

    return value;
}


float FstPlugin::Parameter::get_last_set_value() noexcept
{
    return this->value;
}


void FstPlugin::Parameter::set_value(float const value) noexcept
{
    this->value = value;
    is_dirty_ = true;
}


void FstPlugin::Parameter::clear() noexcept
{
    is_dirty_ = false;
}


bool FstPlugin::Parameter::is_dirty() const noexcept
{
    return is_dirty_;
}


float FstPlugin::get_parameter(size_t index) noexcept
{
    return parameters[index].get_value();
}


void FstPlugin::set_parameter(size_t index, float value) noexcept
{
    parameters[index].set_value(value);
}


void FstPlugin::get_param_label(size_t index, char* buffer) const noexcept
{
    strncpy(buffer, index == 0 ? "" : "%", kVstMaxParamStrLen);
    buffer[kVstMaxParamStrLen - 1] = '\x00';
}


void FstPlugin::get_param_display(size_t index, char* buffer) noexcept
{
    if (index == 0) {
        size_t const program_index = (
            Bank::normalized_parameter_value_to_program_index(
                (Number)parameters[0].get_last_set_value()
            )
        );

        if (program_index < Bank::NUMBER_OF_PROGRAMS) {
            strncpy(
                buffer,
                bank[program_index].get_short_name().c_str(),
                kVstMaxParamStrLen - 1
            );
        } else {
            strncpy(buffer, "???", kVstMaxParamStrLen - 1);
        }
    } else {
        float const value = get_parameter(index);

        snprintf(buffer, kVstMaxParamStrLen, "%.2f", value * 100.0f);
    }

    buffer[kVstMaxParamStrLen - 1] = '\x00';
}


void FstPlugin::get_param_name(size_t index, char* buffer) const noexcept
{
    strncpy(buffer, parameters[index].get_name(), kVstMaxParamStrLen);
    buffer[kVstMaxParamStrLen - 1] = '\x00';
}

}
