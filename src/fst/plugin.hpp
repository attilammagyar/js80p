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

#ifndef JS80P__FST__PLUGIN_HPP
#define JS80P__FST__PLUGIN_HPP

#include <string>

#include "aeffect.h"
#include "aeffectx.h"

#include "gui/gui.hpp"

#include "js80p.hpp"
#include "synth.hpp"
#include "serializer.hpp"


namespace JS80P
{

class FstPlugin
{
    public:
        static constexpr int OUT_CHANNELS = (int)Synth::OUT_CHANNELS;
        static constexpr VstInt32 VERSION = 1000;

        static AEffect* create_instance(
            audioMasterCallback const host_callback,
            GUI::PlatformData const platform_data
        ) noexcept;

        static VstIntPtr VSTCALLBACK dispatch(
            AEffect* effect,
            VstInt32 op_code,
            VstInt32 index,
            VstIntPtr ivalue,
            void* pointer,
            float fvalue
        );

        static void VSTCALLBACK process_accumulating(
            AEffect* effect,
            float** indata,
            float** outdata,
            VstInt32 frames
        );

        static void VSTCALLBACK process_replacing(
            AEffect* effect,
            float** indata,
            float** outdata,
            VstInt32 frames
        );

        static void VSTCALLBACK process_double_replacing(
            AEffect* effect,
            double** indata,
            double** outdata,
            VstInt32 frames
        );

        FstPlugin(
            AEffect* const effect,
            audioMasterCallback const host_callback,
            GUI::PlatformData const platform_data
        ) noexcept;
        ~FstPlugin();

        void set_sample_rate(float const new_sample_rate) noexcept;
        void set_block_size(VstIntPtr const new_block_size) noexcept;
        void suspend() noexcept;
        void resume() noexcept;
        void process_events(VstEvents const* const events) noexcept;
        void process_midi_event(VstMidiEvent const* const event) noexcept;

        template<typename NumberType>
        void generate_samples(
            VstInt32 const sample_count,
            NumberType** samples
        ) noexcept;

        void generate_and_add_samples(
            VstInt32 const sample_count,
            float** samples
        ) noexcept;

        VstIntPtr get_chunk(void** chunk) noexcept;
        void set_chunk(void const* chunk, VstIntPtr const size) noexcept;

        void open_gui(GUI::PlatformWidget parent_window);
        void close_gui();

        Synth synth;

    private:
        static constexpr Integer ROUND_MASK = 0x7fff;

        Sample const* const* render_next_round(VstInt32 sample_count) noexcept;

        AEffect* const effect;
        audioMasterCallback const host_callback;
        GUI::PlatformData const platform_data;

        ERect window_rect;
        Integer round;
        GUI* gui;
        std::string serialized;
};

}

#endif
