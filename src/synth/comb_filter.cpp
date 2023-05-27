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

#ifndef JS80P__SYNTH__COMB_FILTER_CPP
#define JS80P__SYNTH__COMB_FILTER_CPP

#include "synth/comb_filter.hpp"
#include "synth/math.hpp"


namespace JS80P
{


template<class InputSignalProducerClass>
CombFilter<InputSignalProducerClass>::CombFilter(
        InputSignalProducerClass& input,
        StereoMode const stereo_mode,
        ToggleParam const* tempo_sync
) : Filter< HighShelfDelay<InputSignalProducerClass> >(
        high_shelf_filter,
        5,
        input.get_channels()
    ),
    is_flipped(stereo_mode == StereoMode::FLIPPED),
    high_shelf_filter_type(""),
    high_shelf_filter_q(
        "",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    stereo_gain_buffer(NULL),
    panning_buffer(NULL),
    panning("", -1.0, 1.0, 0.0),
    delay(input, tempo_sync),
    high_shelf_filter("", delay, high_shelf_filter_type)
{
    initialize_instance();

    high_shelf_filter.frequency.set_value(Constants::BIQUAD_FILTER_FREQUENCY_MAX);
    high_shelf_filter.q.set_value(Constants::BIQUAD_FILTER_Q_DEFAULT);
    high_shelf_filter.gain.set_value(0.0);
}


template<class InputSignalProducerClass>
CombFilter<InputSignalProducerClass>::CombFilter(
        InputSignalProducerClass& input,
        StereoMode const stereo_mode,
        FloatParam& panning_leader,
        FloatParam& delay_gain_leader,
        FloatParam& delay_time_leader,
        FloatParam& high_shelf_filter_frequency_leader,
        FloatParam& high_shelf_filter_gain_leader,
        ToggleParam const* tempo_sync
) : Filter< HighShelfDelay<InputSignalProducerClass> >(
        high_shelf_filter,
        5,
        input.get_channels()
    ),
    is_flipped(stereo_mode == StereoMode::FLIPPED),
    high_shelf_filter_type(""),
    high_shelf_filter_q(
        "",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    stereo_gain_buffer(NULL),
    panning_buffer(NULL),
    panning(panning_leader),
    delay(input, delay_gain_leader, delay_time_leader, tempo_sync),
    high_shelf_filter(
        delay,
        high_shelf_filter_type,
        high_shelf_filter_frequency_leader,
        high_shelf_filter_q,
        high_shelf_filter_gain_leader
    )
{
    initialize_instance();
}


template<class InputSignalProducerClass>
CombFilter<InputSignalProducerClass>::CombFilter(
        InputSignalProducerClass& input,
        StereoMode const stereo_mode,
        FloatParam& panning_leader,
        FloatParam& delay_gain_leader,
        Seconds const delay_time,
        FloatParam& high_shelf_filter_frequency_leader,
        FloatParam& high_shelf_filter_gain_leader,
        ToggleParam const* tempo_sync
) : Filter< HighShelfDelay<InputSignalProducerClass> >(
        high_shelf_filter,
        5,
        input.get_channels()
    ),
    is_flipped(stereo_mode == StereoMode::FLIPPED),
    high_shelf_filter_type(""),
    high_shelf_filter_q(
        "",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    stereo_gain_buffer(NULL),
    panning_buffer(NULL),
    panning(panning_leader),
    delay(input, delay_gain_leader, delay_time, tempo_sync),
    high_shelf_filter(
        delay,
        high_shelf_filter_type,
        high_shelf_filter_frequency_leader,
        high_shelf_filter_q,
        high_shelf_filter_gain_leader
    )
{
    initialize_instance();
}


template<class InputSignalProducerClass>
void CombFilter<InputSignalProducerClass>::initialize_instance() noexcept
{
    stereo_gain_buffer = SignalProducer::allocate_buffer();

    high_shelf_filter_type.set_value(
        HighShelfDelay<InputSignalProducerClass>::HIGH_SHELF
    );

    this->register_child(high_shelf_filter_type);
    this->register_child(high_shelf_filter_q);
    this->register_child(panning);
    this->register_child(delay);
    this->register_child(high_shelf_filter);
}


template<class InputSignalProducerClass>
CombFilter<InputSignalProducerClass>::~CombFilter()
{
    SignalProducer::free_buffer(stereo_gain_buffer);
}


template<class InputSignalProducerClass>
void CombFilter<InputSignalProducerClass>::set_block_size(
        Integer const new_block_size
) noexcept {
    SignalProducer::set_block_size(new_block_size);

    stereo_gain_buffer = SignalProducer::reallocate_buffer(stereo_gain_buffer);
}


template<class InputSignalProducerClass>
Sample const* const* CombFilter<InputSignalProducerClass>::initialize_rendering(
    Integer const round,
    Integer const sample_count
) noexcept {
    Filter< HighShelfDelay<InputSignalProducerClass> >::initialize_rendering(
        round, sample_count
    );

    /* https://www.w3.org/TR/webaudio/#stereopanner-algorithm */

    panning_buffer = FloatParam::produce_if_not_constant(
        &panning, round, sample_count
    );

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


template<class InputSignalProducerClass>
void CombFilter<InputSignalProducerClass>::render(
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

}

#endif
