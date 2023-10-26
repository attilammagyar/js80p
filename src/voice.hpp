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
#include <type_traits>

#include "js80p.hpp"
#include "midi.hpp"

#include "dsp/biquad_filter.hpp"
#include "dsp/distortion.hpp"
#include "dsp/filter.hpp"
#include "dsp/math.hpp"
#include "dsp/oscillator.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"
#include "dsp/wavefolder.hpp"


namespace JS80P
{

constexpr int VOICE_TUNINGS = 16;

typedef Frequency FrequencyTable[VOICE_TUNINGS - 2][Midi::NOTES];
typedef Frequency PerChannelFrequencyTable[Midi::CHANNELS][Midi::NOTES];


class Inaccuracy
{
    public:
        static Number calculate_new_inaccuracy(Number const seed) noexcept;

        Inaccuracy(Number const seed);

        Number get_inaccuracy() const noexcept;

        void update(Integer const round) noexcept;
        void reset() noexcept;

    private:
        Number const seed;

        Number inaccuracy;
        Integer last_update_round;
};


template<class ModulatorSignalProducerClass>
class Voice : public SignalProducer
{
    friend class SignalProducer;

    private:
        static constexpr bool IS_MODULATOR = (
            std::is_same<ModulatorSignalProducerClass, SignalProducer>::value
        );

        static constexpr bool IS_CARRIER = !IS_MODULATOR;

        static constexpr Integer NUMBER_OF_CHILDREN = 10;

    public:
        enum State {
            OFF = 0,
            ON = 1,
        };

        typedef Oscillator<ModulatorSignalProducerClass> Oscillator_;
        typedef BiquadFilter<Oscillator_> Filter1;
        typedef Wavefolder<Filter1> Wavefolder_;
        typedef Distortion::Distortion<Wavefolder_> Distortion_;

        typedef typename std::conditional<IS_CARRIER, Distortion_, Wavefolder_>::type Filter2Input;

        typedef BiquadFilter<Filter2Input> Filter2;

        class VolumeApplier;

        typedef VolumeApplier ModulationOut;

        typedef Byte Tuning;

        class TuningParam : public Param<Tuning, ParamEvaluation::BLOCK>
        {
            public:
                TuningParam(std::string const name) noexcept;
        };

        class Dummy
        {
            public:
                Dummy();

                Dummy(
                    std::string const a,
                    Number const b,
                    Number const c,
                    Number const d
                );
        };

        class Params
        {
            public:
                Params(std::string const name) noexcept;

                TuningParam tuning;

                typename Oscillator_::WaveformParam waveform;
                FloatParamS amplitude;
                FloatParamB velocity_sensitivity;
                FloatParamS folding;
                FloatParamB portamento_length;
                FloatParamB portamento_depth;
                FloatParamS detune;
                FloatParamS fine_detune;
                FloatParamB width;
                FloatParamS panning;
                FloatParamS volume;

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

                typename Filter1::TypeParam filter_1_type;
                ToggleParam filter_1_log_scale;
                FloatParamS filter_1_frequency;
                FloatParamS filter_1_q;
                FloatParamS filter_1_gain;

                typename Filter2::TypeParam filter_2_type;
                ToggleParam filter_2_log_scale;
                FloatParamS filter_2_frequency;
                FloatParamS filter_2_q;
                FloatParamS filter_2_gain;

                typename std::conditional<IS_MODULATOR, FloatParamS, Dummy>::type subharmonic_amplitude;
                typename std::conditional<IS_CARRIER, FloatParamS, Dummy>::type distortion;
        };

        class VolumeApplier : public Filter<Filter2>
        {
            friend class SignalProducer;

            public:
                VolumeApplier(
                    Filter2& input,
                    FloatParamS& velocity,
                    FloatParamS& volume
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
                FloatParamS& volume;
                FloatParamS& velocity;

                Sample const* volume_buffer;
                Sample const* velocity_buffer;
                Sample volume_value;
                Sample velocity_value;
        };

        static constexpr Integer CHANNELS = 2;

        static constexpr Tuning TUNING_440HZ_12TET = 0;
        static constexpr Tuning TUNING_440HZ_12TET_INACCURATE_1 = 1;
        static constexpr Tuning TUNING_440HZ_12TET_INACCURATE_2_SYNCED = 2;
        static constexpr Tuning TUNING_440HZ_12TET_INACCURATE_3 = 3;
        static constexpr Tuning TUNING_440HZ_12TET_INACCURATE_4 = 4;
        static constexpr Tuning TUNING_440HZ_12TET_INACCURATE_5_SYNCED = 5;
        static constexpr Tuning TUNING_440HZ_12TET_INACCURATE_6 = 6;
        static constexpr Tuning TUNING_432HZ_12TET = 7;
        static constexpr Tuning TUNING_432HZ_12TET_INACCURATE_1 = 8;
        static constexpr Tuning TUNING_432HZ_12TET_INACCURATE_2_SYNCED = 9;
        static constexpr Tuning TUNING_432HZ_12TET_INACCURATE_3 = 10;
        static constexpr Tuning TUNING_432HZ_12TET_INACCURATE_4 = 11;
        static constexpr Tuning TUNING_432HZ_12TET_INACCURATE_5_SYNCED = 12;
        static constexpr Tuning TUNING_432HZ_12TET_INACCURATE_6 = 13;
        static constexpr Tuning TUNING_MTS_ESP_NOTE_ON = 14;
        static constexpr Tuning TUNING_MTS_ESP_REALTIME = 15;

        static bool is_tuning_unstable(Tuning const tuning) noexcept;

        static bool is_tuning_synced_unstable(Tuning const tuning) noexcept;

        Voice(
            FrequencyTable const& frequencies,
            PerChannelFrequencyTable const& per_channel_frequencies,
            Inaccuracy& synced_inaccuracy,
            Number const inaccuracy_seed,
            Params& param_leaders,
            BiquadFilterSharedCache* filter_1_shared_cache = NULL,
            BiquadFilterSharedCache* filter_2_shared_cache = NULL
        ) noexcept;

        Voice(
            FrequencyTable const& frequencies,
            PerChannelFrequencyTable const& per_channel_frequencies,
            Inaccuracy& synced_inaccuracy,
            Number const inaccuracy_seed,
            Params& param_leaders,
            ModulatorSignalProducerClass& modulator,
            FloatParamS& amplitude_modulation_level_leader,
            FloatParamS& frequency_modulation_level_leader,
            FloatParamS& phase_modulation_level_leader,
            BiquadFilterSharedCache* filter_1_shared_cache = NULL,
            BiquadFilterSharedCache* filter_2_shared_cache = NULL
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
        Number get_inaccuracy() const noexcept;

        void update_note_frequency_for_realtime_mts_esp() noexcept;

        template<bool is_synced>
        void update_unstable_note_frequency(Integer const round) noexcept;

        void render_oscillator(Integer const round, Integer const sample_count) noexcept;

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
        typedef typename std::conditional<IS_CARRIER, Distortion_, Dummy>::type DistortionInstance;

        static constexpr Number NOTE_PANNING_SCALE = 2.0 / (Number)Midi::NOTE_MAX;

        static constexpr Seconds SMOOTH_NOTE_CANCELLATION_DURATION = 0.01;

        void initialize_instance(Number const inaccuracy_seed) noexcept;

        void save_note_info(
            Integer const note_id,
            Midi::Note const note,
            Midi::Channel const channel
        ) noexcept;

        void update_inaccuracy() noexcept;

        Frequency calculate_note_frequency(
            Midi::Note const note,
            Midi::Channel const channel
        ) const noexcept;

        Frequency calculate_inaccurate_note_frequency(
            Tuning const tuning,
            Midi::Note const note,
            Midi::Channel const channel
        ) const noexcept;

        Number calculate_note_velocity(Number const raw_velocity) const noexcept;
        Number calculate_note_panning(Midi::Note const note) const noexcept;

        void set_up_oscillator_frequency(
            Seconds const time_offset,
            Midi::Note const note,
            Midi::Channel const channel,
            Midi::Note const previous_note
        ) noexcept;

        bool is_oscillator_starting_or_stopping_or_expecting_glide() const noexcept;

        bool has_decayed(FloatParamS const& param) const noexcept;

        Number const inaccuracy_seed;

        Params& param_leaders;
        FrequencyTable const& frequencies;
        PerChannelFrequencyTable const& per_channel_frequencies;
        Inaccuracy& synced_inaccuracy;
        Oscillator_ oscillator;
        Filter1 filter_1;
        Wavefolder_ wavefolder;
        DistortionInstance distortion;
        Filter2 filter_2;
        FloatParamS note_velocity;
        FloatParamS note_panning;
        FloatParamS panning;
        FloatParamS volume;
        VolumeApplier volume_applier;
        Sample const* volume_applier_buffer;
        Sample const* panning_buffer;
        Sample const* note_panning_buffer;
        Number inaccuracy;
        Number panning_value;
        Number note_panning_value;
        State state;
        Integer note_id;
        Midi::Note note;
        Midi::Channel channel;

    public:
        ModulationOut& modulation_out;
};


typedef Voice<SignalProducer> Modulator;
typedef Voice<Modulator::ModulationOut> Carrier;

}

#endif
