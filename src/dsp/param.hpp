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

#ifndef JS80P__DSP__PARAM_HPP
#define JS80P__DSP__PARAM_HPP

#include <string>

#include "js80p.hpp"

#include "dsp/midi_controller.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

class Envelope;


class FlexibleController;


class LFO;


/**
 * \brief A variable that can influence the synthesized sound or other
 *        parameters.
 */
template<typename NumberType>
class Param : public SignalProducer
{
    friend class SignalProducer;

    public:
        Param(
            std::string const name,
            NumberType const min_value,
            NumberType const max_value,
            NumberType const default_value
        ) noexcept;

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

        void set_midi_controller(MidiController const* midi_controller) noexcept;
        MidiController const* get_midi_controller() const noexcept;

        /**
         * \brief Whenever the value of the parameter changes, the change index
         *        gets incremented. You may cache the result of a slow
         *        calculation that depends on a parameter value, as long as the
         *        change index remains unchanged.
         */
        Integer get_change_index() const noexcept;

    protected:
        NumberType get_raw_value() const noexcept;

        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        NumberType clamp(NumberType const value) const noexcept;
        void store_new_value(NumberType const new_value) noexcept;

        MidiController const* midi_controller;
        std::string const name;
        NumberType const min_value;
        NumberType const max_value;
        NumberType const range;
        NumberType const default_value;

    private:
        Number const range_inv;
        Integer change_index;
        NumberType value;
};


typedef Byte Toggle;


class ToggleParam : public Param<Toggle>
{
    friend class SignalProducer;

    public:
        static constexpr Toggle OFF = 0;
        static constexpr Toggle ON = 1;

        ToggleParam(std::string const name, Toggle const default_value);
};


/**
 * \brief Parameter with floating point values. Values can be scheduled at
 *        time offsets, or can be approached linearly over a given duration of
 *        time.
 */
class FloatParam : public Param<Number>
{
    friend class SignalProducer;

    public:
        static constexpr Event::Type EVT_SET_VALUE = 1;
        static constexpr Event::Type EVT_LINEAR_RAMP = 2;
        static constexpr Event::Type EVT_LOG_RAMP = 3;
        static constexpr Event::Type EVT_ENVELOPE_START = 4;
        static constexpr Event::Type EVT_ENVELOPE_END = 5;

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
         *        during the round, then skip this round for it and return
         *        \c NULL. A shortcut for the following construct:
         *
         *        \code{.cpp}
         *        if (param.is_constant_in_next_round(round, sample_count) {
         *            param.skip_round(round, sample_count);
         *        } else {
         *            param_buffer = FloatParam::produce<FloatParam>(
         *                param, round, sample_count
         *            )[0];
         *        }
         *        \endcode
         */
        template<class FloatParamClass = FloatParam>
        static Sample const* produce_if_not_constant(
            FloatParamClass& float_param,
            Integer const round,
            Integer const sample_count
        ) noexcept;

        FloatParam(
            std::string const name = "",
            Number const min_value = -1.0,
            Number const max_value = 1.0,
            Number const default_value = 0.0,
            Number const round_to = 0.0,
            ToggleParam const* log_scale_toggle = NULL,
            Number const* log_scale_table = NULL,
            Number const* log_scale_inv_table = NULL,
            int const log_scale_table_max_index = 0,
            Number const log_scale_table_scale = 0.0,
            Number const log_scale_inv_table_scale = 0.0
        ) noexcept;

        /**
         * \warning When the leader needs to be rendered, it will be rendered as
         *          a \c FloatParam, even if it's a descendant. Practically,
         *          this means that only \c FloatParam objects can be leaders.
         */
        FloatParam(FloatParam& leader) noexcept;

        bool is_logarithmic() const noexcept;

        void set_value(Number const new_value) noexcept;
        Number get_value() const noexcept;
        void set_ratio(Number const ratio) noexcept;
        Number get_ratio() const noexcept;
        Number get_default_ratio() const noexcept;

        Number ratio_to_value(Number const ratio) const noexcept;
        Number value_to_ratio(Number const value) const noexcept;

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

        void set_midi_controller(MidiController const* midi_controller) noexcept;

        void set_flexible_controller(
            FlexibleController* flexible_controller
        ) noexcept;

        FlexibleController const* get_flexible_controller() const noexcept;

        void set_envelope(Envelope* const envelope) noexcept;
        Envelope* get_envelope() const noexcept;
        void start_envelope(Seconds const time_offset) noexcept;
        Seconds end_envelope(Seconds const time_offset) noexcept;

        void set_lfo(LFO* lfo) noexcept;
        LFO const* get_lfo() const noexcept;

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
        enum EnvelopeStage {
            NONE = 0,
            DAHDS = 1,
            R = 2,
        };

        class LinearRampState
        {
            public:
                LinearRampState() noexcept;

                void init(
                    Seconds const start_time_offset,
                    Number const done_samples,
                    Number const initial_value,
                    Number const target_value,
                    Number const duration_in_samples,
                    Seconds const duration,
                    bool const is_logarithmic
                ) noexcept;

                Number get_next_value() noexcept;
                Number get_value_at(Seconds const time_offset) const noexcept;

                Seconds start_time_offset;
                Number done_samples;
                Number initial_value;
                Number target_value;
                Number duration_in_samples;
                Seconds duration;
                Number delta;
                Number speed;
                bool is_logarithmic;
                bool is_done;
        };

        Number round_value(Number const value) const noexcept;

        void handle_set_value_event(Event const& event) noexcept;
        void handle_linear_ramp_event(Event const& event) noexcept;
        void handle_log_ramp_event(Event const& event) noexcept;
        void handle_envelope_start_event(Event const& event) noexcept;
        void handle_envelope_end_event(Event const& event) noexcept;
        void handle_cancel_event(Event const& event) noexcept;

        bool is_following_leader() const noexcept;

        Sample const* const* process_lfo(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        Sample const* const* process_midi_controller_events() noexcept;

        Sample const* const* process_flexible_controller(
            Integer const sample_count
        ) noexcept;

        Seconds smooth_change_duration(
            Number const previous_value,
            Number const controller_value,
            Seconds const duration
        ) const noexcept;

        Sample const* const* process_envelope(
            Envelope* const envelope
        ) noexcept;

        Seconds schedule_envelope_value_if_not_reached(
            Seconds const next_event_time_offset,
            FloatParam const& time_param,
            FloatParam const& value_param,
            Number const amount
        ) noexcept;

        void update_envelope();

        ToggleParam const* const log_scale_toggle;
        Number const* const log_scale_table;
        Number const* const log_scale_inv_table;
        int const log_scale_table_max_index;
        Number const log_scale_table_scale;
        Number const log_scale_inv_table_scale;
        Number const log_min_minus;
        Number const log_range_inv;

        FloatParam* const leader;

        FlexibleController* flexible_controller;
        Integer flexible_controller_change_index;

        LFO* lfo;
        Sample const* const* lfo_buffer;

        Envelope* envelope;
        Integer envelope_change_index;
        Seconds envelope_end_time_offset;
        Seconds envelope_position;
        Seconds envelope_release_time;
        Number envelope_final_value;
        EnvelopeStage envelope_stage;
        bool envelope_end_scheduled;

        bool const should_round;
        bool const is_ratio_same_as_value;
        Number const round_to;
        Number const round_to_inv;

        LinearRampState linear_ramp_state;
        Integer constantness_round;
        bool constantness;
        Event::Type latest_event_type;
};


/**
 * \brief A parameter that can be modulated by the output of other
 *        \c SignalProducer objects.
 */
template<class ModulatorSignalProducerClass>
class ModulatableFloatParam : public FloatParam
{
    friend class SignalProducer;

    public:
        static constexpr Number MODULATION_LEVEL_INSIGNIFICANT = 0.000001;

        ModulatableFloatParam(
            ModulatorSignalProducerClass* modulator,
            FloatParam& modulation_level_leader,
            std::string const name = "",
            Number const min_value = -1.0,
            Number const max_value = 1.0,
            Number const default_value = 0.0
        ) noexcept;

        ModulatableFloatParam(FloatParam& leader) noexcept;

        bool is_constant_in_next_round(
            Integer const round, Integer const sample_count
        ) noexcept;

        void skip_round(Integer const round, Integer const sample_count) noexcept;

        void start_envelope(Seconds const time_offset) noexcept;
        Seconds end_envelope(Seconds const time_offset) noexcept;

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
        FloatParam modulation_level;
        ModulatorSignalProducerClass* modulator;
        Sample const* modulator_buffer;
        Sample const* modulation_level_buffer;
        bool modulation_level_is_constant;
        bool is_no_op;
};

}

#endif
