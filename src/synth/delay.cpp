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
Delay<InputSignalProducerClass>::Delay(
        InputSignalProducerClass& input,
        ToggleParam const* tempo_sync
) noexcept
    : Filter<InputSignalProducerClass>(input, 2),
    tempo_sync(tempo_sync),
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
    ),
    delay_buffer_oversize(
        tempo_sync != NULL ? OVERSIZE_DELAY_BUFFER_FOR_TEMPO_SYNC : 1
    )
{
    initialize_instance();
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::initialize_instance() noexcept
{
    feedback_signal_producer = NULL;
    delay_buffer = NULL;
    gain_buffer = NULL;
    time_buffer = NULL;
    delay_buffer_size = 0;

    reallocate_delay_buffer_if_needed();
    reset();

    this->register_child(gain);
    this->register_child(time);
}


template<class InputSignalProducerClass>
Delay<InputSignalProducerClass>::Delay(
        InputSignalProducerClass& input,
        FloatParam& gain_leader,
        FloatParam& time_leader,
        ToggleParam const* tempo_sync
) noexcept
    : Filter<InputSignalProducerClass>(input, 2),
    tempo_sync(tempo_sync),
    gain(gain_leader),
    time(time_leader),
    delay_buffer_oversize(
        tempo_sync != NULL ? OVERSIZE_DELAY_BUFFER_FOR_TEMPO_SYNC : 1
    )
{
    initialize_instance();
}


template<class InputSignalProducerClass>
Delay<InputSignalProducerClass>::Delay(
        InputSignalProducerClass& input,
        FloatParam& gain_leader,
        Seconds const time,
        ToggleParam const* tempo_sync
) noexcept
    : Filter<InputSignalProducerClass>(input, 2),
    tempo_sync(tempo_sync),
    gain(gain_leader),
    time("", Constants::DELAY_TIME_MIN, Constants::DELAY_TIME_MAX, time),
    delay_buffer_oversize(
        tempo_sync != NULL ? OVERSIZE_DELAY_BUFFER_FOR_TEMPO_SYNC : 1
    )
{
    initialize_instance();
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::reallocate_delay_buffer_if_needed() noexcept
{
    Integer const new_delay_buffer_size = this->block_size * 2 + std::max(
        (Integer)(this->sample_rate * Constants::DELAY_TIME_MAX) + 1,
        this->block_size
    ) * delay_buffer_oversize;

    if (new_delay_buffer_size != delay_buffer_size) {
        free_delay_buffer();
        delay_buffer_size = new_delay_buffer_size;
        clear_index = this->block_size;
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

    delay_buffer = NULL;
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::allocate_delay_buffer() noexcept
{
    if (this->channels <= 0) {
        reset();
        return;
    }

    delay_buffer = new Sample*[this->channels];

    for (Integer c = 0; c != this->channels; ++c) {
        delay_buffer[c] = new Sample[delay_buffer_size];
    }

    reset();
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::reset() noexcept
{
    Filter<InputSignalProducerClass>::reset();

    for (Integer c = 0; c != this->channels; ++c) {
        std::fill_n(delay_buffer[c], delay_buffer_size, 0.0);
    }

    write_index_input = 0;
    write_index_feedback = 0;
    clear_index = this->block_size;
    is_starting = true;
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
    if (new_block_size == this->get_block_size()) {
        return;
    }

    SignalProducer::set_block_size(new_block_size);

    reallocate_delay_buffer_if_needed();
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::set_sample_rate(
        Frequency const new_sample_rate
) noexcept {
    SignalProducer::set_sample_rate(new_sample_rate);

    reallocate_delay_buffer_if_needed();
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

    read_index = write_index_input;

    clear_delay_buffer(sample_count);
    mix_feedback_into_delay_buffer(sample_count);
    mix_input_into_delay_buffer(sample_count);

    gain_buffer = FloatParam::produce_if_not_constant(
        &gain, round, sample_count
    );
    time_buffer = FloatParam::produce_if_not_constant(
        &time, round, sample_count
    );

    time_scale = (
        tempo_sync != NULL && tempo_sync->get_value() == ToggleParam::ON
            ? (ONE_MINUTE / std::max(BPM_MIN, this->bpm)) * this->sample_rate
            : this->sample_rate
    );

    return NULL;
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::clear_delay_buffer(
        Integer const sample_count
) noexcept {
    Integer const channels = this->channels;
    Integer delay_buffer_index = clear_index;

    for (Integer c = 0; c != channels; ++c) {
        delay_buffer_index = clear_index;

        for (Integer i = 0; i != sample_count; ++i) {
            delay_buffer[c][delay_buffer_index] = 0.0;

            ++delay_buffer_index;

            if (delay_buffer_index == delay_buffer_size) {
                delay_buffer_index = 0;
            }
        }
    }

    clear_index = delay_buffer_index;
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::mix_feedback_into_delay_buffer(
        Integer const sample_count
) noexcept {
    if (UNLIKELY(feedback_signal_producer == NULL)) {
        return;
    }

    if (UNLIKELY(is_starting)) {
        is_starting = false;
        write_index_feedback += sample_count;

        if (write_index_feedback >= delay_buffer_size) {
            write_index_feedback %= delay_buffer_size;
        }

        return;
    }

    Integer feedback_sample_count = 0;

    Sample const* const* feedback_signal_producer_buffer = (
        feedback_signal_producer->get_last_rendered_block(
            feedback_sample_count
        )
    );

    Integer const channels = this->channels;
    Integer delay_buffer_index = write_index_feedback;

    for (Integer c = 0; c != channels; ++c) {
        Sample const* feedback_samples = feedback_signal_producer_buffer[c];
        delay_buffer_index = write_index_feedback;

        for (Integer i = 0; i != feedback_sample_count; ++i) {
            delay_buffer[c][delay_buffer_index] += feedback_samples[i];

            ++delay_buffer_index;

            if (delay_buffer_index == delay_buffer_size) {
                delay_buffer_index = 0;
            }
        }
    }

    write_index_feedback = delay_buffer_index;
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::mix_input_into_delay_buffer(
        Integer const sample_count
) noexcept {
    Integer const channels = this->channels;
    Integer delay_buffer_index = write_index_input;

    for (Integer c = 0; c != channels; ++c) {
        Sample const* input = this->input_buffer[c];
        delay_buffer_index = write_index_input;

        for (Integer i = 0; i != sample_count; ++i) {
            delay_buffer[c][delay_buffer_index] += input[i];

            ++delay_buffer_index;

            if (delay_buffer_index == delay_buffer_size) {
                delay_buffer_index = 0;
            }
        }
    }

    write_index_input = delay_buffer_index;
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Integer const channels = this->channels;
    Number const read_index_float = (Number)this->read_index;

    if (time_buffer == NULL) {
        Number const time_value = time.get_value() * time_scale;

        for (Integer c = 0; c != channels; ++c) {
            Sample const* const delay_channel = delay_buffer[c];
            Number read_index = read_index_float - time_value;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] = Math::lookup_periodic(
                    delay_channel, delay_buffer_size, read_index
                );

                read_index += 1.0;
            }
        }
    } else {
        for (Integer c = 0; c != channels; ++c) {
            Sample const* const delay_channel = delay_buffer[c];
            Number read_index = read_index_float;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[c][i] = Math::lookup_periodic(
                    delay_channel,
                    delay_buffer_size,
                    read_index - time_buffer[i] * time_scale
                );

                read_index += 1.0;
            }
        }
    }

    apply_gain(round, first_sample_index, last_sample_index, buffer);
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
