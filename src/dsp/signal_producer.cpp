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

#ifndef JS80P__DSP__SIGNAL_PRODUCER_CPP
#define JS80P__DSP__SIGNAL_PRODUCER_CPP

#include <algorithm>

#include "dsp/signal_producer.hpp"


namespace JS80P
{

template<class SignalProducerClass>
Sample const* const* SignalProducer::produce(
        SignalProducerClass& signal_producer,
        Integer const round,
        Integer const sample_count
) noexcept {
    if (signal_producer.cached_round == round) {
        return signal_producer.cached_buffer;
    }

    Seconds const start_time = signal_producer.current_time;
    Integer const count = signal_producer.sample_count_or_block_size(sample_count);

    signal_producer.cached_round = round;
    signal_producer.cached_buffer = signal_producer.initialize_rendering(round, count);
    signal_producer.last_sample_count = count;

    if (signal_producer.cached_buffer != NULL) {
        return signal_producer.cached_buffer;
    }

    Sample** buffer = signal_producer.buffer;

    signal_producer.cached_buffer = buffer;

    if (signal_producer.has_upcoming_events(count)) {
        Integer current_sample_index = 0;
        Integer next_stop;

        while (current_sample_index != count) {
            handle_events<SignalProducerClass>(
                signal_producer, current_sample_index, count, next_stop
            );
            signal_producer.render(
                round, current_sample_index, next_stop, buffer
            );
            current_sample_index = next_stop;
            signal_producer.current_time = (
                start_time
                + (Seconds)current_sample_index * signal_producer.sampling_period
            );
        }
    } else {
        signal_producer.render(round, 0, count, buffer);
        signal_producer.current_time += (
            (Seconds)count * signal_producer.sampling_period
        );
    }

    signal_producer.finalize_rendering(round, count);

    if (signal_producer.events.is_empty()) {
        signal_producer.current_time = 0.0;
    }

    return buffer;
}


void SignalProducer::find_peak(
        Sample const* const* samples,
        Integer const channels,
        Integer const size,
        Sample& peak,
        Integer& peak_index
) noexcept {
    peak = 0.0;
    peak_index = 0;

    for (Integer c = 0; c != channels; ++c) {
        for (Integer i = 0; i != size; ++i) {
            Sample const sample = std::fabs(samples[c][i]);

            if (sample >= peak) {
                peak = sample;
                peak_index = i;
            }
        }
    }
}


SignalProducer::SignalProducer(
        Integer const channels,
        Integer const number_of_children
) noexcept
    : channels(0 <= channels ? channels : 0),
    buffer(NULL),
    last_sample_count(0),
    block_size(DEFAULT_BLOCK_SIZE),
    sample_rate(DEFAULT_SAMPLE_RATE),
    sampling_period(1.0 / (Seconds)DEFAULT_SAMPLE_RATE),
    nyquist_frequency(DEFAULT_SAMPLE_RATE * 0.5),
    bpm(DEFAULT_BPM),
    current_time(0.0),
    cached_round(-1),
    cached_buffer(NULL),
    cached_silence_round(-1),
    cached_silence(false)
{
    children.reserve(number_of_children);

    buffer = allocate_buffer();
}


void SignalProducer::set_block_size(Integer const new_block_size) noexcept
{
    if (new_block_size != block_size) {
        block_size = new_block_size;
        buffer = reallocate_buffer(buffer);
        last_sample_count = 0;
        cached_round = -1;
        cached_buffer = NULL;

        for (Children::iterator it = children.begin(); it != children.end(); ++it) {
            (*it)->set_block_size(new_block_size);
        }
    }
}


Sample** SignalProducer::reallocate_buffer(Sample** old_buffer) const noexcept
{
    free_buffer(old_buffer);

    return allocate_buffer();
}


Sample** SignalProducer::allocate_buffer() const noexcept
{
    if (channels <= 0) {
        return NULL;
    }

    Sample** new_buffer = new Sample*[channels];

    for (Integer c = 0; c != channels; ++c) {
        new_buffer[c] = new Sample[block_size];
        std::fill_n(new_buffer[c], block_size, 0.0);
    }

    return new_buffer;
}


void SignalProducer::set_sample_rate(Frequency const new_sample_rate) noexcept
{
    sample_rate = new_sample_rate;
    sampling_period = 1.0 / (Seconds)new_sample_rate;
    nyquist_frequency = new_sample_rate * 0.5;

    for (Children::iterator it = children.begin(); it != children.end(); ++it) {
        (*it)->set_sample_rate(new_sample_rate);
    }
}


SignalProducer::~SignalProducer() noexcept
{
    buffer = free_buffer(buffer);
}


Sample** SignalProducer::free_buffer(Sample** old_buffer) const noexcept
{
    if (old_buffer == NULL) {
        return NULL;
    }

    for (Integer i = 0; i != channels; ++i) {
        delete[] old_buffer[i];

        old_buffer[i] = NULL;
    }

    delete[] old_buffer;

    return NULL;
}


Integer SignalProducer::get_channels() const noexcept
{
    return channels;
}


Frequency SignalProducer::get_sample_rate() const noexcept
{
    return sample_rate;
}


Integer SignalProducer::get_block_size() const noexcept
{
    return block_size;
}


void SignalProducer::reset() noexcept
{
    cancel_events();

    render_silence(-1, 0, block_size, buffer);

    for (Children::iterator it = children.begin(); it != children.end(); ++it) {
        (*it)->reset();
    }
}


void SignalProducer::set_bpm(Number const new_bpm) noexcept
{
    constexpr Number threshold = 0.000001;

    if (new_bpm < threshold || std::fabs(bpm - new_bpm) < threshold) {
        return;
    }

    bpm = new_bpm;

    for (Children::iterator it = children.begin(); it != children.end(); ++it) {
        (*it)->set_bpm(new_bpm);
    }
}


Number SignalProducer::get_bpm() const noexcept
{
    return bpm;
}


bool SignalProducer::is_silent(
        Integer const round,
        Integer const sample_count
) noexcept {
    if (cached_buffer == NULL) {
        return true;
    }

    if (round == cached_silence_round) {
        return cached_silence;
    }

    cached_silence_round = round;

    for (Integer c = 0; c != channels; ++c) {
        for (Integer i = 0; i != sample_count; ++i) {
            if (std::fabs(cached_buffer[c][i]) > SILENCE_THRESHOLD) {
                cached_silence = false;

                return false;
            }
        }
    }

    cached_silence = true;

    return true;
}


void SignalProducer::mark_round_as_silent(Integer const round) noexcept
{
    cached_silence_round = round;
    cached_silence = true;
}


Sample const* const* SignalProducer::get_last_rendered_block(
        Integer& sample_count
) const noexcept {
    if (cached_buffer == NULL) {
        sample_count = 0;

        return NULL;
    }

    sample_count = last_sample_count;

    return cached_buffer;
}


Seconds SignalProducer::sample_count_to_time_offset(
        Integer const sample_count
) const noexcept {
    return current_time + sample_count_to_relative_time_offset(sample_count);
}


Seconds SignalProducer::sample_count_to_relative_time_offset(
        Integer const sample_count
) const noexcept {
    return (Seconds)sample_count * sampling_period;
}


void SignalProducer::schedule(
        Event::Type const type,
        Seconds const time_offset,
        Integer const int_param,
        Number const number_param_1,
        Number const number_param_2
) noexcept {
    Seconds const time = time_offset + current_time;

    Event event(type, time, int_param, number_param_1, number_param_2);
    events.push(event);
}


void SignalProducer::cancel_events() noexcept
{
    events.drop(0);
    schedule(EVT_CANCEL, 0.0);
}


void SignalProducer::cancel_events_at(Seconds const time_offset) noexcept
{
    Seconds const time = time_offset + current_time;

    for (Queue<Event>::SizeType i = 0, l = events.length(); i != l; ++i) {
        if (events[i].time_offset >= time) {
            events.drop(i);
            break;
        }
    }

    schedule(EVT_CANCEL, time_offset);
}


void SignalProducer::cancel_events_after(Seconds const time_offset) noexcept
{
    Seconds const time = time_offset + current_time;

    for (Queue<Event>::SizeType i = 0, l = events.length(); i != l; ++i) {
        if (events[i].time_offset > time) {
            events.drop(i);
            break;
        }
    }

    schedule(EVT_CANCEL, time_offset);
}


bool SignalProducer::has_events_after(Seconds const time_offset) const noexcept
{
    return !events.is_empty() && events.back().time_offset > time_offset;
}


Seconds SignalProducer::get_last_event_time_offset() const noexcept
{
    return events.is_empty() ? 0.0 : events.back().time_offset - current_time;
}


Sample const* const* SignalProducer::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    return NULL;
}


void SignalProducer::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
}


void SignalProducer::finalize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
}


void SignalProducer::render_silence(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    for (Integer c = 0; c != channels; ++c) {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[c][i] = 0.0;
        }
    }
}


void SignalProducer::handle_event(Event const& event) noexcept
{
}


bool SignalProducer::has_upcoming_events(
        Integer const sample_count
) const noexcept {
    return (
        !events.is_empty()
        && is_time_offset_before_sample_count(
            events.front().time_offset, sample_count
        )
    );
}


bool SignalProducer::is_time_offset_before_sample_count(
            Seconds const time_offset,
            Integer const sample_count
) const noexcept {
    return time_offset <= sample_count_to_time_offset(sample_count);
}


Integer SignalProducer::sample_count_or_block_size(
        Integer const sample_count
) const noexcept {
    return sample_count == -1 ? (Integer)get_block_size() : sample_count;
}


void SignalProducer::register_child(SignalProducer& signal_producer) noexcept
{
    children.push_back(&signal_producer);
}


SignalProducer::Event::Event(Type const type) noexcept
    : time_offset(0.0),
    int_param(0),
    number_param_1(0.0),
    number_param_2(0.0),
    type(type)
{
}


SignalProducer::Event::Event(
        Type const type,
        Seconds const time_offset,
        Integer const int_param,
        Number const number_param_1,
        Number const number_param_2
) noexcept
    : time_offset(time_offset),
    int_param(int_param),
    number_param_1(number_param_1),
    number_param_2(number_param_2),
    type(type)
{
}


template<class SignalProducerClass>
void SignalProducer::handle_events(
        SignalProducerClass& signal_producer,
        Integer const current_sample_index,
        Integer const sample_count,
        Integer& next_stop
) noexcept {
    Seconds const handle_until = signal_producer.current_time;

    while (!signal_producer.events.is_empty()) {
        Event const& next_event = signal_producer.events.front();

        if (next_event.time_offset > handle_until) {
            next_stop = current_sample_index + (Integer)std::ceil(
                (next_event.time_offset - signal_producer.current_time)
                * signal_producer.sample_rate
            );

            if (next_stop > sample_count) {
                next_stop = sample_count;
            }

            return;
        }

        signal_producer.handle_event(next_event);
        signal_producer.events.pop();
    }

    next_stop = sample_count;
}

}

#endif
