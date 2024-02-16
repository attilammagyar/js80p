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

#ifndef JS80P__DSP__PARAM_CPP
#define JS80P__DSP__PARAM_CPP

#include <algorithm>
#include <cmath>
#include <utility>

#include "dsp/param.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

template<typename NumberType, ParamEvaluation evaluation>
Param<NumberType, evaluation>::Param(
        std::string const name,
        NumberType const min_value,
        NumberType const max_value,
        NumberType const default_value,
        Integer const number_of_events,
        SignalProducer* const buffer_owner
) noexcept
    : SignalProducer(
        evaluation == ParamEvaluation::SAMPLE ? 1 : 0,
        0,
        number_of_events,
        buffer_owner
    ),
    name(name),
    min_value(min_value),
    max_value(max_value),
    range((NumberType)(max_value - min_value)),
    default_value(default_value),
    midi_controller(NULL),
    macro(NULL),
    macro_change_index(-1),
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
    } else if (this->macro != NULL) {
        this->macro->update();

        return ratio_to_value(this->macro->get_value());
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
    } else if (this->macro != NULL) {
        this->macro->update();

        return this->macro->get_value();
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
    } else if (this->macro != NULL) {
        this->macro->update();

        return this->macro->get_change_index();
    }

    return change_index;
}


template<typename NumberType, ParamEvaluation evaluation>
NumberType Param<NumberType, evaluation>::ratio_to_value(
        Number const ratio
) const noexcept {
    if constexpr (std::is_floating_point<NumberType>::value) {
        return clamp(min_value + (NumberType)((Number)range * ratio));
    } else {
        return clamp(min_value + (NumberType)std::round((Number)range * ratio));
    }
}


template<typename NumberType, ParamEvaluation evaluation>
Number Param<NumberType, evaluation>::value_to_ratio(NumberType const value) const noexcept
{
    return ((Number)value - (Number)min_value) * range_inv;
}


template<typename NumberType, ParamEvaluation evaluation>
void Param<NumberType, evaluation>::set_midi_controller(
        MidiController* midi_controller
) noexcept {
    set_midi_controller< Param<NumberType, evaluation> >(*this, midi_controller);
}


template<typename NumberType, ParamEvaluation evaluation>
template<class ParamClass>
void Param<NumberType, evaluation>::set_midi_controller(
        ParamClass& param,
        MidiController* midi_controller
) noexcept {
    MidiController* const old_midi_controller = param.midi_controller;

    if (old_midi_controller != NULL) {
        old_midi_controller->released();

        if (midi_controller == NULL) {
            param.set_value(
                param.ratio_to_value(old_midi_controller->get_value())
            );
        }
    }

    if (midi_controller != NULL) {
        midi_controller->assigned();
        param.set_value(param.ratio_to_value(midi_controller->get_value()));
    }

    param.midi_controller = midi_controller;
}


template<typename NumberType, ParamEvaluation evaluation>
MidiController* Param<NumberType, evaluation>::get_midi_controller() const noexcept
{
    return midi_controller;
}


template<typename NumberType, ParamEvaluation evaluation>
void Param<NumberType, evaluation>::set_macro(Macro* macro) noexcept
{
    set_macro< Param<NumberType, evaluation> >(*this, macro);
}


template<typename NumberType, ParamEvaluation evaluation>
template<class ParamClass>
void Param<NumberType, evaluation>::set_macro(
        ParamClass& param,
        Macro* macro
) noexcept {
    Macro* const old_macro = param.macro;

    if (old_macro != NULL) {
        if (macro == NULL) {
            old_macro->update();
            param.set_value(param.ratio_to_value(old_macro->get_value()));
        }

        old_macro->released();
    }

    if (macro != NULL) {
        macro->assigned();
        macro->update();
        param.set_value(param.ratio_to_value(macro->get_value()));
        param.macro_change_index = macro->get_change_index();
    }

    param.macro = macro;
}


template<typename NumberType, ParamEvaluation evaluation>
Macro* Param<NumberType, evaluation>::get_macro() const noexcept
{
    return macro;
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


template<ParamEvaluation evaluation>
template<class FloatParamClass>
Sample const* const* FloatParam<evaluation>::produce(
        FloatParamClass& float_param,
        Integer const round,
        Integer const sample_count
) noexcept {
    Envelope* const envelope = float_param.get_envelope();

    if (envelope != NULL && envelope->is_dynamic()) {
        envelope->update();
    }

    if (float_param.is_following_leader()) {
        return SignalProducer::produce< FloatParam<evaluation> >(
            *float_param.leader, round, sample_count
        );
    }

    return SignalProducer::produce<FloatParamClass>(
        float_param, round, sample_count
    );
}


template<ParamEvaluation evaluation>
template<class FloatParamClass>
Sample const* FloatParam<evaluation>::produce_if_not_constant(
        FloatParamClass& float_param,
        Integer const round,
        Integer const sample_count
) noexcept {
    if (float_param.is_constant_in_next_round(round, sample_count)) {
        float_param.skip_round(round, sample_count);

        return NULL;
    }

    Sample const* const* const rendered = (
        FloatParam<evaluation>::produce<FloatParamClass>(
            float_param, round, sample_count
        )
    );

    if (rendered == NULL) {
        return NULL;
    }

    return (
        float_param.get_evaluation() == ParamEvaluation::SAMPLE
            ? rendered[0]
            : NULL
    );
}


template<ParamEvaluation evaluation>
FloatParam<evaluation>::FloatParam(
        std::string const name,
        Number const min_value,
        Number const max_value,
        Number const default_value,
        Number const round_to,
        ToggleParam const* log_scale_toggle,
        Number const* log_scale_table,
        int const log_scale_table_max_index,
        Number const log_scale_table_index_scale,
        Number const log_scale_value_offset
) noexcept
    : Param<Number, evaluation>(
        name, min_value, max_value, default_value, NUMBER_OF_EVENTS
    ),
    leader(NULL),
    round_to(round_to),
    round_to_inv(round_to > 0.0 ? 1.0 / round_to : 0.0),
    log_scale_toggle(log_scale_toggle),
    log_scale_table(log_scale_table),
    log_scale_table_index_scale(log_scale_table_index_scale),
    log_scale_value_offset(log_scale_value_offset),
    log_min_minus(
        log_scale_toggle != NULL
            ? -std::log2(min_value + log_scale_value_offset)
            : 0.0
    ),
    log_range_inv(
        log_scale_toggle != NULL
            ? 1.0 / (std::log2(max_value + log_scale_value_offset) + log_min_minus)
            : 1.0
    ),
    log_scale_table_max_index(log_scale_table_max_index),
    should_round(round_to > 0.0),
    is_ratio_same_as_value(
        log_scale_toggle == NULL
        && Math::is_close(min_value, 0.0)
        && Math::is_close(max_value, 1.0)
    )
{
    initialize_instance();
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::initialize_instance() noexcept
{
    lfo = NULL;

    random_seed = 0.5;

    envelope = NULL;
    active_envelope_snapshot_id = -1;
    scheduled_envelope_snapshot_id = -1;
    envelope_stage = EnvelopeStage::ENV_STG_NONE;
    envelope_time = 0.0;
    envelope_canceled = false;
    envelope_is_constant = true;

    std::fill_n(envelope_randoms, ENVELOPE_RANDOMS_COUNT, 0.0);

    constantness_round = -1;
    constantness = false;

    latest_event_type = EVT_SET_VALUE;
}


template<ParamEvaluation evaluation>
FloatParam<evaluation>::FloatParam(FloatParam<evaluation>& leader) noexcept
    : Param<Number, evaluation>(
        leader.get_name(),
        leader.get_min_value(),
        leader.get_max_value(),
        leader.get_default_value(),
        NUMBER_OF_EVENTS,
        (SignalProducer*)&leader
    ),
    leader(&leader),
    round_to(0.0),
    round_to_inv(0.0),
    log_scale_toggle(leader.get_log_scale_toggle()),
    log_scale_table(leader.get_log_scale_table()),
    log_scale_table_index_scale(leader.get_log_scale_table_index_scale()),
    log_scale_value_offset(leader.get_log_scale_value_offset()),
    log_min_minus(
        log_scale_toggle != NULL
            ? -std::log2(leader.get_min_value() + log_scale_value_offset)
            : 0.0
    ),
    log_range_inv(
        log_scale_toggle != NULL
            ? 1.0 / (std::log2(leader.get_max_value() + log_scale_value_offset) + log_min_minus)
            : 1.0
    ),
    log_scale_table_max_index(leader.get_log_scale_table_max_index()),
    should_round(false),
    is_ratio_same_as_value(
        leader.get_log_scale_toggle() == NULL
        && Math::is_close(leader.get_min_value(), 0.0)
        && Math::is_close(leader.get_max_value(), 1.0)
    )
{
    initialize_instance();
}


template<ParamEvaluation evaluation>
Number FloatParam<evaluation>::get_value() const noexcept
{
    if (is_following_leader()) {
        return leader->get_value();
    } else if (this->midi_controller != NULL) {
        return round_value(ratio_to_value(this->midi_controller->get_value()));
    } else if (this->macro != NULL) {
        this->macro->update();

        return round_value(ratio_to_value(this->macro->get_value()));
    } else {
        return this->get_raw_value();
    }
}


template<ParamEvaluation evaluation>
bool FloatParam<evaluation>::is_following_leader() const noexcept
{
    return leader != NULL && leader->get_envelope() == NULL;
}


template<ParamEvaluation evaluation>
bool FloatParam<evaluation>::is_logarithmic() const noexcept
{
    return (
        log_scale_toggle != NULL
        && log_scale_toggle->get_value() == ToggleParam::ON
    );
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::set_value(Number const new_value) noexcept
{
    latest_event_type = EVT_SET_VALUE;

    Param<Number, evaluation>::set_value(round_value(new_value));
}


template<ParamEvaluation evaluation>
Number FloatParam<evaluation>::round_value(Number const value) const noexcept
{
    if (should_round) {
        return std::round(value * round_to_inv) * round_to;
    }

    return value;
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::set_ratio(Number const ratio) noexcept
{
    set_value(ratio_to_value(ratio));
}


template<ParamEvaluation evaluation>
Number FloatParam<evaluation>::get_ratio() const noexcept
{
    if (is_following_leader()) {
        return leader->get_ratio();
    } else if (this->macro != NULL) {
        this->macro->update();

        return this->macro->get_value();
    } else if (this->midi_controller != NULL) {
        return this->midi_controller->get_value();
    }

    return std::min(1.0, std::max(0.0, value_to_ratio(this->get_raw_value())));
}


template<ParamEvaluation evaluation>
Number FloatParam<evaluation>::get_default_ratio() const noexcept
{
    return value_to_ratio(this->get_default_value());
}


template<ParamEvaluation evaluation>
ToggleParam const* FloatParam<evaluation>::get_log_scale_toggle() const noexcept
{
    return log_scale_toggle;
}


template<ParamEvaluation evaluation>
Number const* FloatParam<evaluation>::get_log_scale_table() const noexcept
{
    return log_scale_table;
}


template<ParamEvaluation evaluation>
int FloatParam<evaluation>::get_log_scale_table_max_index() const noexcept
{
    return log_scale_table_max_index;
}


template<ParamEvaluation evaluation>
Number FloatParam<evaluation>::get_log_scale_table_index_scale() const noexcept
{
    return log_scale_table_index_scale;
}


template<ParamEvaluation evaluation>
Number FloatParam<evaluation>::get_log_scale_value_offset() const noexcept
{
    return log_scale_value_offset;
}


template<ParamEvaluation evaluation>
Number FloatParam<evaluation>::ratio_to_value(Number const ratio) const noexcept
{
    if (is_logarithmic()) {
        return ratio_to_value_log(ratio);
    }

    return ratio_to_value_raw(ratio);
}


template<ParamEvaluation evaluation>
Number FloatParam<evaluation>::ratio_to_value_log(Number const ratio) const noexcept
{
    return Math::lookup(
        log_scale_table,
        log_scale_table_max_index,
        ratio * log_scale_table_index_scale
    );
}


template<ParamEvaluation evaluation>
Number FloatParam<evaluation>::ratio_to_value_raw(Number const ratio) const noexcept
{
    return Param<Number, evaluation>::ratio_to_value(ratio);
}


template<ParamEvaluation evaluation>
Number FloatParam<evaluation>::value_to_ratio(Number const value) const noexcept
{
    if (is_logarithmic()) {
        return (
            (std::log2(value + log_scale_value_offset) + log_min_minus) * log_range_inv
        );
    }

    return Param<Number, evaluation>::value_to_ratio(value);
}


template<ParamEvaluation evaluation>
Integer FloatParam<evaluation>::get_change_index() const noexcept
{
    if (is_following_leader()) {
        return leader->get_change_index();
    } else if (this->macro != NULL) {
        this->macro->update();

        return this->macro->get_change_index();
    } else {
        return Param<Number, evaluation>::get_change_index();
    }
}


template<ParamEvaluation evaluation>
bool FloatParam<evaluation>::is_constant_in_next_round(
        Integer const round,
        Integer const sample_count
) noexcept {
    if (round == constantness_round) {
        return constantness;
    }

    constantness_round = round;

    constantness = is_constant_until(sample_count);

    return constantness;
}


template<ParamEvaluation evaluation>
bool FloatParam<evaluation>::is_constant_until(
        Integer const sample_count
) const noexcept {
    if (is_following_leader()) {
        return leader->is_constant_until(sample_count);
    }

    if (lfo != NULL) {
        return false;
    }

    Integer const last_sample_idx = sample_count - 1;

    if (is_ramping() || this->has_upcoming_events(last_sample_idx)) {
        return false;
    }

    Envelope* const envelope = get_envelope();

    if (envelope != NULL) {
        if (
                envelope->is_dynamic()
                && active_envelope_snapshot_id >= 0
                && (
                    envelope_stage == EnvelopeStage::ENV_STG_SUSTAIN
                    || envelope_stage == EnvelopeStage::ENV_STG_RELEASED
                )
        ) {
            if (envelope_is_constant) {
                return true;
            }

            envelope->update();

            Integer const envelope_change_index = envelope->get_change_index();
            Integer const snapshot_change_index = (
                envelope_snapshots[active_envelope_snapshot_id].change_index
            );

            return snapshot_change_index == envelope_change_index;
        }

        return envelope_is_constant || envelope_stage == EnvelopeStage::ENV_STG_NONE;
    }

    if (this->midi_controller != NULL) {
        return this->midi_controller->events.is_empty();
    }

    if (this->macro != NULL) {
        this->macro->update();

        return this->macro->get_change_index() == this->macro_change_index;
    }

    return true;
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::skip_round(
        Integer const round,
        Integer const sample_count
) noexcept {
    if (is_following_leader()) {
        leader->skip_round(round, sample_count);
    } else if (this->cached_round != round && !this->events.is_empty()) {
        this->current_time += (Seconds)sample_count * this->sampling_period;
        this->cached_round = round;

        if (envelope_stage != EnvelopeStage::ENV_STG_NONE) {
            Seconds const offset = this->sample_count_to_relative_time_offset(sample_count);

            envelope_time += offset;
        }
    }
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::schedule_value(
        Seconds const time_offset,
        Number const new_value
) noexcept {
    this->schedule(EVT_SET_VALUE, time_offset, 0, 0.0, new_value);
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::schedule_linear_ramp(
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


template<ParamEvaluation evaluation>
bool FloatParam<evaluation>::is_ramping() const noexcept
{
    return latest_event_type == EVT_LINEAR_RAMP;
}


template<ParamEvaluation evaluation>
Seconds FloatParam<evaluation>::get_remaining_time_from_linear_ramp() const noexcept
{
    if (is_ramping()) {
        return linear_ramp_state.get_remaining_samples() * this->sampling_period;
    }

    return 0.0;
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::handle_event(
        SignalProducer::Event const& event
) noexcept {
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

        case EVT_ENVELOPE_UPDATE:
            handle_envelope_update_event(event);
            break;

        case EVT_ENVELOPE_END:
            handle_envelope_end_event(event);
            break;

        case EVT_ENVELOPE_CANCEL:
            handle_envelope_cancel_event(event);
            break;

        default:
            break;
    }
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::handle_set_value_event(
        SignalProducer::Event const& event
) noexcept {
    set_value(event.number_param_2);
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::handle_linear_ramp_event(
        SignalProducer::Event const& event
) noexcept {
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


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::handle_log_ramp_event(
        SignalProducer::Event const& event
) noexcept {
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


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::handle_envelope_start_event(
        SignalProducer::Event const& event
) noexcept {
    Envelope* const envelope = get_envelope();

    if (envelope == NULL) {
        return;
    }

    active_envelope_snapshot_id = event.int_param;
    EnvelopeSnapshot& snapshot = envelope_snapshots[active_envelope_snapshot_id];

    if (envelope->is_dynamic()) {
        envelope->update();
        envelope->make_snapshot(envelope_randoms, snapshot);
    }

    Seconds const latency = this->current_time - event.time_offset;

    envelope_stage = EnvelopeStage::ENV_STG_DAHD;
    envelope_time = latency;

    this->store_new_value(ratio_to_value(snapshot.initial_value));
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::handle_envelope_update_event(
        SignalProducer::Event const& event
) noexcept {
    Envelope* const envelope = get_envelope();

    if (envelope == NULL || active_envelope_snapshot_id < 0) {
        return;
    }

    unused_envelope_snapshots.push(active_envelope_snapshot_id);

    active_envelope_snapshot_id = event.int_param;
    EnvelopeSnapshot& new_snapshot = envelope_snapshots[active_envelope_snapshot_id];

    envelope->update();
    envelope->make_snapshot(envelope_randoms, new_snapshot);

    if (
            envelope_stage == EnvelopeStage::ENV_STG_SUSTAIN
            || envelope_stage == EnvelopeStage::ENV_STG_RELEASED
    ) {
        envelope_time = 0.0;
    }
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::handle_envelope_end_event(
        SignalProducer::Event const& event
) noexcept {
    if (
            JS80P_UNLIKELY(
                envelope_stage == EnvelopeStage::ENV_STG_RELEASED
                || envelope_stage == EnvelopeStage::ENV_STG_NONE
            )
    ) {
        return;
    }

    Seconds const latency = this->current_time - event.time_offset;

    store_envelope_value_at_event(latency);

    envelope_stage = EnvelopeStage::ENV_STG_RELEASE;
    envelope_time = latency;
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::store_envelope_value_at_event(
        Seconds const latency
) noexcept {
    if (active_envelope_snapshot_id < 0) {
        return;
    }

    EnvelopeSnapshot const& snapshot = envelope_snapshots[active_envelope_snapshot_id];

    Number const ratio_at_time_of_event = Envelope::get_value_at_time(
        snapshot,
        envelope_time - latency,
        envelope_stage,
        value_to_ratio(this->get_raw_value()),
        this->sampling_period
    );

    this->store_new_value(ratio_to_value(ratio_at_time_of_event));
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::handle_envelope_cancel_event(
        SignalProducer::Event const& event
) noexcept {
    if (active_envelope_snapshot_id >= 0) {
        EnvelopeSnapshot& snapshot = envelope_snapshots[active_envelope_snapshot_id];

        snapshot.release_time = std::min((Seconds)event.number_param_1, snapshot.release_time);
    }

    handle_envelope_end_event(event);
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::handle_cancel_event(
        SignalProducer::Event const& event
) noexcept {
    if (is_ramping()) {
        Number const stop_value = linear_ramp_state.get_value_at(
            event.time_offset - linear_ramp_state.start_time_offset
        );

        if (linear_ramp_state.is_logarithmic) {
            this->store_new_value(ratio_to_value_log(stop_value));
        } else {
            this->store_new_value(stop_value);
        }
    } else {
        Envelope* const envelope = get_envelope();

        if (envelope != NULL) {
            Seconds const latency = this->current_time - event.time_offset;

            store_envelope_value_at_event(latency);
        }
    }

    latest_event_type = EVT_SET_VALUE;
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::set_midi_controller(
        MidiController* midi_controller
) noexcept {
    Param<Number, evaluation>::template set_midi_controller< FloatParam<evaluation> >(*this, midi_controller);
}


template<ParamEvaluation evaluation>
MidiController* FloatParam<evaluation>::get_midi_controller() const noexcept
{
    return (
        leader == NULL
            ? Param<Number, evaluation>::get_midi_controller()
            : leader->get_midi_controller()
    );
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::set_macro(Macro* macro) noexcept
{
    Param<Number, evaluation>::template set_macro< FloatParam<evaluation> >(*this, macro);
}


template<ParamEvaluation evaluation>
Macro* FloatParam<evaluation>::get_macro() const noexcept
{
    return (
        leader == NULL
            ? Param<Number, evaluation>::get_macro()
            : leader->get_macro()
    );
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::set_random_seed(Number const seed) noexcept
{
    random_seed = seed;
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::set_envelope(Envelope* const envelope) noexcept
{
    this->envelope = envelope;

    if (envelope != NULL) {
        envelope_snapshots.reserve(2);
        unused_envelope_snapshots.reserve(2);
        envelope->update();
    }

    this->cancel_events();

    envelope_snapshots.clear();
    unused_envelope_snapshots.drop(0);

    envelope_stage = EnvelopeStage::ENV_STG_NONE;
    envelope_time = 0.0;
    active_envelope_snapshot_id = -1;
    scheduled_envelope_snapshot_id = -1;
    envelope_canceled = false;
}


template<ParamEvaluation evaluation>
Envelope* FloatParam<evaluation>::get_envelope() const noexcept
{
    return leader == NULL ? envelope : leader->get_envelope();
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::start_envelope(
        Seconds const time_offset,
        Number const random_1,
        Number const random_2
) noexcept {
    Envelope* const envelope = get_envelope();

    if (envelope == NULL) {
        return;
    }

    update_envelope_randoms(random_1, random_2);

    envelope->update();
    Integer const snapshot_id = make_envelope_snapshot(envelope);

    scheduled_envelope_snapshot_id = snapshot_id;
    envelope_canceled = false;

    this->cancel_events_after(time_offset);
    this->schedule(EVT_ENVELOPE_START, time_offset, snapshot_id);
}


template<ParamEvaluation evaluation>
Integer FloatParam<evaluation>::make_envelope_snapshot(Envelope* const envelope) noexcept
{
    EnvelopeSnapshot snapshot;

    envelope->make_snapshot(envelope_randoms, snapshot);

    std::vector<EnvelopeSnapshot>::size_type snapshot_id;

    if (unused_envelope_snapshots.is_empty()) {
        snapshot_id = envelope_snapshots.size();
        envelope_snapshots.push_back(std::move(snapshot));
    } else {
        snapshot_id = (
            (std::vector<EnvelopeSnapshot>::size_type)unused_envelope_snapshots.pop()
        );
        envelope_snapshots[snapshot_id] = std::move(snapshot);
    }

    return (Integer)snapshot_id;
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::update_envelope_randoms(
        Number const random_1,
        Number const random_2
) noexcept {
    Number const random_avg = (random_1 + random_2 + random_seed) * 0.333;
    Number const random_1_with_seed = (
        Math::randomize(1.0, 0.5 * (random_1 + random_seed))
    );
    Number const random_2_with_seed = (
        Math::randomize(1.0, 0.5 * (random_2 + random_seed))
    );

    envelope_randoms[0] = random_1_with_seed;
    envelope_randoms[1] = random_2_with_seed;
    envelope_randoms[2] = Math::randomize(1.0, random_avg);
    envelope_randoms[3] = Math::randomize(1.0, random_1_with_seed);
    envelope_randoms[4] = Math::randomize(1.0, random_2_with_seed);
    envelope_randoms[5] = Math::randomize(1.0, 1.0 - random_avg);
    envelope_randoms[6] = Math::randomize(1.0, 1.0 - random_1_with_seed);
    envelope_randoms[7] = Math::randomize(1.0, 1.0 - random_2_with_seed);
    envelope_randoms[8] = Math::randomize(1.0, 0.3 + 0.7 * random_1_with_seed);
}


template<ParamEvaluation evaluation>
Seconds FloatParam<evaluation>::end_envelope(Seconds const time_offset) noexcept
{
    if (envelope_canceled) {
        return envelope_cancel_duration;
    }

    return end_envelope<EVT_ENVELOPE_END>(time_offset);
}


template<ParamEvaluation evaluation>
template<SignalProducer::Event::Type event>
Seconds FloatParam<evaluation>::end_envelope(
        Seconds const time_offset,
        Seconds const duration
) noexcept {
    Envelope* const envelope = get_envelope();

    if (envelope == NULL || scheduled_envelope_snapshot_id < 0) {
        return 0.0;
    }

    EnvelopeSnapshot& snapshot = envelope_snapshots[scheduled_envelope_snapshot_id];

    if (envelope->is_dynamic()) {
        envelope->update();
        envelope->make_end_snapshot(envelope_randoms, snapshot);
    }

    if constexpr (event == EVT_ENVELOPE_CANCEL) {
        this->schedule(event, time_offset, 0, std::min(snapshot.release_time, duration));
    } else {
        this->schedule(event, time_offset);
    }

    return snapshot.release_time;
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::cancel_envelope(
        Seconds const time_offset,
        Seconds const duration
) noexcept {
    envelope_canceled = true;
    envelope_cancel_duration = end_envelope<EVT_ENVELOPE_CANCEL>(time_offset, duration);
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::update_envelope(Seconds const time_offset) noexcept
{
    Envelope* const envelope = get_envelope();

    if (envelope == NULL) {
        return;
    }

    envelope->update();
    Integer const snapshot_id = make_envelope_snapshot(envelope);

    scheduled_envelope_snapshot_id = snapshot_id;

    this->schedule(EVT_ENVELOPE_UPDATE, time_offset, snapshot_id);
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::set_lfo(LFO* lfo) noexcept
{
    this->lfo = lfo;
}


template<ParamEvaluation evaluation>
LFO const* FloatParam<evaluation>::get_lfo() const noexcept
{
    return leader == NULL ? lfo : leader->get_lfo();
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::reset() noexcept
{
    SignalProducer::reset();

    active_envelope_snapshot_id = -1;
    scheduled_envelope_snapshot_id = -1;
    envelope_stage = EnvelopeStage::ENV_STG_NONE;
    envelope_time = 0.0;
    envelope_canceled = false;
    envelope_is_constant = true;

    envelope_snapshots.clear();
    unused_envelope_snapshots.drop(0);
}


template<ParamEvaluation evaluation>
Sample const* const* FloatParam<evaluation>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Param<Number, evaluation>::initialize_rendering(round, sample_count);

    if (lfo != NULL) {
        return process_lfo(round, sample_count);
    } else if (this->midi_controller != NULL) {
        if (is_logarithmic()) {
            process_midi_controller_events<true>();
        } else {
            process_midi_controller_events<false>();
        }
    } else if (this->macro != NULL) {
        process_macro(sample_count);
    } else {
        Envelope* const envelope = get_envelope();

        if (envelope != NULL) {
            process_envelope(*envelope);
        }
    }

    return NULL;
}


template<ParamEvaluation evaluation>
Sample const* const* FloatParam<evaluation>::process_lfo(
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


template<ParamEvaluation evaluation>
template<bool is_logarithmic_>
void FloatParam<evaluation>::process_midi_controller_events() noexcept
{
    Queue<SignalProducer::Event>::SizeType const number_of_ctl_events = (
        this->midi_controller->events.length()
    );

    if (number_of_ctl_events == 0) {
        return;
    }

    this->cancel_events_at(this->midi_controller->events[0].time_offset);

    if (should_round) {
        for (Queue<SignalProducer::Event>::SizeType i = 0; i != number_of_ctl_events; ++i) {
            Seconds const time_offset = this->midi_controller->events[i].time_offset;
            Number const controller_value = this->midi_controller->events[i].number_param_1;

            if constexpr (is_logarithmic_) {
                schedule_value(time_offset, ratio_to_value_log(controller_value));
            } else {
                schedule_value(time_offset, ratio_to_value_raw(controller_value));
            }
        }

        return;
    }

    Queue<SignalProducer::Event>::SizeType const last_ctl_event_index = number_of_ctl_events - 1;

    Seconds previous_time_offset = 0.0;
    Number previous_ratio = value_to_ratio(this->get_raw_value());

    for (Queue<SignalProducer::Event>::SizeType i = 0; i != number_of_ctl_events; ++i) {
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

        if constexpr (is_logarithmic_) {
            schedule_linear_ramp(duration, ratio_to_value_log(controller_value));
        } else {
            schedule_linear_ramp(duration, ratio_to_value_raw(controller_value));
        }

        previous_time_offset = time_offset;
    }
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::process_macro(Integer const sample_count) noexcept
{
    this->macro->update();

    Integer const new_change_index = this->macro->get_change_index();

    if (new_change_index == this->macro_change_index) {
        return;
    }

    this->macro_change_index = new_change_index;

    this->cancel_events_at(0.0);

    Number const macro_value = this->macro->get_value();

    if (should_round) {
        set_value(ratio_to_value(macro_value));
    } else {
        Seconds const duration = smooth_change_duration(
            value_to_ratio(this->get_raw_value()),
            macro_value,
            (Seconds)std::max((Integer)0, sample_count - 1) * this->sampling_period
        );
        schedule_linear_ramp(duration, ratio_to_value(macro_value));
    }
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::process_envelope(Envelope& envelope) noexcept
{
    if (!envelope.is_dynamic() || active_envelope_snapshot_id < 0) {
        return;
    }

    EnvelopeSnapshot& snapshot = envelope_snapshots[active_envelope_snapshot_id];

    envelope.update();

    if (snapshot.change_index == envelope.get_change_index()) {
        return;
    }

    Seconds const old_release_time = snapshot.release_time;

    if (
            envelope_stage == EnvelopeStage::ENV_STG_RELEASE
            || envelope_stage == EnvelopeStage::ENV_STG_RELEASED
    ) {
        envelope_time = 0.0;
        envelope.make_end_snapshot(envelope_randoms, snapshot);
    } else {
        envelope.make_snapshot(envelope_randoms, snapshot);

        if (envelope_stage == EnvelopeStage::ENV_STG_SUSTAIN) {
            envelope_time = 0.0;
        }
    }

    snapshot.release_time = std::min(old_release_time, snapshot.release_time);
}


template<ParamEvaluation evaluation>
Seconds FloatParam<evaluation>::smooth_change_duration(
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


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    if constexpr (evaluation == ParamEvaluation::SAMPLE) {
        if (lfo != NULL) {
            render_with_lfo(round, first_sample_index, last_sample_index, buffer);
        } else if (is_ramping()) {
            render_linear_ramp(round, first_sample_index, last_sample_index, buffer);
        } else if (get_envelope() != NULL) {
            render_with_envelope(round, first_sample_index, last_sample_index, buffer);
        } else {
            Param<Number, evaluation>::render(
                round, first_sample_index, last_sample_index, buffer
            );
        }
    }
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::render_with_lfo(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Sample sample;

    if (is_logarithmic()) {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[0][i] = sample = (Sample)ratio_to_value_log(lfo_buffer[0][i]);
        }
    } else {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[0][i] = sample = (Sample)ratio_to_value_raw(lfo_buffer[0][i]);
        }
    }

    if (last_sample_index != first_sample_index) {
        this->store_new_value((Number)sample);
    }
}


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::render_linear_ramp(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Sample sample;

    if (linear_ramp_state.is_logarithmic) {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[0][i] = sample = (
                (Sample)ratio_to_value_log(linear_ramp_state.advance())
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


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::render_with_envelope(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    if (active_envelope_snapshot_id < 0) {
        Param<Number, evaluation>::render(
            round, first_sample_index, last_sample_index, buffer
        );

        return;
    }

    EnvelopeSnapshot& snapshot = envelope_snapshots[active_envelope_snapshot_id];
    Sample* buffer_ = buffer[0];
    Sample ratio = value_to_ratio(this->get_raw_value());

    Envelope::render(
        snapshot,
        envelope_time,
        envelope_stage,
        envelope_is_constant,
        ratio,
        this->sample_rate,
        this->sampling_period,
        first_sample_index,
        last_sample_index,
        buffer_
    );

    if (is_ratio_same_as_value) {
        this->store_new_value(ratio);
    } else if (is_logarithmic()) {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer_[i] = (Sample)ratio_to_value_log(buffer_[i]);
        }

        this->store_new_value(ratio_to_value_log(ratio));
    } else {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer_[i] = (Sample)ratio_to_value_raw(buffer_[i]);
        }

        this->store_new_value(ratio_to_value_raw(ratio));
    }

    if (envelope_stage == EnvelopeStage::ENV_STG_RELEASED) {
        unused_envelope_snapshots.push(active_envelope_snapshot_id);
        active_envelope_snapshot_id = -1;
        envelope_is_constant = true;
    }
}


template<ParamEvaluation evaluation>
FloatParam<evaluation>::LinearRampState::LinearRampState() noexcept
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


template<ParamEvaluation evaluation>
void FloatParam<evaluation>::LinearRampState::init(
        Seconds const start_time_offset,
        Number const done_samples,
        Number const initial_value,
        Number const target_value,
        Number const duration_in_samples,
        Seconds const duration,
        bool const is_logarithmic
) noexcept {
    this->is_logarithmic = is_logarithmic;

    if (duration_in_samples > 0.0) {
        is_done = false;

        this->start_time_offset = start_time_offset;
        this->done_samples = done_samples;
        this->initial_value = initial_value;
        this->target_value = target_value;
        this->duration_in_samples = duration_in_samples;
        this->duration = duration;

        delta = target_value - initial_value;
        speed = 1.0 / duration_in_samples;
    } else {
        is_done = true;
        this->done_samples = 0.0;
        this->target_value = target_value;
        this->duration_in_samples = 0.0;
    }
}


template<ParamEvaluation evaluation>
Number FloatParam<evaluation>::LinearRampState::advance() noexcept
{
    if (is_done) {
        return target_value;
    }

    Number const next_value = initial_value + (done_samples * speed) * delta;

    done_samples += 1.0;

    if (done_samples >= duration_in_samples) {
        done_samples = duration_in_samples;
        is_done = true;
    }

    return next_value;
}


template<ParamEvaluation evaluation>
Number FloatParam<evaluation>::LinearRampState::get_value_at(
        Seconds const time_offset
) const noexcept {
    if (duration > 0.0 && time_offset <= duration) {
        return initial_value + (time_offset / duration) * delta;
    } else {
        return target_value;
    }
}


template<ParamEvaluation evaluation>
Number FloatParam<evaluation>::LinearRampState::get_remaining_samples() const noexcept
{
    return is_done ? 0.0 : (duration_in_samples - done_samples);
}


template<class ModulatorSignalProducerClass>
ModulatableFloatParam<ModulatorSignalProducerClass>::ModulatableFloatParam(
        ModulatorSignalProducerClass& modulator,
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
bool ModulatableFloatParam<ModulatorSignalProducerClass>::is_constant_in_next_round(
        Integer const round,
        Integer const sample_count
) noexcept {
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

    modulation_level_buffer = FloatParamS::produce_if_not_constant(
        modulation_level, round, sample_count
    );

    if (modulation_level_buffer == NULL) {
        is_no_op = modulation_level.get_value() <= MODULATION_LEVEL_INSIGNIFICANT;

        if (is_no_op) {
            return buffer;
        } else {
            modulator_buffer = SignalProducer::produce<ModulatorSignalProducerClass>(
                modulator, round, sample_count
            )[0];
        }
    } else {
        is_no_op = false;
        modulator_buffer = SignalProducer::produce<ModulatorSignalProducerClass>(
            modulator, round, sample_count
        )[0];
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
void ModulatableFloatParam<ModulatorSignalProducerClass>::set_random_seed(
        Number const seed
) noexcept {
    FloatParamS::set_random_seed(seed);

    modulation_level.set_random_seed(Math::randomize(1.0, 1.0 - seed));
}


template<class ModulatorSignalProducerClass>
void ModulatableFloatParam<ModulatorSignalProducerClass>::start_envelope(
        Seconds const time_offset,
        Number const random_1,
        Number const random_2
) noexcept {
    FloatParamS::start_envelope(time_offset, random_1, random_2);

    modulation_level.start_envelope(time_offset, random_2, random_1);
}


template<class ModulatorSignalProducerClass>
Seconds ModulatableFloatParam<ModulatorSignalProducerClass>::end_envelope(
        Seconds const time_offset
) noexcept {
    Seconds const envelope_end = FloatParamS::end_envelope(time_offset);

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

    modulation_level.cancel_envelope(time_offset, duration);
}


template<class ModulatorSignalProducerClass>
void ModulatableFloatParam<ModulatorSignalProducerClass>::update_envelope(
        Seconds const time_offset
) noexcept {
    FloatParamS::update_envelope(time_offset);

    modulation_level.update_envelope(time_offset);
}


template<class ModulatorSignalProducerClass>
void ModulatableFloatParam<ModulatorSignalProducerClass>::skip_round(
        Integer const round,
        Integer const sample_count
) noexcept {
    FloatParamS::skip_round(round, sample_count);

    modulation_level.skip_round(round, sample_count);
}

}

#endif
