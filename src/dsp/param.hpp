/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025  Attila M. Magyar
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

#ifndef JS80P__DSP__PARAM_HPP
#define JS80P__DSP__PARAM_HPP

#include <string>
#include <type_traits>
#include <vector>

#include "js80p.hpp"

#include "dsp/math.hpp"
#include "dsp/midi_controller.hpp"
#include "dsp/signal_producer.hpp"
#include "dsp/queue.hpp"
#include "dsp/wavetable.hpp"


namespace JS80P
{

class Envelope;


class Macro;


class LFO;


enum ParamEvaluation {
    BLOCK = 0,  ///< The parameter is evaluated once at the beginning of each rendering block
    SAMPLE = 1, ///< The parameter is evaluated for each rendered sample
};


/**
 * \brief A variable that can influence the synthesized sound or other
 *        parameters.
 */
template<typename NumberType, ParamEvaluation evaluation = ParamEvaluation::SAMPLE>
class Param : public SignalProducer
{
    friend class SignalProducer;

    public:
        Param(
            std::string const& name,
            NumberType const min_value,
            NumberType const max_value,
            NumberType const default_value,
            Integer const number_of_events = 0,
            SignalProducer* const buffer_owner = NULL,
            Integer const number_of_children = 0
        ) noexcept;

        ParamEvaluation get_evaluation() const noexcept;
        std::string const& get_name() const noexcept;
        NumberType get_default_value() const noexcept;
        NumberType get_value() const noexcept;
        NumberType get_min_value() const noexcept;
        NumberType get_max_value() const noexcept;
        void set_value(NumberType const new_value) noexcept;

        Number get_ratio() const noexcept;
        Number get_default_ratio() const noexcept;
        void set_ratio(Number const ratio) noexcept;

        NumberType ratio_to_value(Number const ratio) const noexcept;
        Number value_to_ratio(NumberType const value) const noexcept;

        void set_midi_controller(MidiController* midi_controller) noexcept;
        MidiController* get_midi_controller() const noexcept;

        void set_macro(Macro* macro) noexcept;
        Macro* get_macro() const noexcept;

        /**
         * \brief Whenever the value of the parameter changes, the change index
         *        gets incremented. You may cache the result of a slow
         *        calculation that depends on a parameter value, as long as the
         *        change index remains unchanged.
         */
        Integer get_change_index() const noexcept;

    protected:
        template<class ParamClass>
        static void set_midi_controller(
            ParamClass& param,
            MidiController* midi_controller
        ) noexcept;

        template<class ParamClass>
        static void set_macro(ParamClass& param, Macro* macro) noexcept;

        NumberType get_raw_value() const noexcept;

        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        NumberType clamp(NumberType const value) const noexcept;
        void store_new_value(NumberType const new_value) noexcept;

        std::string const name;
        NumberType const min_value;
        NumberType const max_value;
        NumberType const range;
        NumberType const default_value;

        MidiController* midi_controller;

        Macro* macro;
        Integer macro_change_index;

    private:
        Number const range_as_float_number;
        Number const range_inv;

        Integer change_index;
        NumberType value;
};


class ByteParam : public Param<Byte, ParamEvaluation::BLOCK>
{
    friend class SignalProducer;

    public:
        ByteParam(
            std::string const& name,
            Byte const min_value,
            Byte const max_value,
            Byte const default_value
        ) noexcept;
};


template class Param<Byte, ParamEvaluation::BLOCK>;


class ToggleParam : public ByteParam
{
    friend class SignalProducer;

    public:
        static constexpr Byte OFF = 0;
        static constexpr Byte ON = 1;

        ToggleParam(std::string const& name, Byte const default_value);
};


template<ParamEvaluation evaluation>
class FloatParam;


typedef FloatParam<ParamEvaluation::SAMPLE> FloatParamS;
typedef FloatParam<ParamEvaluation::BLOCK> FloatParamB;


constexpr size_t ENVELOPE_RANDOMS_COUNT = 9;


typedef Number EnvelopeRandoms[ENVELOPE_RANDOMS_COUNT];


typedef Byte EnvelopeShape;


/* The VST 3 SDK uses a macro named "RELEASE", hence the prefix. */
enum EnvelopeStage {
    ENV_STG_NONE = 0,
    ENV_STG_DAHD = 1,
    ENV_STG_SUSTAIN = 2,
    ENV_STG_RELEASE = 3,
    ENV_STG_RELEASED = 4,
};


class EnvelopeSnapshot
{
    public:
        EnvelopeSnapshot() noexcept;

        EnvelopeSnapshot(EnvelopeSnapshot const& snapshot) noexcept = default;
        EnvelopeSnapshot(EnvelopeSnapshot&& snapshot) noexcept = default;

        EnvelopeSnapshot& operator=(EnvelopeSnapshot const& snapshot) noexcept = default;
        EnvelopeSnapshot& operator=(EnvelopeSnapshot&& snapshot) noexcept = default;

        Number initial_value;
        Number peak_value;
        Number sustain_value;
        Number final_value;

        Seconds delay_time;
        Seconds attack_time;
        Seconds hold_time;
        Seconds decay_time;
        Seconds release_time;

        Integer change_index;

        EnvelopeShape attack_shape;
        EnvelopeShape decay_shape;
        EnvelopeShape release_shape;

        Byte envelope_index;
};


class LFOEnvelopeState
{
    public:
        LFOEnvelopeState() = default;

        WavetableState wavetable_state;
        EnvelopeSnapshot snapshot;
        Integer active_snapshot_id;
        Integer scheduled_snapshot_id;
        Seconds time;
        Sample value;
        EnvelopeStage stage;
        Byte active_snapshot_envelope_index;
        Byte scheduled_snapshot_envelope_index;
        bool is_wavetable_initialized;
};


typedef LFOEnvelopeState LFOEnvelopeStates[Constants::PARAM_LFO_ENVELOPE_STATES];


/**
 * \brief Parameter with floating point values. Values can be scheduled at
 *        time offsets, or can be approached linearly over a given duration of
 *        time.
 */
template<ParamEvaluation evaluation>
class FloatParam : public Param<Number, evaluation>
{
    friend class SignalProducer;

    private:
        static constexpr Integer NUMBER_OF_EVENTS = (
            evaluation == ParamEvaluation::SAMPLE ? 32 : 0
        );

    public:
        static constexpr SignalProducer::Event::Type EVT_SET_VALUE = 1;
        static constexpr SignalProducer::Event::Type EVT_LINEAR_RAMP = 2;
        static constexpr SignalProducer::Event::Type EVT_LOG_RAMP = 3;
        static constexpr SignalProducer::Event::Type EVT_CURVED_RAMP = 4;
        static constexpr SignalProducer::Event::Type EVT_ENVELOPE_START = 5;
        static constexpr SignalProducer::Event::Type EVT_ENVELOPE_UPDATE = 6;
        static constexpr SignalProducer::Event::Type EVT_ENVELOPE_END = 7;
        static constexpr SignalProducer::Event::Type EVT_ENVELOPE_CANCEL = 8;
        static constexpr SignalProducer::Event::Type EVT_LFO_ENVELOPE_RESET = 9;
        static constexpr SignalProducer::Event::Type EVT_LFO_ENVELOPE_START = 10;
        static constexpr SignalProducer::Event::Type EVT_LFO_ENVELOPE_UPDATE = 11;
        static constexpr SignalProducer::Event::Type EVT_LFO_ENVELOPE_END = 12;
        static constexpr SignalProducer::Event::Type EVT_LFO_ENVELOPE_CANCEL = 13;

        /*
        Some MIDI controllers seem to send multiple changes of the same value with
        the same timestamp (on the same channel). In order to avoid zero duration
        ramps and sudden jumps, we force every value change to take place gradually,
        over a duration which correlates with the magnitude of the change.
        */
        static constexpr Seconds MIDI_CTL_BIG_CHANGE_DURATION = 0.20;
        static constexpr Seconds MIDI_CTL_SMALL_CHANGE_DURATION = (
            MIDI_CTL_BIG_CHANGE_DURATION / 2.5
        );

        /**
         * \brief Orchestrate rendering signals and handling events.
         *        See \c SignalProducer::process()
         */
        template<class FloatParamClass>
        static Sample const* const* produce(
            FloatParamClass& float_param,
            Integer const round,
            Integer const sample_count = -1
        ) noexcept;

        /**
         * \brief Render the single channel of the parameter if it has scheduled
         *        changes during this round, but if the parameter is constant
         *        during the round, then return \c NULL. A shortcut for the
         *        following construct:
         *
         *        \code{.cpp}
         *        if (param.is_constant_in_next_round(round, sample_count) {
         *            param.skip_round(round, sample_count);
         *        } else {
         *            param_buffer = FloatParamS::produce<FloatParamS>(
         *                param, round, sample_count
         *            )[0];
         *        }
         *        \endcode
         */
        template<class FloatParamClass = FloatParam<ParamEvaluation::SAMPLE> >
        static Sample const* produce_if_not_constant(
            FloatParamClass& float_param,
            Integer const round,
            Integer const sample_count
        ) noexcept;

        explicit FloatParam(
            std::string const& name = "",
            Number const min_value = -1.0,
            Number const max_value = 1.0,
            Number const default_value = 0.0,
            Number const round_to = 0.0,
            Envelope* const* const envelopes = NULL,
            ToggleParam const* log_scale_toggle = NULL,
            Number const* log_scale_table = NULL,
            int const log_scale_table_max_index = 0,
            Number const log_scale_table_index_scale = 0.0,
            Number const log_scale_value_offset = 0.0,
            Number const number_of_children = 0
        ) noexcept;

        explicit FloatParam(
            Byte const& voice_status,
            std::string const& name = "",
            Number const min_value = -1.0,
            Number const max_value = 1.0,
            Number const default_value = 0.0,
            Number const round_to = 0.0,
            Envelope* const* const envelopes = NULL,
            ToggleParam const* log_scale_toggle = NULL,
            Number const* log_scale_table = NULL,
            int const log_scale_table_max_index = 0,
            Number const log_scale_table_index_scale = 0.0,
            Number const log_scale_value_offset = 0.0,
            Number const number_of_children = 0
        ) noexcept;

        /**
         * \warning When the leader needs to be rendered, it will be rendered as
         *          a \c FloatParam<evaluation>, even if it's a descendant.
         *          Practically, this means that only \c FloatParam objects can
         *          be leaders.
         *
         * \warning When a polyphonic \c FloatParam has a leader, then its
         *          rendered buffer must be immediately used (or copied) for
         *          each polyphonic voice after rendering it, in order to avoid
         *          getting overwritten by other polyphonic voices. All
         *          \c FloatParam instances with the same leader use the
         *          leader's buffer, even when they are running with envelopes
         *          or polyphonic LFOs.
         */
        FloatParam(FloatParam<evaluation>& leader) noexcept;

        FloatParam(FloatParam<evaluation>& leader, Byte const& voice_status) noexcept;

        ~FloatParam() override;

        bool is_logarithmic() const noexcept;

        void set_value(Number const new_value) noexcept;
        Number get_value() const noexcept;
        void set_ratio(Number const ratio) noexcept;
        Number get_ratio() const noexcept;
        Number get_default_ratio() const noexcept;
        ToggleParam const* get_log_scale_toggle() const noexcept;
        Number const* get_log_scale_table() const noexcept;
        int get_log_scale_table_max_index() const noexcept;
        Number get_log_scale_table_index_scale() const noexcept;
        Number get_log_scale_value_offset() const noexcept;

        Number ratio_to_value(Number const ratio) const noexcept;
        Number value_to_ratio(Number const value) const noexcept;

        void ratios_to_values(
            Sample* const buffer,
            Integer const first_sample_index,
            Integer const last_sample_index
        ) const noexcept;

        void ratios_to_values(
            Sample const* const source_buffer,
            Sample* const target_buffer,
            Integer const first_sample_index,
            Integer const last_sample_index
        ) const noexcept;

        Integer get_change_index() const noexcept;

        bool is_constant_in_next_round(
            Integer const round, Integer const sample_count
        ) noexcept;

        bool is_constant_until(Integer const sample_count) const noexcept;

        void skip_round(Integer const round, Integer const sample_count) noexcept;

        void schedule_value(
            Seconds const time_offset,
            Number const new_value
        ) noexcept;

        void schedule_linear_ramp(
            Seconds const duration,
            Number const target_value
        ) noexcept;

        void schedule_curved_ramp(
            Seconds const duration,
            Number const target_value,
            Math::EnvelopeShape const shape
        ) noexcept;

        bool is_ramping() const noexcept;
        Seconds get_remaining_time_from_linear_ramp() const noexcept;

        void set_midi_controller(MidiController* midi_controller) noexcept;
        MidiController* get_midi_controller() const noexcept;

        void set_macro(Macro* macro) noexcept;
        Macro* get_macro() const noexcept;

        void set_random_seed(Number const seed) noexcept;
        void set_envelope(Envelope* const envelope) noexcept;
        Envelope* get_envelope() const noexcept;
        Envelope* const* get_envelopes() const noexcept;

        bool has_lfo_with_envelope() const noexcept;

        bool is_polyphonic() const noexcept;

        void start_envelope(
            Seconds const time_offset,
            Number const random_1,
            Number const random_2
        ) noexcept;

        Seconds end_envelope(Seconds const time_offset) noexcept;
        void cancel_envelope(Seconds const time_offset, Seconds const duration) noexcept;
        void update_envelope(Seconds const time_offset) noexcept;

        bool has_envelope_decayed() const noexcept;

        void set_lfo(LFO* lfo) noexcept;
        LFO* get_lfo() const noexcept;

        virtual void reset() noexcept override;

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

        void handle_event(SignalProducer::Event const& event) noexcept;

        bool should_update_envelope(Envelope const& envelope) const noexcept;

    private:
        class LinearRampState
        {
            public:
                enum Type {
                    RAMP_LINEAR = 0,
                    RAMP_LOGARITHMIC = 1,
                    RAMP_CURVED = 2,
                };

                LinearRampState() noexcept;

                void init(
                    Seconds const start_time_offset,
                    Number const done_samples,
                    Number const initial_value,
                    Number const target_value,
                    Number const duration_in_samples,
                    Seconds const duration,
                    Type const type,
                    Math::EnvelopeShape const curve_shape = Math::EnvelopeShape::ENV_SHAPE_SHARP_SMOOTH,
                    Number const curved_initial_value = 0.0,
                    Number const curved_delta = 0.0
                ) noexcept;

                Number advance() noexcept;
                Number get_value_at(Seconds const time_offset) const noexcept;
                Number get_remaining_samples() const noexcept;

                Seconds start_time_offset;
                Number done_samples;
                Number initial_value;
                Number target_value;
                Number duration_in_samples;
                Seconds duration;
                Number delta;
                Number speed;
                Number curved_initial_value;
                Number curved_delta;
                Math::EnvelopeShape curve_shape;
                Type type;
                bool is_done;
        };

        class EnvelopeState
        {
            public:
                explicit EnvelopeState(Envelope* const* const envelopes);

                void clear() noexcept;

                EnvelopeSnapshot& get_active_snapshot() noexcept;
                EnvelopeSnapshot const& get_active_snapshot() const noexcept;

                void activate_snapshot(Integer const snapshot_id) noexcept;

                void activate_lfo_snapshot(
                    Byte const lfo_state_index,
                    Integer const snapshot_id
                ) noexcept;

                Envelope* const* const envelopes;

                std::vector<EnvelopeSnapshot> snapshots;
                Queue<std::vector<EnvelopeSnapshot>::size_type> unused_snapshots;
                EnvelopeRandoms randoms;
                LFOEnvelopeStates lfo_states;
                Number random_seed;
                Seconds time;
                Seconds cancel_duration;
                Integer lfo_envelope_sample_count;
                Integer active_snapshot_id;
                Integer scheduled_snapshot_id;
                EnvelopeStage stage;
                Byte active_snapshot_envelope_index;
                Byte scheduled_snapshot_envelope_index;
                bool is_canceled;
                bool is_constant;
                bool lfo_has_envelope;
        };

        static constexpr Integer INVALID_ENVELOPE_SNAPSHOT_ID = -1;

        void initialize_instance() noexcept;

        Number round_value(Number const value) const noexcept;
        Number ratio_to_value_log(Number const ratio) const noexcept;
        Number ratio_to_value_raw(Number const ratio) const noexcept;
        Number value_to_ratio_raw(Number const value) const noexcept;

        void handle_cancel_event(SignalProducer::Event const& event) noexcept;
        void handle_set_value_event(SignalProducer::Event const& event) noexcept;
        void handle_linear_ramp_event(SignalProducer::Event const& event) noexcept;
        void handle_log_ramp_event(SignalProducer::Event const& event) noexcept;
        void handle_curved_ramp_event(SignalProducer::Event const& event) noexcept;
        void handle_envelope_start_event(SignalProducer::Event const& event) noexcept;
        void handle_envelope_update_event(SignalProducer::Event const& event) noexcept;

        Number prepare_linear_ramp(
            SignalProducer::Event const& event,
            Seconds& duration,
            Number& target_value
        ) const noexcept;

        void update_envelope_state_if_required(
            Envelope& envelope,
            EnvelopeSnapshot& envelope_snapshot,
            Byte const envelope_index
        ) noexcept;

        void update_envelope_state_if_required(
            Envelope& envelope,
            EnvelopeSnapshot& envelope_snapshot,
            Seconds& time,
            Byte const envelope_index,
            EnvelopeStage const stage
        ) noexcept;

        template<SignalProducer::Event::Type event_type>
        void handle_envelope_end_event(SignalProducer::Event const& event) noexcept;

        void handle_lfo_envelope_reset_event(SignalProducer::Event const& event) noexcept;
        void handle_lfo_envelope_start_event(SignalProducer::Event const& event) noexcept;
        void handle_lfo_envelope_update_event(SignalProducer::Event const& event) noexcept;

        template<SignalProducer::Event::Type event_type>
        void handle_lfo_envelope_end_event(SignalProducer::Event const& event) noexcept;

        void store_envelope_value_at_event(Seconds const latency) noexcept;

        bool is_following_leader() const noexcept;

        Sample const* const* process_lfo(
            LFO& lfo,
            Integer const round,
            Integer const sample_count
        ) noexcept;

        template<bool is_logarithmic_>
        void process_midi_controller_events() noexcept;

        void process_macro(Integer const sample_count) noexcept;

        void process_envelope(Envelope& envelope) noexcept;

        Seconds smooth_change_duration(
            Number const previous_value,
            Number const controller_value,
            Seconds const duration
        ) const noexcept;

        void clear_envelope_state() noexcept;

        void update_envelope_randoms(
            Number const random_1,
            Number const random_2
        ) noexcept;

        Integer make_envelope_snapshot(
            Envelope const& envelope,
            Byte const envelope_index
        ) noexcept;

        void render_with_lfo(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        void render_with_lfo_envelope(
            LFO& lfo,
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample* buffer
        ) noexcept;

        void render_linear_ramp(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        void render_with_envelope(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        void start_envelope(
            Envelope& envelope,
            Seconds const time_offset,
            Number const random_1,
            Number const random_2
        ) noexcept;

        void start_lfo_envelope(
            LFO& lfo,
            Seconds const time_offset,
            Number const random_1,
            Number const random_2
        ) noexcept;

        template<SignalProducer::Event::Type event_type>
        Seconds end_envelope(
            Envelope& envelope,
            Seconds const time_offset,
            Seconds const duration = 0.0
        ) noexcept;

        template<SignalProducer::Event::Type event_type>
        Seconds end_lfo_envelope(
            Seconds const time_offset,
            Seconds const duration = 0.0
        ) noexcept;

        void update_envelope(Envelope& envelope, Seconds const time_offset) noexcept;
        void update_lfo_envelope(Seconds const time_offset) noexcept;

        void update_envelope_release_if_not_static(
            Envelope& envelope,
            Byte const envelope_index,
            EnvelopeSnapshot& snapshot
        ) noexcept;

        void check_leaked_envelope_snapshots(char const* event) const noexcept;

        FloatParam<evaluation>* const leader;
        Envelope* const* const envelopes;
        EnvelopeState* const envelope_state;
        Number const round_to;
        Number const round_to_inv;
        ToggleParam const* const log_scale_toggle;
        Number const* const log_scale_table;
        Number const log_scale_table_index_scale;
        Number const log_scale_value_offset;
        Number const log_min_minus;
        Number const log_range_inv;
        int const log_scale_table_max_index;
        Byte const& voice_status;
        bool const should_round;
        bool const is_ratio_same_as_value;

        LinearRampState linear_ramp_state;
        LFO* lfo;
        Sample const* const* lfo_buffer;
        Envelope* envelope;
        Integer constantness_round;
        SignalProducer::Event::Type latest_event_type;
        bool constantness;
};


/**
 * \brief A parameter that can be modulated by the output of other
 *        \c SignalProducer objects.
 */
template<class ModulatorSignalProducerClass>
class ModulatableFloatParam : public FloatParamS
{
    friend class SignalProducer;

    public:
        static constexpr Number MODULATION_LEVEL_INSIGNIFICANT = 0.000001;

        ModulatableFloatParam(
            ModulatorSignalProducerClass& modulator,
            FloatParamS& modulation_level_leader,
            std::string const& name = "",
            Number const min_value = -1.0,
            Number const max_value = 1.0,
            Number const default_value = 0.0,
            Envelope* const* const envelopes = NULL
        ) noexcept;

        ModulatableFloatParam(
            ModulatorSignalProducerClass& modulator,
            FloatParamS& modulation_level_leader,
            Byte const& voice_status,
            std::string const& name = "",
            Number const min_value = -1.0,
            Number const max_value = 1.0,
            Number const default_value = 0.0,
            Envelope* const* const envelopes = NULL
        ) noexcept;

        bool is_constant_in_next_round(
            Integer const round, Integer const sample_count
        ) noexcept;

        void skip_round(Integer const round, Integer const sample_count) noexcept;

        void set_random_seed(Number const seed) noexcept;

        void start_envelope(
            Seconds const time_offset,
            Number const random_1,
            Number const random_2
        ) noexcept;

        Seconds end_envelope(Seconds const time_offset) noexcept;
        void cancel_envelope(Seconds const time_offset, Seconds const duration) noexcept;
        void update_envelope(Seconds const time_offset) noexcept;

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
        FloatParamS modulation_level;
        ModulatorSignalProducerClass& modulator;
        Sample const* modulator_buffer;
        Sample const* modulation_level_buffer;
        bool modulation_level_is_constant;
        bool is_no_op;
};

}

#endif
