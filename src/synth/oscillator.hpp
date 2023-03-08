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

#ifndef JS80P__SYNTH__OSCILLATOR_HPP
#define JS80P__SYNTH__OSCILLATOR_HPP

#include <string>

#include "js80p.hpp"

#include "synth/param.hpp"
#include "synth/signal_producer.hpp"
#include "synth/wavetable.hpp"


namespace JS80P
{

template<class ModulatorSignalProducerClass>
class Oscillator;


typedef Oscillator<SignalProducer> SimpleOscillator;


template<class ModulatorSignalProducerClass>
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

        class WaveformParam : public Param<Waveform>
        {
            public:
                WaveformParam(std::string const name);
        };

        static constexpr Event::Type EVT_START = 1;
        static constexpr Event::Type EVT_STOP = 2;

        static FloatParam dummy_param;

        Oscillator(
            WaveformParam& waveform,
            ModulatorSignalProducerClass* modulator = NULL,
            FloatParam& amplitude_modulation_level_leader = dummy_param,
            FloatParam& frequency_modulation_level_leader = dummy_param
        );

        Oscillator(
            WaveformParam& waveform,
            FloatParam& amplitude_leader,
            FloatParam& detune_leader,
            FloatParam& fine_detune_leader,
            FloatParam& harmonic_0_leader,
            FloatParam& harmonic_1_leader,
            FloatParam& harmonic_2_leader,
            FloatParam& harmonic_3_leader,
            FloatParam& harmonic_4_leader,
            FloatParam& harmonic_5_leader,
            FloatParam& harmonic_6_leader,
            FloatParam& harmonic_7_leader,
            FloatParam& harmonic_8_leader,
            FloatParam& harmonic_9_leader,
            ModulatorSignalProducerClass* modulator = NULL,
            FloatParam& amplitude_modulation_level_leader = dummy_param,
            FloatParam& frequency_modulation_level_leader = dummy_param
        );

        ~Oscillator() override;

        virtual void set_block_size(Integer const new_block_size) override;

        void start(Seconds const time_offset);
        void stop(Seconds const time_offset);

        WaveformParam& waveform;

        ModulatedFloatParam modulated_amplitude;
        FloatParam amplitude;
        ModulatedFloatParam frequency;
        FloatParam detune;
        FloatParam fine_detune;

        FloatParam harmonic_0;
        FloatParam harmonic_1;
        FloatParam harmonic_2;
        FloatParam harmonic_3;
        FloatParam harmonic_4;
        FloatParam harmonic_5;
        FloatParam harmonic_6;
        FloatParam harmonic_7;
        FloatParam harmonic_8;
        FloatParam harmonic_9;

    protected:
        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        );

        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        );

        void handle_event(Event const& event);

    private:
        static constexpr Number FREQUENCY_MIN = 0.001;
        static constexpr Number FREQUENCY_MAX = 24000.0;
        static constexpr Number FREQUENCY_DEFAULT = 440.0;

        static constexpr Integer CUSTOM_WAVEFORM_HARMONICS = 10;

        void initialize_instance();
        void allocate_buffers(Integer const size);
        void free_buffers();

        void compute_amplitude_buffer(
            Integer const round,
            Integer const sample_count
        );
        void compute_frequency_buffer(
            Integer const round,
            Integer const sample_count
        );
        Frequency compute_frequency(
            Number const frequency,
            Number const detune,
            Number const fine_detune
        ) const;
        void handle_start_event(Event const& event);
        void handle_stop_event(Event const& event);

        WavetableState wavetable_state;
        Wavetable const* wavetables[WAVEFORMS];
        Wavetable const* wavetable;
        Wavetable* custom_waveform;
        Sample* computed_amplitude_buffer;
        Frequency* computed_frequency_buffer;
        FloatParam* custom_waveform_params[CUSTOM_WAVEFORM_HARMONICS];
        Number custom_waveform_coefficients[CUSTOM_WAVEFORM_HARMONICS];
        Integer custom_waveform_change_indices[CUSTOM_WAVEFORM_HARMONICS];
        Number computed_amplitude_value;
        Frequency computed_frequency_value;
        Seconds start_time_offset;
        bool is_on;
        bool is_starting;
        bool computed_frequency_is_constant;
        bool computed_amplitude_is_constant;
};

}

#endif
