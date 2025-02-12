/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025  Attila M. Magyar
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

#include <cstdio>
#include <cstring>

#include "plugin/fst/plugin.hpp"

// #include "debug.hpp"
#include "serializer.hpp"
#include "spscqueue.cpp"


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
    "Idle",                         /*   53 */
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
        audioMasterCallback const host_callback_ptr,
        GUI::PlatformData const platform_data
) noexcept {
    AEffect* effect = new AEffect();

    FstPlugin* fst_plugin = new FstPlugin(
        effect, host_callback_ptr, platform_data
    );

    memset(effect, 0, sizeof(AEffect));

    effect->magic = kEffectMagic;
    effect->dispatcher = &dispatch;
    effect->process = &process_accumulating;
    effect->getParameter = &get_parameter;
    effect->setParameter = &set_parameter;
    effect->numPrograms = (VstInt32)Bank::NUMBER_OF_PROGRAMS;
    effect->numParams = (VstInt32)NUMBER_OF_PARAMETERS;
    effect->numInputs = FstPlugin::IN_CHANNELS;
    effect->numOutputs = FstPlugin::OUT_CHANNELS;
    effect->flags = (
        effFlagsHasEditor
        | effFlagsIsSynth
        | effFlagsCanReplacing
        | effFlagsCanDoubleReplacing
        | effFlagsProgramChunks
    );
    effect->initialDelay = fst_plugin->get_latency_samples();
    effect->object = (void*)fst_plugin;
    effect->uniqueID = CCONST('a', 'm', 'j', '8');
    effect->version = FstPlugin::VERSION;
    effect->processReplacing = &process_replacing;
    effect->processDoubleReplacing = &process_double_replacing;

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
            // && op_code != effIdle
            // && op_code != effGetProgram
            // && op_code != effGetProgramName
            // && op_code != effGetProductString
            // && op_code != 67
            // && op_code != 68
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

        case effOpen:
            fst_plugin->initialize();
            return 0;

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
            return fst_plugin->is_automatable((size_t)index) ? 1 : 0;

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
            /*
            Though receiveVstMidiEvent should be enough, JUCE's implementation
            of effCanDo checks the others as well, probably for good reason,
            e.g. compatibility with certain hosts.
            */
            if (
                    strcmp("receiveVstMidiEvent", (char const*)pointer) == 0
                    || strcmp("receiveVstMidiEvents", (char const*)pointer) == 0
                    || strcmp("receiveVstEvents", (char const*)pointer) == 0
                    || strcmp("receiveVstTimeInfo", (char const*)pointer) == 0
            ) {
                return 1;
            }

            if (strcmp("openCloseAnyThread", (char const*)pointer) == 0) {
                return -1;
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

        case effIdle:
            return fst_plugin->idle();

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

    fst_plugin->generate_and_add_samples(frames, indata, outdata);
}


void VSTCALLBACK FstPlugin::process_replacing(
        AEffect* effect,
        float** indata,
        float** outdata,
        VstInt32 frames
) {
    JS80P::FstPlugin* fst_plugin = (JS80P::FstPlugin*)effect->object;

    fst_plugin->generate_samples<float>(frames, indata, outdata);
}


void VSTCALLBACK FstPlugin::process_double_replacing(
        AEffect* effect,
        double** indata,
        double** outdata,
        VstInt32 frames
) {
    JS80P::FstPlugin* fst_plugin = (JS80P::FstPlugin*)effect->object;

    fst_plugin->generate_samples<double>(frames, indata, outdata);
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


void FstPlugin::populate_parameters(
        Synth& synth,
        Parameters& parameters
) noexcept {
    parameters[0] = Parameter("Program", NULL, Synth::ControllerId::NONE);
    parameters[1] = create_midi_ctl_param(
        Synth::ControllerId::PITCH_WHEEL, &synth.pitch_wheel, synth
    );
    parameters[2] = create_midi_ctl_param(
        Synth::ControllerId::CHANNEL_PRESSURE,
        &synth.channel_pressure_ctl,
        synth
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

        Similarly, CC 88 was erroneously missing, and it was added after v3.1.0.
        */
        if (midi_controller == Midi::SUSTAIN_PEDAL || midi_controller == Midi::UNDEFINED_20) {
            continue;
        }

        parameters[index] = create_midi_ctl_param(
            (Synth::ControllerId)cc,
            synth.midi_controllers[cc],
            synth
        );
        ++index;
    }

    parameters[index++] = create_midi_ctl_param(
        Synth::ControllerId::SUSTAIN_PEDAL,
        synth.midi_controllers[Synth::ControllerId::SUSTAIN_PEDAL],
        synth
    );

    Parameter patch_changed = Parameter(
        PATCH_CHANGED_PARAMETER_SHORT_NAME, NULL, Synth::ControllerId::NONE
    );

    patch_changed.set_value(0.0f);

    parameters[PATCH_CHANGED_PARAMETER_INDEX] = patch_changed;

    parameters[PATCH_CHANGED_PARAMETER_INDEX + 1] = create_midi_ctl_param(
        Synth::ControllerId::UNDEFINED_20,
        synth.midi_controllers[Synth::ControllerId::UNDEFINED_20],
        synth
    );
}


FstPlugin::Parameter FstPlugin::create_midi_ctl_param(
        Synth::ControllerId const controller_id,
        MidiController* midi_controller,
        Synth& synth
) noexcept {
    return Parameter(
        GUI::get_controller(controller_id)->short_name,
        midi_controller != NULL
            ? midi_controller
            : synth.midi_controllers[controller_id],
        (Midi::Controller)controller_id
    );
}


FstPlugin::FstPlugin(
        AEffect* const effect,
        audioMasterCallback const host_callback_ptr,
        GUI::PlatformData const platform_data
) noexcept
    : synth(),
    effect(effect),
    host_callback_ptr(host_callback_ptr),
    platform_data(platform_data),
    gui(NULL),
    renderer(synth),
    to_audio_messages(1024),
    to_audio_string_messages(256),
    to_gui_messages(1024),
    mts_esp(synth),
    serialized_bank(""),
    current_patch(""),
    current_program_index(0),
    min_samples_before_next_cc_ui_update(8192),
    remaining_samples_before_next_cc_ui_update(0),
    min_samples_before_next_bank_update(16384),
    remaining_samples_before_next_bank_update(0),
    prev_logged_op_code(-1),
    had_midi_cc_event(false),
    need_bank_update(false),
    need_host_update(false)
{
    clear_received_midi_cc();

    window_rect.top = 0;
    window_rect.left = 0;
    window_rect.bottom = GUI::HEIGHT;
    window_rect.right = GUI::WIDTH;

    populate_parameters(synth, parameters);

    serialized_bank = bank.serialize();
    current_patch = bank[current_program_index].serialize();

    program_names.import_names(serialized_bank);
}


FstPlugin::~FstPlugin()
{
    close_gui();
}


void FstPlugin::process_internal_messages_in_audio_thread(
        SPSCQueue<FstPlugin::Message>& messages
) noexcept {
    SPSCQueue<Message>::SizeType const message_count = messages.length();

    for (size_t i = 0; i != message_count; ++i) {
        Message message;

        if (!messages.pop(message)) {
            continue;
        }

        switch (message.get_type()) {
            case MessageType::CHANGE_PROGRAM:
                handle_change_program(message.get_index());
                break;

            case MessageType::RENAME_PROGRAM:
                handle_rename_program(message.get_serialized_data());
                break;

            case MessageType::CHANGE_PARAM:
                handle_change_param(
                    message.get_controller_id(),
                    message.get_new_value(),
                    message.get_midi_controller()
                );
                break;

            case MessageType::IMPORT_PATCH:
                handle_import_patch(message.get_serialized_data());
                break;

            case MessageType::IMPORT_BANK:
                handle_import_bank(message.get_serialized_data());
                break;

            default:
                break;
        }
    }
}


void FstPlugin::handle_change_program(size_t const new_program) noexcept
{
    if (new_program >= Bank::NUMBER_OF_PROGRAMS) {
        return;
    }

    size_t const old_program = bank.get_current_program_index();

    if (new_program == old_program) {
        return;
    }

    std::string const new_patch(bank[new_program].serialize());

    synth.process_messages();
    bank[old_program].import(Serializer::serialize(synth));
    Serializer::import_patch_in_audio_thread(synth, new_patch);
    synth.clear_dirty_flag();
    renderer.reset();
    bank.set_current_program_index(new_program);

    need_bank_update = true;
}


void FstPlugin::handle_rename_program(std::string const& name) noexcept
{
    size_t const current_program_index = bank.get_current_program_index();
    Bank::Program& current_program = bank[current_program_index];

    current_program.set_name(name);

    need_bank_update = true;
}


void FstPlugin::handle_change_param(
        Midi::Controller const controller_id,
        Number const new_value,
        MidiController* const midi_controller
) noexcept {
    if (Synth::is_supported_midi_controller(controller_id)) {
        if (midi_cc_received[(size_t)controller_id]) {
            return;
        }

        /*
        Some hosts (e.g. FL Studio 21) swallow most MIDI CC messages, and
        the only way to make physical knobs and faders on a MIDI keyboard
        work in the plugin is to export parameters to which those MIDI CC
        messages can be assigned in the host, and then interpret the
        changes of these parameters as if the corresponding MIDI CC message
        had been received.
        */

        synth.control_change(
            0.0, 0, controller_id, float_to_midi_byte(new_value)
        );
    } else if (JS80P_LIKELY(midi_controller != NULL)) {
        midi_controller->change(0.0, new_value);
    }
}


void FstPlugin::handle_import_patch(std::string const& patch) noexcept
{
    size_t const current_program = bank.get_current_program_index();

    Serializer::import_patch_in_audio_thread(synth, patch);
    synth.clear_dirty_flag();
    renderer.reset();

    std::string const& serialized_patch(Serializer::serialize(synth));

    bank[current_program].import(serialized_patch);

    need_bank_update = true;
}


void FstPlugin::handle_import_bank(std::string const& serialized_bank) noexcept
{
    size_t const current_program = bank.get_current_program_index();

    bank.import(serialized_bank);

    Serializer::import_patch_in_audio_thread(
        synth, bank[current_program].serialize()
    );
    synth.clear_dirty_flag();
    renderer.reset();

    need_bank_update = true;
}


void FstPlugin::process_internal_messages_in_gui_thread() noexcept
{
    SPSCQueue<Message>::SizeType const message_count = to_gui_messages.length();

    for (size_t i = 0; i != message_count; ++i) {
        Message message;

        if (!to_gui_messages.pop(message)) {
            continue;
        }

        switch (message.get_type()) {
            case MessageType::PROGRAM_CHANGED:
                handle_program_changed(
                    message.get_index(), message.get_serialized_data()
                );
                break;

            case MessageType::BANK_CHANGED:
                handle_bank_changed(message.get_serialized_data());
                break;

            case MessageType::PARAMS_CHANGED:
                handle_params_changed();
                break;

            case MessageType::SYNTH_WAS_DIRTY:
                handle_synth_was_dirty();
                break;

            default:
                break;
        }
    }
}


void FstPlugin::handle_program_changed(
        size_t const new_program,
        std::string const& patch
) noexcept {
    Bank::Program program;

    current_program_index = new_program;
    current_patch = patch;

    program.import(patch);
    program_names[current_program_index].set_name(program.get_name());

    parameters[0].set_value(
        Bank::program_index_to_normalized_parameter_value(new_program)
    );
}


void FstPlugin::handle_bank_changed(std::string const& serialized_bank) noexcept
{
    this->serialized_bank = serialized_bank;

    program_names.import_names(serialized_bank);
}


void FstPlugin::handle_params_changed() noexcept
{
    need_host_update = true;
}


void FstPlugin::handle_synth_was_dirty() noexcept
{
    Parameter& dirty = parameters[PATCH_CHANGED_PARAMETER_INDEX];

    float const new_value = dirty.get_value() + 0.01f;

    dirty.set_value(new_value < 1.0f ? new_value : 0.0f);
    need_host_update = true;

    VstInt32 const index = (VstInt32)PATCH_CHANGED_PARAMETER_INDEX;

    host_callback(audioMasterBeginEdit, index);
    host_callback(audioMasterAutomate, index, 0, NULL, new_value);
    host_callback(audioMasterEndEdit, index);
}


VstInt32 FstPlugin::get_latency_samples() const noexcept
{
    return (VstInt32)renderer.get_latency_samples();
}


void FstPlugin::initialize() noexcept
{
    need_idle();
}


void FstPlugin::need_idle() noexcept
{
    host_callback(audioMasterNeedIdle);
}


VstIntPtr FstPlugin::idle() noexcept
{
    process_internal_messages_in_gui_thread();
    update_host_display();

    return 1;
}


void FstPlugin::set_sample_rate(float const new_sample_rate) noexcept
{
    process_internal_messages_in_gui_thread();

    if (new_sample_rate > HOST_CC_UI_UPDATE_FREQUENCY) {
        min_samples_before_next_cc_ui_update = 1 + (Integer)(
            new_sample_rate * HOST_CC_UI_UPDATE_FREQUENCY_INV
        );
        remaining_samples_before_next_cc_ui_update = min_samples_before_next_cc_ui_update;

        min_samples_before_next_bank_update = 1 + (Integer)(
            new_sample_rate * BANK_UPDATE_FREQUENCY_INV
        );
        remaining_samples_before_next_bank_update = min_samples_before_next_bank_update;
    }

    synth.set_sample_rate((Frequency)new_sample_rate);
    synth.running_status = 0;
    this->running_status = 0;
    renderer.reset();
}


void FstPlugin::set_block_size(VstIntPtr const new_block_size) noexcept
{
    process_internal_messages_in_gui_thread();
    renderer.reset();
    synth.running_status = 0;
    this->running_status = 0;
}


void FstPlugin::suspend() noexcept
{
    process_internal_messages_in_gui_thread();
    need_idle();
    synth.suspend();
    synth.running_status = 0;
    this->running_status = 0;
    renderer.reset();
}


void FstPlugin::resume() noexcept
{
    synth.resume();
    synth.running_status = 0;
    this->running_status = 0;
    renderer.reset();
    host_callback(audioMasterWantMidi, 0, 1);
    process_internal_messages_in_gui_thread();
    need_idle();
}


void FstPlugin::process_vst_events(VstEvents const* const events) noexcept
{
    clear_received_midi_cc();

    for (VstInt32 i = 0; i < events->numEvents; ++i) {
        VstEvent* event = events->events[i];

        if (event->type == kVstMidiType) {
            process_vst_midi_event((VstMidiEvent*)event);
        }
    }

    if (had_midi_cc_event && remaining_samples_before_next_cc_ui_update == 0) {
        had_midi_cc_event = false;
        remaining_samples_before_next_cc_ui_update = min_samples_before_next_cc_ui_update;
        to_gui_messages.push(Message(MessageType::PARAMS_CHANGED));
    }
}


VstIntPtr FstPlugin::host_callback(
        VstInt32 op_code,
        VstInt32 index,
        VstIntPtr ivalue,
        void* pointer,
        float fvalue
) const noexcept {
    if (host_callback_ptr == NULL) {
        return 0;
    }

    return host_callback_ptr(effect, op_code, index, ivalue, pointer, fvalue);
}


void FstPlugin::clear_received_midi_cc() noexcept
{
    midi_cc_received.reset();
    received_midi_cc_cleared = true;
}


void FstPlugin::process_vst_midi_event(VstMidiEvent const* const event) noexcept
{
    Seconds const time_offset = (
        synth.sample_count_to_time_offset((Integer)event->deltaFrames)
    );
    Midi::Byte const* const midi_bytes = (Midi::Byte const*)event->midiData;

    Midi::EventDispatcher<FstPlugin>::dispatch_event(
        *this, time_offset, midi_bytes, 4
    );
    Midi::EventDispatcher<Synth>::dispatch_event(
        synth, time_offset, midi_bytes, 4
    );
}


template<typename NumberType>
void FstPlugin::generate_samples(
        VstInt32 const sample_count,
        NumberType const* const* const in_samples,
        NumberType** out_samples
) noexcept {
    if (sample_count < 1) {
        return;
    }

    prepare_rendering(sample_count);
    renderer.render<NumberType>(sample_count, in_samples, out_samples);
    finalize_rendering(sample_count);

    /*
    It would be nice to notify the host about param changes that originate from
    the plugin, but since these parameters only ever change due to MIDI CC
    messages, we don't want the host to record them both as MIDI CC and as
    parameter automation.

    Also, since parameter handling seems to be done in the GUI thread and
    generate_samples() is run in the audio thread, calling audioMasterAutomate
    would cross threads, which is probably unsafe in most hosts.
    */
}


void FstPlugin::prepare_rendering(Integer const sample_count) noexcept
{
    if (!received_midi_cc_cleared) {
        clear_received_midi_cc();
    }

    received_midi_cc_cleared = false;

    process_internal_messages_in_audio_thread(to_audio_string_messages);
    process_internal_messages_in_audio_thread(to_audio_messages);

    update_bpm();

    if (had_midi_cc_event) {
        if (remaining_samples_before_next_cc_ui_update >= sample_count) {
            remaining_samples_before_next_cc_ui_update -= sample_count;
        } else {
            remaining_samples_before_next_cc_ui_update = 0;
        }
    }

    mts_esp.update_active_notes_tuning();
}


void FstPlugin::finalize_rendering(Integer const sample_count) noexcept
{
    if (remaining_samples_before_next_bank_update >= sample_count) {
        remaining_samples_before_next_bank_update -= sample_count;

        return;
    } else if (remaining_samples_before_next_bank_update > 0) {
        remaining_samples_before_next_bank_update = 0;

        return;
    }

    mts_esp.update_connection_status();

    bool const is_dirty = synth.is_dirty();

    if (JS80P_LIKELY(!(is_dirty || need_bank_update))) {
        return;
    }

    remaining_samples_before_next_bank_update = min_samples_before_next_bank_update;
    need_bank_update = false;
    synth.clear_dirty_flag();

    size_t const current_program = bank.get_current_program_index();
    std::string const& current_patch = Serializer::serialize(synth);

    bank[current_program].import(current_patch);

    std::string const& serialized_bank(bank.serialize());

    to_gui_messages.push(
        Message(MessageType::PROGRAM_CHANGED, current_program, current_patch)
    );
    to_gui_messages.push(Message(MessageType::BANK_CHANGED, 0, serialized_bank));

    if (is_dirty) {
        to_gui_messages.push(Message(MessageType::SYNTH_WAS_DIRTY));
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
        (VstTimeInfo const*)host_callback(audioMasterGetTime, 0, kVstTempoValid)
    );

    if (time_info == NULL || (time_info->flags & kVstTempoValid) == 0) {
        return;
    }

    synth.set_bpm((Number)time_info->tempo);
}


void FstPlugin::update_host_display() noexcept
{
    if (need_host_update) {
        need_host_update = false;
        host_callback(audioMasterUpdateDisplay);
    }
}


void FstPlugin::generate_and_add_samples(
        VstInt32 const sample_count,
        float const* const* const in_samples,
        float** out_samples
) noexcept {
    if (sample_count < 1) {
        return;
    }

    prepare_rendering(sample_count);
    renderer.render<float, Renderer::Operation::ADD>(
        sample_count, in_samples, out_samples
    );
    finalize_rendering(sample_count);
}


VstIntPtr FstPlugin::get_chunk(void** chunk, bool is_preset) noexcept
{
    process_internal_messages_in_gui_thread();

    if (is_preset) {
        Bank::Program program;

        program.import(current_patch);
        program.set_name(program_names[current_program_index].get_name());

        current_patch = program.serialize();

        *chunk = (void*)current_patch.c_str();

        return (VstIntPtr)current_patch.length();
    } else {
        *chunk = (void*)serialized_bank.c_str();

        return (VstIntPtr)serialized_bank.length();
    }
}


void FstPlugin::set_chunk(void const* chunk, VstIntPtr const size, bool is_preset) noexcept
{
    process_internal_messages_in_gui_thread();

    std::string buffer((char const*)chunk, (std::string::size_type)size);

    if (is_preset) {
        current_patch = buffer;

        Bank::Program program;
        program.import(current_patch);

        std::string const& name(program.get_name());

        program_names[current_program_index].set_name(name);

        to_audio_string_messages.push(
            Message(MessageType::IMPORT_PATCH, 0, current_patch)
        );
        to_audio_string_messages.push(
            Message(MessageType::RENAME_PROGRAM, 0, name)
        );
    } else {
        serialized_bank = buffer;

        program_names.import_names(serialized_bank);

        to_audio_string_messages.push(
            Message(MessageType::IMPORT_BANK, 0, serialized_bank)
        );
    }
}


void FstPlugin::note_on(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Note const note,
        Midi::Byte const velocity
) noexcept {
    mts_esp.update_note_tuning(channel, note);
}


void FstPlugin::control_change(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Controller const controller,
        Midi::Byte const new_value
) noexcept {
    had_midi_cc_event = true;

    if (synth.is_supported_midi_controller(controller)) {
        midi_cc_received[(size_t)controller] = true;
    }
}


void FstPlugin::program_change(
        Seconds const time_offset,
        Midi::Channel const channel,
        Midi::Byte const new_program
) noexcept {
    had_midi_cc_event = true;
    handle_change_program((size_t)new_program);
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


VstIntPtr FstPlugin::get_program() noexcept
{
    return current_program_index;
}


void FstPlugin::set_program(size_t index) noexcept
{
    current_program_index = index;
    parameters[0].set_value(
        Bank::program_index_to_normalized_parameter_value(index)
    );

    to_audio_messages.push(Message(MessageType::CHANGE_PROGRAM, index));
}


VstIntPtr FstPlugin::get_program_name(char* name, size_t index) noexcept
{
    process_internal_messages_in_gui_thread();

    if (index >= Bank::NUMBER_OF_PROGRAMS) {
        return 0;
    }

    strncpy(
        name,
        program_names[index].get_name().c_str(),
        kVstMaxProgNameLen - 1
    );
    name[kVstMaxProgNameLen - 1] = '\x00';

    return 1;
}


void FstPlugin::get_program_name(char* name) noexcept
{
    process_internal_messages_in_gui_thread();

    strncpy(
        name,
        program_names[current_program_index].get_name().c_str(),
        kVstMaxProgNameLen - 1
    );
    name[kVstMaxProgNameLen - 1] = '\x00';
}


void FstPlugin::set_program_name(const char* name)
{
    process_internal_messages_in_gui_thread();

    to_audio_string_messages.push(Message(MessageType::RENAME_PROGRAM, 0, name));
    program_names[current_program_index].set_name(name);
}


void FstPlugin::open_gui(GUI::PlatformWidget parent_window)
{
    process_internal_messages_in_gui_thread();

    close_gui();
    gui = new GUI(FST_H_VERSION, platform_data, parent_window, synth, false);
    gui->show();
}


void FstPlugin::gui_idle()
{
    process_internal_messages_in_gui_thread();
    update_host_display();

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
    process_internal_messages_in_gui_thread();

    if (gui == NULL) {
        need_idle();

        return;
    }

    delete gui;

    gui = NULL;
    need_idle();
}


FstPlugin::Parameter::Parameter()
    : midi_controller(NULL),
    name("unknown"),
    controller_id(0),
    // change_index(-1), /* See FstPlugin::generate_samples() */
    value(0.5f)
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
    value(midi_controller != NULL ? (float)midi_controller->get_value() : 0.5f)
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
    // if (JS80P_UNLIKELY(midi_controller == NULL)) {
        // return false;
    // }

    // return change_index != midi_controller->get_change_index();
// }


float FstPlugin::Parameter::get_value() noexcept
{
    if (JS80P_UNLIKELY(midi_controller == NULL)) {
        return get_last_set_value();
    }

    float const value = (float)midi_controller->get_value();

    /* See FstPlugin::generate_samples() */
    // change_index = midi_controller->get_change_index();

    return value;
}


float FstPlugin::Parameter::get_last_set_value() const noexcept
{
    return this->value;
}


void FstPlugin::Parameter::set_value(float const value) noexcept
{
    this->value = value;
}


float FstPlugin::get_parameter(size_t index) noexcept
{
    return parameters[index].get_value();
}


void FstPlugin::set_parameter(size_t index, float value) noexcept
{
    if (index == PATCH_CHANGED_PARAMETER_INDEX) {
        return;
    }

    Parameter& param = parameters[index];

    param.set_value(value);

    if (index == 0) {
        size_t const program = (
            Bank::normalized_parameter_value_to_program_index((Number)value)
        );

        to_audio_messages.push(Message(MessageType::CHANGE_PROGRAM, program));
        current_program_index = program;
    } else {
        to_audio_messages.push(
            Message(
                param.get_controller_id(),
                (Number)value,
                param.get_midi_controller()
            )
        );
    }
}


bool FstPlugin::is_automatable(size_t index) noexcept
{
    return (
        index != PATCH_CHANGED_PARAMETER_INDEX && index < NUMBER_OF_PARAMETERS
    );
}


void FstPlugin::get_param_label(size_t index, char* buffer) noexcept
{
    process_internal_messages_in_gui_thread();

    strncpy(buffer, index == 0 ? "" : "%", kVstMaxParamStrLen);
    buffer[kVstMaxParamStrLen - 1] = '\x00';
}


void FstPlugin::get_param_display(size_t index, char* buffer) noexcept
{
    process_internal_messages_in_gui_thread();

    if (index == 0) {
        size_t const program_index = (
            Bank::normalized_parameter_value_to_program_index(
                (Number)parameters[0].get_last_set_value()
            )
        );

        if (program_index < Bank::NUMBER_OF_PROGRAMS) {
            strncpy(
                buffer,
                program_names[program_index].get_short_name().c_str(),
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


void FstPlugin::get_param_name(size_t index, char* buffer) noexcept
{
    process_internal_messages_in_gui_thread();

    strncpy(buffer, parameters[index].get_name(), kVstMaxParamStrLen);
    buffer[kVstMaxParamStrLen - 1] = '\x00';
}


FstPlugin::Message::Message() : Message(MessageType::NONE)
{
}


FstPlugin::Message::Message(
        MessageType const type,
        size_t const index,
        std::string const& serialized_data
) : serialized_data(serialized_data),
    midi_controller(NULL),
    new_value(0.0),
    index(index),
    type(type),
    controller_id(0)
{
}


FstPlugin::Message::Message(
        Midi::Controller const controller_id,
        Number const new_value,
        MidiController* const midi_controller
) : serialized_data(""),
    midi_controller(midi_controller),
    new_value(new_value),
    index(0),
    type(MessageType::CHANGE_PARAM),
    controller_id(controller_id)
{
}


FstPlugin::MessageType FstPlugin::Message::get_type() const noexcept
{
    return type;
}


size_t FstPlugin::Message::get_index() const noexcept
{
    return index;
}


std::string const& FstPlugin::Message::get_serialized_data() const noexcept
{
    return serialized_data;
}


Midi::Controller FstPlugin::Message::get_controller_id() const noexcept
{
    return controller_id;
}


Number FstPlugin::Message::get_new_value() const noexcept
{
    return new_value;
}


MidiController* FstPlugin::Message::get_midi_controller() const noexcept
{
    return midi_controller;
}

}
