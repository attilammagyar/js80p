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

#ifndef JS80P__DSP__DELAY_CPP
#define JS80P__DSP__DELAY_CPP

#include <algorithm>
#include <cmath>

#include "dsp/delay.hpp"
#include "dsp/math.hpp"


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
    ),
    is_gain_constant_1(false)
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
        FloatParamS& time_leader,
        ToggleParam const* tempo_sync
) noexcept
    : Filter<InputSignalProducerClass>(input, 2),
    tempo_sync(tempo_sync),
    gain("", 0.0, 1.0, 1.0),
    time(time_leader),
    delay_buffer_oversize(
        tempo_sync != NULL ? OVERSIZE_DELAY_BUFFER_FOR_TEMPO_SYNC : 1
    ),
    is_gain_constant_1(true)
{
    initialize_instance();
}


template<class InputSignalProducerClass>
Delay<InputSignalProducerClass>::Delay(
        InputSignalProducerClass& input,
        FloatParamS& gain_leader,
        FloatParamS& time_leader,
        ToggleParam const* tempo_sync
) noexcept
    : Filter<InputSignalProducerClass>(input, 2),
    tempo_sync(tempo_sync),
    gain(gain_leader),
    time(time_leader),
    delay_buffer_oversize(
        tempo_sync != NULL ? OVERSIZE_DELAY_BUFFER_FOR_TEMPO_SYNC : 1
    ),
    is_gain_constant_1(false)
{
    initialize_instance();
}


template<class InputSignalProducerClass>
Delay<InputSignalProducerClass>::Delay(
        InputSignalProducerClass& input,
        FloatParamS& gain_leader,
        Seconds const time,
        ToggleParam const* tempo_sync
) noexcept
    : Filter<InputSignalProducerClass>(input, 2),
    tempo_sync(tempo_sync),
    gain(gain_leader),
    time("", Constants::DELAY_TIME_MIN, Constants::DELAY_TIME_MAX, time),
    delay_buffer_oversize(
        tempo_sync != NULL ? OVERSIZE_DELAY_BUFFER_FOR_TEMPO_SYNC : 1
    ),
    is_gain_constant_1(false)
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
    mix_input_into_delay_buffer(round, sample_count);

    if (is_gain_constant_1) {
        gain_buffer = NULL;
        need_gain = false;
    } else {
        gain_buffer = FloatParamS::produce_if_not_constant(gain, round, sample_count);
        need_gain = gain_buffer != NULL || std::fabs(1.0 - gain.get_value()) > 0.000001;
    }

    time_buffer = FloatParamS::produce_if_not_constant(time, round, sample_count);

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

            if (UNLIKELY(delay_buffer_index == delay_buffer_size)) {
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

        if (UNLIKELY(write_index_feedback >= delay_buffer_size)) {
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

            if (UNLIKELY(delay_buffer_index == delay_buffer_size)) {
                delay_buffer_index = 0;
            }
        }
    }

    write_index_feedback = delay_buffer_index;
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::mix_input_into_delay_buffer(
        Integer const round,
        Integer const sample_count
) noexcept {
    if (this->input.is_silent(round, sample_count)) {
        write_index_input = (write_index_input + sample_count) % delay_buffer_size;

        return;
    }

    Integer const channels = this->channels;
    Integer delay_buffer_index = write_index_input;

    for (Integer c = 0; c != channels; ++c) {
        Sample const* input = this->input_buffer[c];
        delay_buffer_index = write_index_input;

        for (Integer i = 0; i != sample_count; ++i) {
            delay_buffer[c][delay_buffer_index] += input[i];

            ++delay_buffer_index;

            if (UNLIKELY(delay_buffer_index == delay_buffer_size)) {
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

    if (need_gain) {
        apply_gain(round, first_sample_index, last_sample_index, buffer);
    }
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


template<class InputSignalProducerClass, class FilterInputClass>
PannedDelay<InputSignalProducerClass, FilterInputClass>::PannedDelay(
        InputSignalProducerClass& input,
        PannedDelayStereoMode const stereo_mode,
        ToggleParam const* tempo_sync
) : PannedDelay<InputSignalProducerClass, FilterInputClass>(
        input, delay, stereo_mode, tempo_sync
    )
{
}


template<class InputSignalProducerClass, class FilterInputClass>
PannedDelay<InputSignalProducerClass, FilterInputClass>::PannedDelay(
        InputSignalProducerClass& input,
        PannedDelayStereoMode const stereo_mode,
        FloatParamS& delay_time_leader,
        ToggleParam const* tempo_sync
) :  Filter<FilterInputClass>(delay, NUMBER_OF_CHILDREN, input.get_channels()),
    is_flipped(stereo_mode == PannedDelayStereoMode::FLIPPED),
    panning("", -1.0, 1.0, 0.0),
    delay(input, delay_time_leader, tempo_sync)
{
    initialize_instance();
}


template<class InputSignalProducerClass, class FilterInputClass>
PannedDelay<InputSignalProducerClass, FilterInputClass>::PannedDelay(
        InputSignalProducerClass& input,
        PannedDelayStereoMode const stereo_mode,
        FloatParamS& panning_leader,
        FloatParamS& delay_time_leader,
        ToggleParam const* tempo_sync,
        Integer const number_of_children
) : PannedDelay<InputSignalProducerClass, FilterInputClass>(
        input,
        delay,
        stereo_mode,
        panning_leader,
        delay_time_leader,
        tempo_sync,
        number_of_children
    )
{
}


template<class InputSignalProducerClass, class FilterInputClass>
PannedDelay<InputSignalProducerClass, FilterInputClass>::PannedDelay(
        InputSignalProducerClass& delay_input,
        FilterInputClass& filter_input,
        PannedDelayStereoMode const stereo_mode,
        ToggleParam const* tempo_sync,
        Integer const number_of_children
) : Filter<FilterInputClass>(
        filter_input,
        number_of_children + NUMBER_OF_CHILDREN,
        delay_input.get_channels()
    ),
    is_flipped(stereo_mode == PannedDelayStereoMode::FLIPPED),
    panning("", -1.0, 1.0, 0.0),
    delay(delay_input, tempo_sync)
{
    initialize_instance();
}


template<class InputSignalProducerClass, class FilterInputClass>
PannedDelay<InputSignalProducerClass, FilterInputClass>::PannedDelay(
        InputSignalProducerClass& delay_input,
        FilterInputClass& filter_input,
        PannedDelayStereoMode const stereo_mode,
        FloatParamS& panning_leader,
        FloatParamS& delay_time_leader,
        ToggleParam const* tempo_sync,
        Integer const number_of_children
) : Filter<FilterInputClass>(
        filter_input,
        number_of_children + NUMBER_OF_CHILDREN,
        delay_input.get_channels()
    ),
    is_flipped(stereo_mode == PannedDelayStereoMode::FLIPPED),
    panning(panning_leader),
    delay(delay_input, delay_time_leader, tempo_sync)
{
    initialize_instance();
}


template<class InputSignalProducerClass, class FilterInputClass>
PannedDelay<InputSignalProducerClass, FilterInputClass>::PannedDelay(
        InputSignalProducerClass& delay_input,
        FilterInputClass& filter_input,
        PannedDelayStereoMode const stereo_mode,
        FloatParamS& panning_leader,
        FloatParamS& delay_gain_leader,
        FloatParamS& delay_time_leader,
        ToggleParam const* tempo_sync,
        Integer const number_of_children
) : Filter<FilterInputClass>(
        filter_input,
        number_of_children + NUMBER_OF_CHILDREN,
        delay_input.get_channels()
    ),
    is_flipped(stereo_mode == PannedDelayStereoMode::FLIPPED),
    panning(panning_leader),
    delay(delay_input, delay_gain_leader, delay_time_leader, tempo_sync)
{
    initialize_instance();
}


template<class InputSignalProducerClass, class FilterInputClass>
PannedDelay<InputSignalProducerClass, FilterInputClass>::PannedDelay(
        InputSignalProducerClass& delay_input,
        FilterInputClass& filter_input,
        PannedDelayStereoMode const stereo_mode,
        FloatParamS& panning_leader,
        FloatParamS& delay_gain_leader,
        Seconds const delay_time,
        ToggleParam const* tempo_sync,
        Integer const number_of_children
) : Filter<FilterInputClass>(
        filter_input,
        number_of_children + NUMBER_OF_CHILDREN,
        delay_input.get_channels()
    ),
    is_flipped(stereo_mode == PannedDelayStereoMode::FLIPPED),
    panning(panning_leader),
    delay(delay_input, delay_gain_leader, delay_time, tempo_sync)
{
    initialize_instance();
}


template<class InputSignalProducerClass, class FilterInputClass>
void PannedDelay<InputSignalProducerClass, FilterInputClass>::initialize_instance() noexcept
{
    panning_buffer = NULL;
    stereo_gain_buffer = SignalProducer::allocate_buffer();

    this->register_child(panning);
    this->register_child(delay);
}


template<class InputSignalProducerClass, class FilterInputClass>
PannedDelay<InputSignalProducerClass, FilterInputClass>::~PannedDelay()
{
    SignalProducer::free_buffer(stereo_gain_buffer);
}


template<class InputSignalProducerClass, class FilterInputClass>
void PannedDelay<InputSignalProducerClass, FilterInputClass>::set_block_size(
        Integer const new_block_size
) noexcept {
    SignalProducer::set_block_size(new_block_size);

    stereo_gain_buffer = SignalProducer::reallocate_buffer(stereo_gain_buffer);
}


template<class InputSignalProducerClass, class FilterInputClass>
Sample const* const* PannedDelay<InputSignalProducerClass, FilterInputClass>::initialize_rendering(
    Integer const round,
    Integer const sample_count
) noexcept {
    Filter<FilterInputClass>::initialize_rendering(round, sample_count);

    /* https://www.w3.org/TR/webaudio/#stereopanner-algorithm */

    panning_buffer = FloatParamS::produce_if_not_constant(panning, round, sample_count);

    if (panning_buffer == NULL) {
        panning_value = is_flipped ? -panning.get_value() : panning.get_value();
        Number const x = (
            (panning_value <= 0.0 ? panning_value + 1.0 : panning_value) * Math::PI_HALF
        );

        stereo_gain_value[0] = Math::cos(x);
        stereo_gain_value[1] = Math::sin(x);
    } else {
        if (is_flipped) {
            for (Integer i = 0; i != sample_count; ++i) {
                Number const p = -panning_buffer[i];
                Number const x = (p <= 0.0 ? p + 1.0 : p) * Math::PI_HALF;
                stereo_gain_buffer[0][i] = Math::cos(x);
                stereo_gain_buffer[1][i] = Math::sin(x);
            }
        } else {
            for (Integer i = 0; i != sample_count; ++i) {
                Number const p = panning_buffer[i];
                Number const x = (p <= 0.0 ? p + 1.0 : p) * Math::PI_HALF;
                stereo_gain_buffer[0][i] = Math::cos(x);
                stereo_gain_buffer[1][i] = Math::sin(x);
            }
        }
    }

    return NULL;
}


template<class InputSignalProducerClass, class FilterInputClass>
void PannedDelay<InputSignalProducerClass, FilterInputClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Sample const* const* const input_buffer = this->input_buffer;

    if (panning_buffer == NULL) {
        if (panning_value <= 0) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = (
                    input_buffer[0][i] + input_buffer[1][i] * stereo_gain_value[0]
                );
                buffer[1][i] = input_buffer[1][i] * stereo_gain_value[1];
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = input_buffer[0][i] * stereo_gain_value[0];
                buffer[1][i] = (
                    input_buffer[1][i] + input_buffer[0][i] * stereo_gain_value[1]
                );
            }
        }
    } else {
        if (is_flipped) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                if (panning_buffer[i] > 0) {
                    buffer[0][i] = (
                        input_buffer[0][i]
                        + input_buffer[1][i] * stereo_gain_buffer[0][i]
                    );
                    buffer[1][i] = input_buffer[1][i] * stereo_gain_buffer[1][i];
                } else {
                    buffer[0][i] = input_buffer[0][i] * stereo_gain_buffer[0][i];
                    buffer[1][i] = (
                        input_buffer[1][i]
                        + input_buffer[0][i] * stereo_gain_buffer[1][i]
                    );
                }
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                if (panning_buffer[i] <= 0) {
                    buffer[0][i] = (
                        input_buffer[0][i]
                        + input_buffer[1][i] * stereo_gain_buffer[0][i]
                    );
                    buffer[1][i] = input_buffer[1][i] * stereo_gain_buffer[1][i];
                } else {
                    buffer[0][i] = input_buffer[0][i] * stereo_gain_buffer[0][i];
                    buffer[1][i] = (
                        input_buffer[1][i]
                        + input_buffer[0][i] * stereo_gain_buffer[1][i]
                    );
                }
            }
        }
    }
}


template<class InputSignalProducerClass>
HighShelfPannedDelay<InputSignalProducerClass>::HighShelfPannedDelay(
    InputSignalProducerClass& input,
    PannedDelayStereoMode const stereo_mode,
    ToggleParam const* tempo_sync
) : HighShelfPannedDelayBase<InputSignalProducerClass>(
        input, high_shelf_filter, stereo_mode, tempo_sync, NUMBER_OF_CHILDREN
    ),
    high_shelf_filter_type(""),
    high_shelf_filter_q(
        "",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    high_shelf_filter("", this->delay, high_shelf_filter_type)
{
    initialize_instance();

    high_shelf_filter.frequency.set_value(Constants::BIQUAD_FILTER_FREQUENCY_MAX);
    high_shelf_filter.q.set_value(Constants::BIQUAD_FILTER_Q_DEFAULT);
    high_shelf_filter.gain.set_value(0.0);
}


template<class InputSignalProducerClass>
HighShelfPannedDelay<InputSignalProducerClass>::HighShelfPannedDelay(
    InputSignalProducerClass& input,
    PannedDelayStereoMode const stereo_mode,
    FloatParamS& panning_leader,
    FloatParamS& delay_gain_leader,
    FloatParamS& delay_time_leader,
    FloatParamS& high_shelf_filter_frequency_leader,
    FloatParamS& high_shelf_filter_gain_leader,
    ToggleParam const* tempo_sync
) : HighShelfPannedDelayBase<InputSignalProducerClass>(
        input,
        high_shelf_filter,
        stereo_mode,
        panning_leader,
        delay_gain_leader,
        delay_time_leader,
        tempo_sync,
        NUMBER_OF_CHILDREN
    ),
    high_shelf_filter_type(""),
    high_shelf_filter_q(
        "",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    high_shelf_filter(
        this->delay,
        high_shelf_filter_type,
        high_shelf_filter_frequency_leader,
        high_shelf_filter_q,
        high_shelf_filter_gain_leader
    )
{
    initialize_instance();
}


template<class InputSignalProducerClass>
HighShelfPannedDelay<InputSignalProducerClass>::HighShelfPannedDelay(
    InputSignalProducerClass& input,
    PannedDelayStereoMode const stereo_mode,
    FloatParamS& panning_leader,
    FloatParamS& delay_gain_leader,
    Seconds const delay_time,
    FloatParamS& high_shelf_filter_frequency_leader,
    FloatParamS& high_shelf_filter_gain_leader,
    ToggleParam const* tempo_sync
) : HighShelfPannedDelayBase<InputSignalProducerClass>(
        input,
        high_shelf_filter,
        stereo_mode,
        panning_leader,
        delay_gain_leader,
        delay_time,
        tempo_sync,
        NUMBER_OF_CHILDREN
    ),
    high_shelf_filter_type(""),
    high_shelf_filter_q(
        "",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    high_shelf_filter(
        this->delay,
        high_shelf_filter_type,
        high_shelf_filter_frequency_leader,
        high_shelf_filter_q,
        high_shelf_filter_gain_leader
    )
{
    initialize_instance();
}


template<class InputSignalProducerClass>
void HighShelfPannedDelay<InputSignalProducerClass>::initialize_instance() noexcept
{
    this->register_child(high_shelf_filter_type);
    this->register_child(high_shelf_filter_q);
    this->register_child(high_shelf_filter);

    high_shelf_filter_type.set_value(
        HighShelfDelay<InputSignalProducerClass>::HIGH_SHELF
    );
}

}

#endif
