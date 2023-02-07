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

#ifndef JS80P__SYNTH__VOICE_HPP
#define JS80P__SYNTH__VOICE_HPP

#include <string>

#include "js80p.hpp"
#include "midi.hpp"

#include "synth/biquad_filter.hpp"
#include "synth/oscillator.hpp"
#include "synth/param.hpp"
#include "synth/signal_producer.hpp"
#include "synth/wavefolder.hpp"


namespace JS80P
{

template<class ModulatorSignalProducerClass>
class Voice : public SignalProducer
{
    public:
        enum State {
            OFF = 0,
            ON = 1,
        };

        typedef Oscillator<ModulatorSignalProducerClass> Oscillator_;
        typedef BiquadFilter<Oscillator_> Filter1;
        typedef Wavefolder<Filter1> Wavefolder_;
        typedef BiquadFilter<Wavefolder_> Filter2;

        // TODO: modulation output should incorporate the calculated velocity and the volume parameter as well
        typedef Filter2 ModulationOut;

        class Params
        {
            public:
                Params(std::string const name);

                typename Oscillator_::WaveformParam waveform;
                FloatParam amplitude;
                FloatParam velocity_sensitivity;
                FloatParam folding;
                FloatParam portamento_length;
                FloatParam portamento_depth;
                FloatParam detune;
                FloatParam fine_detune;
                FloatParam width;
                FloatParam panning;
                FloatParam volume;

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

                typename Filter1::TypeParam filter_1_type;
                FloatParam filter_1_frequency;
                FloatParam filter_1_q;
                FloatParam filter_1_gain;

                typename Filter2::TypeParam filter_2_type;
                FloatParam filter_2_frequency;
                FloatParam filter_2_q;
                FloatParam filter_2_gain;
        };

        static constexpr Integer CHANNELS = 2;

        Voice(
            Frequency const* frequencies,
            Midi::Note const notes,
            Params& param_leaders,
            ModulatorSignalProducerClass* modulator = NULL,
            FloatParam& amplitude_modulation_level_leader = Oscillator_::dummy_param,
            FloatParam& frequency_modulation_level_leader = Oscillator_::dummy_param
        );

        bool is_on() const;
        bool is_off_after(Seconds const time_offset) const;

        void note_on(
            Seconds const time_offset,
            Midi::Note const note,
            Number const velocity,
            Midi::Note const previous_note
        );
        void note_off(
            Seconds const time_offset,
            Midi::Note const note,
            Number const velocity
        );

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

    private:
        Number calculate_velocity(Number const raw_velocity) const;
        void set_up_oscillator_frequency(
            Seconds const time_offset,
            Midi::Note const note,
            Midi::Note const previous_note
        );

        Midi::Note const notes;

        Oscillator_ oscillator;
        Filter1 filter_1;
        Wavefolder_ wavefolder;
        Filter2 filter_2;
        FloatParam velocity_sensitivity;
        FloatParam portamento_length;
        FloatParam portamento_depth;
        FloatParam width; // TODO: implement
        FloatParam panning; // TODO: implement
        FloatParam volume; // TODO: implement
        Frequency const* frequencies;
        Sample const* oscillator_buffer;
        Seconds off_after;
        Number velocity;
        State state;
        Midi::Note note;

    public:
        ModulationOut& modulation_out;
};

}

#endif
