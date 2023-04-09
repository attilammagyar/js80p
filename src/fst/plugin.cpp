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

#include "fst/plugin.hpp"

#include "midi.hpp"


namespace JS80P
{

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
    effect->numOutputs = OUT_CHANNELS;
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
            // op_code != effEditIdle
            // && op_code != effProcessEvents
            // && op_code != 53
            // && op_code != effGetProgram
            // && op_code != effEditGetRect
    // ) {
        // JS80P_DEBUG(
            // "op_code=%d, op_code_name=%s, index=%d, ivalue=%d, fvalue=%f",
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

        case effEditClose:
            fst_plugin->close_gui();
            return 0;

        case effGetChunk:
            return fst_plugin->get_chunk((void**)pointer);

        case effSetChunk:
            fst_plugin->set_chunk((void const*)pointer, ivalue);
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


VstIntPtr FstPlugin::get_chunk(void** chunk) noexcept
{
    serialized = Serializer::serialize(&synth);

    *chunk = (void*)serialized.c_str();

    return (VstIntPtr)serialized.size();
}


void FstPlugin::set_chunk(void const* chunk, VstIntPtr const size) noexcept
{
    std::string serialized((char const*)chunk, (std::string::size_type)size);

    synth.process_messages();
    Serializer::import(&synth, serialized);
    synth.process_messages();
}


void FstPlugin::open_gui(GUI::PlatformWidget parent_window)
{
    close_gui();
    gui = new GUI(platform_data, parent_window, &synth, false);
    gui->show();
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
