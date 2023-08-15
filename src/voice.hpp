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

#ifndef JS80P__VOICE_HPP
#define JS80P__VOICE_HPP

#include <string>

#include "js80p.hpp"
#include "midi.hpp"

#include "dsp/biquad_filter.hpp"
#include "dsp/filter.hpp"
#include "dsp/oscillator.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"
#include "dsp/wavefolder.hpp"


namespace JS80P
{

template<class ModulatorSignalProducerClass>
class Voice : public SignalProducer
{
    friend class SignalProducer;

    public:
        enum State {
            OFF = 0,
            ON = 1,
        };

        typedef Oscillator<ModulatorSignalProducerClass> Oscillator_;
        typedef BiquadFilter<Oscillator_> Filter1;
        typedef Wavefolder<Filter1> Wavefolder_;
        typedef BiquadFilter<Wavefolder_> Filter2;

        class VolumeApplier;

        typedef VolumeApplier ModulationOut;

        class Params
        {
            public:
                Params(std::string const name) noexcept;

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
                ToggleParam filter_1_log_scale;
                FloatParam filter_1_frequency;
                FloatParam filter_1_q;
                FloatParam filter_1_gain;

                typename Filter2::TypeParam filter_2_type;
                ToggleParam filter_2_log_scale;
                FloatParam filter_2_frequency;
                FloatParam filter_2_q;
                FloatParam filter_2_gain;
        };

        class VolumeApplier : public Filter<Filter2>
        {
            friend class SignalProducer;

            public:
                VolumeApplier(
                    Filter2& input,
                    FloatParam& velocity,
                    FloatParam& volume
                ) noexcept;

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

            private:
                FloatParam& volume;
                FloatParam& velocity;

                Sample const* volume_buffer;
                Sample const* velocity_buffer;
                Sample volume_value;
                Sample velocity_value;
        };

        static constexpr Integer CHANNELS = 2;

        Voice(
            Frequency const* frequencies,
            Midi::Note const notes,
            Params& param_leaders,
            BiquadFilterSharedCache* filter_1_shared_cache = NULL,
            BiquadFilterSharedCache* filter_2_shared_cache = NULL,
            ModulatorSignalProducerClass* modulator = NULL,
            FloatParam& amplitude_modulation_level_leader = Oscillator_::dummy_param,
            FloatParam& frequency_modulation_level_leader = Oscillator_::dummy_param,
            FloatParam& phase_modulation_level_leader = Oscillator_::dummy_param
        ) noexcept;

        virtual void reset() noexcept override;

        bool is_on() const noexcept;
        bool is_off_after(Seconds const time_offset) const noexcept;
        bool is_released() const noexcept;

        void note_on(
            Seconds const time_offset,
            Integer const note_id,
            Midi::Note const note,
            Midi::Channel const channel,
            Number const velocity,
            Midi::Note const previous_note
        ) noexcept;

        void retrigger(
            Seconds const time_offset,
            Integer const note_id,
            Midi::Note const note,
            Midi::Channel const channel,
            Number const velocity,
            Midi::Note const previous_note
        ) noexcept;

        void glide_to(
            Seconds const time_offset,
            Integer const note_id,
            Midi::Note const note,
            Midi::Channel const channel,
            Number const velocity,
            Midi::Note const previous_note
        ) noexcept;

        void note_off(
            Seconds const time_offset,
            Integer const note_id,
            Midi::Note const note,
            Number const velocity
        ) noexcept;

        void cancel_note() noexcept;

        void cancel_note_smoothly(Seconds const time_offset) noexcept;

        bool has_decayed_during_envelope_dahds() const noexcept;

        Integer get_note_id() const noexcept;
        Midi::Note get_note() const noexcept;
        Midi::Channel get_channel() const noexcept;

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

    private:
        static constexpr Number NOTE_PANNING_SCALE = 2.0 / (Number)Midi::NOTE_MAX;

        static constexpr Seconds SMOOTH_NOTE_CANCELLATION_DURATION = 0.01;

        void save_note_info(
            Integer const note_id,
            Midi::Note const note,
            Midi::Channel const channel
        ) noexcept;

        Number calculate_note_velocity(Number const raw_velocity) const noexcept;
        Number calculate_note_panning(Midi::Note const note) const noexcept;

        void set_up_oscillator_frequency(
            Seconds const time_offset,
            Midi::Note const note,
            Midi::Note const previous_note
        ) noexcept;

        bool has_decayed(FloatParam const& param) const noexcept;

        Midi::Note const notes;

        Params& param_leaders;
        Oscillator_ oscillator;
        Filter1 filter_1;
        Wavefolder_ wavefolder;
        Filter2 filter_2;
        FloatParam velocity_sensitivity;
        FloatParam note_velocity;
        FloatParam note_panning;
        FloatParam portamento_length;
        FloatParam portamento_depth;
        FloatParam panning;
        FloatParam volume;
        VolumeApplier volume_applier;
        Sample const* volume_applier_buffer;
        Sample const* panning_buffer;
        Sample const* note_panning_buffer;
        Frequency const* frequencies;
        Number panning_value;
        Number note_panning_value;
        State state;
        Integer note_id;
        Midi::Note note;
        Midi::Channel channel;

    public:
        ModulationOut& modulation_out;
};

}

#endif
