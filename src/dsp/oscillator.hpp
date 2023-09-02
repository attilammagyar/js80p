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

#ifndef JS80P__DSP__OSCILLATOR_HPP
#define JS80P__DSP__OSCILLATOR_HPP

#include <string>
#include <type_traits>

#include "js80p.hpp"

#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"
#include "dsp/wavetable.hpp"


namespace JS80P
{

template<class ModulatorSignalProducerClass, bool is_lfo>
class Oscillator;


typedef Oscillator<SignalProducer, false> SimpleOscillator;


template<class ModulatorSignalProducerClass, bool is_lfo = false>
class Oscillator : public SignalProducer
{
    friend class SignalProducer;

    public:
        typedef ModulatableFloatParam<ModulatorSignalProducerClass> ModulatedFloatParam;

        typedef Byte Waveform;

        static constexpr Waveform SINE = 0;
        static constexpr Waveform SAWTOOTH = 1;
        static constexpr Waveform SOFT_SAWTOOTH = 2;
        static constexpr Waveform INVERSE_SAWTOOTH = 3;
        static constexpr Waveform SOFT_INVERSE_SAWTOOTH = 4;
        static constexpr Waveform TRIANGLE = 5;
        static constexpr Waveform SOFT_TRIANGLE = 6;
        static constexpr Waveform SQUARE = 7;
        static constexpr Waveform SOFT_SQUARE = 8;
        static constexpr Waveform CUSTOM = 9;

        static constexpr int WAVEFORMS = 10;

        class WaveformParam : public Param<Waveform, ParamEvaluation::BLOCK>
        {
            public:
                WaveformParam(
                    std::string const name,
                    Waveform const max_value = CUSTOM
                ) noexcept;
        };

        static constexpr Event::Type EVT_START = 1;
        static constexpr Event::Type EVT_STOP = 2;

        static FloatParamS dummy_param;
        static ToggleParam dummy_toggle;

        Oscillator(
            WaveformParam& waveform,
            ModulatorSignalProducerClass* modulator = NULL,
            FloatParamS& amplitude_modulation_level_leader = dummy_param,
            FloatParamS& frequency_modulation_level_leader = dummy_param,
            FloatParamS& phase_modulation_level_leader = dummy_param,
            ToggleParam& tempo_sync = dummy_toggle,
            ToggleParam& center = dummy_toggle
        ) noexcept;

        Oscillator(
            WaveformParam& waveform,
            FloatParamS& amplitude_leader,
            FloatParamS& frequency_leader,
            FloatParamS& phase_leader,
            ToggleParam& tempo_sync = dummy_toggle,
            ToggleParam& center = dummy_toggle
        ) noexcept;

        Oscillator(
            WaveformParam& waveform,
            FloatParamS& amplitude_leader,
            FloatParamS& detune_leader,
            FloatParamS& fine_detune_leader,
            FloatParamB& harmonic_0_leader,
            FloatParamB& harmonic_1_leader,
            FloatParamB& harmonic_2_leader,
            FloatParamB& harmonic_3_leader,
            FloatParamB& harmonic_4_leader,
            FloatParamB& harmonic_5_leader,
            FloatParamB& harmonic_6_leader,
            FloatParamB& harmonic_7_leader,
            FloatParamB& harmonic_8_leader,
            FloatParamB& harmonic_9_leader,
            ModulatorSignalProducerClass* modulator = NULL,
            FloatParamS& amplitude_modulation_level_leader = dummy_param,
            FloatParamS& frequency_modulation_level_leader = dummy_param,
            FloatParamS& phase_modulation_level_leader = dummy_param
        ) noexcept;

        ~Oscillator() override;

        virtual void set_block_size(Integer const new_block_size) noexcept override;
        virtual void reset() noexcept override;

        void start(Seconds const time_offset) noexcept;
        void stop(Seconds const time_offset) noexcept;
        bool is_on() const noexcept;

        void skip_round(Integer const round, Integer const sample_count) noexcept;

        WaveformParam& waveform;

        ModulatedFloatParam modulated_amplitude;
        FloatParamS amplitude;
        ModulatedFloatParam frequency;
        ModulatedFloatParam phase;
        FloatParamS detune;
        FloatParamS fine_detune;

        FloatParamB harmonic_0;
        FloatParamB harmonic_1;
        FloatParamB harmonic_2;
        FloatParamB harmonic_3;
        FloatParamB harmonic_4;
        FloatParamB harmonic_5;
        FloatParamB harmonic_6;
        FloatParamB harmonic_7;
        FloatParamB harmonic_8;
        FloatParamB harmonic_9;

    protected:
        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        void handle_event(Event const& event) noexcept;

    private:
        static constexpr Number FREQUENCY_MIN = 0.001;
        static constexpr Number FREQUENCY_MAX = 24000.0;
        static constexpr Number FREQUENCY_DEFAULT = 440.0;

        static constexpr Number TEMPO_SYNC_FREQUENCY_SCALE = 1.0 / 60.0;

        static constexpr Integer NUMBER_OF_CHILDREN = 17;

        static constexpr Integer CUSTOM_WAVEFORM_HARMONICS = 10;

        void initialize_instance() noexcept;
        void allocate_buffers(Integer const size) noexcept;
        void free_buffers() noexcept;

        template<bool is_lfo_>
        void apply_toggle_params(
            typename std::enable_if<is_lfo_, Number const>::type bpm
        ) noexcept;

        template<bool is_lfo_>
        void apply_toggle_params(
            typename std::enable_if<!is_lfo_, Number const>::type bpm
        ) noexcept;

        void compute_amplitude_buffer(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        void compute_frequency_buffer(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        Frequency compute_frequency(
            Number const frequency,
            Number const detune,
            Number const fine_detune
        ) const noexcept;

        void compute_phase_buffer(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        void handle_start_event(Event const& event) noexcept;
        void handle_stop_event(Event const& event) noexcept;

        void initialize_first_round(Frequency const frequency) noexcept;

        template<Wavetable::Interpolation interpolation, bool single_partial>
        void render_with_constant_frequency(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        template<bool single_partial>
        void render_with_changing_frequency(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        template<bool is_lfo_, bool single_partial, Wavetable::Interpolation interpolation = Wavetable::Interpolation::DYNAMIC>
        Sample render_sample(
            typename std::enable_if<is_lfo_, Sample const>::type amplitude,
            typename std::enable_if<is_lfo_, Sample const>::type frequency,
            typename std::enable_if<is_lfo_, Sample const>::type phase
        ) noexcept;

        template<bool is_lfo_, bool single_partial = false, Wavetable::Interpolation interpolation = Wavetable::Interpolation::DYNAMIC>
        Sample render_sample(
            typename std::enable_if<!is_lfo_, Sample const>::type amplitude,
            typename std::enable_if<!is_lfo_, Sample const>::type frequency,
            typename std::enable_if<!is_lfo_, Sample const>::type phase
        ) noexcept;

        ToggleParam& tempo_sync;
        ToggleParam& center;
        WavetableState wavetable_state;
        Wavetable const* wavetables[WAVEFORMS];
        Wavetable const* wavetable;
        Wavetable* custom_waveform;
        Sample* computed_amplitude_buffer;
        Frequency* computed_frequency_buffer;
        Sample* phase_buffer;
        FloatParamB* custom_waveform_params[CUSTOM_WAVEFORM_HARMONICS];
        Number custom_waveform_coefficients[CUSTOM_WAVEFORM_HARMONICS];
        Integer custom_waveform_change_indices[CUSTOM_WAVEFORM_HARMONICS];
        Number computed_amplitude_value;
        Frequency computed_frequency_value;
        Number phase_value;
        Seconds start_time_offset;
        Number frequency_scale;
        Sample sample_offset_scale;
        bool is_on_;
        bool is_starting;
        bool computed_frequency_is_constant;
        bool computed_amplitude_is_constant;
        bool phase_is_constant;
        bool can_use_fast_interpolation;
};

}

#endif
