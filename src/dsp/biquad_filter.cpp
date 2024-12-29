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

#ifndef JS80P__DSP__BIQUAD_FILTER_CPP
#define JS80P__DSP__BIQUAD_FILTER_CPP

#include <algorithm>

#include "dsp/biquad_filter.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

BiquadFilterTypeParam dummy_filter_type("dummy_filter_type");


BiquadFilterSharedBuffers::BiquadFilterSharedBuffers()
    : round(-1),
    b0_buffer(NULL),
    b1_buffer(NULL),
    b2_buffer(NULL),
    a1_buffer(NULL),
    a2_buffer(NULL),
    are_coefficients_constant(false),
    is_silent(false),
    is_no_op(false)
{
}


BiquadFilterTypeParam::BiquadFilterTypeParam(std::string const& name) noexcept
    : ByteParam(
        name,
        SimpleBiquadFilter::LOW_PASS,
        SimpleBiquadFilter::HIGH_SHELF,
        SimpleBiquadFilter::LOW_PASS
    )
{
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
BiquadFilter<InputSignalProducerClass, fixed_type>::BiquadFilter(
        std::string const& name,
        InputSignalProducerClass& input,
        BiquadFilterTypeParam& type,
        BiquadFilterSharedBuffers* shared_buffers,
        Number const inaccuracy_seed,
        FloatParamB const* freq_inaccuracy_param,
        FloatParamB const* q_inaccuracy_param,
        SignalProducer* const buffer_owner
) noexcept
    : Filter<InputSignalProducerClass>(input, 3, 0, buffer_owner),
    frequency(
        name + "FRQ",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_DEFAULT
    ),
    q(
        name + "Q",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT
    ),
    gain(
        name + "G",
        Constants::BIQUAD_FILTER_GAIN_MIN,
        Constants::BIQUAD_FILTER_GAIN_MAX,
        Constants::BIQUAD_FILTER_GAIN_DEFAULT
    ),
    type(type),
    inaccuracy_seed(inaccuracy_seed),
    freq_inaccuracy_param(freq_inaccuracy_param),
    q_inaccuracy_param(q_inaccuracy_param),
    shared_buffers(shared_buffers)
{
    initialize_instance();
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
BiquadFilter<InputSignalProducerClass, fixed_type>::BiquadFilter(
        std::string const& name,
        InputSignalProducerClass& input,
        SignalProducer* const buffer_owner
) noexcept
    : BiquadFilter<InputSignalProducerClass, fixed_type>::BiquadFilter(
        name,
        input,
        dummy_filter_type,
        NULL,
        0.0,
        NULL,
        NULL,
        buffer_owner
    )
{
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::initialize_instance() noexcept
{
    register_children();

    this->allocate_buffers();

    x_n_m1 = new Sample[this->channels];
    x_n_m2 = new Sample[this->channels];
    y_n_m1 = new Sample[this->channels];
    y_n_m2 = new Sample[this->channels];

    BiquadFilter<InputSignalProducerClass, fixed_type>::reset();
    update_helper_variables();
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::update_helper_variables() noexcept
{
    w0_scale = (Sample)Math::PI_DOUBLE * (Sample)this->sampling_period;
    low_pass_no_op_frequency = std::min(
        (Number)this->nyquist_frequency, frequency.get_max_value()
    );
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
BiquadFilter<InputSignalProducerClass, fixed_type>::BiquadFilter(
        std::string const& name,
        InputSignalProducerClass& input,
        BiquadFilterTypeParam& type,
        ToggleParam const& freq_log_scale_toggle,
        ToggleParam const& q_log_scale_toggle,
        SignalProducer* const buffer_owner
) noexcept
    : Filter<InputSignalProducerClass>(input, 3, 0, buffer_owner),
    frequency(
        name + "FRQ",
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_DEFAULT,
        0.0,
        NULL,
        &freq_log_scale_toggle,
        Math::log_biquad_filter_freq_table(),
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_INDEX_SCALE
    ),
    q(
        name + "Q",
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT,
        0.0,
        NULL,
        &q_log_scale_toggle,
        Math::log_biquad_filter_q_table(),
        Math::LOG_BIQUAD_FILTER_Q_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_Q_TABLE_INDEX_SCALE,
        Math::LOG_BIQUAD_FILTER_Q_VALUE_OFFSET
    ),
    gain(
        name + "G",
        Constants::BIQUAD_FILTER_GAIN_MIN,
        Constants::BIQUAD_FILTER_GAIN_MAX,
        Constants::BIQUAD_FILTER_GAIN_DEFAULT
    ),
    type(type),
    inaccuracy_seed(0.0),
    freq_inaccuracy_param(NULL),
    q_inaccuracy_param(NULL),
    shared_buffers(NULL)
{
    initialize_instance();
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
BiquadFilter<InputSignalProducerClass, fixed_type>::BiquadFilter(
        InputSignalProducerClass& input,
        BiquadFilterTypeParam& type,
        FloatParamS& frequency_leader,
        FloatParamS& q_leader,
        FloatParamS& gain_leader,
        BiquadFilterSharedBuffers* shared_buffers,
        Number const inaccuracy_seed,
        FloatParamB const* freq_inaccuracy_param,
        FloatParamB const* q_inaccuracy_param,
        SignalProducer* buffer_owner
) noexcept
    : Filter<InputSignalProducerClass>(input, 3, 0, buffer_owner),
    frequency(frequency_leader),
    q(q_leader),
    gain(gain_leader),
    type(type),
    inaccuracy_seed(inaccuracy_seed),
    freq_inaccuracy_param(freq_inaccuracy_param),
    q_inaccuracy_param(q_inaccuracy_param),
    shared_buffers(shared_buffers)
{
    initialize_instance();
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
BiquadFilter<InputSignalProducerClass, fixed_type>::BiquadFilter(
        InputSignalProducerClass& input,
        FloatParamS& frequency_leader,
        FloatParamS& q_leader,
        FloatParamS& gain_leader,
        BiquadFilterSharedBuffers* shared_buffers,
        Number const inaccuracy_seed,
        FloatParamB const* freq_inaccuracy_param,
        FloatParamB const* q_inaccuracy_param,
        SignalProducer* buffer_owner
) noexcept
    : BiquadFilter<InputSignalProducerClass, fixed_type>(
        input,
        dummy_filter_type,
        frequency_leader,
        q_leader,
        gain_leader,
        shared_buffers,
        inaccuracy_seed,
        freq_inaccuracy_param,
        q_inaccuracy_param,
        buffer_owner
    )
{
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
BiquadFilter<InputSignalProducerClass, fixed_type>::BiquadFilter(
        InputSignalProducerClass& input,
        BiquadFilterTypeParam& type,
        FloatParamS& frequency_leader,
        FloatParamS& q_leader,
        FloatParamS& gain_leader,
        Byte const& voice_status,
        BiquadFilterSharedBuffers* shared_buffers,
        Number const inaccuracy_seed,
        FloatParamB const* freq_inaccuracy_param,
        FloatParamB const* q_inaccuracy_param,
        SignalProducer* buffer_owner
) noexcept
    : Filter<InputSignalProducerClass>(input, 3, 0, buffer_owner),
    frequency(frequency_leader, voice_status),
    q(q_leader, voice_status),
    gain(gain_leader, voice_status),
    type(type),
    inaccuracy_seed(inaccuracy_seed),
    freq_inaccuracy_param(freq_inaccuracy_param),
    q_inaccuracy_param(q_inaccuracy_param),
    shared_buffers(shared_buffers)
{
    initialize_instance();
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
BiquadFilter<InputSignalProducerClass, fixed_type>::~BiquadFilter()
{
    delete[] x_n_m1;
    delete[] x_n_m2;
    delete[] y_n_m1;
    delete[] y_n_m2;

    free_buffers();
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::reallocate_buffers() noexcept
{
    free_buffers();
    allocate_buffers();
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::free_buffers() noexcept
{
    if (shared_buffers != NULL) {
        return;
    }

    delete[] b0_buffer;
    delete[] b1_buffer;
    delete[] b2_buffer;
    delete[] a1_buffer;
    delete[] a2_buffer;

    b0_buffer = NULL;
    b1_buffer = NULL;
    b2_buffer = NULL;
    a1_buffer = NULL;
    a2_buffer = NULL;
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::allocate_buffers() noexcept
{
    if (shared_buffers != NULL) {
        return;
    }

    b0_buffer = new Sample[this->block_size];
    b1_buffer = new Sample[this->block_size];
    b2_buffer = new Sample[this->block_size];
    a1_buffer = new Sample[this->block_size];
    a2_buffer = new Sample[this->block_size];
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::register_children() noexcept
{
    this->register_child(frequency);
    this->register_child(q);
    this->register_child(gain);
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::set_sample_rate(
        Frequency const new_sample_rate
) noexcept {
    Filter<InputSignalProducerClass>::set_sample_rate(new_sample_rate);

    update_helper_variables();
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::set_block_size(
        Integer const new_block_size
) noexcept {
    if (new_block_size != this->block_size) {
        Filter<InputSignalProducerClass>::set_block_size(new_block_size);

        reallocate_buffers();
    }
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::reset() noexcept
{
    Filter<InputSignalProducerClass>::reset();

    for (Integer c = 0; c != this->channels; ++c) {
        x_n_m1[c] = x_n_m2[c] = y_n_m1[c] = y_n_m2[c] = 0.0;
    }
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::update_inaccuracy(
        Number const random_1,
        Number const random_2
) noexcept {
    freq_inaccuracy = Math::randomize(1.0, 0.5 * (inaccuracy_seed + random_1));
    q_inaccuracy = Math::randomize(1.0, 0.5 * (inaccuracy_seed + random_2));
}


#define JS80P_BF_CALL_INIT_FQ(func)                                     \
    do {                                                                \
        if (is_freq_inaccurate) {                                       \
            if (is_q_inaccurate) {                                      \
                is_no_op = func<true, true>(round, sample_count);       \
            } else {                                                    \
                is_no_op = func<true, false>(round, sample_count);      \
            }                                                           \
        } else {                                                        \
            if (is_q_inaccurate) {                                      \
                is_no_op = func<false, true>(round, sample_count);      \
            } else {                                                    \
                is_no_op = func<false, false>(round, sample_count);     \
            }                                                           \
        }                                                               \
    } while (false)


#define JS80P_BF_CALL_INIT_F(func)                                      \
    do {                                                                \
        if (is_freq_inaccurate) {                                       \
            is_no_op = func<true>(round, sample_count);                 \
        } else {                                                        \
            is_no_op = func<false>(round, sample_count);                \
        }                                                               \
    } while (false)


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
Sample const* const* BiquadFilter<InputSignalProducerClass, fixed_type>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    freq_inaccuracy_param_value = (
        freq_inaccuracy_param != NULL ? freq_inaccuracy_param->get_value() : 0.0
    );
    q_inaccuracy_param_value = (
        q_inaccuracy_param != NULL ? q_inaccuracy_param->get_value() : 0.0
    );

    bool const is_freq_inaccurate = freq_inaccuracy_param_value > 0.000001;
    bool const is_q_inaccurate = q_inaccuracy_param_value > 0.000001;

    if (shared_buffers == NULL) {
        can_use_shared_coefficients = false;
    } else {
        can_use_shared_coefficients = !is_freq_inaccurate && !is_q_inaccurate;
        b0_buffer = shared_buffers->b0_buffer;
        b1_buffer = shared_buffers->b1_buffer;
        b2_buffer = shared_buffers->b2_buffer;
        a1_buffer = shared_buffers->a1_buffer;
        a2_buffer = shared_buffers->a2_buffer;
    }

    Filter<InputSignalProducerClass>::initialize_rendering(
        round, sample_count
    );

    if (this->input.is_silent(round, sample_count)) {
        initialize_rendering_no_op(round, sample_count);

        return this->input_was_silent(round);
    }

    if (can_use_shared_coefficients && shared_buffers->round == round) {
        return initialize_rendering_with_shared_coefficients(
            round, sample_count
        );
    }

    bool is_no_op = false;

    is_silent_ = false;

    if constexpr (fixed_type == BiquadFilterFixedType::BFFT_CUSTOMIZABLE) {
        switch (type.get_value()) {
            case LOW_PASS:
                JS80P_BF_CALL_INIT_FQ(initialize_low_pass_rendering);
                break;

            case HIGH_PASS:
                JS80P_BF_CALL_INIT_FQ(initialize_high_pass_rendering);
                break;

            case BAND_PASS:
                JS80P_BF_CALL_INIT_FQ(initialize_band_pass_rendering);
                break;

            case NOTCH:
                JS80P_BF_CALL_INIT_FQ(initialize_notch_rendering);
                break;

            case PEAKING:
                JS80P_BF_CALL_INIT_FQ(initialize_peaking_rendering);
                break;

            case LOW_SHELF:
                JS80P_BF_CALL_INIT_F(initialize_low_shelf_rendering);
                break;

            case HIGH_SHELF:
                JS80P_BF_CALL_INIT_F(initialize_high_shelf_rendering);
                break;

            default:
                is_no_op = true;
                break;
        }
    } else if constexpr (fixed_type == BiquadFilterFixedType::BFFT_HIGH_PASS) {
        JS80P_BF_CALL_INIT_FQ(initialize_high_pass_rendering);
    } else if constexpr (fixed_type == BiquadFilterFixedType::BFFT_HIGH_SHELF) {
        JS80P_BF_CALL_INIT_F(initialize_high_shelf_rendering);
    } else {
        JS80P_ASSERT_NOT_REACHED();
        is_no_op = true;
    }

    if (can_use_shared_coefficients) {
        shared_buffers->round = round;
        shared_buffers->are_coefficients_constant = are_coefficients_constant;
        shared_buffers->is_no_op = is_no_op;
        shared_buffers->is_silent = is_silent_;
    } else if (shared_buffers != NULL) {
        shared_buffers->round = -1;
    }

    if (is_no_op) {
        return initialize_rendering_no_op(round, sample_count);
    }

    if constexpr (fixed_type != BiquadFilterFixedType::BFFT_HIGH_SHELF) {
        /*
        Cppcheck seems to ignore the constexpr if statement.
        */
        // cppcheck-suppress knownConditionTrueFalse
        if (JS80P_UNLIKELY(is_silent_)) {
            update_state_for_silent_round(round, sample_count);
        }
    }

    return NULL;
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
Sample const* const* BiquadFilter<InputSignalProducerClass, fixed_type>::initialize_rendering_no_op(
        Integer const round,
        Integer const sample_count
) noexcept {
    FloatParamS::produce_if_not_constant(frequency, round, sample_count);
    FloatParamS::produce_if_not_constant(q, round, sample_count);
    FloatParamS::produce_if_not_constant(gain, round, sample_count);

    update_state_for_no_op_round(sample_count);

    return this->input_buffer;
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::update_state_for_no_op_round(
        Integer const sample_count
) noexcept {
    if (JS80P_UNLIKELY(sample_count < 1)) {
        return;
    }

    Integer const channels = this->channels;

    if (JS80P_UNLIKELY(sample_count == 1)) {
        for (Integer c = 0; c != channels; ++c) {
            this->x_n_m2[c] = this->x_n_m1[c];
            this->y_n_m2[c] = this->y_n_m1[c];
            this->x_n_m1[c] = this->input_buffer[c][0];
            this->y_n_m1[c] = this->input_buffer[c][0];
        }
    } else {
        Integer const last_sample_index = sample_count - 1;
        Integer const penultimate_sample_index = sample_count - 2;

        for (Integer c = 0; c != channels; ++c) {
            this->x_n_m2[c] = this->input_buffer[c][penultimate_sample_index];
            this->y_n_m2[c] = this->input_buffer[c][penultimate_sample_index];
            this->x_n_m1[c] = this->input_buffer[c][last_sample_index];
            this->y_n_m1[c] = this->input_buffer[c][last_sample_index];
        }
    }
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::update_state_for_silent_round(
        Integer const round,
        Integer const sample_count
) noexcept {
    this->render_silence(round, 0, sample_count, this->get_buffer());
    this->mark_round_as_silent(round);

    if (JS80P_UNLIKELY(sample_count < 1)) {
        return;
    }

    Integer const channels = this->channels;

    if (JS80P_UNLIKELY(sample_count == 1)) {
        for (Integer c = 0; c != channels; ++c) {
            this->x_n_m2[c] = this->x_n_m1[c];
            this->y_n_m2[c] = this->y_n_m1[c];
            this->x_n_m1[c] = 0.0;
            this->y_n_m1[c] = 0.0;
        }
    } else {
        for (Integer c = 0; c != channels; ++c) {
            this->x_n_m2[c] = 0.0;
            this->y_n_m2[c] = 0.0;
            this->x_n_m1[c] = 0.0;
            this->y_n_m1[c] = 0.0;
        }
    }
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
Sample const* const* BiquadFilter<InputSignalProducerClass, fixed_type>::initialize_rendering_with_shared_coefficients(
        Integer const round,
        Integer const sample_count
) noexcept {
    if (shared_buffers->is_no_op) {
        return initialize_rendering_no_op(round, sample_count);
    }

    is_silent_ = shared_buffers->is_silent;
    are_coefficients_constant = (
        shared_buffers->are_coefficients_constant
    );

    if constexpr (fixed_type != BiquadFilterFixedType::BFFT_HIGH_SHELF) {
        if (JS80P_UNLIKELY(is_silent_)) {
            update_state_for_silent_round(round, sample_count);
        }
    }

    return NULL;
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate>
Number BiquadFilter<InputSignalProducerClass, fixed_type>::apply_freq_inaccuracy(
        Number const frequency_value
) const noexcept {
    if constexpr (is_freq_inaccurate) {
        return std::min(
            this->low_pass_no_op_frequency,
            Math::detune(
                frequency_value,
                (freq_inaccuracy * 2400.0 - 1200.0) * freq_inaccuracy_param_value
            )
        );
    } else {
        return frequency_value;
    }
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_q_inaccurate>
Number BiquadFilter<InputSignalProducerClass, fixed_type>::apply_q_inaccuracy(
        Number const q_value
) const noexcept {
    if constexpr (is_q_inaccurate) {
        return q_value * ((q_inaccuracy - 0.5) * q_inaccuracy_param_value + 1.0);
    } else {
        return q_value;
    }
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate, bool is_q_inaccurate>
bool BiquadFilter<InputSignalProducerClass, fixed_type>::initialize_low_pass_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Number const low_pass_no_op_frequency = this->low_pass_no_op_frequency;

    /* JS80P doesn't let the frequency go below 1.0 Hz */
    // Number const low_pass_silent_frequency = THRESHOLD;

    are_coefficients_constant = (
        frequency.is_constant_in_next_round(round, sample_count)
        && q.is_constant_in_next_round(round, sample_count)
    );
    can_use_shared_coefficients = (
        can_use_shared_coefficients
        && !frequency.is_polyphonic()
        && !q.is_polyphonic()
    );

    FloatParamS::produce_if_not_constant(gain, round, sample_count);

    if (are_coefficients_constant) {
        Number const frequency_value = frequency.get_value();

        if (frequency_value >= low_pass_no_op_frequency) {
            return true;
        }

        Number const q_value = q.get_value();

        frequency.skip_round(round, sample_count);
        q.skip_round(round, sample_count);

        /* JS80P doesn't let the frequency go below 1.0 Hz */
        // is_silent_ = frequency_value <= low_pass_silent_frequency;

        // if (JS80P_UNLIKELY(is_silent_)) {
            // return false;
        // }

        store_low_pass_coefficient_samples<is_freq_inaccurate, is_q_inaccurate>(
            0, frequency_value, q_value
        );

    } else {
        Sample const* frequency_buffer = (
            FloatParamS::produce<FloatParamS>(frequency, round, sample_count)[0]
        );
        Sample const* q_buffer = (
            FloatParamS::produce<FloatParamS>(q, round, sample_count)[0]
        );

        for (Integer i = 0; i != sample_count; ++i) {
            Number const frequency_value = frequency_buffer[i];

            if (frequency_value >= low_pass_no_op_frequency) {
                store_no_op_coefficient_samples(i);

                continue;
            }

            /* JS80P doesn't let the frequency go below 1.0 Hz */
            // if (JS80P_UNLIKELY(frequency_value <= low_pass_silent_frequency)) {
                // store_silent_coefficient_samples(i);

                // continue;
            // }

            store_low_pass_coefficient_samples<is_freq_inaccurate, is_q_inaccurate>(
                i, frequency_value, (Number)q_buffer[i]
            );
        }
    }

    return false;
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate, bool is_q_inaccurate>
void BiquadFilter<InputSignalProducerClass, fixed_type>::store_low_pass_coefficient_samples(
        Integer const index,
        Number const frequency_value,
        Number const q_value
) noexcept {
    Sample const w0 = w0_scale * apply_freq_inaccuracy<is_freq_inaccurate>(frequency_value);

    Sample sin_w0;
    Sample cos_w0;

    Math::sincos(w0, sin_w0, cos_w0);

    Sample const alpha_qdb = (
        0.5 * sin_w0 * Math::pow_10_inv(
            apply_q_inaccuracy<is_q_inaccurate>(q_value) * Constants::BIQUAD_FILTER_Q_SCALE
        )
    );

    Sample const b1 = 1.0 - cos_w0;
    Sample const b0_b2 = 0.5 * b1;

    /*
    The a1 and a2 coefficients are multiplied by -1 here so that rendering can
    be done with additions and multiplications only.
    */
    store_normalized_coefficient_samples(
        index, b0_b2, b1, b0_b2, 1.0 + alpha_qdb, 2.0 * cos_w0, alpha_qdb - 1.0
    );
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate, bool is_q_inaccurate>
bool BiquadFilter<InputSignalProducerClass, fixed_type>::initialize_high_pass_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    /* JS80P doesn't let the frequency go below 1.0 Hz */
    // Number const no_op_frequency = THRESHOLD;
    Frequency const silent_frequency = this->nyquist_frequency;

    are_coefficients_constant = (
        frequency.is_constant_in_next_round(round, sample_count)
        && q.is_constant_in_next_round(round, sample_count)
    );
    can_use_shared_coefficients = (
        can_use_shared_coefficients
        && !frequency.is_polyphonic()
        && !q.is_polyphonic()
    );

    FloatParamS::produce_if_not_constant(gain, round, sample_count);

    if (are_coefficients_constant) {
        Number const frequency_value = frequency.get_value();

        /* JS80P doesn't let the frequency go below 1.0 Hz */
        // if (frequency_value <= no_op_frequency) {
            // return true;
        // }

        Number const q_value = q.get_value();

        frequency.skip_round(round, sample_count);
        q.skip_round(round, sample_count);

        is_silent_ = frequency_value >= silent_frequency;

        if (JS80P_UNLIKELY(is_silent_)) {
            return false;
        }

        store_high_pass_coefficient_samples<is_freq_inaccurate, is_q_inaccurate>(
            0, frequency_value, q_value
        );

    } else {
        Sample const* frequency_buffer = (
            FloatParamS::produce<FloatParamS>(frequency, round, sample_count)[0]
        );
        Sample const* q_buffer = (
            FloatParamS::produce<FloatParamS>(q, round, sample_count)[0]
        );

        for (Integer i = 0; i != sample_count; ++i) {
            Number const frequency_value = frequency_buffer[i];

            /* JS80P doesn't let the frequency go below 1.0 Hz */
            // if (frequency_value <= no_op_frequency) {
                // store_no_op_coefficient_samples(i);

                // continue;
            // }

            if (JS80P_UNLIKELY(frequency_value >= silent_frequency)) {
                store_silent_coefficient_samples(i);

                continue;
            }

            store_high_pass_coefficient_samples<is_freq_inaccurate, is_q_inaccurate>(
                i, frequency_value, (Number)q_buffer[i]
            );
        }
    }

    return false;
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate, bool is_q_inaccurate>
void BiquadFilter<InputSignalProducerClass, fixed_type>::store_high_pass_coefficient_samples(
        Integer const index,
        Number const frequency_value,
        Number const q_value
) noexcept {
    Sample const w0 = w0_scale * apply_freq_inaccuracy<is_freq_inaccurate>(frequency_value);

    Sample sin_w0;
    Sample cos_w0;

    Math::sincos(w0, sin_w0, cos_w0);

    Sample const alpha_qdb = (
        0.5 * sin_w0 * Math::pow_10_inv(
            apply_q_inaccuracy<is_q_inaccurate>(q_value) * Constants::BIQUAD_FILTER_Q_SCALE
        )
    );

    Sample const b1 = -1.0 - cos_w0;
    Sample const b0_b2 = -0.5 * b1;

    /*
    The a1 and a2 coefficients are multiplied by -1 here so that rendering can
    be done with additions and multiplications only.
    */
    store_normalized_coefficient_samples(
        index, b0_b2, b1, b0_b2, 1.0 + alpha_qdb, 2.0 * cos_w0, alpha_qdb - 1.0
    );
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate, bool is_q_inaccurate>
bool BiquadFilter<InputSignalProducerClass, fixed_type>::initialize_band_pass_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Number const band_pass_silent_frequency = low_pass_no_op_frequency;

    are_coefficients_constant = (
        frequency.is_constant_in_next_round(round, sample_count)
        && q.is_constant_in_next_round(round, sample_count)
    );
    can_use_shared_coefficients = (
        can_use_shared_coefficients
        && !frequency.is_polyphonic()
        && !q.is_polyphonic()
    );

    FloatParamS::produce_if_not_constant(gain, round, sample_count);

    if (are_coefficients_constant) {
        Number const frequency_value = frequency.get_value();
        Number const q_value = q.get_value();

        if (q_value < THRESHOLD) {
            return true;
        }

        is_silent_ = frequency_value >= band_pass_silent_frequency;

        if (JS80P_UNLIKELY(is_silent_)) {
            return false;
        }

        frequency.skip_round(round, sample_count);
        q.skip_round(round, sample_count);

        store_band_pass_coefficient_samples<is_freq_inaccurate, is_q_inaccurate>(
            0, frequency_value, q_value
        );

    } else {
        Sample const* frequency_buffer = (
            FloatParamS::produce<FloatParamS>(frequency, round, sample_count)[0]
        );
        Sample const* q_buffer = (
            FloatParamS::produce<FloatParamS>(q, round, sample_count)[0]
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

            store_band_pass_coefficient_samples<is_freq_inaccurate, is_q_inaccurate>(
                i, frequency_value, q_value
            );
        }
    }

    return false;
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate, bool is_q_inaccurate>
void BiquadFilter<InputSignalProducerClass, fixed_type>::store_band_pass_coefficient_samples(
        Integer const index,
        Number const frequency_value,
        Number const q_value
) noexcept {
    Sample const w0 = w0_scale * apply_freq_inaccuracy<is_freq_inaccurate>(frequency_value);

    Sample sin_w0;
    Sample cos_w0;

    Math::sincos(w0, sin_w0, cos_w0);

    Sample q;

    if constexpr (is_q_inaccurate) {
        q = std::max(THRESHOLD, apply_q_inaccuracy<true>(q_value));
    } else {
        q = q_value;

        /* The caller should have handled low Q values. */
        JS80P_ASSERT(q >= THRESHOLD);
    }

    Sample const alpha_q = 0.5 * sin_w0 / q;

    /*
    The a1 and a2 coefficients are multiplied by -1 here so that rendering can
    be done with additions and multiplications only.
    */
    store_normalized_coefficient_samples(
        index, alpha_q, 0.0, -alpha_q, 1.0 + alpha_q, 2.0 * cos_w0, alpha_q - 1.0
    );
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate, bool is_q_inaccurate>
bool BiquadFilter<InputSignalProducerClass, fixed_type>::initialize_notch_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Number const notch_no_op_frequency = low_pass_no_op_frequency;

    are_coefficients_constant = (
        frequency.is_constant_in_next_round(round, sample_count)
        && q.is_constant_in_next_round(round, sample_count)
    );
    can_use_shared_coefficients = (
        can_use_shared_coefficients
        && !frequency.is_polyphonic()
        && !q.is_polyphonic()
    );

    FloatParamS::produce_if_not_constant(gain, round, sample_count);

    if (are_coefficients_constant) {
        Number const frequency_value = frequency.get_value();
        Number const q_value = q.get_value();

        if (frequency_value >= notch_no_op_frequency) {
            return true;
        }

        is_silent_ = q_value < THRESHOLD;

        if (JS80P_UNLIKELY(is_silent_)) {
            return false;
        }

        frequency.skip_round(round, sample_count);
        q.skip_round(round, sample_count);

        store_notch_coefficient_samples<is_freq_inaccurate, is_q_inaccurate>(
            0, frequency_value, q_value
        );

    } else {
        Sample const* frequency_buffer = (
            FloatParamS::produce<FloatParamS>(frequency, round, sample_count)[0]
        );
        Sample const* q_buffer = (
            FloatParamS::produce<FloatParamS>(q, round, sample_count)[0]
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

            store_notch_coefficient_samples<is_freq_inaccurate, is_q_inaccurate>(
                i, frequency_value, q_value
            );
        }
    }

    return false;
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate, bool is_q_inaccurate>
void BiquadFilter<InputSignalProducerClass, fixed_type>::store_notch_coefficient_samples(
        Integer const index,
        Number const frequency_value,
        Number const q_value
) noexcept {
    Sample const w0 = w0_scale * apply_freq_inaccuracy<is_freq_inaccurate>(frequency_value);

    Sample sin_w0;
    Sample cos_w0;

    Math::sincos(w0, sin_w0, cos_w0);

    Sample q;

    if constexpr (is_q_inaccurate) {
        q = std::max(THRESHOLD, apply_q_inaccuracy<true>(q_value));
    } else {
        q = q_value;

        /* The caller should have handled low Q values. */
        JS80P_ASSERT(q >= THRESHOLD);
    }

    Sample const alpha_q = 0.5 * sin_w0 / q;

    Sample const b1_a1 = -2.0 * cos_w0;

    /*
    The a1 and a2 coefficients are multiplied by -1 here so that rendering can
    be done with additions and multiplications only.
    */
    store_normalized_coefficient_samples(
        index, 1.0, b1_a1, 1.0, 1.0 + alpha_q, - b1_a1, alpha_q - 1.0
    );
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate, bool is_q_inaccurate>
bool BiquadFilter<InputSignalProducerClass, fixed_type>::initialize_peaking_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Number const peaking_no_op_frequency = low_pass_no_op_frequency;

    are_coefficients_constant = (
        frequency.is_constant_in_next_round(round, sample_count)
        && q.is_constant_in_next_round(round, sample_count)
        && gain.is_constant_in_next_round(round, sample_count)
    );
    can_use_shared_coefficients = (
        can_use_shared_coefficients
        && !frequency.is_polyphonic()
        && !q.is_polyphonic()
        && !gain.is_polyphonic()
    );

    if (are_coefficients_constant) {
        Number const frequency_value = frequency.get_value();
        Number const gain_value = gain.get_value();

        if (
                Math::is_abs_small(gain_value, THRESHOLD)
                || frequency_value >= peaking_no_op_frequency
        ) {
            return true;
        }

        Number const q_value = q.get_value();

        frequency.skip_round(round, sample_count);
        q.skip_round(round, sample_count);
        gain.skip_round(round, sample_count);

        if (q_value >= THRESHOLD) {
            store_peaking_coefficient_samples<is_freq_inaccurate, is_q_inaccurate>(
                0, frequency_value, q_value, gain_value
            );
        } else {
            store_gain_coefficient_samples(0, gain_value);
        }

    } else {
        Sample const* frequency_buffer = (
            FloatParamS::produce<FloatParamS>(frequency, round, sample_count)[0]
        );
        Sample const* q_buffer = (
            FloatParamS::produce<FloatParamS>(q, round, sample_count)[0]
        );
        Sample const* gain_buffer = (
            FloatParamS::produce<FloatParamS>(gain, round, sample_count)[0]
        );

        for (Integer i = 0; i != sample_count; ++i) {
            Number const frequency_value = (Number)frequency_buffer[i];
            Number const gain_value = (Number)gain_buffer[i];

            if (
                    Math::is_abs_small(gain_value, THRESHOLD)
                    || frequency_value >= peaking_no_op_frequency
            ) {
                store_no_op_coefficient_samples(i);

                continue;
            }

            Number const q_value = (Number)q_buffer[i];

            if (q_value >= THRESHOLD) {
                store_peaking_coefficient_samples<is_freq_inaccurate, is_q_inaccurate>(
                    i, frequency_value, q_value, gain_value
                );
            } else {
                store_gain_coefficient_samples(i, gain_value);
            }
        }
    }

    return false;
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate, bool is_q_inaccurate>
void BiquadFilter<InputSignalProducerClass, fixed_type>::store_peaking_coefficient_samples(
        Integer const index,
        Number const frequency_value,
        Number const q_value,
        Number const gain_value
) noexcept {
    Sample const w0 = w0_scale * apply_freq_inaccuracy<is_freq_inaccurate>(frequency_value);

    Sample sin_w0;
    Sample cos_w0;

    Math::sincos(w0, sin_w0, cos_w0);

    Sample const b1_a1 = -2.0 * cos_w0;

    Sample q;

    if constexpr (is_q_inaccurate) {
        q = std::max(THRESHOLD, apply_q_inaccuracy<true>(q_value));
    } else {
        q = q_value;

        /* The caller should have handled low Q values. */
        JS80P_ASSERT(q >= THRESHOLD);
    }

    Sample const alpha_q = 0.5 * sin_w0 / q;

    Sample const a = Math::pow_10(
        (Sample)gain_value * Constants::BIQUAD_FILTER_GAIN_SCALE
    );

    Sample const alpha_q_times_a = alpha_q * a;
    Sample const alpha_q_over_a = alpha_q / a;

    /*
    The a1 and a2 coefficients are multiplied by -1 here so that rendering can
    be done with additions and multiplications only.
    */
    store_normalized_coefficient_samples(
        index,
        1.0 + alpha_q_times_a,
        b1_a1,
        1.0 - alpha_q_times_a,
        1.0 + alpha_q_over_a,
        - b1_a1,
        alpha_q_over_a - 1.0
    );
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate>
bool BiquadFilter<InputSignalProducerClass, fixed_type>::initialize_low_shelf_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Frequency const becomes_gain_frequency = this->nyquist_frequency;
    /* JS80P doesn't let the frequency go below 1.0 Hz */
    // Number const no_op_frequency = THRESHOLD;

    are_coefficients_constant = (
        frequency.is_constant_in_next_round(round, sample_count)
        && gain.is_constant_in_next_round(round, sample_count)
    );
    can_use_shared_coefficients = (
        can_use_shared_coefficients
        && !frequency.is_polyphonic()
        && !gain.is_polyphonic()
    );

    FloatParamS::produce_if_not_constant(q, round, sample_count);

    if (are_coefficients_constant) {
        Number const frequency_value = frequency.get_value();

        /* JS80P doesn't let the frequency go below 1.0 Hz */
        // if (frequency_value <= no_op_frequency) {
            // return true;
        // }

        Number const gain_value = gain.get_value();

        frequency.skip_round(round, sample_count);
        gain.skip_round(round, sample_count);

        if (JS80P_UNLIKELY(frequency_value >= becomes_gain_frequency)) {
            store_gain_coefficient_samples(0, gain_value);

            return false;
        }

        store_low_shelf_coefficient_samples<is_freq_inaccurate>(
            0, frequency_value, gain_value
        );

    } else {
        Sample const* frequency_buffer = (
            FloatParamS::produce<FloatParamS>(frequency, round, sample_count)[0]
        );
        Sample const* gain_buffer = (
            FloatParamS::produce<FloatParamS>(gain, round, sample_count)[0]
        );

        for (Integer i = 0; i != sample_count; ++i) {
            Number const frequency_value = (Number)frequency_buffer[i];

            /* JS80P doesn't let the frequency go below 1.0 Hz */
            // if (frequency_value <= no_op_frequency) {
                // store_no_op_coefficient_samples(i);

                // continue;
            // }

            Number const gain_value = (Number)gain_buffer[i];

            if (JS80P_UNLIKELY(frequency_value >= becomes_gain_frequency)) {
                store_gain_coefficient_samples(i, gain_value);

                continue;
            }

            store_low_shelf_coefficient_samples<is_freq_inaccurate>(
                i, frequency_value, gain_value
            );
        }
    }

    return false;
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate>
void BiquadFilter<InputSignalProducerClass, fixed_type>::store_low_shelf_coefficient_samples(
        Integer const index,
        Number const frequency_value,
        Number const gain_value
) noexcept {
    Sample const a = Math::pow_10(
        (Sample)gain_value * Constants::BIQUAD_FILTER_GAIN_SCALE
    );
    Sample const a_p_1 = a + 1.0;
    Sample const a_m_1 = a - 1.0;

    /* Recalculating the power seems to be slightly faster than std::sqrt(a). */
    Sample const a_sqrt = Math::pow_10((Sample)gain_value * GAIN_SCALE_HALF);

    Sample const w0 = w0_scale * apply_freq_inaccuracy<is_freq_inaccurate>(frequency_value);

    Sample sin_w0;
    Sample cos_w0;

    Math::sincos(w0, sin_w0, cos_w0);

    Sample const a_m_1_cos_w0 = a_m_1 * cos_w0;
    Sample const a_p_1_cos_w0 = a_p_1 * cos_w0;

    /*
    S = 1 makes sqrt((A + 1/A) * (1/S - 1) + 2) collapse to just sqrt(2). Also,
    alpha_s is always multiplied by 2, which cancels dividing the sine by 2.
    */
    Sample const alpha_s_double_a_sqrt = sin_w0 * FREQUENCY_SINE_SCALE * a_sqrt;

    /*
    The a1 and a2 coefficients are multiplied by -1 here so that rendering can
    be done with additions and multiplications only.
    */
    store_normalized_coefficient_samples(
        index,
        a * (a_p_1 - a_m_1_cos_w0 + alpha_s_double_a_sqrt),
        2.0 * a * (a_m_1 - a_p_1_cos_w0),
        a * (a_p_1 - a_m_1_cos_w0 - alpha_s_double_a_sqrt),
        a_p_1 + a_m_1_cos_w0 + alpha_s_double_a_sqrt,
        2.0 * (a_m_1 + a_p_1_cos_w0),
        alpha_s_double_a_sqrt - a_p_1 - a_m_1_cos_w0
    );
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate>
bool BiquadFilter<InputSignalProducerClass, fixed_type>::initialize_high_shelf_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Number const high_shelf_no_op_frequency = low_pass_no_op_frequency;
    /* JS80P doesn't let the frequency go below 1.0 Hz */
    // Number const becomes_gain_frequency = THRESHOLD;

    are_coefficients_constant = (
        frequency.is_constant_in_next_round(round, sample_count)
        && gain.is_constant_in_next_round(round, sample_count)
    );
    can_use_shared_coefficients = (
        can_use_shared_coefficients
        && !frequency.is_polyphonic()
        && !gain.is_polyphonic()
    );

    FloatParamS::produce_if_not_constant(q, round, sample_count);

    if (are_coefficients_constant) {
        Number const frequency_value = frequency.get_value();

        if (frequency_value >= high_shelf_no_op_frequency) {
            return true;
        }

        Number const gain_value = gain.get_value();

        frequency.skip_round(round, sample_count);
        gain.skip_round(round, sample_count);

        /* JS80P doesn't let the frequency go below 1.0 Hz */
        // if (JS80P_UNLIKELY(frequency_value <= becomes_gain_frequency)) {
            // store_gain_coefficient_samples(0, gain_value);

            // return false;
        // }

        store_high_shelf_coefficient_samples<is_freq_inaccurate>(
            0, frequency_value, gain_value
        );

    } else {
        Sample const* frequency_buffer = (
            FloatParamS::produce<FloatParamS>(frequency, round, sample_count)[0]
        );
        Sample const* gain_buffer = (
            FloatParamS::produce<FloatParamS>(gain, round, sample_count)[0]
        );

        for (Integer i = 0; i != sample_count; ++i) {
            Number const frequency_value = frequency_buffer[i];

            if (frequency_value >= high_shelf_no_op_frequency) {
                store_no_op_coefficient_samples(i);

                continue;
            }

            Number const gain_value = gain_buffer[i];

            /* JS80P doesn't let the frequency go below 1.0 Hz */
            // if (JS80P_UNLIKELY(frequency_value <= becomes_gain_frequency)) {
                // store_gain_coefficient_samples(i, gain_value);

                // continue;
            // }

            store_high_shelf_coefficient_samples<is_freq_inaccurate>(
                i, frequency_value, gain_value
            );
        }
    }

    return false;
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
template<bool is_freq_inaccurate>
void BiquadFilter<InputSignalProducerClass, fixed_type>::store_high_shelf_coefficient_samples(
        Integer const index,
        Number const frequency_value,
        Number const gain_value
) noexcept {
    Sample const a = Math::pow_10(
        (Sample)gain_value * Constants::BIQUAD_FILTER_GAIN_SCALE
    );
    Sample const a_p_1 = a + 1.0;
    Sample const a_m_1 = a - 1.0;

    /* Recalculating the power seems to be slightly faster than std::sqrt(a). */
    Sample const a_sqrt = Math::pow_10((Sample)gain_value * GAIN_SCALE_HALF);

    Sample const w0 = w0_scale * apply_freq_inaccuracy<is_freq_inaccurate>(frequency_value);

    Sample sin_w0;
    Sample cos_w0;

    Math::sincos(w0, sin_w0, cos_w0);

    Sample const a_m_1_cos_w0 = a_m_1 * cos_w0;
    Sample const a_p_1_cos_w0 = a_p_1 * cos_w0;

    /*
    S = 1 makes sqrt((A + 1/A) * (1/S - 1) + 2) collapse to just sqrt(2). Also,
    alpha_s is always multiplied by 2, which cancels dividing the sine by 2.
    */
    Sample const alpha_s_double_a_sqrt = (
        sin_w0 * FREQUENCY_SINE_SCALE * a_sqrt
    );

    /*
    The a1 and a2 coefficients are multiplied by -1 here so that rendering can
    be done with additions and multiplications only.
    */
    store_normalized_coefficient_samples(
        index,
        a * (a_p_1 + a_m_1_cos_w0 + alpha_s_double_a_sqrt),
        -2.0 * a * (a_m_1 + a_p_1_cos_w0),
        a * (a_p_1 + a_m_1_cos_w0 - alpha_s_double_a_sqrt),
        a_p_1 - a_m_1_cos_w0 + alpha_s_double_a_sqrt,
        -2.0 * (a_m_1 - a_p_1_cos_w0),
        a_m_1_cos_w0 + alpha_s_double_a_sqrt - a_p_1
    );
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::store_gain_coefficient_samples(
        Integer const index,
        Number const gain_value
) noexcept {
    store_normalized_coefficient_samples(
        index,
        (Sample)Math::db_to_linear(gain_value),
        0.0,
        0.0,
        1.0,
        0.0,
        0.0
    );
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::store_normalized_coefficient_samples(
        Integer const index,
        Sample const b0,
        Sample const b1,
        Sample const b2,
        Sample const a0,
        Sample const a1,
        Sample const a2
) noexcept {
    Sample const a0_inv = 1.0 / a0;

    b0_buffer[index] = b0 * a0_inv;
    b1_buffer[index] = b1 * a0_inv;
    b2_buffer[index] = b2 * a0_inv;
    a1_buffer[index] = a1 * a0_inv;
    a2_buffer[index] = a2 * a0_inv;
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::store_no_op_coefficient_samples(
        Integer const index
) noexcept {
    b0_buffer[index] = 1.0;
    b1_buffer[index] =
        b2_buffer[index] =
        a1_buffer[index] =
        a2_buffer[index] = 0.0;
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::store_silent_coefficient_samples(
        Integer const index
) noexcept {
    b0_buffer[index] =
        b1_buffer[index] =
        b2_buffer[index] =
        a1_buffer[index] =
        a2_buffer[index] = 0.0;
}


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
void BiquadFilter<InputSignalProducerClass, fixed_type>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    if constexpr (fixed_type != BiquadFilterFixedType::BFFT_HIGH_SHELF) {
        if (JS80P_UNLIKELY(is_silent_)) {
            this->render_silence(
                round, first_sample_index, last_sample_index, buffer
            );

            return;
        }
    }

    Integer const channels = this->channels;
    Sample const* const* const input_buffer = this->input_buffer;

    if (are_coefficients_constant) {
        Sample b0, b1, b2, a1, a2;

        if (can_use_shared_coefficients) {
            b0 = shared_buffers->b0_buffer[0];
            b1 = shared_buffers->b1_buffer[0];
            b2 = shared_buffers->b2_buffer[0];
            a1 = shared_buffers->a1_buffer[0];
            a2 = shared_buffers->a2_buffer[0];
        } else {
            b0 = b0_buffer[0];
            b1 = b1_buffer[0];
            b2 = b2_buffer[0];
            a1 = a1_buffer[0];
            a2 = a2_buffer[0];
        }

        for (Integer c = 0; c != channels; ++c) {
            Sample x_n_m1 = this->x_n_m1[c];
            Sample x_n_m2 = this->x_n_m2[c];
            Sample y_n_m1 = this->y_n_m1[c];
            Sample y_n_m2 = this->y_n_m2[c];

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const x_n = input_buffer[c][i];
                Sample const y_n = (
                    b0 * x_n + b1 * x_n_m1 + b2 * x_n_m2
                    + a1 * y_n_m1 + a2 * y_n_m2
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

    Sample const* b0;
    Sample const* b1;
    Sample const* b2;
    Sample const* a1;
    Sample const* a2;

    if (can_use_shared_coefficients) {
        b0 = shared_buffers->b0_buffer;
        b1 = shared_buffers->b1_buffer;
        b2 = shared_buffers->b2_buffer;
        a1 = shared_buffers->a1_buffer;
        a2 = shared_buffers->a2_buffer;
    } else {
        b0 = b0_buffer;
        b1 = b1_buffer;
        b2 = b2_buffer;
        a1 = a1_buffer;
        a2 = a2_buffer;
    }

    for (Integer c = 0; c != channels; ++c) {
        Sample x_n_m1 = this->x_n_m1[c];
        Sample x_n_m2 = this->x_n_m2[c];
        Sample y_n_m1 = this->y_n_m1[c];
        Sample y_n_m2 = this->y_n_m2[c];

        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            Sample const x_n = input_buffer[c][i];
            Sample const y_n = (
                b0[i] * x_n + b1[i] * x_n_m1 + b2[i] * x_n_m2
                + a1[i] * y_n_m1 + a2[i] * y_n_m2
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
