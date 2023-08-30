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

#ifndef JS80P__PLUGIN__FST__PLUGIN_HPP
#define JS80P__PLUGIN__FST__PLUGIN_HPP

#include <string>
#include <bitset>

#include <fst/fst.h>

#include "gui/gui.hpp"

#include "bank.hpp"
#include "js80p.hpp"
#include "midi.hpp"
#include "renderer.hpp"
#include "synth.hpp"


namespace JS80P
{

class FstPlugin : public Midi::EventHandler
{
    public:
        static constexpr VstInt32 OUT_CHANNELS = (VstInt32)Synth::OUT_CHANNELS;
        static constexpr VstInt32 VERSION = JS80P::Constants::PLUGIN_VERSION_INT;
        static constexpr VstInt32 NUMBER_OF_PARAMETERS = 72;

        static constexpr char const* FST_H_VERSION = (
            "FST "
            JS80P_TO_STRING(FST_MAJOR_VERSION)
            "."
            JS80P_TO_STRING(FST_MINOR_VERSION)
            "."
            JS80P_TO_STRING(FST_MICRO_VERSION)
        );

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

        static float VSTCALLBACK get_parameter(
            AEffect* effect,
            VstInt32 index
        );

        static void VSTCALLBACK set_parameter(
            AEffect* effect,
            VstInt32 index,
            float fvalue
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
        void process_vst_events(VstEvents const* const events) noexcept;
        void process_vst_midi_event(VstMidiEvent const* const event) noexcept;

        template<typename NumberType>
        void generate_samples(
            VstInt32 const sample_count,
            NumberType** samples
        ) noexcept;

        void generate_and_add_samples(
            VstInt32 const sample_count,
            float** samples
        ) noexcept;

        VstIntPtr get_chunk(void** chunk, bool is_preset) noexcept;
        void set_chunk(void const* chunk, VstIntPtr const size, bool is_preset) noexcept;

        void control_change(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Controller const controller,
            Midi::Byte const new_value
        ) noexcept;

        void program_change(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Byte const new_program
        ) noexcept;

        void channel_pressure(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Byte const pressure
        ) noexcept;

        void pitch_wheel_change(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Word const new_value
        ) noexcept;

        VstIntPtr get_program() const noexcept;
        void set_program(size_t index) noexcept;

        VstIntPtr get_program_name(char* name, size_t index) noexcept;
        void get_program_name(char* name) noexcept;
        void set_program_name(const char* name);

        float get_parameter(size_t index) noexcept;
        void set_parameter(size_t index, float value) noexcept;

        void get_param_label(size_t index, char* buffer) const noexcept;
        void get_param_display(size_t index, char* buffer) noexcept;
        void get_param_name(size_t index, char* buffer) const noexcept;

        void open_gui(GUI::PlatformWidget parent_window);
        void gui_idle();
        void close_gui();

        Synth synth;

    private:
        static constexpr Frequency HOST_CC_UI_UPDATE_FREQUENCY = 6.0;
        static constexpr Frequency HOST_CC_UI_UPDATE_FREQUENCY_INV = (
            1.0 / HOST_CC_UI_UPDATE_FREQUENCY
        );

        class Parameter
        {
            public:
                Parameter();
                Parameter(
                    char const* name,
                    MidiController* midi_controller,
                    Midi::Controller const controller_id
                );
                Parameter(Parameter const& parameter) = default;
                Parameter(Parameter&& parameter) = default;

                Parameter& operator=(Parameter const& parameter) noexcept = default;
                Parameter& operator=(Parameter&& parameter) noexcept = default;

                char const* get_name() const noexcept;
                MidiController* get_midi_controller() const noexcept;
                Midi::Controller get_controller_id() const noexcept;

                // bool needs_host_update() const noexcept; /* See FstPlugin::generate_samples() */

                float get_value() noexcept;
                float get_last_set_value() const noexcept;
                void set_value(float const value) noexcept;

                void clear() noexcept;
                bool is_dirty() const noexcept;

            private:
                MidiController* midi_controller;
                char const* name;
                Midi::Controller controller_id;
                // Integer change_index; /* See FstPlugin::generate_samples() */
                float value;
                bool is_dirty_;
        };

        Parameter create_midi_ctl_param(
            Synth::ControllerId const controller_id,
            MidiController* midi_controller
        ) noexcept;

        void clear_received_midi_cc() noexcept;

        void prepare_rendering(Integer const sample_count) noexcept;

        void update_bpm() noexcept;
        void update_host_display() noexcept;

        void handle_program_change() noexcept;
        void handle_parameter_changes() noexcept;

        Midi::Byte float_to_midi_byte(float const value) const noexcept;

        void import_patch(const std::string& patch) noexcept;

        Parameter parameters[NUMBER_OF_PARAMETERS];

        AEffect* const effect;
        audioMasterCallback const host_callback;
        GUI::PlatformData const platform_data;

        ERect window_rect;
        std::bitset<Midi::MAX_CONTROLLER_ID + 1> midi_cc_received;
        GUI* gui;
        Renderer renderer;
        Bank bank;
        std::string serialized_bank;
        size_t next_program;
        Integer min_samples_before_next_cc_ui_update;
        Integer remaining_samples_before_next_cc_ui_update;
        VstInt32 prev_logged_op_code;
        bool save_current_patch_before_changing_program;
        bool had_midi_cc_event;
        bool received_midi_cc_cleared;
};

}

#endif
