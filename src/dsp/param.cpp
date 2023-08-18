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

#ifndef JS80P__DSP__PARAM_CPP
#define JS80P__DSP__PARAM_CPP

#include <algorithm>

#include "dsp/param.hpp"


namespace JS80P
{

template<typename NumberType, ParamEvaluation evaluation>
Param<NumberType, evaluation>::Param(
        std::string const name,
        NumberType const min_value,
        NumberType const max_value,
        NumberType const default_value
) noexcept
    : SignalProducer(evaluation == ParamEvaluation::SAMPLE ? 1 : 0),
    midi_controller(NULL),
    name(name),
    min_value(min_value),
    max_value(max_value),
    range((NumberType)(max_value - min_value)),
    default_value(default_value),
    range_inv(1.0 / (Number)range),
    change_index(0),
    value(default_value)
{
}


template<typename NumberType, ParamEvaluation evaluation>
ParamEvaluation Param<NumberType, evaluation>::get_evaluation() const noexcept
{
    return evaluation;
}


template<typename NumberType, ParamEvaluation evaluation>
std::string const& Param<NumberType, evaluation>::get_name() const noexcept
{
    return name;
}


template<typename NumberType, ParamEvaluation evaluation>
NumberType Param<NumberType, evaluation>::get_default_value() const noexcept
{
    return default_value;
}


template<typename NumberType, ParamEvaluation evaluation>
NumberType Param<NumberType, evaluation>::get_value() const noexcept
{
    if (midi_controller != NULL) {
        return ratio_to_value(midi_controller->get_value());
    }

    return value;
}


template<typename NumberType, ParamEvaluation evaluation>
NumberType Param<NumberType, evaluation>::get_min_value() const noexcept
{
    return min_value;
}


template<typename NumberType, ParamEvaluation evaluation>
NumberType Param<NumberType, evaluation>::get_max_value() const noexcept
{
    return max_value;
}


template<typename NumberType, ParamEvaluation evaluation>
void Param<NumberType, evaluation>::set_value(NumberType const new_value) noexcept
{
    store_new_value(clamp(new_value));
}


template<typename NumberType, ParamEvaluation evaluation>
void Param<NumberType, evaluation>::store_new_value(NumberType const new_value) noexcept
{
    value = new_value;
    ++change_index;
    change_index &= 0x7fffffff;
}


template<typename NumberType, ParamEvaluation evaluation>
NumberType Param<NumberType, evaluation>::get_raw_value() const noexcept
{
    return value;
}


template<typename NumberType, ParamEvaluation evaluation>
NumberType Param<NumberType, evaluation>::clamp(NumberType const value) const noexcept
{
    return std::min(max_value, std::max(min_value, value));
}


template<typename NumberType, ParamEvaluation evaluation>
Number Param<NumberType, evaluation>::get_ratio() const noexcept
{
    if (midi_controller != NULL) {
        return midi_controller->get_value();
    }

    return std::min(1.0, std::max(0.0, value_to_ratio(value)));
}


template<typename NumberType, ParamEvaluation evaluation>
Number Param<NumberType, evaluation>::get_default_ratio() const noexcept
{
    return value_to_ratio(get_default_value());
}


template<typename NumberType, ParamEvaluation evaluation>
void Param<NumberType, evaluation>::set_ratio(Number const ratio) noexcept
{
    store_new_value(ratio_to_value(ratio));
}


template<typename NumberType, ParamEvaluation evaluation>
Integer Param<NumberType, evaluation>::get_change_index() const noexcept
{
    if (midi_controller != NULL) {
        return midi_controller->get_change_index();
    }

    return change_index;
}


template<typename NumberType, ParamEvaluation evaluation>
NumberType Param<NumberType, evaluation>::ratio_to_value(Number const ratio) const noexcept
{
    return ratio_to_value_for_number_type(ratio);
}


template<typename NumberType, ParamEvaluation evaluation>
template<typename DummyType>
NumberType Param<NumberType, evaluation>::ratio_to_value_for_number_type(
        typename std::enable_if<
            std::is_floating_point<DummyType>::value, Number const
        >::type ratio
) const noexcept {
    return clamp(min_value + (DummyType)((Number)range * ratio));
}


template<typename NumberType, ParamEvaluation evaluation>
template<typename DummyType>
NumberType Param<NumberType, evaluation>::ratio_to_value_for_number_type(
        typename std::enable_if<
            !std::is_floating_point<DummyType>::value, Number const
        >::type ratio
) const noexcept {
    return clamp(min_value + (DummyType)std::round((Number)range * ratio));
}


template<typename NumberType, ParamEvaluation evaluation>
Number Param<NumberType, evaluation>::value_to_ratio(NumberType const value) const noexcept
{
    return ((Number)value - (Number)min_value) * range_inv;
}


template<typename NumberType, ParamEvaluation evaluation>
void Param<NumberType, evaluation>::set_midi_controller(
        MidiController const* midi_controller
) noexcept {
    if (midi_controller == NULL) {
        if (this->midi_controller != NULL) {
            set_value(ratio_to_value(this->midi_controller->get_value()));
        }
    } else {
        set_value(ratio_to_value(midi_controller->get_value()));
    }

    this->midi_controller = midi_controller;
}


template<typename NumberType, ParamEvaluation evaluation>
MidiController const* Param<NumberType, evaluation>::get_midi_controller() const noexcept
{
    return midi_controller;
}


template<typename NumberType, ParamEvaluation evaluation>
void Param<NumberType, evaluation>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Sample const value = (Sample)this->value;

    for (Integer c = 0; c != channels; ++c) {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[c][i] = value;
        }
    }
}


ToggleParam::ToggleParam(std::string const name, Toggle const default_value)
    : Param<Toggle, ParamEvaluation::BLOCK>(name, OFF, ON, default_value)
{
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
template<class FloatParamClass>
Sample const* const* FloatParam<evaluation, leader_evaluation>::produce(
        FloatParamClass& float_param,
        Integer const round,
        Integer const sample_count
) noexcept {
    Envelope* envelope = float_param.get_envelope();

    if (envelope != NULL && envelope->dynamic.get_value() == ToggleParam::ON) {
        envelope->update();
    }

    if (float_param.is_following_leader()) {
        return SignalProducer::produce< FloatParam<leader_evaluation> >(
            *float_param.leader, round, sample_count
        );
    }

    return SignalProducer::produce<FloatParamClass>(
        float_param, round, sample_count
    );
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
template<class FloatParamClass>
Sample const* FloatParam<evaluation, leader_evaluation>::produce_if_not_constant(
        FloatParamClass& float_param,
        Integer const round,
        Integer const sample_count
) noexcept {
    if (float_param.is_constant_in_next_round(round, sample_count)) {
        float_param.skip_round(round, sample_count);

        return NULL;
    }

    Sample const* const* const rendered = (
        FloatParam<evaluation, leader_evaluation>::produce<FloatParamClass>(
            float_param, round, sample_count
        )
    );

    return (
        float_param.get_evaluation() == ParamEvaluation::SAMPLE
            ? rendered[0]
            : NULL
    );
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
FloatParam<evaluation, leader_evaluation>::FloatParam(
        std::string const name,
        Number const min_value,
        Number const max_value,
        Number const default_value,
        Number const round_to,
        ToggleParam const* log_scale_toggle,
        Number const* log_scale_table,
        int const log_scale_table_max_index,
        Number const log_scale_table_scale
) noexcept
    : Param<Number, evaluation>(name, min_value, max_value, default_value),
    log_scale_toggle(log_scale_toggle),
    log_scale_table(log_scale_table),
    log_scale_table_max_index(log_scale_table_max_index),
    log_scale_table_scale(log_scale_table_scale),
    log_min_minus(log_scale_toggle != NULL ? -std::log2(min_value) : 0.0),
    log_range_inv(
        log_scale_toggle != NULL
            ? 1.0 / (std::log2(max_value) + log_min_minus)
            : 1.0
    ),
    leader(NULL),
    flexible_controller(NULL),
    flexible_controller_change_index(-1),
    lfo(NULL),
    envelope(NULL),
    envelope_change_index(-1),
    envelope_stage(EnvelopeStage::NONE),
    envelope_end_scheduled(false),
    envelope_canceled(false),
    should_round(round_to > 0.0),
    is_ratio_same_as_value(
        std::fabs(min_value - 0.0) < 0.000001
        && std::fabs(max_value - 1.0) < 0.000001
    ),
    round_to(round_to),
    round_to_inv(should_round ? 1.0 / round_to : 0.0),
    constantness_round(-1),
    constantness(false),
    latest_event_type(EVT_SET_VALUE)
{
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
FloatParam<evaluation, leader_evaluation>::FloatParam(
        FloatParam<leader_evaluation>& leader
) noexcept
    : Param<Number, evaluation>(
        leader.get_name(),
        leader.get_min_value(),
        leader.get_max_value(),
        leader.get_default_value()
    ),
    log_scale_toggle(leader.get_log_scale_toggle()),
    log_scale_table(leader.get_log_scale_table()),
    log_scale_table_max_index(leader.get_log_scale_table_max_index()),
    log_scale_table_scale(leader.get_log_scale_table_scale()),
    log_min_minus(log_scale_toggle != NULL ? -std::log2(leader.get_min_value()) : 0.0),
    log_range_inv(
        log_scale_toggle != NULL
            ? 1.0 / (std::log2(leader.get_max_value()) + log_min_minus)
            : 1.0
    ),
    leader(&leader),
    flexible_controller(NULL),
    lfo(NULL),
    envelope(NULL),
    envelope_change_index(-1),
    envelope_stage(EnvelopeStage::NONE),
    envelope_end_scheduled(false),
    envelope_canceled(false),
    should_round(false),
    is_ratio_same_as_value(
        std::fabs(leader.get_min_value() - 0.0) < 0.000001
        && std::fabs(leader.get_max_value() - 1.0) < 0.000001
    ),
    round_to(0.0),
    round_to_inv(0.0),
    constantness_round(-1),
    constantness(false),
    latest_event_type(EVT_SET_VALUE)
{
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Number FloatParam<evaluation, leader_evaluation>::get_value() const noexcept
{
    if (is_following_leader()) {
        return leader->get_value();
    } else if (this->midi_controller != NULL) {
        return round_value(ratio_to_value(this->midi_controller->get_value()));
    } else if (flexible_controller != NULL) {
        flexible_controller->update();

        return round_value(ratio_to_value(flexible_controller->get_value()));
    } else {
        return this->get_raw_value();
    }
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
bool FloatParam<evaluation, leader_evaluation>::is_following_leader() const noexcept
{
    return leader != NULL && leader->get_envelope() == NULL;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
bool FloatParam<evaluation, leader_evaluation>::is_logarithmic() const noexcept
{
    return (
        log_scale_toggle != NULL
        && log_scale_toggle->get_value() == ToggleParam::ON
    );
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::set_value(Number const new_value) noexcept
{
    latest_event_type = EVT_SET_VALUE;

    Param<Number, evaluation>::set_value(round_value(new_value));
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Number FloatParam<evaluation, leader_evaluation>::round_value(Number const value) const noexcept
{
    if (should_round) {
        return std::round(value * round_to_inv) * round_to;
    }

    return value;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::set_ratio(Number const ratio) noexcept
{
    set_value(ratio_to_value(ratio));
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Number FloatParam<evaluation, leader_evaluation>::get_ratio() const noexcept
{
    if (is_following_leader()) {
        return leader->get_ratio();
    } else if (flexible_controller != NULL) {
        flexible_controller->update();

        return flexible_controller->get_value();
    } else if (this->midi_controller != NULL) {
        return this->midi_controller->get_value();
    }

    return std::min(1.0, std::max(0.0, value_to_ratio(this->get_raw_value())));
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Number FloatParam<evaluation, leader_evaluation>::get_default_ratio() const noexcept
{
    return value_to_ratio(this->get_default_value());
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
ToggleParam const* FloatParam<evaluation, leader_evaluation>::get_log_scale_toggle() const noexcept
{
    return log_scale_toggle;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Number const* FloatParam<evaluation, leader_evaluation>::get_log_scale_table() const noexcept
{
    return log_scale_table;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
int FloatParam<evaluation, leader_evaluation>::get_log_scale_table_max_index() const noexcept
{
    return log_scale_table_max_index;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Number FloatParam<evaluation, leader_evaluation>::get_log_scale_table_scale() const noexcept
{
    return log_scale_table_scale;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Number FloatParam<evaluation, leader_evaluation>::ratio_to_value(Number const ratio) const noexcept
{
    if (is_logarithmic()) {
        return Math::lookup(
            log_scale_table,
            log_scale_table_max_index,
            ratio * log_scale_table_scale
        );
    }

    return Param<Number, evaluation>::ratio_to_value(ratio);
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Number FloatParam<evaluation, leader_evaluation>::value_to_ratio(Number const value) const noexcept
{
    if (is_logarithmic()) {
        return (std::log2(value) + log_min_minus) * log_range_inv;
    }

    return Param<Number, evaluation>::value_to_ratio(value);
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Integer FloatParam<evaluation, leader_evaluation>::get_change_index() const noexcept
{
    if (is_following_leader()) {
        return leader->get_change_index();
    } else if (flexible_controller != NULL) {
        flexible_controller->update();

        return flexible_controller->get_change_index();
    } else {
        return Param<Number, evaluation>::get_change_index();
    }
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
bool FloatParam<evaluation, leader_evaluation>::is_constant_in_next_round(
        Integer const round, Integer const sample_count
) noexcept {
    if (round == constantness_round) {
        return constantness;
    }

    constantness_round = round;

    return constantness = is_constant_until(sample_count);
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
bool FloatParam<evaluation, leader_evaluation>::is_constant_until(Integer const sample_count) const noexcept
{
    if (is_following_leader()) {
        return leader->is_constant_until(sample_count);
    }

    if (lfo != NULL) {
        return false;
    }

    Integer const last_sample_idx = sample_count - 1;

    if (latest_event_type == EVT_LINEAR_RAMP || this->has_upcoming_events(last_sample_idx)) {
        return false;
    }

    Envelope* envelope = get_envelope();

    if (envelope != NULL && envelope->dynamic.get_value() == ToggleParam::ON) {
        envelope->update();

        return envelope_change_index == envelope->get_change_index();
    }

    if (this->midi_controller != NULL) {
        return (
            this->midi_controller->events.is_empty()
            || !(
                this->is_time_offset_before_sample_count(
                    this->midi_controller->events.front().time_offset, last_sample_idx
                )
            )
        );
    }

    if (flexible_controller != NULL) {
        flexible_controller->update();

        return flexible_controller->get_change_index() == flexible_controller_change_index;
    }

    return true;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::skip_round(
        Integer const round,
        Integer const sample_count
) noexcept {
    if (is_following_leader()) {
        leader->skip_round(round, sample_count);
    } else if (this->cached_round != round && !this->events.is_empty()) {
        this->current_time += (Seconds)sample_count * this->sampling_period;
        this->cached_round = round;

        if (envelope_stage != EnvelopeStage::NONE) {
            Seconds const offset = this->sample_count_to_relative_time_offset(sample_count);

            envelope_position += offset;

            if (envelope_end_scheduled) {
                envelope_end_time_offset -= offset;
            }
        }
    }
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::schedule_value(
        Seconds const time_offset,
        Number const new_value
) noexcept {
    this->schedule(EVT_SET_VALUE, time_offset, 0, 0.0, new_value);
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::schedule_linear_ramp(
        Seconds const duration,
        Number const target_value
) noexcept {
    Seconds const last_event_time_offset = this->get_last_event_time_offset();

    if (is_logarithmic()) {
        this->schedule(
            EVT_LOG_RAMP, last_event_time_offset, 0, duration, target_value
        );
    } else {
        this->schedule(
            EVT_LINEAR_RAMP, last_event_time_offset, 0, duration, target_value
        );
    }

    this->schedule(
        EVT_SET_VALUE, last_event_time_offset + duration, 0, 0.0, target_value
    );
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::handle_event(SignalProducer::Event const& event) noexcept
{
    Param<Number, evaluation>::handle_event(event);

    switch (event.type) {
        case SignalProducer::EVT_CANCEL:
            handle_cancel_event(event);
            break;

        case EVT_SET_VALUE:
            handle_set_value_event(event);
            break;

        case EVT_LINEAR_RAMP:
            handle_linear_ramp_event(event);
            break;

        case EVT_LOG_RAMP:
            handle_log_ramp_event(event);
            break;

        case EVT_ENVELOPE_START:
            handle_envelope_start_event(event);
            break;

        case EVT_ENVELOPE_END:
            handle_envelope_end_event();
            break;

        case EVT_ENVELOPE_CANCEL:
            handle_envelope_cancel_event();
            break;

        default:
            break;
    }
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::handle_set_value_event(SignalProducer::Event const& event) noexcept
{
    set_value(event.number_param_2);
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::handle_linear_ramp_event(SignalProducer::Event const& event) noexcept
{
    Number const value = this->get_raw_value();
    Number const done_samples = (
        (Number)(this->current_time - event.time_offset) * (Number)this->sample_rate
    );
    Seconds duration = (Seconds)event.number_param_1;
    Number target_value = event.number_param_2;

    if (target_value < this->min_value) {
        Number const min_diff = this->min_value - value;
        Number const target_diff = target_value - value;

        duration *= (Seconds)(min_diff / target_diff);
        target_value = this->min_value;
    } else if (target_value > this->max_value) {
        Number const max_diff = this->max_value - value;
        Number const target_diff = target_value - value;

        duration *= (Seconds)(max_diff / target_diff);
        target_value = this->max_value;
    }

    latest_event_type = EVT_LINEAR_RAMP;
    linear_ramp_state.init(
        event.time_offset,
        done_samples,
        value,
        target_value,
        (Number)duration * (Number)this->sample_rate,
        duration,
        false
    );
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::handle_log_ramp_event(SignalProducer::Event const& event) noexcept
{
    Number const value = value_to_ratio(this->get_raw_value());
    Number const done_samples = (
        (Number)(this->current_time - event.time_offset) * (Number)this->sample_rate
    );
    Seconds duration = (Seconds)event.number_param_1;
    Number target_value = value_to_ratio(event.number_param_2);

    if (target_value < 0.0) {
        Number const min_diff = 0.0 - value;
        Number const target_diff = target_value - value;

        duration *= (Seconds)(min_diff / target_diff);
        target_value = 0.0;
    } else if (target_value > 1.0) {
        Number const max_diff = 1.0 - value;
        Number const target_diff = target_value - value;

        duration *= (Seconds)(max_diff / target_diff);
        target_value = 1.0;
    }

    latest_event_type = EVT_LINEAR_RAMP;
    linear_ramp_state.init(
        event.time_offset,
        done_samples,
        value,
        target_value,
        (Number)duration * (Number)this->sample_rate,
        duration,
        true
    );
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::handle_envelope_start_event(SignalProducer::Event const& event) noexcept
{
    envelope_stage = EnvelopeStage::DAHDS;
    envelope_position = this->current_time - event.time_offset;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::handle_envelope_end_event() noexcept
{
    envelope_stage = EnvelopeStage::R;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::handle_envelope_cancel_event() noexcept
{
    envelope_stage = EnvelopeStage::R;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::handle_cancel_event(SignalProducer::Event const& event) noexcept
{
    if (latest_event_type == EVT_LINEAR_RAMP) {
        Number const stop_value = linear_ramp_state.get_value_at(
            event.time_offset - linear_ramp_state.start_time_offset
        );

        if (linear_ramp_state.is_logarithmic) {
            this->store_new_value(ratio_to_value(stop_value));
        } else {
            this->store_new_value(stop_value);
        }
    }

    latest_event_type = EVT_SET_VALUE;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::set_midi_controller(
        MidiController const* midi_controller
) noexcept {
    if (midi_controller == NULL) {
        if (this->midi_controller != NULL) {
            set_value(ratio_to_value(this->midi_controller->get_value()));
        }
    } else {
        set_value(ratio_to_value(midi_controller->get_value()));
    }

    this->midi_controller = midi_controller;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::set_flexible_controller(
        FlexibleController* flexible_controller
) noexcept {
    if (flexible_controller == NULL) {
        if (this->flexible_controller != NULL) {
            this->flexible_controller->update();

            set_value(ratio_to_value(this->flexible_controller->get_value()));
        }
    } else {
        flexible_controller->update();
        set_value(ratio_to_value(flexible_controller->get_value()));
        flexible_controller_change_index = flexible_controller->get_change_index();
    }

    this->flexible_controller = flexible_controller;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
FlexibleController const* FloatParam<evaluation, leader_evaluation>::get_flexible_controller() const noexcept
{
    return flexible_controller;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::set_envelope(Envelope* const envelope) noexcept
{
    this->envelope = envelope;

    if (envelope != NULL) {
        envelope->update();
        envelope_change_index = envelope->get_change_index();
    }

    envelope_stage = EnvelopeStage::NONE;
    envelope_end_scheduled = false;
    envelope_canceled = false;
    envelope_position = 0.0;
    envelope_end_time_offset = 0.0;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Envelope* FloatParam<evaluation, leader_evaluation>::get_envelope() const noexcept
{
    return leader == NULL ? envelope : leader->get_envelope();
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::start_envelope(Seconds const time_offset) noexcept
{
    Seconds next_event_time_offset;
    Number next_value;
    Envelope* const envelope = get_envelope();

    if (envelope == NULL) {
        return;
    }

    envelope_stage = EnvelopeStage::NONE;
    envelope_end_scheduled = false;
    envelope_canceled = false;
    envelope_position = 0.0;
    envelope_end_time_offset = 0.0;

    envelope->update();
    envelope_change_index = envelope->get_change_index();

    /*
    initial-v ==delay-t==> initial-v ==attack-t==> peak-v ==hold-t==> peak-v ==decay-t==> sustain-v
    */

    this->cancel_events_after(time_offset);

    this->schedule(EVT_ENVELOPE_START, time_offset);

    Number const amount = envelope->amount.get_value();
    next_value = ratio_to_value(amount * envelope->initial_value.get_value());

    schedule_value(time_offset, next_value);
    next_event_time_offset = (
        time_offset + (Seconds)envelope->delay_time.get_value()
    );
    schedule_value(next_event_time_offset, next_value);

    Seconds const attack = (Seconds)envelope->attack_time.get_value();
    next_value = ratio_to_value(amount * envelope->peak_value.get_value());
    schedule_linear_ramp(attack, next_value);

    next_event_time_offset += attack + (Seconds)envelope->hold_time.get_value();
    schedule_value(next_event_time_offset, next_value);

    schedule_linear_ramp(
        (Seconds)envelope->decay_time.get_value(),
        ratio_to_value(amount * envelope->sustain_value.get_value())
    );

    envelope_final_value = amount * envelope->final_value.get_value();
    envelope_release_time = (Seconds)envelope->release_time.get_value();
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Seconds FloatParam<evaluation, leader_evaluation>::end_envelope(Seconds const time_offset) noexcept
{
    if (envelope_canceled) {
        return envelope_cancel_duration;
    }

    return end_envelope<EVT_ENVELOPE_END>(time_offset);
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
template<SignalProducer::Event::Type event>
Seconds FloatParam<evaluation, leader_evaluation>::end_envelope(
        Seconds const time_offset,
        Seconds const duration
) noexcept {
    Envelope* const envelope = get_envelope();

    if (envelope == NULL) {
        return 0.0;
    }

    if (envelope->dynamic.get_value() == ToggleParam::ON) {
        envelope->update();
        envelope_change_index = envelope->get_change_index();

        envelope_final_value = (
            envelope->amount.get_value() * envelope->final_value.get_value()
        );

        envelope_release_time = (Seconds)envelope->release_time.get_value();
    }

    if (event == EVT_ENVELOPE_CANCEL) {
        envelope_release_time = duration;
    }

    envelope_end_scheduled = true;
    envelope_end_time_offset = time_offset;

    /* current-v ==release-t==> release-v */

    this->cancel_events_after(time_offset);
    this->schedule(event, time_offset);
    schedule_linear_ramp(envelope_release_time, ratio_to_value(envelope_final_value));

    return envelope_release_time;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::cancel_envelope(
        Seconds const time_offset,
        Seconds const duration
) noexcept {
    envelope_canceled = true;
    envelope_cancel_duration = duration;
    end_envelope<EVT_ENVELOPE_CANCEL>(time_offset, duration);
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::update_envelope(Seconds const time_offset) noexcept
{
    Envelope* envelope = get_envelope();

    if (envelope != NULL) {
        envelope->update();
        process_envelope(*envelope, time_offset);

        if (envelope_end_scheduled) {
            return;
        }

        envelope_final_value = envelope->amount.get_value() * envelope->final_value.get_value();
        envelope_release_time = (Seconds)envelope->release_time.get_value();
    }
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::set_lfo(LFO* lfo) noexcept
{
    this->lfo = lfo;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
LFO const* FloatParam<evaluation, leader_evaluation>::get_lfo() const noexcept
{
    return lfo;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Sample const* const* FloatParam<evaluation, leader_evaluation>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Param<Number, evaluation>::initialize_rendering(round, sample_count);

    if (lfo != NULL) {
        return process_lfo(round, sample_count);
    } else if (this->midi_controller != NULL) {
        return process_midi_controller_events();
    } else if (flexible_controller != NULL) {
        return process_flexible_controller(sample_count);
    } else {
        Envelope* envelope = get_envelope();

        if (envelope != NULL && envelope->dynamic.get_value() == ToggleParam::ON) {
            process_envelope(*envelope);
        }
    }

    return NULL;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Sample const* const* FloatParam<evaluation, leader_evaluation>::process_lfo(
        Integer const round,
        Integer const sample_count
) noexcept {
    lfo_buffer = SignalProducer::produce<LFO>(*lfo, round, sample_count);

    if (is_ratio_same_as_value) {
        if (sample_count > 0) {
            this->store_new_value(lfo_buffer[0][sample_count - 1]);
        }

        return lfo_buffer;
    }

    return NULL;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Sample const* const* FloatParam<evaluation, leader_evaluation>::process_midi_controller_events() noexcept
{
    Queue<SignalProducer::Event>::SizeType const number_of_ctl_events = (
        this->midi_controller->events.length()
    );

    if (number_of_ctl_events == 0) {
        return NULL;
    }

    this->cancel_events_at(0.0);

    if (should_round) {
        for (Queue<SignalProducer::SignalProducer::Event>::SizeType i = 0; i != number_of_ctl_events; ++i) {
            Seconds const time_offset = this->midi_controller->events[i].time_offset;
            Number const controller_value = this->midi_controller->events[i].number_param_1;

            schedule_value(time_offset, ratio_to_value(controller_value));
        }

        return NULL;
    }

    Queue<SignalProducer::SignalProducer::Event>::SizeType const last_ctl_event_index = number_of_ctl_events - 1;

    Seconds previous_time_offset = 0.0;
    Number previous_ratio = value_to_ratio(this->get_raw_value());

    for (Queue<SignalProducer::SignalProducer::Event>::SizeType i = 0; i != number_of_ctl_events; ++i) {
        Seconds time_offset = this->midi_controller->events[i].time_offset;

        while (i != last_ctl_event_index) {
            ++i;

            Seconds const delta = std::fabs(
                this->midi_controller->events[i].time_offset - time_offset
            );

            if (delta >= MIDI_CTL_SMALL_CHANGE_DURATION) {
                --i;
                break;
            }
        }

        time_offset = this->midi_controller->events[i].time_offset;

        Number const controller_value = this->midi_controller->events[i].number_param_1;
        Seconds const duration = smooth_change_duration(
            previous_ratio,
            controller_value,
            time_offset - previous_time_offset
        );
        previous_ratio = controller_value;
        schedule_linear_ramp(duration, ratio_to_value(controller_value));
        previous_time_offset = time_offset;
    }

    return NULL;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Sample const* const* FloatParam<evaluation, leader_evaluation>::process_flexible_controller(
        Integer const sample_count
) noexcept {
    flexible_controller->update();

    Integer const new_change_index = flexible_controller->get_change_index();

    if (new_change_index == flexible_controller_change_index) {
        return NULL;
    }

    flexible_controller_change_index = new_change_index;

    this->cancel_events_at(0.0);

    Number const controller_value = flexible_controller->get_value();

    if (should_round) {
        set_value(ratio_to_value(controller_value));
    } else {
        Seconds const duration = smooth_change_duration(
            value_to_ratio(this->get_raw_value()),
            controller_value,
            (Seconds)std::max((Integer)0, sample_count - 1) * this->sampling_period
        );
        schedule_linear_ramp(duration, ratio_to_value(controller_value));
    }

    return NULL;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Seconds FloatParam<evaluation, leader_evaluation>::smooth_change_duration(
        Number const previous_value,
        Number const controller_value,
        Seconds const duration
) const noexcept {
    Number const change = std::fabs(previous_value - controller_value);

    if (change < 0.000001) {
        return std::max(duration, MIDI_CTL_BIG_CHANGE_DURATION * change);
    }

    Seconds const min_duration = (
        std::max(
            MIDI_CTL_SMALL_CHANGE_DURATION,
            MIDI_CTL_BIG_CHANGE_DURATION * change
        )
    );

    return std::max(min_duration, duration);
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::process_envelope(
        Envelope& envelope,
        Seconds const time_offset
) noexcept {
    if (envelope_stage == EnvelopeStage::NONE) {
        return;
    }

    Integer const new_change_index = envelope.get_change_index();
    bool const has_changed = new_change_index != envelope_change_index;

    envelope_change_index = new_change_index;

    Number const amount = envelope.amount.get_value();

    if (envelope_stage == EnvelopeStage::DAHDS) {
        this->cancel_events_at(time_offset);

        if (envelope_position > envelope.get_dahd_length()) {
            Number const sustain_value = ratio_to_value(
                amount * envelope.sustain_value.get_value()
            );

            if (std::fabs(this->get_raw_value() - sustain_value) > 0.000001) {
                schedule_linear_ramp(0.1, sustain_value);
            }
        } else {
            Seconds next_event_time_offset = -envelope_position;

            next_event_time_offset = schedule_envelope_value_if_not_reached(
                next_event_time_offset,
                envelope.delay_time,
                envelope.initial_value,
                amount
            );
            next_event_time_offset = schedule_envelope_value_if_not_reached(
                next_event_time_offset,
                envelope.attack_time,
                envelope.peak_value,
                amount
            );
            next_event_time_offset = schedule_envelope_value_if_not_reached(
                next_event_time_offset,
                envelope.hold_time,
                envelope.peak_value,
                amount
            );
            next_event_time_offset = schedule_envelope_value_if_not_reached(
                next_event_time_offset,
                envelope.decay_time,
                envelope.sustain_value,
                amount
            );
        }
    }

    if (
            envelope_end_scheduled
            && !envelope_canceled
            && (has_changed || envelope_stage == EnvelopeStage::DAHDS)
    ) {
        if (envelope_end_time_offset < 0.0) {
            envelope_end_time_offset = 0.0;
        }

        envelope_release_time = std::min(
            envelope_release_time, (Seconds)envelope.release_time.get_value()
        );

        this->cancel_events_at(envelope_end_time_offset);
        this->schedule(EVT_ENVELOPE_END, envelope_end_time_offset);
        schedule_linear_ramp(
            envelope_release_time,
            ratio_to_value(amount * envelope.final_value.get_value())
        );
    }
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Seconds FloatParam<evaluation, leader_evaluation>::schedule_envelope_value_if_not_reached(
        Seconds const next_event_time_offset,
        FloatParamB const& time_param,
        FloatParamB const& value_param,
        Number const amount
) noexcept {
    Seconds const duration = next_event_time_offset + time_param.get_value();

    if (duration >= 0.0) {
        schedule_linear_ramp(
            duration, ratio_to_value(amount * value_param.get_value())
        );

        return 0.0;
    }

    return duration;
}


// TODO: enable_if
template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    if (lfo != NULL) {
        if (this->get_evaluation() == ParamEvaluation::BLOCK) {
            // if (last_sample_index != first_sample_index) {
                // this->store_new_value(
                    // (Number)ratio_to_value(lfo_buffer[0][last_sample_index - 1])
                // );
            // }
        } else {
            Sample sample;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = sample = (Sample)ratio_to_value(lfo_buffer[0][i]);
            }

            if (last_sample_index != first_sample_index) {
                this->store_new_value((Number)sample);
            }
        }
    } else if (latest_event_type == EVT_LINEAR_RAMP) {
        Sample sample;

        if (this->get_evaluation() == ParamEvaluation::BLOCK) {
            // if (first_sample_index != last_sample_index) {
                // Integer const skip_samples = last_sample_index - first_sample_index - 1;

                // if (linear_ramp_state.is_logarithmic) {
                    // sample = (Sample)ratio_to_value(
                        // linear_ramp_state.advance(skip_samples)
                    // );
                // } else {
                    // sample = (Sample)linear_ramp_state.advance(skip_samples);
                // }

                // this->store_new_value((Number)sample);
            // }
        } else {
            if (linear_ramp_state.is_logarithmic) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[0][i] = sample = (
                        (Sample)ratio_to_value(linear_ramp_state.advance())
                    );
                }
            } else {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[0][i] = sample = (Sample)linear_ramp_state.advance();
                }
            }

            if (last_sample_index != first_sample_index) {
                this->store_new_value((Number)sample);
            }
        }
    } else {
        Param<Number, evaluation>::render(
            round, first_sample_index, last_sample_index, buffer
        );
    }

    if (envelope_stage != EnvelopeStage::NONE) {
        Seconds const time_delta = this->sample_count_to_relative_time_offset(
            last_sample_index - first_sample_index
        );

        envelope_position += time_delta;

        if (envelope_end_scheduled) {
            envelope_end_time_offset -= time_delta;
        }
    }
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
FloatParam<evaluation, leader_evaluation>::LinearRampState::LinearRampState() noexcept
    : start_time_offset(0.0),
    done_samples(0.0),
    initial_value(0.0),
    target_value(0.0),
    duration_in_samples(0.0),
    duration(0.0),
    delta(0.0),
    speed(0.0),
    is_logarithmic(false),
    is_done(false)
{
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
void FloatParam<evaluation, leader_evaluation>::LinearRampState::init(
        Seconds const start_time_offset,
        Number const done_samples,
        Number const initial_value,
        Number const target_value,
        Number const duration_in_samples,
        Seconds const duration,
        bool const is_logarithmic
) noexcept {
    LinearRampState::is_logarithmic = is_logarithmic;

    if (duration_in_samples > 0.0) {
        is_done = false;

        LinearRampState::start_time_offset = start_time_offset;
        LinearRampState::done_samples = done_samples;
        LinearRampState::initial_value = initial_value;
        LinearRampState::target_value = target_value;
        LinearRampState::duration_in_samples = duration_in_samples;
        LinearRampState::duration = duration;

        delta = target_value - initial_value;
        speed = 1.0 / duration_in_samples;
    } else {
        is_done = true;
        LinearRampState::target_value = target_value;
    }
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Number FloatParam<evaluation, leader_evaluation>::LinearRampState::advance() noexcept
{
    if (is_done) {
        return target_value;
    }

    Number const next_value = initial_value + (done_samples * speed) * delta;

    done_samples += 1.0;

    if (done_samples >= duration_in_samples) {
        is_done = true;
    }

    return next_value;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Number FloatParam<evaluation, leader_evaluation>::LinearRampState::advance(
        Integer const skip_samples
) noexcept {
    if (is_done) {
        return target_value;
    }

    Number const skip_samples_float = std::min(
        (Number)skip_samples, duration_in_samples - done_samples - 1.0
    );

    if (LIKELY(skip_samples_float > 0.0)) {
        done_samples += skip_samples;
    }

    Number const next_value = initial_value + (done_samples * speed) * delta;

    done_samples += 1.0;

    if (done_samples >= duration_in_samples) {
        is_done = true;
    }

    return next_value;
}


template<ParamEvaluation evaluation, ParamEvaluation leader_evaluation>
Number FloatParam<evaluation, leader_evaluation>::LinearRampState::get_value_at(
        Seconds const time_offset
) const noexcept {
    if (duration > 0.0 && time_offset <= duration) {
        return initial_value + (time_offset / duration) * delta;
    } else {
        return target_value;
    }
}


template<class ModulatorSignalProducerClass>
ModulatableFloatParam<ModulatorSignalProducerClass>::ModulatableFloatParam(
        ModulatorSignalProducerClass* modulator,
        FloatParamS& modulation_level_leader,
        std::string const name,
        Number const min_value,
        Number const max_value,
        Number const default_value
) noexcept
    : FloatParamS(name, min_value, max_value, default_value),
    modulation_level(modulation_level_leader),
    modulator(modulator)
{
    register_child(modulation_level);
}


template<class ModulatorSignalProducerClass>
ModulatableFloatParam<ModulatorSignalProducerClass>::ModulatableFloatParam(
        FloatParamS& leader
) noexcept
    : FloatParamS(leader),
    modulation_level("", 0.0, 0.0, 0.0),
    modulator(NULL)
{
    register_child(modulation_level);
}


template<class ModulatorSignalProducerClass>
bool ModulatableFloatParam<ModulatorSignalProducerClass>::is_constant_in_next_round(
        Integer const round, Integer const sample_count
) noexcept {
    if (modulator == NULL) {
        return FloatParamS::is_constant_in_next_round(round, sample_count);
    }

    return (
        modulation_level.is_constant_in_next_round(round, sample_count)
        && FloatParamS::is_constant_in_next_round(round, sample_count)
        && modulation_level.get_value() <= MODULATION_LEVEL_INSIGNIFICANT
    );
}


template<class ModulatorSignalProducerClass>
Sample const* const* ModulatableFloatParam<ModulatorSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Sample const* const* const buffer = FloatParamS::initialize_rendering(
        round, sample_count
    );

    if (modulator == NULL) {
        is_no_op = true;

        return buffer;
    }

    modulation_level_buffer = FloatParamS::produce_if_not_constant(
        modulation_level, round, sample_count
    );

    if (modulation_level_buffer == NULL) {
        is_no_op = modulation_level.get_value() <= MODULATION_LEVEL_INSIGNIFICANT;

        if (!is_no_op) {
            modulator_buffer = SignalProducer::produce<ModulatorSignalProducerClass>(
                *modulator, round, sample_count
            )[0];
        }
    } else {
        is_no_op = false;
        modulator_buffer = SignalProducer::produce<ModulatorSignalProducerClass>(
            *modulator, round, sample_count
        )[0];
    }

    if (is_no_op) {
        return buffer;
    }

    return NULL;
}


template<class ModulatorSignalProducerClass>
void ModulatableFloatParam<ModulatorSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    FloatParamS::render(round, first_sample_index, last_sample_index, buffer);

    if (is_no_op) {
        return;
    }

    Sample const* mod = modulator_buffer;
    Sample const* mod_level = modulation_level_buffer;

    if (mod_level == NULL) {
        Number const mod_level_value = modulation_level.get_value();

        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[0][i] += mod_level_value * mod[i];
        }
    } else {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[0][i] += mod_level[i] * mod[i];
        }
    }
}


template<class ModulatorSignalProducerClass>
void ModulatableFloatParam<ModulatorSignalProducerClass>::start_envelope(
        Seconds const time_offset
) noexcept {
    FloatParamS::start_envelope(time_offset);

    if (modulator != NULL) {
        modulation_level.start_envelope(time_offset);
    }
}


template<class ModulatorSignalProducerClass>
Seconds ModulatableFloatParam<ModulatorSignalProducerClass>::end_envelope(
        Seconds const time_offset
) noexcept {
    Seconds const envelope_end = FloatParamS::end_envelope(time_offset);

    if (modulator == NULL) {
        return envelope_end;
    }

    Seconds const modulation_level_envelope_end = (
        modulation_level.end_envelope(time_offset)
    );

    return std::max(envelope_end, modulation_level_envelope_end);
}


template<class ModulatorSignalProducerClass>
void ModulatableFloatParam<ModulatorSignalProducerClass>::cancel_envelope(
        Seconds const time_offset,
        Seconds const duration
) noexcept {
    FloatParamS::cancel_envelope(time_offset, duration);

    if (modulator != NULL) {
        modulation_level.cancel_envelope(time_offset, duration);
    }
}


template<class ModulatorSignalProducerClass>
void ModulatableFloatParam<ModulatorSignalProducerClass>::update_envelope(
        Seconds const time_offset
) noexcept {
    FloatParamS::update_envelope(time_offset);

    if (modulator != NULL) {
        modulation_level.update_envelope(time_offset);
    }
}


template<class ModulatorSignalProducerClass>
void ModulatableFloatParam<ModulatorSignalProducerClass>::skip_round(
        Integer const round,
        Integer const sample_count
) noexcept {
    FloatParamS::skip_round(round, sample_count);

    if (modulator != NULL) {
        modulation_level.skip_round(round, sample_count);
    }
}

}

#endif
