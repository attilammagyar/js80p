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

template<typename NumberType>
Param<NumberType>::Param(
        std::string const name,
        NumberType const min_value,
        NumberType const max_value,
        NumberType const default_value
) noexcept
    : SignalProducer(1),
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


template<typename NumberType>
std::string const& Param<NumberType>::get_name() const noexcept
{
    return name;
}


template<typename NumberType>
NumberType Param<NumberType>::get_default_value() const noexcept
{
    return default_value;
}


template<typename NumberType>
NumberType Param<NumberType>::get_value() const noexcept
{
    if (midi_controller != NULL) {
        return ratio_to_value(midi_controller->get_value());
    }

    return value;
}


template<typename NumberType>
NumberType Param<NumberType>::get_min_value() const noexcept
{
    return min_value;
}


template<typename NumberType>
NumberType Param<NumberType>::get_max_value() const noexcept
{
    return max_value;
}


template<typename NumberType>
void Param<NumberType>::set_value(NumberType const new_value) noexcept
{
    store_new_value(clamp(new_value));
}


template<typename NumberType>
void Param<NumberType>::store_new_value(NumberType const new_value) noexcept
{
    value = new_value;
    ++change_index;
    change_index &= 0x7fffffff;
}


template<typename NumberType>
NumberType Param<NumberType>::get_raw_value() const noexcept
{
    return value;
}


template<typename NumberType>
NumberType Param<NumberType>::clamp(NumberType const value) const noexcept
{
    return std::min(max_value, std::max(min_value, value));
}


template<typename NumberType>
Number Param<NumberType>::get_ratio() const noexcept
{
    if (midi_controller != NULL) {
        return midi_controller->get_value();
    }

    return std::min(1.0, std::max(0.0, value_to_ratio(value)));
}


template<typename NumberType>
Number Param<NumberType>::get_default_ratio() const noexcept
{
    return value_to_ratio(get_default_value());
}


template<typename NumberType>
void Param<NumberType>::set_ratio(Number const ratio) noexcept
{
    store_new_value(ratio_to_value(ratio));
}


template<typename NumberType>
Integer Param<NumberType>::get_change_index() const noexcept
{
    if (midi_controller != NULL) {
        return midi_controller->get_change_index();
    }

    return change_index;
}


template<typename NumberType>
NumberType Param<NumberType>::ratio_to_value(Number const ratio) const noexcept
{
    if (std::is_floating_point<NumberType>::value) {
        return clamp(min_value + (NumberType)((Number)range * ratio));
    }

    return clamp(min_value + (NumberType)std::round((Number)range * ratio));
}


template<typename NumberType>
Number Param<NumberType>::value_to_ratio(NumberType const value) const noexcept
{
    return ((Number)value - (Number)min_value) * range_inv;
}


template<typename NumberType>
void Param<NumberType>::set_midi_controller(
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


template<typename NumberType>
MidiController const* Param<NumberType>::get_midi_controller() const noexcept
{
    return midi_controller;
}


template<typename NumberType>
void Param<NumberType>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Sample const value = (Sample)this->value;

    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
        buffer[0][i] = value;
    }
}


ToggleParam::ToggleParam(std::string const name, Toggle const default_value)
    : Param<Toggle>(name, OFF, ON, default_value)
{
}


template<class FloatParamClass>
Sample const* const* FloatParam::produce(
        FloatParamClass& float_param,
        Integer const round,
        Integer const sample_count
) noexcept {
    Envelope* envelope = float_param.get_envelope();

    if (envelope != NULL && envelope->dynamic.get_value() == ToggleParam::ON) {
        envelope->update();
    }

    if (float_param.is_following_leader()) {
        return SignalProducer::produce<FloatParam>(
            *float_param.leader, round, sample_count
        );
    }

    return SignalProducer::produce<FloatParamClass>(
        float_param, round, sample_count
    );
}


template<class FloatParamClass>
Sample const* FloatParam::produce_if_not_constant(
        FloatParamClass& float_param,
        Integer const round,
        Integer const sample_count
) noexcept {
    if (float_param.is_constant_in_next_round(round, sample_count)) {
        float_param.skip_round(round, sample_count);

        return NULL;
    }

    return FloatParam::produce<FloatParamClass>(
        float_param, round, sample_count
    )[0];
}


FloatParam::FloatParam(
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
    : Param<Number>(name, min_value, max_value, default_value),
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


FloatParam::FloatParam(FloatParam& leader) noexcept
    : Param<Number>(
        leader.name, leader.min_value, leader.max_value, leader.default_value
    ),
    log_scale_toggle(leader.log_scale_toggle),
    log_scale_table(leader.log_scale_table),
    log_scale_table_max_index(leader.log_scale_table_max_index),
    log_scale_table_scale(leader.log_scale_table_scale),
    log_min_minus(log_scale_toggle != NULL ? -std::log2(min_value) : 0.0),
    log_range_inv(
        log_scale_toggle != NULL
            ? 1.0 / (std::log2(max_value) + log_min_minus)
            : 1.0
    ),
    leader(&leader),
    flexible_controller(NULL),
    lfo(NULL),
    envelope(NULL),
    envelope_change_index(-1),
    envelope_stage(EnvelopeStage::NONE),
    envelope_end_scheduled(false),
    should_round(false),
    is_ratio_same_as_value(
        std::fabs(min_value - 0.0) < 0.000001
        && std::fabs(max_value - 1.0) < 0.000001
    ),
    round_to(0.0),
    round_to_inv(0.0),
    constantness_round(-1),
    constantness(false),
    latest_event_type(EVT_SET_VALUE)
{
}


Number FloatParam::get_value() const noexcept
{
    if (is_following_leader()) {
        return leader->get_value();
    } else if (midi_controller != NULL) {
        return round_value(ratio_to_value(midi_controller->get_value()));
    } else if (flexible_controller != NULL) {
        flexible_controller->update();

        return round_value(ratio_to_value(flexible_controller->get_value()));
    } else {
        return get_raw_value();
    }
}


bool FloatParam::is_following_leader() const noexcept
{
    return leader != NULL && leader->envelope == NULL;
}


bool FloatParam::is_logarithmic() const noexcept
{
    return (
        log_scale_toggle != NULL
        && log_scale_toggle->get_value() == ToggleParam::ON
    );
}


void FloatParam::set_value(Number const new_value) noexcept
{
    latest_event_type = EVT_SET_VALUE;

    Param<Number>::set_value(round_value(new_value));
}


Number FloatParam::round_value(Number const value) const noexcept
{
    if (should_round) {
        return std::round(value * round_to_inv) * round_to;
    }

    return value;
}


void FloatParam::set_ratio(Number const ratio) noexcept
{
    set_value(ratio_to_value(ratio));
}


Number FloatParam::get_ratio() const noexcept
{
    if (is_following_leader()) {
        return leader->get_ratio();
    } else if (flexible_controller != NULL) {
        flexible_controller->update();

        return flexible_controller->get_value();
    } else if (midi_controller != NULL) {
        return midi_controller->get_value();
    }

    return std::min(1.0, std::max(0.0, value_to_ratio(get_raw_value())));
}


Number FloatParam::get_default_ratio() const noexcept
{
    return value_to_ratio(get_default_value());
}


Number FloatParam::ratio_to_value(Number const ratio) const noexcept
{
    if (is_logarithmic()) {
        return Math::lookup(
            log_scale_table,
            log_scale_table_max_index,
            ratio * log_scale_table_scale
        );
    }

    return Param<Number>::ratio_to_value(ratio);
}


Number FloatParam::value_to_ratio(Number const value) const noexcept
{
    if (is_logarithmic()) {
        return (std::log2(value) + log_min_minus) * log_range_inv;
    }

    return Param<Number>::value_to_ratio(value);
}


Integer FloatParam::get_change_index() const noexcept
{
    if (is_following_leader()) {
        return leader->get_change_index();
    } else if (flexible_controller != NULL) {
        flexible_controller->update();

        return flexible_controller->get_change_index();
    } else {
        return Param<Number>::get_change_index();
    }
}


bool FloatParam::is_constant_in_next_round(
        Integer const round, Integer const sample_count
) noexcept {
    if (round == constantness_round) {
        return constantness;
    }

    constantness_round = round;

    return constantness = is_constant_until(sample_count);
}


bool FloatParam::is_constant_until(Integer const sample_count) const noexcept
{
    if (is_following_leader()) {
        return leader->is_constant_until(sample_count);
    }

    if (lfo != NULL) {
        return false;
    }

    Integer const last_sample_idx = sample_count - 1;

    if (latest_event_type == EVT_LINEAR_RAMP || has_upcoming_events(last_sample_idx)) {
        return false;
    }

    Envelope* envelope = get_envelope();

    if (envelope != NULL && envelope->dynamic.get_value() == ToggleParam::ON) {
        envelope->update();

        return envelope_change_index == envelope->get_change_index();
    }

    if (midi_controller != NULL) {
        return (
            midi_controller->events.is_empty()
            || !(
                is_time_offset_before_sample_count(
                    midi_controller->events.front().time_offset, last_sample_idx
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


void FloatParam::skip_round(
        Integer const round,
        Integer const sample_count
) noexcept {
    if (is_following_leader()) {
        leader->skip_round(round, sample_count);
    } else if (cached_round != round && !events.is_empty()) {
        current_time += (Seconds)sample_count * sampling_period;
        cached_round = round;

        if (envelope_stage != EnvelopeStage::NONE) {
            Seconds const offset = sample_count_to_relative_time_offset(sample_count);

            envelope_position += offset;

            if (envelope_end_scheduled) {
                envelope_end_time_offset -= offset;
            }
        }
    }
}


void FloatParam::schedule_value(
        Seconds const time_offset,
        Number const new_value
) noexcept {
    schedule(EVT_SET_VALUE, time_offset, 0, 0.0, new_value);
}


void FloatParam::schedule_linear_ramp(
        Seconds const duration,
        Number const target_value
) noexcept {
    Seconds const last_event_time_offset = get_last_event_time_offset();

    if (is_logarithmic()) {
        schedule(
            EVT_LOG_RAMP, last_event_time_offset, 0, duration, target_value
        );
    } else {
        schedule(
            EVT_LINEAR_RAMP, last_event_time_offset, 0, duration, target_value
        );
    }

    schedule(
        EVT_SET_VALUE, last_event_time_offset + duration, 0, 0.0, target_value
    );
}


void FloatParam::handle_event(Event const& event) noexcept
{
    Param<Number>::handle_event(event);

    switch (event.type) {
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
            handle_envelope_end_event(event);
            break;

        case EVT_CANCEL:
            handle_cancel_event(event);
            break;
    }
}


void FloatParam::handle_set_value_event(Event const& event) noexcept
{
    set_value(event.number_param_2);
}


void FloatParam::handle_linear_ramp_event(Event const& event) noexcept
{
    Number const value = get_raw_value();
    Number const done_samples = (
        (Number)(current_time - event.time_offset) * (Number)sample_rate
    );
    Seconds duration = (Seconds)event.number_param_1;
    Number target_value = event.number_param_2;

    if (target_value < min_value) {
        Number const min_diff = min_value - value;
        Number const target_diff = target_value - value;

        duration *= (Seconds)(min_diff / target_diff);
        target_value = min_value;
    } else if (target_value > max_value) {
        Number const max_diff = max_value - value;
        Number const target_diff = target_value - value;

        duration *= (Seconds)(max_diff / target_diff);
        target_value = max_value;
    }

    latest_event_type = EVT_LINEAR_RAMP;
    linear_ramp_state.init(
        event.time_offset,
        done_samples,
        value,
        target_value,
        (Number)duration * (Number)sample_rate,
        duration,
        false
    );
}


void FloatParam::handle_log_ramp_event(Event const& event) noexcept
{
    Number const value = value_to_ratio(get_raw_value());
    Number const done_samples = (
        (Number)(current_time - event.time_offset) * (Number)sample_rate
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
        (Number)duration * (Number)sample_rate,
        duration,
        true
    );
}


void FloatParam::handle_envelope_start_event(Event const& event) noexcept
{
    envelope_stage = EnvelopeStage::DAHDS;
    envelope_position = current_time - event.time_offset;
}


void FloatParam::handle_envelope_end_event(Event const& event) noexcept
{
    envelope_stage = EnvelopeStage::R;
}


void FloatParam::handle_cancel_event(Event const& event) noexcept
{
    if (latest_event_type == EVT_LINEAR_RAMP) {
        Number const stop_value = linear_ramp_state.get_value_at(
            event.time_offset - linear_ramp_state.start_time_offset
        );

        if (linear_ramp_state.is_logarithmic) {
            store_new_value(ratio_to_value(stop_value));
        } else {
            store_new_value(stop_value);
        }
    }

    latest_event_type = EVT_SET_VALUE;
}


void FloatParam::set_midi_controller(
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


void FloatParam::set_flexible_controller(
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


FlexibleController const* FloatParam::get_flexible_controller() const noexcept
{
    return flexible_controller;
}


void FloatParam::set_envelope(Envelope* const envelope) noexcept
{
    this->envelope = envelope;

    if (envelope != NULL) {
        envelope->update();
        envelope_change_index = envelope->get_change_index();
    }

    envelope_stage = EnvelopeStage::NONE;
    envelope_end_scheduled = false;
    envelope_position = 0.0;
    envelope_end_time_offset = 0.0;
}


Envelope* FloatParam::get_envelope() const noexcept
{
    return leader == NULL ? envelope : leader->envelope;
}


void FloatParam::start_envelope(Seconds const time_offset) noexcept
{
    Seconds next_event_time_offset;
    Number next_value;
    Envelope const* const envelope = get_envelope();

    if (envelope == NULL) {
        return;
    }

    envelope_stage = EnvelopeStage::NONE;
    envelope_end_scheduled = false;
    envelope_position = 0.0;
    envelope_end_time_offset = 0.0;

    // initial-v ==delay-t==> initial-v ==attack-t==> peak-v ==hold-t==> peak-v ==decay-t==> sustain-v

    cancel_events_after(time_offset);

    schedule(EVT_ENVELOPE_START, time_offset);

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


Seconds FloatParam::end_envelope(Seconds const time_offset) noexcept
{
    Envelope const* const envelope = get_envelope();

    if (envelope == NULL) {
        return 0.0;
    }

    if (envelope->dynamic.get_value() == ToggleParam::ON) {
        envelope_final_value = (
            envelope->amount.get_value() * envelope->final_value.get_value()
        );
        envelope_release_time = (Seconds)envelope->release_time.get_value();
    }

    envelope_end_scheduled = true;
    envelope_end_time_offset = time_offset;

    // current-v ==release-t==> release-v

    cancel_events_after(time_offset);
    schedule(EVT_ENVELOPE_END, time_offset);
    schedule_linear_ramp(envelope_release_time, ratio_to_value(envelope_final_value));

    return envelope_release_time;
}


void FloatParam::set_lfo(LFO* lfo) noexcept
{
    this->lfo = lfo;
}


LFO const* FloatParam::get_lfo() const noexcept
{
    return lfo;
}


Sample const* const* FloatParam::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Param<Number>::initialize_rendering(round, sample_count);

    if (lfo != NULL) {
        return process_lfo(round, sample_count);
    } else if (midi_controller != NULL) {
        return process_midi_controller_events();
    } else if (flexible_controller != NULL) {
        return process_flexible_controller(sample_count);
    } else {
        Envelope* const envelope = get_envelope();

        if (envelope != NULL) {
            return process_envelope(envelope);
        }
    }

    return NULL;
}


Sample const* const* FloatParam::process_lfo(
        Integer const round,
        Integer const sample_count
) noexcept {
    lfo_buffer = SignalProducer::produce<LFO>(*lfo, round, sample_count);

    if (is_ratio_same_as_value) {
        if (sample_count > 0) {
            store_new_value(lfo_buffer[0][sample_count - 1]);
        }

        return lfo_buffer;
    }

    return NULL;
}


Sample const* const* FloatParam::process_midi_controller_events() noexcept
{
    Queue<Event>::SizeType const number_of_ctl_events = (
        midi_controller->events.length()
    );

    if (number_of_ctl_events == 0) {
        return NULL;
    }

    cancel_events_at(0.0);

    if (should_round) {
        for (Queue<Event>::SizeType i = 0; i != number_of_ctl_events; ++i) {
            Seconds const time_offset = midi_controller->events[i].time_offset;
            Number const controller_value = midi_controller->events[i].number_param_1;

            schedule_value(time_offset, ratio_to_value(controller_value));
        }

        return NULL;
    }

    Queue<Event>::SizeType const last_ctl_event_index = number_of_ctl_events - 1;

    Seconds previous_time_offset = 0.0;
    Number previous_ratio = value_to_ratio(get_raw_value());

    for (Queue<Event>::SizeType i = 0; i != number_of_ctl_events; ++i) {
        Seconds time_offset = midi_controller->events[i].time_offset;

        while (i != last_ctl_event_index) {
            ++i;

            Seconds const delta = std::fabs(
                midi_controller->events[i].time_offset - time_offset
            );

            if (delta >= MIDI_CTL_SMALL_CHANGE_DURATION) {
                --i;
                break;
            }
        }

        time_offset = midi_controller->events[i].time_offset;

        Number const controller_value = midi_controller->events[i].number_param_1;
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


Sample const* const* FloatParam::process_flexible_controller(
        Integer const sample_count
) noexcept {
    flexible_controller->update();

    Integer const new_change_index = flexible_controller->get_change_index();

    if (new_change_index == flexible_controller_change_index) {
        return NULL;
    }

    flexible_controller_change_index = new_change_index;

    cancel_events_at(0.0);

    Number const controller_value = flexible_controller->get_value();

    if (should_round) {
        set_value(ratio_to_value(controller_value));
    } else {
        Seconds const duration = smooth_change_duration(
            value_to_ratio(get_raw_value()),
            controller_value,
            (Seconds)std::max((Integer)0, sample_count - 1) * sampling_period
        );
        schedule_linear_ramp(duration, ratio_to_value(controller_value));
    }

    return NULL;
}


Seconds FloatParam::smooth_change_duration(
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



Sample const* const* FloatParam::process_envelope(
        Envelope* const envelope
) noexcept {
    if (envelope_stage == EnvelopeStage::NONE) {
        return NULL;
    }

    if (envelope->dynamic.get_value() != ToggleParam::ON) {
        return NULL;
    }

    Integer const new_change_index = envelope->get_change_index();
    bool const has_changed = new_change_index != envelope_change_index;

    envelope_change_index = new_change_index;

    Number const amount = envelope->amount.get_value();

    if (envelope_stage == EnvelopeStage::DAHDS) {
        cancel_events_at(0.0);

        if (envelope_position > envelope->get_dahd_length()) {
            Number const sustain_value = ratio_to_value(
                amount * envelope->sustain_value.get_value()
            );

            if (std::fabs(get_raw_value() - sustain_value) > 0.000001) {
                schedule_linear_ramp(0.1, sustain_value);
            }
        } else {
            Seconds next_event_time_offset = -envelope_position;

            next_event_time_offset = schedule_envelope_value_if_not_reached(
                next_event_time_offset,
                envelope->delay_time,
                envelope->initial_value,
                amount
            );
            next_event_time_offset = schedule_envelope_value_if_not_reached(
                next_event_time_offset,
                envelope->attack_time,
                envelope->peak_value,
                amount
            );
            next_event_time_offset = schedule_envelope_value_if_not_reached(
                next_event_time_offset,
                envelope->hold_time,
                envelope->peak_value,
                amount
            );
            next_event_time_offset = schedule_envelope_value_if_not_reached(
                next_event_time_offset,
                envelope->decay_time,
                envelope->sustain_value,
                amount
            );
        }
    }

    if (envelope_end_scheduled && (has_changed || envelope_stage == EnvelopeStage::DAHDS)) {
        if (envelope_end_time_offset < 0.0) {
            envelope_end_time_offset = 0.0;
        }

        envelope_release_time = std::min(
            envelope_release_time, (Seconds)envelope->release_time.get_value()
        );

        cancel_events_at(envelope_end_time_offset);
        schedule(EVT_ENVELOPE_END, envelope_end_time_offset);
        schedule_linear_ramp(
            envelope_release_time,
            ratio_to_value(amount * envelope->final_value.get_value())
        );
    }

    return NULL;
}


Seconds FloatParam::schedule_envelope_value_if_not_reached(
        Seconds const next_event_time_offset,
        FloatParam const& time_param,
        FloatParam const& value_param,
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


void FloatParam::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    if (lfo != NULL) {
        Sample sample;

        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[0][i] = sample = (Sample)ratio_to_value(lfo_buffer[0][i]);
        }

        if (last_sample_index != first_sample_index) {
            store_new_value((Number)sample);
        }
    } else if (latest_event_type == EVT_LINEAR_RAMP) {
        Sample sample;

        if (linear_ramp_state.is_logarithmic) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = sample = (
                    (Sample)ratio_to_value(linear_ramp_state.get_next_value())
                );
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = sample = (Sample)linear_ramp_state.get_next_value();
            }
        }

        if (last_sample_index != first_sample_index) {
            store_new_value((Number)sample);
        }
    } else {
        Param<Number>::render(round, first_sample_index, last_sample_index, buffer);
    }

    if (envelope_stage != EnvelopeStage::NONE) {
        Seconds const time_delta = sample_count_to_relative_time_offset(
            last_sample_index - first_sample_index
        );

        envelope_position += time_delta;

        if (envelope_end_scheduled) {
            envelope_end_time_offset -= time_delta;
        }
    }
}


FloatParam::LinearRampState::LinearRampState() noexcept
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


void FloatParam::LinearRampState::init(
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


Number FloatParam::LinearRampState::get_next_value() noexcept
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


Number FloatParam::LinearRampState::get_value_at(
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
        FloatParam& modulation_level_leader,
        std::string const name,
        Number const min_value,
        Number const max_value,
        Number const default_value
) noexcept
    : FloatParam(name, min_value, max_value, default_value),
    modulation_level(modulation_level_leader),
    modulator(modulator)
{
    register_child(modulation_level);
}


template<class ModulatorSignalProducerClass>
ModulatableFloatParam<ModulatorSignalProducerClass>::ModulatableFloatParam(
        FloatParam& leader
) noexcept
    : FloatParam(leader),
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
        return FloatParam::is_constant_in_next_round(round, sample_count);
    }

    return (
        modulation_level.is_constant_in_next_round(round, sample_count)
        && FloatParam::is_constant_in_next_round(round, sample_count)
        && modulation_level.get_value() <= MODULATION_LEVEL_INSIGNIFICANT
    );
}


template<class ModulatorSignalProducerClass>
Sample const* const* ModulatableFloatParam<ModulatorSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Sample const* const* const buffer = FloatParam::initialize_rendering(
        round, sample_count
    );

    if (modulator == NULL) {
        is_no_op = true;

        return buffer;
    }

    modulation_level_buffer = FloatParam::produce_if_not_constant(
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
    FloatParam::render(round, first_sample_index, last_sample_index, buffer);

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
    FloatParam::start_envelope(time_offset);

    if (modulator != NULL) {
        modulation_level.start_envelope(time_offset);
    }
}


template<class ModulatorSignalProducerClass>
Seconds ModulatableFloatParam<ModulatorSignalProducerClass>::end_envelope(
        Seconds const time_offset
) noexcept {
    Seconds const envelope_end = FloatParam::end_envelope(time_offset);

    if (modulator == NULL) {
        return envelope_end;
    }

    Seconds const modulation_level_envelope_end = (
        modulation_level.end_envelope(time_offset)
    );

    return std::max(envelope_end, modulation_level_envelope_end);
}


template<class ModulatorSignalProducerClass>
void ModulatableFloatParam<ModulatorSignalProducerClass>::skip_round(
        Integer const round,
        Integer const sample_count
) noexcept {
    FloatParam::skip_round(round, sample_count);

    if (modulator != NULL) {
        modulation_level.skip_round(round, sample_count);
    }
}

}

#endif
