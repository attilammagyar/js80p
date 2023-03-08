// TODO: set_value & co. should be no-op when controllers, envelopes, etc. are assigned

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

#ifndef JS80P__SYNTH__PARAM_HPP
#define JS80P__SYNTH__PARAM_HPP

#include <string>

#include "js80p.hpp"

#include "synth/midi_controller.hpp"
#include "synth/signal_producer.hpp"


namespace JS80P
{

class Envelope;


class FlexibleController;


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
        );

        std::string const get_name() const;
        NumberType get_default_value() const;
        NumberType get_value() const;
        NumberType get_min_value() const;
        NumberType get_max_value() const;
        void set_value(NumberType const new_value);

        Number get_ratio() const;
        Number get_default_ratio() const;
        void set_ratio(Number const ratio);

        NumberType ratio_to_value(Number const ratio) const;
        Number value_to_ratio(NumberType const value) const;

        /**
         * \brief Whenever the value of the param changes, the change index gets
         *        incremented. You may cache the result of a slow calculation
         *        that depends on a parameter value, as long as the change index
         *        remains unchanged.
         */
        Integer get_change_index() const;

    protected:
        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        );

        NumberType clamp(NumberType const value) const;
        void store_new_value(NumberType const new_value);

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

        /**
         * \brief Orchestrate rendering signals and handling events.
         *        See \c SignalProducer::process()
         */
        template<class FloatParamClass>
        static Sample const* const* produce(
            FloatParamClass* float_param,
            Integer const round,
            Integer const sample_count = -1
        );

        /**
         * \brief Render the single channel of the parameter if it has scheduled
         *        changes during this round, but if the parameter is constant
         *        during the round, then skip it and return \c NULL. A shortcut
         *        for the following construct:
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
            FloatParamClass* float_param,
            Integer const round,
            Integer const sample_count
        );

        FloatParam(
            std::string const name = "",
            Number const min_value = -1.0,
            Number const max_value = 1.0,
            Number const default_value = 0.0,
            Number const round_to = 0.0
        );

        FloatParam(FloatParam& leader);

        void set_value(Number const new_value);
        Number get_value() const;
        void set_ratio(Number const ratio);
        Number get_ratio() const;
        Integer get_change_index() const;
        bool is_constant_in_next_round(
            Integer const round, Integer const sample_count
        );
        bool is_constant_until(Integer const sample_count) const;
        void skip_round(Integer const round, Integer const sample_count);
        void schedule_value(Seconds const time_offset, Number const new_value);
        void schedule_linear_ramp(Seconds const duration, Number const target_value);

        void set_midi_controller(MidiController const* const midi_controller);
        MidiController const* get_midi_controller() const;

        void set_flexible_controller(FlexibleController* flexible_controller);
        FlexibleController const* get_flexible_controller();

        void set_envelope(Envelope const* const envelope);
        Envelope const* get_envelope() const;
        void start_envelope(Seconds const time_offset);
        Seconds end_envelope(Seconds const time_offset);

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
        class LinearRampState
        {
            public:
                LinearRampState();

                void init(
                    Seconds const start_time_offset,
                    Number const done_samples,
                    Number const initial_value,
                    Number const target_value,
                    Number const duration_in_samples,
                    Seconds const duration
                );

                Number get_next_value();
                Number get_value_at(Seconds const time_offset) const;

                Seconds start_time_offset;
                Number done_samples;
                Number initial_value;
                Number target_value;
                Number duration_in_samples;
                Seconds duration;
                Number delta;
                Number speed;
                bool is_done;
        };

        Number round_value(Number const value) const;

        void handle_set_value_event(Event const& event);
        void handle_linear_ramp_event(Event const& event);
        void handle_cancel_event(Event const& event);

        bool is_following_leader() const;

        FloatParam* const leader;
        MidiController const* midi_controller;
        FlexibleController* flexible_controller;
        Envelope const* envelope;

        bool const should_round;
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
        );

        bool is_constant_in_next_round(
            Integer const round, Integer const sample_count
        );

        void skip_round(Integer const round, Integer const sample_count);

        void start_envelope(Seconds const time_offset);
        Seconds end_envelope(Seconds const time_offset);

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
