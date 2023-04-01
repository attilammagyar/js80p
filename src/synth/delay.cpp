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

#ifndef JS80P__SYNTH__DELAY_CPP
#define JS80P__SYNTH__DELAY_CPP

#include <algorithm>
#include <cmath>

#include "synth/delay.hpp"
#include "synth/math.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
Delay<InputSignalProducerClass>::Delay(InputSignalProducerClass& input) noexcept
    : Filter<InputSignalProducerClass>(input, 2),
    gain(
        "",
        Constants::DELAY_GAIN_MIN,
        Constants::DELAY_GAIN_MAX,
        Constants::DELAY_GAIN_DEFAULT
    ),
    time(
        "",
        Constants::DELAY_TIME_MIN,
        Constants::DELAY_TIME_MAX,
        Constants::DELAY_TIME_DEFAULT
    )
{
    initialize_instance();
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::initialize_instance() noexcept
{
    feedback_signal_producer = NULL;
    feedback_signal_producer_buffer = NULL;
    delay_buffer = NULL;
    gain_buffer = NULL;
    time_buffer = NULL;
    feedback_sample_count = 0;
    write_index = 0;
    delay_buffer_size = 0;
    read_index = 0.0;

    reallocate_delay_buffer();

    this->register_child(gain);
    this->register_child(time);
}


template<class InputSignalProducerClass>
Delay<InputSignalProducerClass>::Delay(
        InputSignalProducerClass& input,
        FloatParam& gain_leader,
        FloatParam& time_leader
) noexcept
    : Filter<InputSignalProducerClass>(input, 2),
    gain(gain_leader),
    time(time_leader)
{
    initialize_instance();
}


template<class InputSignalProducerClass>
Delay<InputSignalProducerClass>::Delay(
        InputSignalProducerClass& input,
        FloatParam& gain_leader,
        Seconds const time
) noexcept
    : Filter<InputSignalProducerClass>(input, 2),
    gain(gain_leader),
    time("", Constants::DELAY_TIME_MIN, Constants::DELAY_TIME_MAX, time)
{
    initialize_instance();
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::reallocate_delay_buffer() noexcept
{
    Integer const new_delay_buffer_size = std::max(
        (Integer)(this->sample_rate * Constants::DELAY_TIME_MAX) + 1,
        this->block_size
    );

    if (new_delay_buffer_size != delay_buffer_size) {
        free_delay_buffer();
        delay_buffer_size = new_delay_buffer_size;
        allocate_delay_buffer();
    }
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::free_delay_buffer() noexcept
{
    if (delay_buffer == NULL) {
        return;
    }

    for (Integer i = 0; i != this->channels; ++i) {
        delete[] delay_buffer[i];

        delay_buffer[i] = NULL;
    }

    delete[] delay_buffer;
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::allocate_delay_buffer() noexcept
{
    delay_buffer_size_float = (Number)delay_buffer_size;
    delay_buffer_size_inv = 1.0 / delay_buffer_size_float;

    if (this->channels <= 0) {
        clear();
        return;
    }

    delay_buffer = new Sample*[this->channels];

    for (Integer c = 0; c != this->channels; ++c) {
        delay_buffer[c] = new Sample[delay_buffer_size];
    }

    clear();
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::clear() noexcept
{
    for (Integer c = 0; c != this->channels; ++c) {
        std::fill_n(delay_buffer[c], delay_buffer_size, 0.0);
    }

    write_index = 0;
    read_index = 0.0;
}


template<class InputSignalProducerClass>
Delay<InputSignalProducerClass>::~Delay()
{
    free_delay_buffer();
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::set_block_size(
        Integer const new_block_size
) noexcept {
    SignalProducer::set_block_size(new_block_size);

    reallocate_delay_buffer();
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::set_sample_rate(
        Frequency const new_sample_rate
) noexcept {
    SignalProducer::set_sample_rate(new_sample_rate);

    reallocate_delay_buffer();
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::set_feedback_signal_producer(
        SignalProducer const* feedback_signal_producer
) noexcept {
    this->feedback_signal_producer = feedback_signal_producer;
}


template<class InputSignalProducerClass>
Sample const* const* Delay<InputSignalProducerClass>::initialize_rendering(
    Integer const round,
    Integer const sample_count
) noexcept {
    Filter<InputSignalProducerClass>::initialize_rendering(round, sample_count);

    initialize_feedback(round, sample_count);

    read_index = (Number)write_index;
    merge_inputs_into_delay_buffer(sample_count);

    gain_buffer = FloatParam::produce_if_not_constant(
        &gain, round, sample_count
    );
    time_buffer = FloatParam::produce_if_not_constant(
        &time, round, sample_count
    );

    return NULL;
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::initialize_feedback(
        Integer const round,
        Integer const sample_count
) noexcept {
    if (UNLIKELY(feedback_signal_producer == NULL)) {
        feedback_signal_producer_buffer = NULL;
        feedback_sample_count = 0;

        return;
    }

    feedback_signal_producer_buffer = (
        feedback_signal_producer->get_last_rendered_block(
            feedback_sample_count
        )
    );
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::merge_inputs_into_delay_buffer(
        Integer const sample_count
) noexcept {
    copy_input_into_delay_buffer(sample_count);
    mix_feedback_into_delay_buffer(sample_count);

    write_index += sample_count;

    if (write_index >= delay_buffer_size) {
        write_index %= delay_buffer_size;
    }
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::copy_input_into_delay_buffer(
        Integer const sample_count
) noexcept {
    Integer const channels = this->channels;

    for (Integer c = 0; c != channels; ++c) {
        Sample const* input = this->input_buffer[c];
        Integer delay_buffer_index = write_index;

        for (Integer i = 0; i != sample_count; ++i) {
            delay_buffer[c][delay_buffer_index] = input[i];

            ++delay_buffer_index;

            if (delay_buffer_index == delay_buffer_size) {
                delay_buffer_index = 0;
            }
        }
    }
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::mix_feedback_into_delay_buffer(
        Integer const sample_count
) noexcept {
    Integer const channels = this->channels;

    if (UNLIKELY(feedback_sample_count == 0)) {
        return;
    }

    Integer const feedback_sample_count = std::min(
        sample_count, this->feedback_sample_count
    );

    for (Integer c = 0; c != channels; ++c) {
        Sample const* feedback_samples = feedback_signal_producer_buffer[c];
        Integer delay_buffer_index = write_index;

        for (Integer i = 0; i != feedback_sample_count; ++i) {
            delay_buffer[c][delay_buffer_index] += feedback_samples[i];

            ++delay_buffer_index;

            if (delay_buffer_index == delay_buffer_size) {
                delay_buffer_index = 0;
            }
        }
    }
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Integer const channels = this->channels;

    if (time_buffer == NULL) {
        Number const time_value = time.get_value();

        for (Integer c = 0; c != channels; ++c) {
            Sample const* const delay_channel = delay_buffer[c];
            Number read_index = this->read_index - time_value * this->sample_rate;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] = Math::lookup_periodic(
                    delay_channel, delay_buffer_size, read_index
                );

                read_index += 1.0;
            }
        }
    } else {
        Frequency const sample_rate = this->sample_rate;

        for (Integer c = 0; c != channels; ++c) {
            Sample const* const delay_channel = delay_buffer[c];
            Number read_index = this->read_index;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] = Math::lookup_periodic(
                    delay_channel,
                    delay_buffer_size,
                    read_index - time_buffer[i] * sample_rate
                );

                read_index += 1.0;
            }
        }
    }

    apply_gain(round, first_sample_index, last_sample_index, buffer);

    read_index += (Number)(last_sample_index - first_sample_index);
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::apply_gain(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Integer const channels = this->channels;

    if (gain_buffer == NULL) {
        Sample const gain = this->gain.get_value();

        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] *= gain;
            }
        }
    } else {
        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] *= gain_buffer[i];
            }
        }
    }
}

}

#endif
