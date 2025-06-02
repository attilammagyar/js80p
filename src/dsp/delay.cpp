/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025  Attila M. Magyar
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


namespace JS80P
{

class ReverseDelayEnvelope
{
    public:
        static constexpr int TABLE_SIZE = 1024;
        static constexpr int TABLE_MAX_INDEX = TABLE_SIZE - 1;
        static constexpr Number TABLE_MAX_INDEX_FLOAT = (Number)TABLE_MAX_INDEX;

        ReverseDelayEnvelope();

#ifdef JS80P_ASSERTIONS
        void begin_test(Number const value) noexcept;
        void end_test() noexcept;
#endif

        Number const* const table;

    private:

        void reset() noexcept;

        Number table_rw[TABLE_SIZE];
};


#ifndef JS80P_ASSERTIONS
ReverseDelayEnvelope const reverse_delay_envelope;
#else
ReverseDelayEnvelope reverse_delay_envelope;
#endif


ReverseDelayEnvelope::ReverseDelayEnvelope()
    : table(table_rw)
{
    reset();
}


void ReverseDelayEnvelope::reset() noexcept
{
    constexpr int attack = (int)((Number)TABLE_SIZE * 0.07) + 1;
    constexpr int release = (int)((Number)TABLE_SIZE * 0.14) + 1;
    constexpr int hold_end = TABLE_MAX_INDEX - release;
    constexpr Number attack_float = (Number)attack;
    constexpr Number release_float = (Number)release;

    int i = 0;

    for (; i != attack; ++i) {
        Number const x = (Number)i / attack_float;

        table_rw[i] = Math::shape_smooth_smooth_steep(x);
    }

    for (; i != hold_end; ++i) {
        table_rw[i] = 1.0;
    }

    for (; i != TABLE_SIZE; ++i) {
        Number const x = (Number)(TABLE_MAX_INDEX - i) / release_float;

        table_rw[i] = Math::shape_smooth_smooth(x);
    }
}


#ifdef JS80P_ASSERTIONS
void ReverseDelayEnvelope::begin_test(Number const value) noexcept
{
    std::fill_n(table_rw, TABLE_SIZE, value);
}


void ReverseDelayEnvelope::end_test() noexcept
{
    reset();
}
#endif


template<class InputSignalProducerClass, DelayCapabilities capabilities>
Delay<InputSignalProducerClass, capabilities>::Delay(
        InputSignalProducerClass& input,
        ToggleParam const* tempo_sync,
        Seconds const time_max
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
        time_max,
        Constants::DELAY_TIME_DEFAULT
    ),
    delay_buffer_oversize(
        tempo_sync != NULL ? OVERSIZE_DELAY_BUFFER_FOR_TEMPO_SYNC : 1
    ),
    is_gain_constant_1(false)
{
    initialize_instance();
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::initialize_instance() noexcept
{
    shared_buffer_owner = NULL;

    feedback_signal_producer = NULL;
    time_scale_param = NULL;
    reverse_toggle_param = NULL;
    delay_buffer = NULL;
    gain_buffer = NULL;
    time_buffer = NULL;
    time_scale_buffer = NULL;
    delay_buffer_size = 0;
    delay_buffer_size_float = 0.0;
    is_reversed = false;

    reallocate_delay_buffer_if_needed();
    Delay<InputSignalProducerClass, capabilities>::reset();

    if constexpr (capabilities == DelayCapabilities::DC_CHANNEL_LFO) {
        channel_lfos = new LFO*[this->channels];
        channel_lfo_scales = new Sample[this->channels];
        channel_lfo_buffers = new Sample const*[this->channels];

        for (Integer i = 0; i != this->channels; ++i) {
            channel_lfos[i] = NULL;
            channel_lfo_buffers[i] = NULL;
            channel_lfo_scales[i] = 0.0;
        }
    } else {
        channel_lfos = NULL;
        channel_lfo_buffers = NULL;
        channel_lfo_scales = NULL;
    }

    this->register_child(gain);
    this->register_child(time);
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
Delay<InputSignalProducerClass, capabilities>::Delay(
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


template<class InputSignalProducerClass, DelayCapabilities capabilities>
Delay<InputSignalProducerClass, capabilities>::Delay(
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


template<class InputSignalProducerClass, DelayCapabilities capabilities>
Delay<InputSignalProducerClass, capabilities>::Delay(
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


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::reallocate_delay_buffer_if_needed() noexcept
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


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::free_delay_buffer() noexcept
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


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::allocate_delay_buffer() noexcept
{
    if (this->channels <= 0 || shared_buffer_owner != NULL) {
        Delay<InputSignalProducerClass, capabilities>::reset();
        return;
    }

    delay_buffer = new Sample*[this->channels];

    for (Integer c = 0; c != this->channels; ++c) {
        delay_buffer[c] = new Sample[delay_buffer_size];
    }

    Delay<InputSignalProducerClass, capabilities>::reset();
    reset();
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::reset() noexcept
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

    reverse_next_start_index = 0.0;
    reverse_read_index = 0.0;
    reverse_done_samples = 0.0;
    reverse_target_delay_time_in_samples = 0.0;
    reverse_target_delay_time_in_samples_inv = 1.0;
}


#ifdef JS80P_ASSERTIONS
template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::begin_reverse_delay_test() noexcept
{
    reverse_delay_envelope.begin_test(TEST_REVERSE_ENVELOPE);
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::end_reverse_delay_test() noexcept
{
    reverse_delay_envelope.end_test();
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
Integer Delay<InputSignalProducerClass, capabilities>::get_input_channels() const noexcept
{
    return this->input.get_channels();
}
#endif


template<class InputSignalProducerClass, DelayCapabilities capabilities>
Delay<InputSignalProducerClass, capabilities>::~Delay()
{
    free_delay_buffer();

    if constexpr (capabilities == DelayCapabilities::DC_CHANNEL_LFO) {
        delete[] channel_lfos;
        delete[] channel_lfo_buffers;
        delete[] channel_lfo_scales;

        channel_lfos = NULL;
        channel_lfo_buffers = NULL;
        channel_lfo_scales = NULL;
    }
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::set_block_size(
        Integer const new_block_size
) noexcept {
    if (new_block_size == this->get_block_size()) {
        return;
    }

    SignalProducer::set_block_size(new_block_size);

    reallocate_delay_buffer_if_needed();
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::set_sample_rate(
        Frequency const new_sample_rate
) noexcept {
    SignalProducer::set_sample_rate(new_sample_rate);

    reallocate_delay_buffer_if_needed();
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::set_feedback_signal_producer(
        SignalProducer& feedback_signal_producer
) noexcept {
    this->feedback_signal_producer = &feedback_signal_producer;
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::set_time_scale_param(
        FloatParamS& time_scale_param
) noexcept {
    this->time_scale_param = &time_scale_param;
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::set_reverse_toggle_param(
        ToggleParam& reverse_toggle_param
) noexcept {
    this->reverse_toggle_param = &reverse_toggle_param;
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::set_channel_lfo(
        Integer const channel,
        LFO& lfo,
        Sample const scale
) noexcept {
    if constexpr (capabilities != DelayCapabilities::DC_CHANNEL_LFO) {
        JS80P_ASSERT_NOT_REACHED();

        return;
    }

    JS80P_ASSERT(0 <= channel && channel < this->channels);
    JS80P_ASSERT(scale <= time.get_max_value());

    channel_lfos[channel] = &lfo;
    channel_lfo_scales[channel] = - std::min((Sample)time.get_max_value(), scale);
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::use_shared_delay_buffer(
    Delay<InputSignalProducerClass, capabilities> const& shared_buffer_owner
) noexcept {
    free_delay_buffer();

    this->shared_buffer_owner = &shared_buffer_owner;
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
Sample const* const* Delay<InputSignalProducerClass, capabilities>::initialize_rendering(
    Integer const round,
    Integer const sample_count
) noexcept {
    Filter<InputSignalProducerClass>::initialize_rendering(round, sample_count);

    read_index = write_index_input;

    if constexpr (capabilities == DelayCapabilities::DC_REVERSIBLE) {
        if (JS80P_LIKELY(reverse_toggle_param != NULL)) {
            is_reversed = reverse_toggle_param->get_value() == ToggleParam::ON;
            reverse_next_start_index = (Number)read_index;

            if (JS80P_UNLIKELY(is_starting) || !is_reversed) {
                reverse_read_index = reverse_next_start_index;
                reverse_done_samples = 0.0;
            }
        } else {
            JS80P_ASSERT_NOT_REACHED();
            is_reversed = false;
            reverse_read_index = reverse_next_start_index;
            reverse_done_samples = 0.0;
        }
    }

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
            ? (
                Math::SECONDS_IN_ONE_MINUTE / std::max(BPM_MIN, this->bpm)
            ) * this->sample_rate
            : this->sample_rate
    );

    if constexpr (capabilities == DelayCapabilities::DC_SCALABLE) {
        if (JS80P_LIKELY(time_scale_param != NULL)) {
            time_scale_buffer = FloatParamS::produce_if_not_constant(
                *time_scale_param, round, sample_count
            );

            if (time_scale_buffer == NULL) {
                time_scale *= time_scale_param->get_value();
            }
        }
    }

    previous_round = round;

    if (is_delay_buffer_silent()) {
        if (JS80P_UNLIKELY(need_to_render_silence)) {
            need_to_render_silence = false;

            if constexpr (capabilities == DelayCapabilities::DC_REVERSIBLE) {
                reverse_done_samples = 0.0;
                reverse_read_index = reverse_next_start_index;
            }

            this->render_silence(round, 0, this->block_size, this->buffer);
        }

        this->mark_round_as_silent(round);

        return this->buffer;
    }

    if constexpr (capabilities == DelayCapabilities::DC_CHANNEL_LFO) {
        for (Integer c = 0; c != this->channels; ++c) {
            channel_lfo_buffers[c] = SignalProducer::produce<LFO>(
                *channel_lfos[c], round, sample_count
            )[0];
        }
    }

    need_to_render_silence = true;

    return NULL;
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
Integer Delay<InputSignalProducerClass, capabilities>::advance_delay_buffer_index(
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


template<class InputSignalProducerClass, DelayCapabilities capabilities>
template<bool is_delay_buffer_shared>
void Delay<InputSignalProducerClass, capabilities>::clear_delay_buffer(
        Integer const sample_count
) noexcept {
    if constexpr (!is_delay_buffer_shared) {
        clear_index = write_delay_buffer<DelayBufferWritingMode::CLEAR>(
            NULL, clear_index, sample_count
        );
    }
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
template<bool is_delay_buffer_shared>
void Delay<InputSignalProducerClass, capabilities>::mix_feedback_into_delay_buffer(
        Integer const sample_count
) noexcept {
    if (JS80P_UNLIKELY(feedback_signal_producer == NULL)) {
        is_starting = false;
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


template<class InputSignalProducerClass, DelayCapabilities capabilities>
template<typename Delay<InputSignalProducerClass, capabilities>::DelayBufferWritingMode mode>
Integer Delay<InputSignalProducerClass, capabilities>::write_delay_buffer(
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


template<class InputSignalProducerClass, DelayCapabilities capabilities>
template<bool is_delay_buffer_shared>
void Delay<InputSignalProducerClass, capabilities>::mix_input_into_delay_buffer(
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


template<class InputSignalProducerClass, DelayCapabilities capabilities>
bool Delay<InputSignalProducerClass, capabilities>::is_delay_buffer_silent() const noexcept
{
    return (
        silent_input_samples >= delay_buffer_size
        && silent_feedback_samples >= delay_buffer_size
    );
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    if (need_gain) {
        if (gain_buffer == NULL) {
            if constexpr (capabilities == DelayCapabilities::DC_SCALABLE) {
                if (time_scale_buffer == NULL) {
                    render<true, true, true, false>(
                        round,
                        first_sample_index,
                        last_sample_index,
                        buffer,
                        this->gain.get_value()
                    );
                } else {
                    render<true, true, false, false>(
                        round,
                        first_sample_index,
                        last_sample_index,
                        buffer,
                        this->gain.get_value()
                    );
                }
            } else if constexpr (capabilities == DelayCapabilities::DC_REVERSIBLE) {
                if (is_reversed) {
                    render<true, true, true, true>(
                        round,
                        first_sample_index,
                        last_sample_index,
                        buffer,
                        this->gain.get_value()
                    );
                } else {
                    render<true, true, true, false>(
                        round,
                        first_sample_index,
                        last_sample_index,
                        buffer,
                        this->gain.get_value()
                    );
                }
            } else {
                render<true, true, true, false>(
                    round,
                    first_sample_index,
                    last_sample_index,
                    buffer,
                    this->gain.get_value()
                );
            }
        } else {
            if constexpr (capabilities == DelayCapabilities::DC_SCALABLE) {
                if (time_scale_buffer == NULL) {
                    render<true, false, true, false>(
                        round,
                        first_sample_index,
                        last_sample_index,
                        buffer,
                        1.0
                    );
                } else {
                    render<true, false, false, false>(
                        round,
                        first_sample_index,
                        last_sample_index,
                        buffer,
                        1.0
                    );
                }
            } else if constexpr (capabilities == DelayCapabilities::DC_REVERSIBLE) {
                if (is_reversed) {
                    render<true, false, true, true>(
                        round,
                        first_sample_index,
                        last_sample_index,
                        buffer,
                        1.0
                    );
                } else {
                    render<true, false, true, false>(
                        round,
                        first_sample_index,
                        last_sample_index,
                        buffer,
                        1.0
                    );
                }
            } else {
                render<true, false, true, false>(
                    round,
                    first_sample_index,
                    last_sample_index,
                    buffer,
                    1.0
                );
            }
        }
    } else {
        if constexpr (capabilities == DelayCapabilities::DC_SCALABLE) {
            if (time_scale_buffer == NULL) {
                render<false, true, true, false>(
                    round,
                    first_sample_index,
                    last_sample_index,
                    buffer,
                    1.0
                );
            } else {
                render<false, true, false, false>(
                    round,
                    first_sample_index,
                    last_sample_index,
                    buffer,
                    1.0
                );
            }
        } else if constexpr (capabilities == DelayCapabilities::DC_REVERSIBLE) {
            if (is_reversed) {
                render<false, true, true, true>(
                    round,
                    first_sample_index,
                    last_sample_index,
                    buffer,
                    1.0
                );
            } else {
                render<false, true, true, false>(
                    round,
                    first_sample_index,
                    last_sample_index,
                    buffer,
                    1.0
                );
            }
        } else {
            render<false, true, true, false>(
                round,
                first_sample_index,
                last_sample_index,
                buffer,
                1.0
            );
        }
    }
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
template<bool need_gain, bool is_gain_constant, bool is_time_scale_constant, bool is_reversed>
void Delay<InputSignalProducerClass, capabilities>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** const buffer,
        Sample const gain
) noexcept {
    JS80P_ASSERT(is_time_scale_constant || !is_reversed);

    JS80P_ASSERT(
        capabilities != DelayCapabilities::DC_CHANNEL_LFO
        || (is_time_scale_constant && !is_reversed)
    );

    Integer const channels = this->channels;
    Number const read_index_orig = (Number)this->read_index;
    Sample const* const* delay_buffer = (
        shared_buffer_owner != NULL
            ? shared_buffer_owner->delay_buffer
            : this->delay_buffer
    );
    Number const delay_buffer_size_float = this->delay_buffer_size_float;

    Number reverse_delta_samples = 0;
    Number read_index = 0;
    Number reverse_done_samples = 0;
    Number reverse_target_delay_time_in_samples = 0;
    Number reverse_target_delay_time_in_samples_inv = 0;

    if constexpr (is_reversed) {
        reverse_target_delay_time_in_samples = this->reverse_target_delay_time_in_samples;
        reverse_target_delay_time_in_samples_inv = this->reverse_target_delay_time_in_samples_inv;
    }

    if (time_buffer == NULL) {
        Number const time_value_in_samples = time.get_value() * time_scale;

        for (Integer c = 0; c != channels; ++c) {
            Sample const* const delay_channel = delay_buffer[c];
            Sample* const out_channel = buffer[c];
            Number processed_samples;
            Sample const* channel_lfo_buffer = NULL;
            Sample channel_lfo_scale = 0.0;

            if constexpr (is_time_scale_constant) {
                if constexpr (is_reversed) {
                    initialize_reverse_rendering(
                        read_index,
                        reverse_done_samples,
                        delay_buffer_size_float
                    );
                    adjust_reverse_target_delay_time(
                        reverse_target_delay_time_in_samples,
                        reverse_target_delay_time_in_samples_inv,
                        reverse_done_samples,
                        time_value_in_samples
                    );
                    reverse_delta_samples = calculate_reverse_delta_samples(
                        time_value_in_samples, reverse_target_delay_time_in_samples
                    );
                } else {
                    if constexpr (capabilities == DelayCapabilities::DC_CHANNEL_LFO) {
                        channel_lfo_buffer = channel_lfo_buffers[c];
                        channel_lfo_scale = channel_lfo_scales[c] * this->sample_rate;
                    }

                    read_index = read_index_orig - time_value_in_samples;

                    if (JS80P_UNLIKELY(read_index < 0.0)) {
                        read_index += delay_buffer_size_float;
                    }
                }
            } else {
                processed_samples = 0.0;
            }

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                if constexpr (!is_time_scale_constant) {
                    read_index = (
                        read_index_orig
                        - time_value_in_samples * time_scale_buffer[i]
                        + processed_samples
                    );

                    if (JS80P_UNLIKELY(read_index < 0.0)) {
                        read_index += delay_buffer_size_float;
                    }
                }

                if constexpr (need_gain) {
                    if constexpr (is_gain_constant) {
                        out_channel[i] = gain * lookup_sample(
                            delay_channel,
                            read_index,
                            i,
                            channel_lfo_buffer,
                            channel_lfo_scale
                        );
                    } else {
                        out_channel[i] = gain_buffer[i] * lookup_sample(
                            delay_channel,
                            read_index,
                            i,
                            channel_lfo_buffer,
                            channel_lfo_scale
                        );
                    }
                } else {
                    out_channel[i] = lookup_sample(
                        delay_channel,
                        read_index,
                        i,
                        channel_lfo_buffer,
                        channel_lfo_scale
                    );
                }

                if constexpr (is_time_scale_constant) {
                    if constexpr (is_reversed) {
                        apply_reverse_delay_envelope(
                            out_channel[i],
                            reverse_done_samples,
                            reverse_target_delay_time_in_samples_inv
                        );

                        advance_reverse_rendering(
                            read_index,
                            reverse_delta_samples,
                            reverse_done_samples,
                            reverse_target_delay_time_in_samples,
                            reverse_target_delay_time_in_samples_inv,
                            time_value_in_samples,
                            delay_buffer_size_float
                        );

                        if (JS80P_UNLIKELY(read_index < 0.0)) {
                            read_index += delay_buffer_size_float;
                        }
                    } else {
                        read_index += 1.0;
                    }
                } else {
                    processed_samples += 1.0;
                }
            }
        }
    } else {
        for (Integer c = 0; c != channels; ++c) {
            Sample const* const delay_channel = delay_buffer[c];
            Sample* const out_channel = buffer[c];
            Number processed_samples = 0.0;
            Sample const* channel_lfo_buffer = NULL;
            Sample channel_lfo_scale = 0.0;

            if constexpr (is_reversed) {
                initialize_reverse_rendering(
                    read_index,
                    reverse_done_samples,
                    delay_buffer_size_float
                );
            } else if constexpr (capabilities == DelayCapabilities::DC_CHANNEL_LFO) {
                channel_lfo_buffer = channel_lfo_buffers[c];
                channel_lfo_scale = channel_lfo_scales[c] * this->sample_rate;
            }

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                if constexpr (is_time_scale_constant) {
                    if constexpr (!is_reversed) {
                        read_index = (
                            read_index_orig
                            - time_buffer[i] * time_scale
                            + processed_samples
                        );
                    }
                } else {
                    read_index = (
                        read_index_orig
                        - time_buffer[i] * time_scale_buffer[i] * time_scale
                        + processed_samples
                    );
                }

                if (JS80P_UNLIKELY(read_index < 0.0)) {
                    read_index += delay_buffer_size_float;
                }

                if constexpr (need_gain) {
                    if constexpr (is_gain_constant) {
                        out_channel[i] = gain * lookup_sample(
                            delay_channel,
                            read_index,
                            i,
                            channel_lfo_buffer,
                            channel_lfo_scale
                        );
                    } else {
                        out_channel[i] = gain_buffer[i] * lookup_sample(
                            delay_channel,
                            read_index,
                            i,
                            channel_lfo_buffer,
                            channel_lfo_scale
                        );
                    }
                } else {
                    out_channel[i] = lookup_sample(
                        delay_channel,
                        read_index,
                        i,
                        channel_lfo_buffer,
                        channel_lfo_scale
                    );
                }

                if constexpr (is_reversed) {
                    apply_reverse_delay_envelope(
                        out_channel[i],
                        reverse_done_samples,
                        reverse_target_delay_time_in_samples_inv
                    );

                    Number const time_value_in_samples = time_buffer[i] * time_scale;

                    adjust_reverse_target_delay_time(
                        reverse_target_delay_time_in_samples,
                        reverse_target_delay_time_in_samples_inv,
                        reverse_done_samples,
                        time_value_in_samples
                    );
                    reverse_delta_samples = calculate_reverse_delta_samples(
                        time_value_in_samples, reverse_target_delay_time_in_samples
                    );
                    advance_reverse_rendering(
                        read_index,
                        reverse_delta_samples,
                        reverse_done_samples,
                        reverse_target_delay_time_in_samples,
                        reverse_target_delay_time_in_samples_inv,
                        time_value_in_samples,
                        delay_buffer_size_float
                    );
                } else {
                    processed_samples += 1.0;
                }
            }
        }
    }

    if constexpr (is_reversed) {
        this->reverse_read_index = read_index;
        this->reverse_done_samples = reverse_done_samples;
        this->reverse_target_delay_time_in_samples = reverse_target_delay_time_in_samples;
        this->reverse_target_delay_time_in_samples_inv = reverse_target_delay_time_in_samples_inv;
    }
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::initialize_reverse_rendering(
        Number& read_index,
        Number& reverse_done_samples,
        Number const& delay_buffer_size_float
) const noexcept {
    read_index = this->reverse_read_index;
    reverse_done_samples = this->reverse_done_samples;

    if (JS80P_UNLIKELY(read_index < 0.0)) {
        read_index += delay_buffer_size_float;
    }
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::adjust_reverse_target_delay_time(
        Number& reverse_target_delay_time_in_samples,
        Number& reverse_target_delay_time_in_samples_inv,
        Number const& reverse_done_samples,
        Number const& time_value_in_samples
) const noexcept {
    if (reverse_done_samples < 0.000001) {
        reverse_target_delay_time_in_samples = time_value_in_samples;
        reverse_target_delay_time_in_samples_inv = (
            (time_value_in_samples > 0.1) ? 1.0 / time_value_in_samples : 1.0
        );
    }
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
Number Delay<InputSignalProducerClass, capabilities>::calculate_reverse_delta_samples(
        Number const& time_value_in_samples,
        Number const& reverse_target_delay_time_in_samples
) const noexcept {
    return std::max(
        std::min(
            (time_value_in_samples > 0.01)
                ? reverse_target_delay_time_in_samples / time_value_in_samples
                : 1.0,
            32.0
        ),
        0.125
    );
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::apply_reverse_delay_envelope(
        Sample& sample,
        Number const reverse_done_samples,
        Number const reverse_target_delay_time_in_samples_inv
) const noexcept {
    sample *= Math::lookup(
        reverse_delay_envelope.table,
        ReverseDelayEnvelope::TABLE_MAX_INDEX,
        ReverseDelayEnvelope::TABLE_MAX_INDEX_FLOAT * reverse_done_samples * reverse_target_delay_time_in_samples_inv
    );
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void Delay<InputSignalProducerClass, capabilities>::advance_reverse_rendering(
        Number& read_index,
        Number& reverse_delta_samples,
        Number& reverse_done_samples,
        Number& reverse_target_delay_time_in_samples,
        Number& reverse_target_delay_time_in_samples_inv,
        Number const& time_value_in_samples,
        Number const& delay_buffer_size_float
) const noexcept {
    read_index -= reverse_delta_samples;
    reverse_done_samples += reverse_delta_samples;

    if (reverse_done_samples > reverse_target_delay_time_in_samples) {
        reverse_done_samples = reverse_done_samples - reverse_target_delay_time_in_samples;

        if (reverse_done_samples >= 1.0) {
            reverse_done_samples = 0.0;
        }

        reverse_target_delay_time_in_samples = time_value_in_samples;
        reverse_target_delay_time_in_samples_inv = (
            (time_value_in_samples > 0.1) ? 1.0 / time_value_in_samples : 1.0
        );
        reverse_delta_samples = 1.0;

        read_index = reverse_next_start_index - 1.0 + reverse_done_samples;

        if (read_index > delay_buffer_size_float) {
            read_index -= delay_buffer_size_float;
        }
    }
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
Sample Delay<InputSignalProducerClass, capabilities>::lookup_sample(
        Sample const* const delay_channel,
        Number const read_index,
        Integer const i,
        Sample const* const channel_lfo_buffer,
        Sample const channel_lfo_scale
) const noexcept {
    if constexpr (capabilities == DelayCapabilities::DC_CHANNEL_LFO) {
        return Math::lookup_periodic<false>(
            delay_channel,
            delay_buffer_size,
            read_index + channel_lfo_scale * channel_lfo_buffer[i]
        );
    } else {
        return Math::lookup_periodic<true>(
            delay_channel, delay_buffer_size, read_index
        );
    }
}


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::PannedDelay(
        InputSignalProducerClass& input,
        PannedDelayStereoMode const stereo_mode,
        ToggleParam const* tempo_sync
) : PannedDelay<InputSignalProducerClass, FilterInputClass>(
        input, delay, stereo_mode, tempo_sync
    )
{
}


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::PannedDelay(
        InputSignalProducerClass& input,
        PannedDelayStereoMode const stereo_mode,
        FloatParamS& delay_time_leader,
        ToggleParam const* tempo_sync
) : Filter<FilterInputClass>(delay, NUMBER_OF_CHILDREN, CHANNELS),
    is_flipped(stereo_mode == PannedDelayStereoMode::FLIPPED),
    panning_scale(1.0),
    panning("", -1.0, 1.0, 0.0),
    delay(input, delay_time_leader, tempo_sync)
{
    initialize_instance();
}


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::PannedDelay(
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


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::PannedDelay(
        InputSignalProducerClass& delay_input,
        FilterInputClass& filter_input,
        PannedDelayStereoMode const stereo_mode,
        ToggleParam const* tempo_sync,
        Integer const number_of_children
) : Filter<FilterInputClass>(
        filter_input,
        number_of_children + NUMBER_OF_CHILDREN,
        CHANNELS
    ),
    is_flipped(stereo_mode == PannedDelayStereoMode::FLIPPED),
    panning_scale(1.0),
    panning("", -1.0, 1.0, 0.0),
    delay(delay_input, tempo_sync)
{
    initialize_instance();
}


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::PannedDelay(
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
        CHANNELS
    ),
    is_flipped(stereo_mode == PannedDelayStereoMode::FLIPPED),
    panning_scale(1.0),
    panning(panning_leader),
    delay(delay_input, delay_time_leader, tempo_sync)
{
    initialize_instance();
}


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::PannedDelay(
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
        CHANNELS
    ),
    is_flipped(stereo_mode == PannedDelayStereoMode::FLIPPED),
    panning_scale(1.0),
    panning(panning_leader),
    delay(delay_input, delay_gain_leader, delay_time_leader, tempo_sync)
{
    initialize_instance();
}


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::PannedDelay(
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
        CHANNELS
    ),
    is_flipped(stereo_mode == PannedDelayStereoMode::FLIPPED),
    panning_scale(1.0),
    panning(panning_leader),
    delay(delay_input, delay_gain_leader, delay_time, delay_time_max, tempo_sync)
{
    initialize_instance();
}


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
void PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::initialize_instance() noexcept
{
    panning_buffer = NULL;
    stereo_gain_buffer = SignalProducer::allocate_buffer();
    panning_buffer_scaled = SignalProducer::allocate_buffer();

    this->register_child(panning);
    this->register_child(delay);
}


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::~PannedDelay()
{
    SignalProducer::free_buffer(stereo_gain_buffer);
    SignalProducer::free_buffer(panning_buffer_scaled);
    panning_buffer = NULL;
}


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
void PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::set_block_size(
        Integer const new_block_size
) noexcept {
    SignalProducer::set_block_size(new_block_size);

    stereo_gain_buffer = SignalProducer::reallocate_buffer(stereo_gain_buffer);
    panning_buffer_scaled = SignalProducer::reallocate_buffer(panning_buffer_scaled);
    panning_buffer = NULL;
}


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
void PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::set_panning_scale(
        Number const scale
) noexcept {
    panning_scale = scale;
}


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
Sample const* const* PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    JS80P_ASSERT(this->input.get_channels() == CHANNELS);
    JS80P_ASSERT(this->delay.get_input_channels() == CHANNELS);
    JS80P_ASSERT(this->get_channels() == CHANNELS);

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


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
void PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::render(
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


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
template<int channel_1, int channel_2>
void PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::render_with_constant_panning(
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


template<class InputSignalProducerClass, class FilterInputClass, DelayCapabilities capabilities>
template<int channel_1, int channel_2>
void PannedDelay<InputSignalProducerClass, FilterInputClass, capabilities>::render_with_changing_panning(
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


template<class InputSignalProducerClass, DelayCapabilities capabilities>
DistortedHighShelfPannedDelay<InputSignalProducerClass, capabilities>::DistortedHighShelfPannedDelay(
    InputSignalProducerClass& input,
    PannedDelayStereoMode const stereo_mode,
    FloatParamS& distortion_level_leader,
    Distortion::TypeParam const& distortion_type,
    ToggleParam const* tempo_sync
) : DistortedHighShelfPannedDelayBase<InputSignalProducerClass, capabilities>(
        input, high_shelf_filter, stereo_mode, tempo_sync, NUMBER_OF_CHILDREN
    ),
    high_shelf_filter_q(
        "",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    distortion(
        "",
        distortion_type,
        this->delay,
        distortion_level_leader,
        &this->delay
    ),
    high_shelf_filter("", this->distortion, &this->delay)
{
    initialize_instance();

    high_shelf_filter.frequency.set_value(Constants::BIQUAD_FILTER_FREQUENCY_MAX);
    high_shelf_filter.q.set_value(Constants::BIQUAD_FILTER_Q_DEFAULT);
    high_shelf_filter.gain.set_value(0.0);
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
DistortedHighShelfPannedDelay<InputSignalProducerClass, capabilities>::DistortedHighShelfPannedDelay(
    InputSignalProducerClass& input,
    PannedDelayStereoMode const stereo_mode,
    FloatParamS& panning_leader,
    FloatParamS& delay_gain_leader,
    FloatParamS& delay_time_leader,
    BiquadFilterSharedBuffers& high_shelf_filter_shared_buffers,
    FloatParamS& high_shelf_filter_frequency_leader,
    FloatParamS& high_shelf_filter_gain_leader,
    FloatParamS& distortion_level_leader,
    Distortion::TypeParam const& distortion_type,
    ToggleParam const* tempo_sync
) : DistortedHighShelfPannedDelayBase<InputSignalProducerClass, capabilities>(
        input,
        high_shelf_filter,
        stereo_mode,
        panning_leader,
        delay_gain_leader,
        delay_time_leader,
        tempo_sync,
        NUMBER_OF_CHILDREN
    ),
    high_shelf_filter_q(
        "",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    distortion(
        "",
        distortion_type,
        this->delay,
        distortion_level_leader,
        &this->delay
    ),
    high_shelf_filter(
        this->distortion,
        high_shelf_filter_frequency_leader,
        high_shelf_filter_q,
        high_shelf_filter_gain_leader,
        &high_shelf_filter_shared_buffers,
        0.0,
        NULL,
        NULL,
        &this->delay
    )
{
    initialize_instance();
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
DistortedHighShelfPannedDelay<InputSignalProducerClass, capabilities>::DistortedHighShelfPannedDelay(
    InputSignalProducerClass& input,
    PannedDelayStereoMode const stereo_mode,
    FloatParamS& panning_leader,
    FloatParamS& delay_gain_leader,
    Seconds const delay_time,
    Seconds const delay_time_max,
    BiquadFilterSharedBuffers& high_shelf_filter_shared_buffers,
    FloatParamS& high_shelf_filter_frequency_leader,
    FloatParamS& high_shelf_filter_gain_leader,
    FloatParamS& distortion_level_leader,
    Distortion::TypeParam const& distortion_type,
    ToggleParam const* tempo_sync
) : DistortedHighShelfPannedDelayBase<InputSignalProducerClass, capabilities>(
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
    high_shelf_filter_q(
        "",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    distortion(
        "",
        distortion_type,
        this->delay,
        distortion_level_leader,
        &this->delay
    ),
    high_shelf_filter(
        this->distortion,
        high_shelf_filter_frequency_leader,
        high_shelf_filter_q,
        high_shelf_filter_gain_leader,
        &high_shelf_filter_shared_buffers,
        0.0,
        NULL,
        NULL,
        &this->delay
    )
{
    initialize_instance();
}


template<class InputSignalProducerClass, DelayCapabilities capabilities>
void DistortedHighShelfPannedDelay<InputSignalProducerClass, capabilities>::initialize_instance() noexcept
{
    this->register_child(high_shelf_filter_q);
    this->register_child(distortion);
    this->register_child(high_shelf_filter);
}

}

#endif
