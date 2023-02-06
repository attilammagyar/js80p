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

#ifndef JS80P__SYNTH__BIQUAD_FILTER_CPP
#define JS80P__SYNTH__BIQUAD_FILTER_CPP

#include <cmath>

#include "synth/biquad_filter.hpp"

#include "synth/math.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
BiquadFilter<InputSignalProducerClass>::TypeParam::TypeParam(
        std::string const name
)
    : Param<Type>(name, LOW_PASS, HIGH_SHELF, LOW_PASS)
{
}


template<class InputSignalProducerClass>
BiquadFilter<InputSignalProducerClass>::BiquadFilter(
        InputSignalProducerClass& input,
        TypeParam& type
) : Filter<InputSignalProducerClass>(input, 4),
    frequency(
        "",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX
    ),
    q(
        "", Constants::BIQUAD_FILTER_Q_MIN, Constants::BIQUAD_FILTER_Q_MAX, 1.0
    ),
    gain(
        "",
        Constants::BIQUAD_FILTER_GAIN_MIN,
        Constants::BIQUAD_FILTER_GAIN_MAX,
        0.0
    ),
    type(type)
{
    initialize_instance();
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::initialize_instance()
{
    register_children();

    this->allocate_buffers();

    x_n_m1 = new Sample[this->channels];
    x_n_m2 = new Sample[this->channels];
    y_n_m1 = new Sample[this->channels];
    y_n_m2 = new Sample[this->channels];

    clear();

    w0_scale = (Sample)Math::PI_DOUBLE * (Sample)this->sampling_period;
    low_pass_no_op_frequency = std::min(
        (Number)this->nyquist_frequency, frequency.get_max_value()
    );
}



template<class InputSignalProducerClass>
BiquadFilter<InputSignalProducerClass>::BiquadFilter(
        InputSignalProducerClass& input,
        TypeParam& type,
        FloatParam& frequency_leader,
        FloatParam& q_leader,
        FloatParam& gain_leader
) : Filter<InputSignalProducerClass>(input, 4),
    frequency(frequency_leader),
    q(q_leader),
    gain(gain_leader),
    type(type)
{
    initialize_instance();
}


template<class InputSignalProducerClass>
BiquadFilter<InputSignalProducerClass>::~BiquadFilter()
{
    delete[] x_n_m1;
    delete[] x_n_m2;
    delete[] y_n_m1;
    delete[] y_n_m2;

    free_buffers();
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::reallocate_buffers()
{
    free_buffers();
    allocate_buffers();
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::free_buffers()
{
    delete[] b0_buffer;
    delete[] b1_buffer;
    delete[] b2_buffer;
    delete[] a1_buffer;
    delete[] a2_buffer;
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::allocate_buffers()
{
    b0_buffer = new Sample[this->block_size];
    b1_buffer = new Sample[this->block_size];
    b2_buffer = new Sample[this->block_size];
    a1_buffer = new Sample[this->block_size];
    a2_buffer = new Sample[this->block_size];
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::register_children()
{
    this->register_child(type);
    this->register_child(frequency);
    this->register_child(q);
    this->register_child(gain);
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::set_sample_rate(
        Frequency const new_sample_rate
) {
    Filter<InputSignalProducerClass>::set_sample_rate(new_sample_rate);

    w0_scale = (Sample)Math::PI_DOUBLE * (Sample)this->sampling_period;
    low_pass_no_op_frequency = std::min(
        (Number)this->nyquist_frequency, frequency.get_max_value()
    );
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::set_block_size(
        Integer const new_block_size
) {
    if (new_block_size != this->block_size) {
        Filter<InputSignalProducerClass>::set_block_size(new_block_size);

        reallocate_buffers();
    }
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::clear()
{
    for (Integer c = 0; c != this->channels; ++c) {
        x_n_m1[c] = x_n_m2[c] = y_n_m1[c] = y_n_m2[c] = 0.0;
    }
}


template<class InputSignalProducerClass>
Sample const* const* BiquadFilter<InputSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) {
    bool is_no_op = false;

    is_silent = false;

    switch (type.get_value()) {
        case LOW_PASS:
            is_no_op = initialize_low_pass_rendering(round, sample_count);
            break;

        case HIGH_PASS:
            is_no_op = initialize_high_pass_rendering(round, sample_count);
            break;

        case BAND_PASS:
            is_no_op = initialize_band_pass_rendering(round, sample_count);
            break;

        case NOTCH:
            is_no_op = initialize_notch_rendering(round, sample_count);
            break;

        case PEAKING:
            is_no_op = initialize_peaking_rendering(round, sample_count);
            break;

        case LOW_SHELF:
            is_no_op = initialize_low_shelf_rendering(round, sample_count);
            break;

        case HIGH_SHELF:
            is_no_op = initialize_high_shelf_rendering(round, sample_count);
            break;

        default:
            is_no_op = true;
            break;
    }

    Filter<InputSignalProducerClass>::initialize_rendering(
        round, sample_count
    );

    if (is_no_op) {
        frequency.skip_round(round, sample_count);
        q.skip_round(round, sample_count);
        gain.skip_round(round, sample_count);

        return this->input_buffer;
    }

    return NULL;
}


template<class InputSignalProducerClass>
bool BiquadFilter<InputSignalProducerClass>::initialize_low_pass_rendering(
        Integer const round,
        Integer const sample_count
) {
    Number const low_pass_no_op_frequency = this->low_pass_no_op_frequency;
    Number const low_pass_silent_frequency = frequency.get_min_value();

    are_coefficients_constant = (
        frequency.is_constant_in_next_round(round, sample_count)
        && q.is_constant_in_next_round(round, sample_count)
    );

    FloatParam::produce_if_not_constant(&gain, round, sample_count);

    if (are_coefficients_constant) {
        Number const frequency_value = frequency.get_value();

        if (frequency_value >= low_pass_no_op_frequency) {
            return true;
        }

        Number const q_value = q.get_value();

        frequency.skip_round(round, sample_count);
        q.skip_round(round, sample_count);

        is_silent = frequency_value <= low_pass_silent_frequency;

        if (UNLIKELY(is_silent)) {
            return false;
        }

        store_low_pass_coefficient_samples(0, frequency_value, q_value);

    } else {
        Sample const* frequency_buffer = (
            FloatParam::produce<FloatParam>(&frequency, round, sample_count)[0]
        );
        Sample const* q_buffer = (
            FloatParam::produce<FloatParam>(&q, round, sample_count)[0]
        );

        for (Integer i = 0; i != sample_count; ++i) {
            Number const frequency_value = frequency_buffer[i];

            if (frequency_value >= low_pass_no_op_frequency) {
                store_no_op_coefficient_samples(i);

                continue;
            }

            if (UNLIKELY(frequency_value <= low_pass_silent_frequency)) {
                store_silent_coefficient_samples(i);

                continue;
            }

            store_low_pass_coefficient_samples(
                i, frequency_value, (Number)q_buffer[i]
            );
        }
    }

    return false;
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::store_low_pass_coefficient_samples(
        Integer const index,
        Number const frequency_value,
        Number const q_value
) {
    Sample const w0 = w0_scale * frequency_value;
    Sample const alpha_qdb = (
        0.5 * Math::sin(w0)
        * Math::pow_10_inv(q_value * Constants::BIQUAD_FILTER_Q_SCALE)
    );

    Sample const cos_w0 = Math::cos(w0);
    Sample const b1 = 1.0 - cos_w0;
    Sample const b0_b2 = 0.5 * b1;

    store_normalized_coefficient_samples(
        index, b0_b2, b1, b0_b2, 1.0 + alpha_qdb, -2.0 * cos_w0, 1.0 - alpha_qdb
    );
}


template<class InputSignalProducerClass>
bool BiquadFilter<InputSignalProducerClass>::initialize_high_pass_rendering(
        Integer const round,
        Integer const sample_count
) {
    Number const no_op_frequency = frequency.get_min_value();
    Frequency const silent_frequency = this->nyquist_frequency;

    are_coefficients_constant = (
        frequency.is_constant_in_next_round(round, sample_count)
        && q.is_constant_in_next_round(round, sample_count)
    );

    FloatParam::produce_if_not_constant(&gain, round, sample_count);

    if (are_coefficients_constant) {
        Number const frequency_value = frequency.get_value();

        if (frequency_value <= no_op_frequency) {
            return true;
        }

        Number const q_value = q.get_value();

        frequency.skip_round(round, sample_count);
        q.skip_round(round, sample_count);

        is_silent = frequency_value >= silent_frequency;

        if (UNLIKELY(is_silent)) {
            return false;
        }

        store_high_pass_coefficient_samples(0, frequency_value, q_value);

    } else {
        Sample const* frequency_buffer = (
            FloatParam::produce<FloatParam>(&frequency, round, sample_count)[0]
        );
        Sample const* q_buffer = (
            FloatParam::produce<FloatParam>(&q, round, sample_count)[0]
        );

        for (Integer i = 0; i != sample_count; ++i) {
            Number const frequency_value = frequency_buffer[i];

            if (frequency_value <= no_op_frequency) {
                store_no_op_coefficient_samples(i);

                continue;
            }

            if (UNLIKELY(frequency_value >= silent_frequency)) {
                store_silent_coefficient_samples(i);

                continue;
            }

            store_high_pass_coefficient_samples(
                i, frequency_value, (Number)q_buffer[i]
            );
        }
    }

    return false;
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::store_high_pass_coefficient_samples(
        Integer const index,
        Number const frequency_value,
        Number const q_value
) {
    Sample const w0 = w0_scale * frequency_value;
    Sample const alpha_qdb = (
        0.5 * Math::sin(w0)
        * Math::pow_10_inv(q_value * Constants::BIQUAD_FILTER_Q_SCALE)
    );

    Sample const cos_w0 = Math::cos(w0);
    Sample const b1 = -1.0 - cos_w0;
    Sample const b0_b2 = -0.5 * b1;

    store_normalized_coefficient_samples(
        index, b0_b2, b1, b0_b2, 1.0 + alpha_qdb, -2.0 * cos_w0, 1.0 - alpha_qdb
    );
}


template<class InputSignalProducerClass>
bool BiquadFilter<InputSignalProducerClass>::initialize_band_pass_rendering(
        Integer const round,
        Integer const sample_count
) {
    Number const band_pass_silent_frequency = low_pass_no_op_frequency;

    are_coefficients_constant = (
        frequency.is_constant_in_next_round(round, sample_count)
        && q.is_constant_in_next_round(round, sample_count)
    );

    FloatParam::produce_if_not_constant(&gain, round, sample_count);

    if (are_coefficients_constant) {
        Number const frequency_value = frequency.get_value();
        Number const q_value = q.get_value();

        if (q_value < THRESHOLD) {
            return true;
        }

        is_silent = frequency_value >= band_pass_silent_frequency;

        if (UNLIKELY(is_silent)) {
            return false;
        }

        frequency.skip_round(round, sample_count);
        q.skip_round(round, sample_count);

        store_band_pass_coefficient_samples(0, frequency_value, q_value);

    } else {
        Sample const* frequency_buffer = (
            FloatParam::produce<FloatParam>(&frequency, round, sample_count)[0]
        );
        Sample const* q_buffer = (
            FloatParam::produce<FloatParam>(&q, round, sample_count)[0]
        );

        for (Integer i = 0; i != sample_count; ++i) {
            Number const frequency_value = (Number)frequency_buffer[i];
            Number const q_value = (Number)q_buffer[i];

            if (q_value < THRESHOLD) {
                store_no_op_coefficient_samples(i);

                continue;
            }

            if (frequency_value > band_pass_silent_frequency) {
                store_silent_coefficient_samples(i);

                continue;
            }

            store_band_pass_coefficient_samples(i, frequency_value, q_value);
        }
    }

    return false;
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::store_band_pass_coefficient_samples(
        Integer const index,
        Number const frequency_value,
        Number const q_value
) {
    Sample const w0 = w0_scale * frequency_value;
    Sample const alpha_q = 0.5 * Math::sin(w0) / q_value;
    Sample const cos_w0 = Math::cos(w0);

    store_normalized_coefficient_samples(
        index, alpha_q, 0.0, -alpha_q, 1.0 + alpha_q, -2.0 * cos_w0, 1.0 - alpha_q
    );
}


template<class InputSignalProducerClass>
bool BiquadFilter<InputSignalProducerClass>::initialize_notch_rendering(
        Integer const round,
        Integer const sample_count
) {
    Number const notch_no_op_frequency = low_pass_no_op_frequency;

    are_coefficients_constant = (
        frequency.is_constant_in_next_round(round, sample_count)
        && q.is_constant_in_next_round(round, sample_count)
    );

    FloatParam::produce_if_not_constant(&gain, round, sample_count);

    if (are_coefficients_constant) {
        Number const frequency_value = frequency.get_value();
        Number const q_value = q.get_value();

        if (frequency_value >= notch_no_op_frequency) {
            return true;
        }

        is_silent = q_value < THRESHOLD;

        if (UNLIKELY(is_silent)) {
            return false;
        }

        frequency.skip_round(round, sample_count);
        q.skip_round(round, sample_count);

        store_notch_coefficient_samples(0, frequency_value, q_value);

    } else {
        Sample const* frequency_buffer = (
            FloatParam::produce<FloatParam>(&frequency, round, sample_count)[0]
        );
        Sample const* q_buffer = (
            FloatParam::produce<FloatParam>(&q, round, sample_count)[0]
        );

        for (Integer i = 0; i != sample_count; ++i) {
            Number const frequency_value = (Number)frequency_buffer[i];
            Number const q_value = (Number)q_buffer[i];

            if (q_value < THRESHOLD) {
                store_silent_coefficient_samples(i);

                continue;
            }

            if (frequency_value > notch_no_op_frequency) {
                store_no_op_coefficient_samples(i);

                continue;
            }

            store_notch_coefficient_samples(i, frequency_value, q_value);
        }
    }

    return false;
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::store_notch_coefficient_samples(
        Integer const index,
        Number const frequency_value,
        Number const q_value
) {
    Sample const w0 = w0_scale * frequency_value;
    Sample const alpha_q = 0.5 * Math::sin(w0) / q_value;
    Sample const cos_w0 = Math::cos(w0);

    Sample const b1_a1 = -2.0 * cos_w0;

    store_normalized_coefficient_samples(
        index, 1.0, b1_a1, 1.0, 1.0 + alpha_q, b1_a1, 1.0 - alpha_q
    );
}


template<class InputSignalProducerClass>
bool BiquadFilter<InputSignalProducerClass>::initialize_peaking_rendering(
        Integer const round,
        Integer const sample_count
) {
    Number const peaking_no_op_frequency = low_pass_no_op_frequency;

    are_coefficients_constant = (
        frequency.is_constant_in_next_round(round, sample_count)
        && q.is_constant_in_next_round(round, sample_count)
        && gain.is_constant_in_next_round(round, sample_count)
    );

    if (are_coefficients_constant) {
        Number const frequency_value = frequency.get_value();
        Number const gain_value = gain.get_value();

        if (
                std::fabs(gain_value) < THRESHOLD
                || frequency_value >= peaking_no_op_frequency
        ) {
            return true;
        }

        Number const q_value = q.get_value();

        frequency.skip_round(round, sample_count);
        q.skip_round(round, sample_count);
        gain.skip_round(round, sample_count);

        if (q_value >= THRESHOLD) {
            store_peaking_coefficient_samples(
                0, frequency_value, q_value, gain_value
            );
        } else {
            store_gain_coefficient_samples(0, gain_value);
        }

    } else {
        Sample const* frequency_buffer = (
            FloatParam::produce<FloatParam>(&frequency, round, sample_count)[0]
        );
        Sample const* q_buffer = (
            FloatParam::produce<FloatParam>(&q, round, sample_count)[0]
        );
        Sample const* gain_buffer = (
            FloatParam::produce<FloatParam>(&gain, round, sample_count)[0]
        );

        for (Integer i = 0; i != sample_count; ++i) {
            Number const frequency_value = (Number)frequency_buffer[i];
            Number const gain_value = (Number)gain_buffer[i];

            if (
                std::fabs(gain_value) < THRESHOLD
                || frequency_value >= peaking_no_op_frequency
            ) {
                store_no_op_coefficient_samples(i);

                continue;
            }

            Number const q_value = (Number)q_buffer[i];

            if (q_value >= THRESHOLD) {
                store_peaking_coefficient_samples(
                    i, frequency_value, q_value, gain_value
                );
            } else {
                store_gain_coefficient_samples(i, gain_value);
            }
        }
    }

    return false;
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::store_peaking_coefficient_samples(
        Integer const index,
        Number const frequency_value,
        Number const q_value,
        Number const gain_value
) {
    Sample const w0 = w0_scale * frequency_value;
    Sample const alpha_q = 0.5 * Math::sin(w0) / q_value;
    Sample const cos_w0 = Math::cos(w0);
    Sample const a = Math::pow_10(
        (Sample)gain_value * Constants::BIQUAD_FILTER_GAIN_SCALE
    );

    Sample const alpha_q_times_a = alpha_q * a;
    Sample const alpha_q_over_a = alpha_q / a;

    Sample const b1_a1 = -2.0 * cos_w0;

    store_normalized_coefficient_samples(
        index,
        1.0 + alpha_q_times_a,
        b1_a1,
        1.0 - alpha_q_times_a,
        1.0 + alpha_q_over_a,
        b1_a1,
        1.0 - alpha_q_over_a
    );
}


template<class InputSignalProducerClass>
bool BiquadFilter<InputSignalProducerClass>::initialize_low_shelf_rendering(
        Integer const round,
        Integer const sample_count
) {
    Frequency const becomes_gain_frequency = this->nyquist_frequency;
    Number const no_op_frequency = frequency.get_min_value();

    are_coefficients_constant = (
        frequency.is_constant_in_next_round(round, sample_count)
        && gain.is_constant_in_next_round(round, sample_count)
    );

    FloatParam::produce_if_not_constant(&q, round, sample_count);

    if (are_coefficients_constant) {
        Number const frequency_value = frequency.get_value();

        if (frequency_value <= no_op_frequency) {
            return true;
        }

        Number const gain_value = gain.get_value();

        frequency.skip_round(round, sample_count);
        gain.skip_round(round, sample_count);

        if (UNLIKELY(frequency_value >= becomes_gain_frequency)) {
            store_gain_coefficient_samples(0, gain_value);

            return false;
        }

        store_low_shelf_coefficient_samples(0, frequency_value, gain_value);

    } else {
        Sample const* frequency_buffer = (
            FloatParam::produce<FloatParam>(&frequency, round, sample_count)[0]
        );
        Sample const* gain_buffer = (
            FloatParam::produce<FloatParam>(&gain, round, sample_count)[0]
        );

        for (Integer i = 0; i != sample_count; ++i) {
            Number const frequency_value = (Number)frequency_buffer[i];

            if (frequency_value <= no_op_frequency) {
                store_no_op_coefficient_samples(i);

                continue;
            }

            Number const gain_value = (Number)gain_buffer[i];

            if (UNLIKELY(frequency_value >= becomes_gain_frequency)) {
                store_gain_coefficient_samples(i, gain_value);

                continue;
            }

            store_low_shelf_coefficient_samples(i, frequency_value, gain_value);
        }
    }

    return false;
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::store_low_shelf_coefficient_samples(
        Integer const index,
        Number const frequency_value,
        Number const gain_value
) {
    Sample const a = Math::pow_10(
        (Sample)gain_value * Constants::BIQUAD_FILTER_GAIN_SCALE
    );

    // Recalculating the power seems to be slightly faster than std::sqrt(a).
    Sample const a_sqrt = Math::pow_10(
        (Sample)gain_value * Constants::BIQUAD_FILTER_GAIN_SCALE_HALF
    );

    Sample const w0 = w0_scale * (Sample)frequency_value;
    Sample const cos_w0 = Math::cos(w0);

    Sample const a_p_1 = a + 1.0;
    Sample const a_m_1 = a - 1.0;
    Sample const a_m_1_cos_w0 = a_m_1 * cos_w0;
    Sample const a_p_1_cos_w0 = a_p_1 * cos_w0;

    // S = 1 makes sqrt((A + 1/A) * (1/S - 1) + 2) collapse to just sqrt(2).
    // Also, alpha_s is always multiplied by 2, which cancels dividing the
    // sine by 2.
    Sample const alpha_s_double_a_sqrt = (
        Math::sin(w0) * Constants::BIQUAD_FILTER_FREQUENCY_SINE_SCALE * a_sqrt
    );

    store_normalized_coefficient_samples(
        index,
        a * (a_p_1 - a_m_1_cos_w0 + alpha_s_double_a_sqrt),
        2.0 * a * (a_m_1 - a_p_1_cos_w0),
        a * (a_p_1 - a_m_1_cos_w0 - alpha_s_double_a_sqrt),
        a_p_1 + a_m_1_cos_w0 + alpha_s_double_a_sqrt,
        -2.0 * (a_m_1 + a_p_1_cos_w0),
        a_p_1 + a_m_1_cos_w0 - alpha_s_double_a_sqrt
    );
}


template<class InputSignalProducerClass>
bool BiquadFilter<InputSignalProducerClass>::initialize_high_shelf_rendering(
        Integer const round,
        Integer const sample_count
) {
    Number const high_shelf_no_op_frequency = low_pass_no_op_frequency;
    Number const becomes_gain_frequency = frequency.get_min_value();

    are_coefficients_constant = (
        frequency.is_constant_in_next_round(round, sample_count)
        && gain.is_constant_in_next_round(round, sample_count)
    );

    FloatParam::produce_if_not_constant(&q, round, sample_count);

    if (are_coefficients_constant) {
        Number const frequency_value = frequency.get_value();

        if (frequency_value >= high_shelf_no_op_frequency) {
            return true;
        }

        Number const gain_value = gain.get_value();

        frequency.skip_round(round, sample_count);
        gain.skip_round(round, sample_count);

        if (UNLIKELY(frequency_value <= becomes_gain_frequency)) {
            store_gain_coefficient_samples(0, gain_value);

            return false;
        }

        store_high_shelf_coefficient_samples(0, frequency_value, gain_value);

    } else {
        Sample const* frequency_buffer = (
            FloatParam::produce<FloatParam>(&frequency, round, sample_count)[0]
        );
        Sample const* gain_buffer = (
            FloatParam::produce<FloatParam>(&gain, round, sample_count)[0]
        );

        for (Integer i = 0; i != sample_count; ++i) {
            Number const frequency_value = frequency_buffer[i];

            if (frequency_value >= high_shelf_no_op_frequency) {
                store_no_op_coefficient_samples(i);

                continue;
            }

            Number const gain_value = gain_buffer[i];

            if (UNLIKELY(frequency_value <= becomes_gain_frequency)) {
                store_gain_coefficient_samples(i, gain_value);

                continue;
            }

            store_high_shelf_coefficient_samples(i, frequency_value, gain_value);
        }
    }

    return false;
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::store_high_shelf_coefficient_samples(
        Integer const index,
        Number const frequency_value,
        Number const gain_value
) {
    Sample const a = Math::pow_10(
        (Sample)gain_value * Constants::BIQUAD_FILTER_GAIN_SCALE
    );

    // Recalculating the power seems to be slightly faster than std::sqrt(a).
    Sample const a_sqrt = Math::pow_10(
        (Sample)gain_value * Constants::BIQUAD_FILTER_GAIN_SCALE_HALF
    );

    Sample const w0 = w0_scale * (Sample)frequency_value;
    Sample const cos_w0 = Math::cos(w0);

    Sample const a_p_1 = a + 1.0;
    Sample const a_m_1 = a - 1.0;
    Sample const a_m_1_cos_w0 = a_m_1 * cos_w0;
    Sample const a_p_1_cos_w0 = a_p_1 * cos_w0;

    // S = 1 makes sqrt((A + 1/A) * (1/S - 1) + 2) collapse to just sqrt(2).
    // Also, alpha_s is always multiplied by 2, which eliminates dividing
    // the sine by 2.
    Sample const alpha_s_double_a_sqrt = (
        Math::sin(w0) * Constants::BIQUAD_FILTER_FREQUENCY_SINE_SCALE * a_sqrt
    );

    store_normalized_coefficient_samples(
        index,
        a * (a_p_1 + a_m_1_cos_w0 + alpha_s_double_a_sqrt),
        -2.0 * a * (a_m_1 + a_p_1_cos_w0),
        a * (a_p_1 + a_m_1_cos_w0 - alpha_s_double_a_sqrt),
        a_p_1 - a_m_1_cos_w0 + alpha_s_double_a_sqrt,
        2.0 * (a_m_1 - a_p_1_cos_w0),
        a_p_1 - a_m_1_cos_w0 - alpha_s_double_a_sqrt
    );
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::store_gain_coefficient_samples(
        Integer const index,
        Number const gain_value
) {
    store_normalized_coefficient_samples(
        index,
        Math::pow_10((Sample)gain_value * Constants::DB_TO_LINEAR_GAIN_SCALE),
        0.0,
        0.0,
        1.0,
        0.0,
        0.0
    );
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::store_normalized_coefficient_samples(
        Integer const index,
        Sample const b0,
        Sample const b1,
        Sample const b2,
        Sample const a0,
        Sample const a1,
        Sample const a2
) {
    Sample const a0_inv = 1.0 / a0;

    b0_buffer[index] = b0 * a0_inv;
    b1_buffer[index] = b1 * a0_inv;
    b2_buffer[index] = b2 * a0_inv;
    a1_buffer[index] = a1 * a0_inv;
    a2_buffer[index] = a2 * a0_inv;
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::store_no_op_coefficient_samples(
        Integer const index
) {
    b0_buffer[index] = 1.0;
    b1_buffer[index] =
        b2_buffer[index] =
        a1_buffer[index] =
        a2_buffer[index] = 0.0;
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::store_silent_coefficient_samples(
        Integer const index
) {
    b0_buffer[index] =
        b1_buffer[index] =
        b2_buffer[index] =
        a1_buffer[index] =
        a2_buffer[index] = 0.0;
}


template<class InputSignalProducerClass>
void BiquadFilter<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) {
    if (UNLIKELY(is_silent)) {
        this->render_silence(
            round, first_sample_index, last_sample_index, buffer
        );

        return;
    }

    Integer const channels = this->channels;
    Sample const* const* const input_buffer = this->input_buffer;

    if (are_coefficients_constant) {
        Sample const b0 = b0_buffer[0];
        Sample const b1 = b1_buffer[0];
        Sample const b2 = b2_buffer[0];
        Sample const a1 = a1_buffer[0];
        Sample const a2 = a2_buffer[0];

        for (Integer c = 0; c != channels; ++c) {
            Sample x_n_m1 = this->x_n_m1[c];
            Sample x_n_m2 = this->x_n_m2[c];
            Sample y_n_m1 = this->y_n_m1[c];
            Sample y_n_m2 = this->y_n_m2[c];

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const x_n = input_buffer[c][i];
                Sample const y_n = (
                    b0 * x_n + b1 * x_n_m1 + b2 * x_n_m2
                    - a1 * y_n_m1 - a2 * y_n_m2
                );

                buffer[c][i] = y_n;

                x_n_m2 = x_n_m1;
                x_n_m1 = x_n;
                y_n_m2 = y_n_m1;
                y_n_m1 = y_n;
            }

            this->x_n_m1[c] = x_n_m1;
            this->x_n_m2[c] = x_n_m2;
            this->y_n_m1[c] = y_n_m1;
            this->y_n_m2[c] = y_n_m2;
        }

        return;
    }

    Sample const* b0 = b0_buffer;
    Sample const* b1 = b1_buffer;
    Sample const* b2 = b2_buffer;
    Sample const* a1 = a1_buffer;
    Sample const* a2 = a2_buffer;

    for (Integer c = 0; c != channels; ++c) {
        Sample x_n_m1 = this->x_n_m1[c];
        Sample x_n_m2 = this->x_n_m2[c];
        Sample y_n_m1 = this->y_n_m1[c];
        Sample y_n_m2 = this->y_n_m2[c];

        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            Sample const x_n = input_buffer[c][i];
            Sample const y_n = (
                b0[i] * x_n + b1[i] * x_n_m1 + b2[i] * x_n_m2
                - a1[i] * y_n_m1 - a2[i] * y_n_m2
            );

            buffer[c][i] = y_n;

            x_n_m2 = x_n_m1;
            x_n_m1 = x_n;
            y_n_m2 = y_n_m1;
            y_n_m1 = y_n;
        }

        this->x_n_m1[c] = x_n_m1;
        this->x_n_m2[c] = x_n_m2;
        this->y_n_m1[c] = y_n_m1;
        this->y_n_m2[c] = y_n_m2;
    }
}

}

#endif
