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


namespace JS80P
{

template<class ModulatorSignalProducerClass>
class Oscillator;


typedef Oscillator<SignalProducer> SimpleOscillator;


template<class ModulatorSignalProducerClass>
class Oscillator : public SignalProducer
{
    public:
        typedef ModulatableFloatParam<ModulatorSignalProducerClass> ModulatedFloatParam;

        typedef Byte Waveform;

        static constexpr Waveform SINE = 0;
        static constexpr Waveform SAWTOOTH = 1;
        static constexpr Waveform INVERSE_SAWTOOTH = 2;
        static constexpr Waveform TRIANGLE = 3;
        static constexpr Waveform SQUARE = 4;
        static constexpr Waveform CUSTOM = 5;

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

    private:
        // https://www.music.mcgill.ca/~gary/307/week4/wavetables.html
        // https://www.music.mcgill.ca/~gary/307/week5/node12.html

        static constexpr Number FREQUENCY_MIN = 0.001;
        static constexpr Number FREQUENCY_MAX = 24000.0;
        static constexpr Number FREQUENCY_DEFAULT = 440.0;

        static constexpr Integer CUSTOM_WAVEFORM_HARMONICS = 10;

        class WavetableState
        {
            public:
                WavetableState();

                Number scale;
                Number sample_index;
                Number table_weights[2];
                Frequency nyquist_frequency;
                Frequency interpolation_limit;
                Integer table_indices[2];
                bool needs_table_interpolation;
        };

        class Wavetable
        {
            public:
                /*
                20 Hz at 48 kHz sampling rate has a wavelength of 2400 samples,
                so 2048 samples per waveform with linear interpolation should be
                good enough for most of the audible spectrum.

                Better interpolation is needed though when frequency is
                significantly lower than sample_rate / SIZE.
                */
                static constexpr Integer SIZE = 0x0800;
                static constexpr Integer MASK = 0x07ff;

                /*
                The Nyquist limit for 48 kHz sampling rate is 24 kHz, which
                can represent up to 384 partials of a 62.5 Hz sawtooth wave.
                So with 384 partials, we only start to loose high frequencies
                for notes below B1.
                */
                static constexpr Integer PARTIALS = 384;

                static constexpr Number SIZE_FLOAT = (Number)SIZE;
                static constexpr Number SIZE_INV = 1.0 / SIZE_FLOAT;
                static constexpr Frequency INTERPOLATION_LIMIT_SCALE = (
                    1.0 / (2.0 * (Frequency)SIZE_FLOAT)
                );

                static void initialize();

                static void reset_state(
                    WavetableState* state,
                    Seconds const sampling_period,
                    Frequency const nyquist_frequency,
                    Frequency const frequency,
                    Seconds const time_offset
                );

                Wavetable(
                    Number const coefficients[],
                    Integer const coefficients_length
                );

                ~Wavetable();

                Sample lookup(
                    WavetableState* state,
                    Frequency const frequency
                ) const;

                void update_coefficients(
                    Number const coefficients[],
                    bool const normalize
                );

            private:
                static Number sines[SIZE];
                static bool is_initialized;

                Number wrap_around(Number const index) const;

                Sample interpolate(
                    WavetableState const* state,
                    Frequency const frequency,
                    Number const sample_index
                ) const;

                Sample interpolate_sample_linear(
                    WavetableState const* state,
                    Number const sample_index
                ) const;

                Sample interpolate_sample_lagrange(
                    WavetableState const* state,
                    Number const sample_index
                ) const;

                Integer const partials;

                Sample** samples;
        };

        static void initialize_wavetables();
        static void free_wavetables();

        static Wavetable* sine;
        static Wavetable* sawtooth;
        static Wavetable* inverse_sawtooth;
        static Wavetable* triangle;
        static Wavetable* square;
        static Integer instances;

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
        Wavetable* wavetables[6];
        Wavetable* wavetable;
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
