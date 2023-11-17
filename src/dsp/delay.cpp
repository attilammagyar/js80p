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
    shared_buffer_owner = NULL;

    feedback_signal_producer = NULL;
    delay_buffer = NULL;
    gain_buffer = NULL;
    time_buffer = NULL;
    delay_buffer_size = 0;
    delay_buffer_size_float = 0.0;

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
        Seconds const time_max,
        ToggleParam const* tempo_sync
) noexcept
    : Filter<InputSignalProducerClass>(input, 2),
    tempo_sync(tempo_sync),
    gain(gain_leader),
    time("", Constants::DELAY_TIME_MIN, time_max, time),
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
        (Integer)(this->sample_rate * time.get_max_value()) + 1,
        this->block_size
    ) * delay_buffer_oversize;

    if (new_delay_buffer_size != delay_buffer_size) {
        free_delay_buffer();
        delay_buffer_size = new_delay_buffer_size;
        delay_buffer_size_float = (Number)delay_buffer_size;
        clear_index = this->block_size;
        allocate_delay_buffer();
    }
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::free_delay_buffer() noexcept
{
    if (delay_buffer == NULL || shared_buffer_owner != NULL) {
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
    if (this->channels <= 0 || shared_buffer_owner != NULL) {
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

    if (shared_buffer_owner == NULL) {
        for (Integer c = 0; c != this->channels; ++c) {
            std::fill_n(delay_buffer[c], delay_buffer_size, 0.0);
        }
    }

    write_index_input = 0;
    silent_input_samples = delay_buffer_size;

    write_index_feedback = 0;
    silent_feedback_samples = delay_buffer_size;

    need_to_render_silence = false;

    clear_index = this->block_size;
    is_starting = true;
    previous_round = -1;
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
        SignalProducer* feedback_signal_producer
) noexcept {
    this->feedback_signal_producer = feedback_signal_producer;
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::use_shared_delay_buffer(
    Delay<InputSignalProducerClass> const& shared_buffer_owner
) noexcept {
    free_delay_buffer();

    this->shared_buffer_owner = &shared_buffer_owner;
}


template<class InputSignalProducerClass>
Sample const* const* Delay<InputSignalProducerClass>::initialize_rendering(
    Integer const round,
    Integer const sample_count
) noexcept {
    Filter<InputSignalProducerClass>::initialize_rendering(round, sample_count);

    read_index = write_index_input;

    if (JS80P_UNLIKELY(shared_buffer_owner != NULL)) {
        clear_delay_buffer<true>(sample_count);
        mix_feedback_into_delay_buffer<true>(sample_count);
        mix_input_into_delay_buffer<true>(round, sample_count);
    } else {
        clear_delay_buffer<false>(sample_count);
        mix_feedback_into_delay_buffer<false>(sample_count);
        mix_input_into_delay_buffer<false>(round, sample_count);
    }

    if (is_gain_constant_1) {
        gain_buffer = NULL;
        need_gain = false;
    } else {
        gain_buffer = FloatParamS::produce_if_not_constant(gain, round, sample_count);
        need_gain = gain_buffer != NULL || !Math::is_close(gain.get_value(), 1.0);
    }

    time_buffer = FloatParamS::produce_if_not_constant(time, round, sample_count);

    time_scale = (
        tempo_sync != NULL && tempo_sync->get_value() == ToggleParam::ON
            ? (ONE_MINUTE / std::max(BPM_MIN, this->bpm)) * this->sample_rate
            : this->sample_rate
    );

    previous_round = round;

    if (is_delay_buffer_silent()) {
        if (JS80P_UNLIKELY(need_to_render_silence)) {
            need_to_render_silence = false;
            this->render_silence(round, 0, this->block_size, this->buffer);
        }

        this->mark_round_as_silent(round);

        return this->buffer;
    }

    need_to_render_silence = true;

    return NULL;
}


template<class InputSignalProducerClass>
Integer Delay<InputSignalProducerClass>::advance_delay_buffer_index(
        Integer const position,
        Integer const increment
) const noexcept {
    Integer const new_position = position + increment;

    return (
        JS80P_UNLIKELY(new_position >= delay_buffer_size)
            ? new_position % delay_buffer_size
            : new_position
    );
}


template<class InputSignalProducerClass>
template<bool is_delay_buffer_shared>
void Delay<InputSignalProducerClass>::clear_delay_buffer(
        Integer const sample_count
) noexcept {
    if constexpr (!is_delay_buffer_shared) {
        clear_index = write_delay_buffer<DelayBufferWritingMode::CLEAR>(
            NULL, clear_index, sample_count
        );
    }
}


template<class InputSignalProducerClass>
template<bool is_delay_buffer_shared>
void Delay<InputSignalProducerClass>::mix_feedback_into_delay_buffer(
        Integer const sample_count
) noexcept {
    if (JS80P_UNLIKELY(feedback_signal_producer == NULL)) {
        return;
    }

    if (JS80P_UNLIKELY(is_starting)) {
        is_starting = false;
        write_index_feedback = advance_delay_buffer_index(
            write_index_feedback, sample_count
        );

        if (silent_feedback_samples < delay_buffer_size) {
            silent_feedback_samples += sample_count;
        }

        return;
    }

    Integer feedback_sample_count = 0;

    Sample const* const* feedback_signal_producer_buffer = (
        feedback_signal_producer->get_last_rendered_block(
            feedback_sample_count
        )
    );

    if (feedback_signal_producer->is_silent(previous_round, feedback_sample_count)) {
        write_index_feedback = advance_delay_buffer_index(
            write_index_feedback, feedback_sample_count
        );

        if (silent_feedback_samples < delay_buffer_size) {
            silent_feedback_samples += feedback_sample_count;
        }

        return;
    }

    silent_feedback_samples = 0;

    if constexpr (!is_delay_buffer_shared) {
        write_index_feedback = write_delay_buffer<DelayBufferWritingMode::ADD>(
            feedback_signal_producer_buffer,
            write_index_feedback,
            feedback_sample_count
        );
    }
}


template<class InputSignalProducerClass>
template<typename Delay<InputSignalProducerClass>::DelayBufferWritingMode mode>
Integer Delay<InputSignalProducerClass>::write_delay_buffer(
        Sample const* const* source_buffer,
        Integer const delay_buffer_index,
        Integer const sample_count
) noexcept {
    Integer const channels = this->channels;
    Integer index = delay_buffer_index;

    for (Integer c = 0; c != channels; ++c) {
        Sample const* source_channel;

        if constexpr (mode == DelayBufferWritingMode::ADD) {
            source_channel = source_buffer[c];
        }

        index = delay_buffer_index;

        for (Integer i = 0; i != sample_count;) {
            Integer const batch_size = std::min(sample_count - i, delay_buffer_size - index);
            Integer const batch_end = index + batch_size;

            for (; index != batch_end; ++index) {
                if constexpr (mode == DelayBufferWritingMode::ADD) {
                    delay_buffer[c][index] += source_channel[i];
                    ++i;
                } else {
                    delay_buffer[c][index] = 0.0;
                }
            }

            if (JS80P_UNLIKELY(index == delay_buffer_size)) {
                index = 0;
            }

            if constexpr (mode != DelayBufferWritingMode::ADD) {
                i += batch_size;
            }
        }
    }

    return index;
}


template<class InputSignalProducerClass>
template<bool is_delay_buffer_shared>
void Delay<InputSignalProducerClass>::mix_input_into_delay_buffer(
        Integer const round,
        Integer const sample_count
) noexcept {
    if (this->input.is_silent(round, sample_count)) {
        write_index_input = advance_delay_buffer_index(
            write_index_input, sample_count
        );

        if (silent_input_samples < delay_buffer_size) {
            silent_input_samples += sample_count;
        }

        return;
    }

    silent_input_samples = 0;

    if constexpr (is_delay_buffer_shared) {
        write_index_input = advance_delay_buffer_index(
            write_index_input, sample_count
        );
    } else {
        write_index_input = write_delay_buffer<DelayBufferWritingMode::ADD>(
            this->input_buffer, write_index_input, sample_count
        );
    }
}


template<class InputSignalProducerClass>
bool Delay<InputSignalProducerClass>::is_delay_buffer_silent() const noexcept
{
    return (
        silent_input_samples >= delay_buffer_size
        && silent_feedback_samples >= delay_buffer_size
    );
}


template<class InputSignalProducerClass>
void Delay<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    if (need_gain) {
        if (gain_buffer == NULL) {
            render<true, true>(
                round,
                first_sample_index,
                last_sample_index,
                buffer,
                this->gain.get_value()
            );
        } else {
            render<true, false>(
                round,
                first_sample_index,
                last_sample_index,
                buffer,
                1.0
            );
        }
    } else {
        render<false, true>(
            round,
            first_sample_index,
            last_sample_index,
            buffer,
            1.0
        );
    }
}


template<class InputSignalProducerClass>
template<bool need_gain, bool is_gain_constant>
void Delay<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer,
        Sample const gain
) noexcept {
    Integer const channels = this->channels;
    Number const read_index_float = (Number)this->read_index;
    Sample const* const* delay_buffer = (
        shared_buffer_owner != NULL
            ? shared_buffer_owner->delay_buffer
            : this->delay_buffer
    );

    if (time_buffer == NULL) {
        Number const time_value = time.get_value() * time_scale;

        for (Integer c = 0; c != channels; ++c) {
            Sample const* const delay_channel = delay_buffer[c];
            Number read_index = read_index_float - time_value;

            if (read_index < 0.0) {
                read_index += delay_buffer_size_float;
            }

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                if constexpr (need_gain) {
                    if constexpr (is_gain_constant) {
                        buffer[c][i] = gain * Math::lookup_periodic<true>(
                            delay_channel, delay_buffer_size, read_index
                        );
                    } else {
                        buffer[c][i] = gain_buffer[i] * Math::lookup_periodic<true>(
                            delay_channel, delay_buffer_size, read_index
                        );
                    }
                } else {
                    buffer[c][i] = Math::lookup_periodic<true>(
                        delay_channel, delay_buffer_size, read_index
                    );
                }

                read_index += 1.0;
            }
        }
    } else {
        for (Integer c = 0; c != channels; ++c) {
            Sample const* const delay_channel = delay_buffer[c];
            Number read_index = read_index_float;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                if constexpr (need_gain) {
                    if constexpr (is_gain_constant) {
                        buffer[c][i] = gain * Math::lookup_periodic<false>(
                            delay_channel,
                            delay_buffer_size,
                            read_index - time_buffer[i] * time_scale
                        );
                    } else {
                        buffer[c][i] = gain_buffer[i] * Math::lookup_periodic<false>(
                            delay_channel,
                            delay_buffer_size,
                            read_index - time_buffer[i] * time_scale
                        );
                    }
                } else {
                    buffer[c][i] = Math::lookup_periodic<false>(
                        delay_channel,
                        delay_buffer_size,
                        read_index - time_buffer[i] * time_scale
                    );
                }

                read_index += 1.0;
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
    panning_scale(1.0),
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
    panning_scale(1.0),
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
    panning_scale(1.0),
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
    panning_scale(1.0),
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
        Seconds const delay_time_max,
        ToggleParam const* tempo_sync,
        Integer const number_of_children
) : Filter<FilterInputClass>(
        filter_input,
        number_of_children + NUMBER_OF_CHILDREN,
        delay_input.get_channels()
    ),
    is_flipped(stereo_mode == PannedDelayStereoMode::FLIPPED),
    panning_scale(1.0),
    panning(panning_leader),
    delay(delay_input, delay_gain_leader, delay_time, delay_time_max, tempo_sync)
{
    initialize_instance();
}


template<class InputSignalProducerClass, class FilterInputClass>
void PannedDelay<InputSignalProducerClass, FilterInputClass>::initialize_instance() noexcept
{
    panning_buffer = NULL;
    stereo_gain_buffer = SignalProducer::allocate_buffer();
    panning_buffer_scaled = SignalProducer::allocate_buffer();

    this->register_child(panning);
    this->register_child(delay);
}


template<class InputSignalProducerClass, class FilterInputClass>
PannedDelay<InputSignalProducerClass, FilterInputClass>::~PannedDelay()
{
    SignalProducer::free_buffer(stereo_gain_buffer);
    SignalProducer::free_buffer(panning_buffer_scaled);
    panning_buffer = NULL;
}


template<class InputSignalProducerClass, class FilterInputClass>
void PannedDelay<InputSignalProducerClass, FilterInputClass>::set_block_size(
        Integer const new_block_size
) noexcept {
    SignalProducer::set_block_size(new_block_size);

    stereo_gain_buffer = SignalProducer::reallocate_buffer(stereo_gain_buffer);
    panning_buffer_scaled = SignalProducer::reallocate_buffer(panning_buffer_scaled);
    panning_buffer = NULL;
}


template<class InputSignalProducerClass, class FilterInputClass>
void PannedDelay<InputSignalProducerClass, FilterInputClass>::set_panning_scale(
        Number const scale
) noexcept {
    panning_scale = scale;
}


template<class InputSignalProducerClass, class FilterInputClass>
Sample const* const* PannedDelay<InputSignalProducerClass, FilterInputClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Filter<FilterInputClass>::initialize_rendering(round, sample_count);

    panning_buffer = FloatParamS::produce_if_not_constant(panning, round, sample_count);

    if (this->input.is_silent(round, sample_count)) {
        return this->input_was_silent(round);
    }

    if (panning_buffer == NULL) {
        panning_value = (
            is_flipped
                ? -panning.get_value() * panning_scale
                : (panning.get_value() * panning_scale)
        );

        Number const x = (
            (panning_value <= 0.0 ? panning_value + 1.0 : panning_value) * Math::PI_HALF
        );

        Math::sincos(x, stereo_gain_value[1], stereo_gain_value[0]);
    } else {
        if (!Math::is_close(panning_scale, 1.0)) {
            for (Integer i = 0; i != sample_count; ++i) {
                panning_buffer_scaled[0][i] = panning_buffer[i] * panning_scale;
            }

            panning_buffer = panning_buffer_scaled[0];
        }

        /* https://www.w3.org/TR/webaudio/#stereopanner-algorithm */

        if (is_flipped) {
            for (Integer i = 0; i != sample_count; ++i) {
                Number const p = -panning_buffer[i];
                Number const x = (p <= 0.0 ? p + 1.0 : p) * Math::PI_HALF;

                Math::sincos(x, stereo_gain_buffer[1][i], stereo_gain_buffer[0][i]);
            }
        } else {
            for (Integer i = 0; i != sample_count; ++i) {
                Number const p = panning_buffer[i];
                Number const x = (p <= 0.0 ? p + 1.0 : p) * Math::PI_HALF;

                Math::sincos(x, stereo_gain_buffer[1][i], stereo_gain_buffer[0][i]);
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
    if (panning_buffer == NULL) {
        if (panning_value > 0.0) {
            render_with_constant_panning<0, 1>(
                round, first_sample_index, last_sample_index, buffer
            );
        } else {
            render_with_constant_panning<1, 0>(
                round, first_sample_index, last_sample_index, buffer
            );
        }
    } else if (is_flipped) {
        render_with_changing_panning<1, 0>(
            round, first_sample_index, last_sample_index, buffer
        );
    } else {
        render_with_changing_panning<0, 1>(
            round, first_sample_index, last_sample_index, buffer
        );
    }
}


template<class InputSignalProducerClass, class FilterInputClass>
template<int channel_1, int channel_2>
void PannedDelay<InputSignalProducerClass, FilterInputClass>::render_with_constant_panning(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Sample const* const* const input_buffer = this->input_buffer;

    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
        buffer[channel_1][i] = (
            input_buffer[channel_1][i] * stereo_gain_value[channel_1]
        );
    }

    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
        buffer[channel_2][i] = (
            input_buffer[channel_2][i]
            + input_buffer[channel_1][i] * stereo_gain_value[channel_2]
        );
    }
}


template<class InputSignalProducerClass, class FilterInputClass>
template<int channel_1, int channel_2>
void PannedDelay<InputSignalProducerClass, FilterInputClass>::render_with_changing_panning(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Sample const* const* const input_buffer = this->input_buffer;

    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
        if (panning_buffer[i] > 0.0) {
            buffer[channel_1][i] = (
                input_buffer[channel_1][i] * stereo_gain_buffer[channel_1][i]
            );
        } else {
            buffer[channel_1][i] = (
                input_buffer[channel_1][i]
                + input_buffer[channel_2][i] * stereo_gain_buffer[channel_1][i]
            );
        }
    }

    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
        if (panning_buffer[i] > 0.0) {
            buffer[channel_2][i] = (
                input_buffer[channel_2][i]
                + input_buffer[channel_1][i] * stereo_gain_buffer[channel_2][i]
            );
        } else {
            buffer[channel_2][i] = (
                input_buffer[channel_2][i] * stereo_gain_buffer[channel_2][i]
            );
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
    Seconds const delay_time_max,
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
        delay_time_max,
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
