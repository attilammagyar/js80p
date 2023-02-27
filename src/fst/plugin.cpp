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

#include "debug.hpp"
#include "midi.hpp"


namespace JS80P
{

AEffect* FstPlugin::create_instance(
        audioMasterCallback const host_callback,
        HINSTANCE const dll_instance
) {
    AEffect* effect = new AEffect();

    FstPlugin* fst_plugin = new FstPlugin(effect, host_callback, dll_instance);

    memset(effect, 0, sizeof(AEffect));

    effect->dispatcher = &dispatch;
    effect->flags = effFlagsHasEditor | effFlagsIsSynth | effFlagsCanReplacing;
    effect->magic = kEffectMagic;
    effect->numInputs = 0;
    effect->numOutputs = OUT_CHANNELS;
    effect->object = (void*)fst_plugin;
    effect->processReplacing = &process_replacing;

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
            fst_plugin->open_gui((GUI::Window)pointer);
            return 1;

        case effEditClose:
            fst_plugin->close_gui();
            return 0;

        case effGetPlugCategory:
            return kPlugCategSynth;

        case effGetEffectName:
        case effGetProductString:
            strncpy((char*)pointer, FstPlugin::NAME, 64);
            return 1;

        case effGetVstVersion:
            return kVstVersion;

        case effCanDo:
            JS80P_DEBUG(
                "op_code=%d, op_code_name=%s, index=%d, ivalue=%d, fvalue=%f, pointer=%s",
                (int)op_code,
                ((op_code < FST_OP_CODE_NAMES_LEN) ? FST_OP_CODE_NAMES[op_code] : "???"),
                (int)index,
                (int)ivalue,
                fvalue,
                (char*)pointer
            );
            return 0;

        default:
            JS80P_DEBUG(
                "op_code=%d, op_code_name=%s, index=%d, ivalue=%d, fvalue=%f",
                (int)op_code,
                ((op_code < FST_OP_CODE_NAMES_LEN) ? FST_OP_CODE_NAMES[op_code] : "???"),
                (int)index,
                (int)ivalue,
                fvalue
            );
            return 0;
    }

    return 0;
}


void VSTCALLBACK FstPlugin::process_replacing(
        AEffect* effect,
        float** indata,
        float** outdata,
        VstInt32 frames
) {
    JS80P::FstPlugin* fst_plugin = (JS80P::FstPlugin*)effect->object;

    fst_plugin->generate_float(frames, outdata);
}


FstPlugin::FstPlugin(
        AEffect* const effect,
        audioMasterCallback const host_callback,
        HINSTANCE const dll_instance
)
    : synth(),
    effect(effect),
    host_callback(host_callback),
    dll_instance(dll_instance),
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


void FstPlugin::set_sample_rate(float const new_sample_rate)
{
    synth.set_sample_rate((Frequency)new_sample_rate);
}


void FstPlugin::set_block_size(VstIntPtr const new_block_size)
{
    synth.set_block_size((Integer)new_block_size);
}


void FstPlugin::suspend()
{
    synth.suspend();
}


void FstPlugin::resume()
{
    synth.resume();
    host_callback(effect, audioMasterWantMidi, 0, 1, NULL, 0.0f);
}


void FstPlugin::process_events(VstEvents const* const events)
{
    VstEvent* event = NULL;

    for (VstInt32 i = 0; i < events->numEvents; ++i) {
        event = events->events[i];

        if (event->type == kVstMidiType) {
            process_midi_event((VstMidiEvent*)event);
        }
    }
}


void FstPlugin::process_midi_event(VstMidiEvent const* const event)
{
    Seconds const time_offset = (
        synth.sample_count_to_time_offset((Integer)event->deltaFrames)
    );

    Midi::Dispatcher::dispatch<Synth>(
        synth, time_offset, (Midi::Byte*)event->midiData
    );
}


void FstPlugin::generate_float(VstInt32 const sample_count, float** samples)
{
    if (sample_count < 1) {
        return;
    }

    Sample const* const* buffer = synth.generate_samples(round, (Integer)sample_count);
    round = (round + 1) & ROUND_MASK;

    for (Integer c = 0; c != Synth::OUT_CHANNELS; ++c) {
        for (VstInt32 i = 0; i != sample_count; ++i) {
            samples[c][i] = (float)buffer[c][i];
        }
    }
}


void FstPlugin::open_gui(GUI::Window parent_window)
{
    close_gui();
    gui = GUI::create_instance((GUI::Application)dll_instance, parent_window, synth);
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
