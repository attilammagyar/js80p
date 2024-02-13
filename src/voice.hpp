/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024  Attila M. Magyar
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
#include "dsp/envelope.hpp"
#include "dsp/filter.hpp"
#include "dsp/lfo.hpp"
#include "dsp/math.hpp"
#include "dsp/oscillator.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"
#include "dsp/wavefolder.hpp"


namespace JS80P
{

constexpr int VOICE_TUNINGS = 4;

typedef Frequency FrequencyTable[VOICE_TUNINGS - 2][Midi::NOTES];
typedef Frequency PerChannelFrequencyTable[Midi::CHANNELS][Midi::NOTES];

typedef Byte OscillatorInaccuracyLevel;


class OscillatorInaccuracyParam : public Param<OscillatorInaccuracyLevel, ParamEvaluation::BLOCK>
{
    public:
        explicit OscillatorInaccuracyParam(std::string const& name) noexcept;
};


class OscillatorInaccuracy
{
    public:
        static constexpr Integer MAX_LEVEL = 60;

        static constexpr Number MIN = 0.1;
        static constexpr Number MAX = 1.0;

        static Frequency detune(
            Frequency const frequency,
            OscillatorInaccuracyParam const& level_param,
            Number const inaccuracy
        ) noexcept;

        static Number calculate_new_inaccuracy(Number const seed) noexcept;

        explicit OscillatorInaccuracy(Number const seed);

        Number get_inaccuracy() const noexcept;

        void update(Integer const round) noexcept;
        void reset() noexcept;

    private:
        static constexpr Number MAX_WIDTH = 110.0;

        static constexpr Number DELTA = MAX - MIN;

        /* False-positive, it is used in voice.cpp for initializing CENTS. */
        // cppcheck-suppress unusedPrivateFunction
        static constexpr Number interval_width(Integer const level);

        /* False-positive, it is used in voice.cpp for initializing CENTS. */
        // cppcheck-suppress unusedPrivateFunction
        static constexpr Number interval_min(Integer const level);

        static Number const CENTS[MAX_LEVEL + 1][2];

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
                explicit TuningParam(std::string const& name) noexcept;
        };

        class Dummy
        {
            public:
                Dummy();

                Dummy(
                    std::string const& a,
                    Number const b,
                    Number const c,
                    Number const d,
                    Number const e,
                    Envelope* const* envelopes,
                    LFO* const* lfos
                );
        };

        class Params
        {
            public:
                explicit Params(
                    std::string const& name,
                    Envelope* const* envelopes = NULL,
                    LFO* const* lfos = NULL
                ) noexcept;

                TuningParam tuning;
                OscillatorInaccuracyParam oscillator_inaccuracy;
                OscillatorInaccuracyParam oscillator_instability;

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
                ToggleParam filter_1_freq_log_scale;
                ToggleParam filter_1_q_log_scale;
                FloatParamS filter_1_frequency;
                FloatParamS filter_1_q;
                FloatParamS filter_1_gain;
                FloatParamB filter_1_freq_inaccuracy;
                FloatParamB filter_1_q_inaccuracy;

                typename Filter2::TypeParam filter_2_type;
                ToggleParam filter_2_freq_log_scale;
                ToggleParam filter_2_q_log_scale;
                FloatParamS filter_2_frequency;
                FloatParamS filter_2_q;
                FloatParamS filter_2_gain;
                FloatParamB filter_2_freq_inaccuracy;
                FloatParamB filter_2_q_inaccuracy;

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
                    FloatParamS& volume,
                    SignalProducer* const buffer_owner = NULL
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
        static constexpr Tuning TUNING_432HZ_12TET = 1;
        static constexpr Tuning TUNING_MTS_ESP_CONTINUOUS = 2;
        static constexpr Tuning TUNING_MTS_ESP_NOTE_ON = 3;

        Voice(
            FrequencyTable const& frequencies,
            PerChannelFrequencyTable const& per_channel_frequencies,
            OscillatorInaccuracy& synced_oscillator_inaccuracy,
            Number const oscillator_inaccuracy_seed,
            Params& param_leaders,
            BiquadFilterSharedBuffers* filter_1_shared_buffers = NULL,
            BiquadFilterSharedBuffers* filter_2_shared_buffers = NULL,
            Envelope* const* envelopes = NULL,
            LFO* const* lfos = NULL
        ) noexcept;

        Voice(
            FrequencyTable const& frequencies,
            PerChannelFrequencyTable const& per_channel_frequencies,
            OscillatorInaccuracy& synced_oscillator_inaccuracy,
            Number const oscillator_inaccuracy_seed,
            Params& param_leaders,
            ModulatorSignalProducerClass& modulator,
            FloatParamS& amplitude_modulation_level_leader,
            FloatParamS& frequency_modulation_level_leader,
            FloatParamS& phase_modulation_level_leader,
            BiquadFilterSharedBuffers* filter_1_shared_buffers = NULL,
            BiquadFilterSharedBuffers* filter_2_shared_buffers = NULL,
            Envelope* const* envelopes = NULL,
            LFO* const* lfos = NULL
        ) noexcept;

        virtual void reset() noexcept override;

        bool is_on() const noexcept;
        bool is_off_after(Seconds const time_offset) const noexcept;
        bool is_released() const noexcept;

        void update_inaccuracy(Integer const round) noexcept;

        void note_on(
            Seconds const time_offset,
            Integer const note_id,
            Midi::Note const note,
            Midi::Channel const channel,
            Number const velocity,
            Midi::Note const previous_note,
            bool const should_sync_oscillator_inaccuracy,
            LFOEnvelopeMapping const& lfo_envelope_mapping
        ) noexcept;

        void retrigger(
            Seconds const time_offset,
            Integer const note_id,
            Midi::Note const note,
            Midi::Channel const channel,
            Number const velocity,
            Midi::Note const previous_note,
            bool const should_sync_oscillator_inaccuracy,
            LFOEnvelopeMapping const& lfo_envelope_mapping
        ) noexcept;

        void glide_to(
            Seconds const time_offset,
            Integer const note_id,
            Midi::Note const note,
            Midi::Channel const channel,
            Number const velocity,
            Midi::Note const previous_note,
            bool const should_sync_oscillator_inaccuracy,
            LFOEnvelopeMapping const& lfo_envelope_mapping
        ) noexcept;

        void note_off(
            Seconds const time_offset,
            Integer const note_id,
            Midi::Note const note,
            Number const velocity
        ) noexcept;

        void cancel_note() noexcept;

        void cancel_note_smoothly(Seconds const time_offset) noexcept;

        bool has_decayed_before_note_off() const noexcept;

        Integer get_note_id() const noexcept;
        Midi::Note get_note() const noexcept;
        Midi::Channel get_channel() const noexcept;
        Number get_inaccuracy() const noexcept;

        template<bool should_sync_oscillator_inaccuracy, bool should_sync_oscillator_instability>
        void update_note_frequency_for_continuous_mts_esp(Integer const round) noexcept;

        template<bool should_sync_oscillator_inaccuracy>
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

        static constexpr Seconds MTS_ESP_CORRECTION_DURATION = 0.003;

        static constexpr Seconds MIN_DRIFT_DURATION = 0.3;
        static constexpr Seconds DRIFT_DURATION_DELTA = 3.2;

        void initialize_instance(Number const oscillator_inaccuracy_seed) noexcept;

        Number make_random_seed(Number const random) const noexcept;

        void save_note_info(
            Integer const note_id,
            Midi::Note const note,
            Midi::Channel const channel
        ) noexcept;

        Frequency get_note_frequency(
            Midi::Note const note,
            Midi::Channel const channel
        ) const noexcept;

        template<bool should_sync>
        Frequency detune(
            Frequency const frequency,
            OscillatorInaccuracyParam const& level_param
        ) const noexcept;

        template<bool should_sync_oscillator_inaccuracy>
        Frequency calculate_note_frequency_drift_target() const noexcept;

        Number calculate_note_velocity(Number const raw_velocity) const noexcept;
        Number calculate_note_panning(Midi::Note const note) const noexcept;

        template<bool should_sync_oscillator_inaccuracy>
        void set_up_oscillator_frequency(
            Seconds const time_offset,
            Midi::Note const note,
            Midi::Channel const channel,
            Midi::Note const previous_note
        ) noexcept;

        bool is_oscillator_starting_or_stopping_or_expecting_glide() const noexcept;

        bool has_decayed(FloatParamS const& param) const noexcept;

        Number const oscillator_inaccuracy_seed;

        Params& param_leaders;
        FrequencyTable const& frequencies;
        PerChannelFrequencyTable const& per_channel_frequencies;
        OscillatorInaccuracy& synced_oscillator_inaccuracy;
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
        Number oscillator_inaccuracy;
        Number panning_value;
        Number note_panning_value;
        Frequency nominal_frequency;
        Frequency note_frequency;
        State state;
        Integer note_id;
        Midi::Note note;
        Midi::Channel channel;
        bool is_drifting;

    public:
        ModulationOut& modulation_out;
};


typedef Voice<SignalProducer> Modulator;
typedef Voice<Modulator::ModulationOut> Carrier;

}

#endif
