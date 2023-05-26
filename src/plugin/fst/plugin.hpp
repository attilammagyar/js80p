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

#ifndef JS80P__PLUGIN__FST__PLUGIN_HPP
#define JS80P__PLUGIN__FST__PLUGIN_HPP

#define _OLD_PARAM_HANDLING

#include <string>
#include <array>

#include <fst/fst.h>

#include "gui/gui.hpp"

#include "js80p.hpp"
#include "synth.hpp"


namespace JS80P
{

class FstPlugin
{
    public:
        static constexpr int OUT_CHANNELS = (int)Synth::OUT_CHANNELS;
        static constexpr VstInt32 VERSION = JS80P::Constants::PLUGIN_VERSION_INT;
        static constexpr size_t NO_OF_PROGRAMS = JS80P::Constants::NO_OF_PROGRAMS;
        static constexpr size_t NO_OF_PARAMS = Synth::ParamId::MAX_PARAM_ID;

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

        VstIntPtr get_chunk(void** chunk, bool isPreset) noexcept;
        void set_chunk(void const* chunk, VstIntPtr const size, bool isPreset) noexcept;

        VstIntPtr get_program() const noexcept;
        void set_program(size_t index) noexcept;

        bool get_program_name_indexed(char* name, size_t index) noexcept;
        void get_program_name(char* name) noexcept;
        void set_program_name(const char* name);

        float get_parameter(size_t index) const noexcept;
        void set_parameter(size_t index, float) noexcept;

        void get_parameter_label(size_t index, char* label) const noexcept;
        void get_parameter_display(size_t index, char* display) const noexcept;
        void get_parameter_name(size_t index, char* name) const noexcept;
#if (defined (_OLD_PARAM_HANDLING))
        bool can_parameter_be_automated(size_t index) const noexcept;
#endif  // #if (defined (_OLD_PARAM_HANDLING))

        void open_gui(GUI::PlatformWidget parent_window);
        void gui_idle();
        void close_gui();

        Synth synth;

    private:
        void import_serialized_program(const std::string& serialized_program) noexcept;
        void get_program_name_short(char* name, size_t index) noexcept;

        static constexpr Integer ROUND_MASK = 0x7fff;

        Sample const* const* render_next_round(VstInt32 sample_count) noexcept;

        AEffect* const effect;
        audioMasterCallback const host_callback;
        GUI::PlatformData const platform_data;

        ERect window_rect;
        Integer round;
        GUI* gui;
        std::array<std::string, NO_OF_PROGRAMS> serialized_programs;
        std::array<std::string, NO_OF_PROGRAMS> serialized_program_names;
        size_t current_program_index{0};
        bool store_state_of_previous_program_in_set_program{true};
        std::string serialized_bank;

#if (defined (_OLD_PARAM_HANDLING))
        struct FloatParamInfo {
            FloatParamInfo(std::string_view n, double s = 100.0, std::string_view f = "%.2f", std::string_view l = "%")
            : name(n), label(l), format(f), scale(s) {
            }
            std::string name; // friendly, should not be longer than 16 bytes including 0 terminator!
            std::string label;
            std::string format;
            double scale;
        };
        using float_param_infos_t = std::array<FloatParamInfo, Synth::FLOAT_PARAMS>;
        static const float_param_infos_t float_param_infos;
//#define N_T_C
#ifdef N_T_C
    public:
        using options_t = std::vector<std::string>;
        static const options_t modes;
        static const options_t waveforms;
        static const options_t biquad_filter_types;
    private:
        struct IntParamInfo {
            IntParamInfo(std::string_view n, const options_t* o)
            : name(n), options(o) {
            }
            std::string name; // friendly, should not be longer than 16 bytes including 0 terminator!
            const options_t* options;
        };
#else   // #ifndef N_T_C
        struct IntParamInfo {
            IntParamInfo(std::string_view n, char const* const* const o, int no_of_o)
            : name(n), options(o), number_of_options(no_of_o) {
            }
            std::string name; // friendly, should not be longer than 16 bytes including 0 terminator!
            char const* const* const options;
            int const number_of_options;
        };
#endif  // #ifndef N_T_C
        using int_param_infos_t = std::array<IntParamInfo, Synth::MAX_PARAM_ID - Synth::FLOAT_PARAMS>;
        static const int_param_infos_t int_param_infos;

#else   // #if (!defined (_OLD_PARAM_HANDLING))

        struct ParamInfo {
            ParamInfo(Synth::ParamId i, std::string_view n)
                : id(i), name(n) {
            }
            void fillName(char* out_name) const noexcept {
                const std::string param_name{name.substr(0, Constants::PARAM_NAME_MAX_LENGTH)};
                size_t i{0};
                for (; i < param_name.length(); ++i) {
                    out_name[i] = param_name.data()[i];
                }
                out_name[i] = '\0';
            }
            virtual void fillLabel(char* out_label) const noexcept = 0;
            virtual void fillDisplay(const Synth& synth, char* out_display) const noexcept = 0;
            Synth::ParamId id;
            std::string name; // friendly, should not be longer than 16 bytes including 0 terminator!
        };
        struct FloatParamInfo : public ParamInfo {
            FloatParamInfo(Synth::ParamId i
                         , std::string_view n
                         , double s = 100.0
                         , std::string_view f = "%.2f"
                         , std::string_view l = "%")
                : ParamInfo(i, n), label(l), format(f), scale(s) {
            }
            virtual void fillLabel(char* out_label) const noexcept override {
                snprintf(out_label, 9, "%s", label.c_str());
            }
            virtual void fillDisplay(const Synth& synth, char* out_display) const noexcept override {
                Number ratio{synth.get_param_ratio_atomic(id)};
                snprintf(out_display, 9, format.c_str(), synth.float_param_ratio_to_display_value(id, ratio) * scale);
            }
            std::string label;
            std::string format;
            double scale;
        };
        struct IntParamInfo : public ParamInfo {
            IntParamInfo(Synth::ParamId i
                         , std::string_view n
                         , char const* const* const o
                         , int no_of_o)
                : ParamInfo(i, n), options(0), number_of_options(no_of_o) {
            }
            virtual void fillLabel(char* out_label) const noexcept override {
                out_label[0] = '\0';
            }
            virtual void fillDisplay(const Synth& synth, char* out_display) const noexcept override {
                Number ratio{synth.get_param_ratio_atomic(id)};
                /*
                auto value{synth.int_param_ratio_to_display_value(id, ratio)};
#ifdef N_T_C
                if (value < options->size()) {
                    snprintf(out_display, 9, "%s", options->at(value).c_str());
#else
                if (value < number_of_options) {
                    snprintf(out_display, 9, "%s", options[value]);
#endif
                } else {
                    strncpy(out_display, "???", 4);
                }
                */
                synth.int_param_ratio_to_display_value(id, ratio);
                //snprintf(out_display, 16, "%d-%p", number_of_options, (void*)options);
                snprintf(out_display, 16, "%d", number_of_options);
            }
            char const* const* const options{nullptr};
            int const number_of_options{0};
        };
        using param_infos_t = std::array<const ParamInfo*, 7>;
        static const param_infos_t param_infos;

#endif  // #if (!defined (_OLD_PARAM_HANDLING))
};

}

#endif
