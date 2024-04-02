/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2022, 2023, 2024  Attila M. Magyar
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

#ifndef JS80P__DSP__MATH_CPP
#define JS80P__DSP__MATH_CPP

#include <limits>

#include "dsp/math.hpp"


namespace JS80P
{

Math const Math::math;


Math::Math() noexcept
{
    init_sines();
    init_randoms();
    init_distortion();
    init_log_biquad_filter_freq();
    init_log_biquad_filter_q();
    init_linear_to_db();
    init_envelope_shapes();
}


void Math::init_sines() noexcept
{
    constexpr Number scale = PI_DOUBLE / (Number)SIN_TABLE_SIZE;

    for (int i = 0; i != SIN_TABLE_SIZE; ++i) {
        Number const x = (Number)i * scale;
        sines[i] = std::sin(x);
        cosines[i] = std::cos(x);
    }
}


void Math::init_randoms() noexcept
{
    /*
    https://en.wikipedia.org/wiki/Multiply-with-carry_pseudorandom_number_generator
    */

    constexpr unsigned int seed = 0x1705;
    constexpr Number scale = 1.0 / 65536.0;

    unsigned int x = seed;
    unsigned int c = (((~seed) >> 3) ^ 0x3cf5) & 0xffff;

    for (int i = 0; i != RANDOMS; ++i) {
        x = 32718 * x + c;
        c = x >> 16;
        x = x & 0xffff;

        randoms[i] = (Number)x * scale;
        randoms_centered_lfo[i] = randoms[i] - 0.5;
    }
}


void Math::init_distortion() noexcept
{
    Number const max_inv = 1.0 / (Number)DISTORTION_TABLE_MAX_INDEX;

    for (int i = 0; i != DISTORTION_TABLE_SIZE; ++i) {
        Number const x = 2.0 * ((Number)i * max_inv) - 1.0;
        distortion_centered_lfo[i] = std::tanh(8.0 * x) * 0.5;
        distortion[i] = distortion_centered_lfo[i] + 0.5;
    }
}


void Math::init_log_table(
        Number* const table,
        int const max_index,
        Number const max_index_inv,
        Number const min,
        Number const max,
        Number const correction_scale,
        Number(*ratio_to_exact_value)(Number const ratio)
) noexcept {
    Number prev_idx = 0.0;
    Number prev = min;

    table[0] = prev;

    for (int i = 1; i != max_index; ++i) {
        Number const current_idx = (Number)i;
        Number const ratio = current_idx * max_index_inv;
        Number const current = ratio_to_exact_value(ratio);

        Number const correction = correction_scale * (
            (current + prev) * 0.5
            - ratio_to_exact_value((prev_idx + 0.5) * max_index_inv)
        );

        table[i] = current - correction;
        prev = current;
        prev_idx = current_idx;
    }

    table[max_index] = max;
}


void Math::init_log_biquad_filter_freq() noexcept
{
    init_log_table(
        log_biquad_filter_freq,
        LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX_INV,
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        0.6683103012188,
        [](Number const ratio) -> Number {
            return ratio_to_exact_log_biquad_filter_frequency(ratio);
        }
    );
}


Number Math::ratio_to_exact_log_biquad_filter_frequency(
        Number const ratio
) noexcept {
    return ratio_to_exact_log_value(
        ratio,
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX
    );
}


void Math::init_log_biquad_filter_q() noexcept
{
    init_log_table(
        log_biquad_filter_q,
        LOG_BIQUAD_FILTER_Q_TABLE_MAX_INDEX,
        LOG_BIQUAD_FILTER_Q_TABLE_MAX_INDEX_INV,
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        0.66898329211,
        [](Number const ratio) -> Number {
            return ratio_to_exact_log_biquad_filter_q(ratio);
        }
    );
}


void Math::init_envelope_shapes() noexcept
{
    constexpr Number end = (Number)ENVELOPE_SHAPE_TABLE_MAX_INDEX;

    envelope_shapes[0][0] = 0.0;
    envelope_shapes[1][0] = 0.0;
    envelope_shapes[2][0] = 0.0;
    envelope_shapes[3][0] = 0.0;
    envelope_shapes[4][0] = 0.0;
    envelope_shapes[5][0] = 0.0;
    envelope_shapes[6][0] = 0.0;
    envelope_shapes[7][0] = 0.0;
    envelope_shapes[8][0] = 0.0;
    envelope_shapes[9][0] = 0.0;
    envelope_shapes[10][0] = 0.0;
    envelope_shapes[11][0] = 0.0;

    for (int i = 1; i != ENVELOPE_SHAPE_TABLE_MAX_INDEX; ++i) {
        Number const ratio = (Number)i / end;

        envelope_shapes[0][i] = env_shape_smooth_smooth(ratio);
        envelope_shapes[1][i] = env_shape_smooth_smooth_steep(ratio);
        envelope_shapes[2][i] = env_shape_smooth_smooth_steeper(ratio);
        envelope_shapes[3][i] = env_shape_smooth_sharp(ratio);
        envelope_shapes[4][i] = env_shape_smooth_sharp_steep(ratio);
        envelope_shapes[5][i] = env_shape_smooth_sharp_steeper(ratio);
        envelope_shapes[6][i] = env_shape_sharp_smooth(ratio);
        envelope_shapes[7][i] = env_shape_sharp_smooth_steep(ratio);
        envelope_shapes[8][i] = env_shape_sharp_smooth_steeper(ratio);
        envelope_shapes[9][i] = env_shape_sharp_sharp(ratio);
        envelope_shapes[10][i] = env_shape_sharp_sharp_steep(ratio);
        envelope_shapes[11][i] = env_shape_sharp_sharp_steeper(ratio);
    }

    envelope_shapes[0][ENVELOPE_SHAPE_TABLE_MAX_INDEX] = 1.0;
    envelope_shapes[1][ENVELOPE_SHAPE_TABLE_MAX_INDEX] = 1.0;
    envelope_shapes[2][ENVELOPE_SHAPE_TABLE_MAX_INDEX] = 1.0;
    envelope_shapes[3][ENVELOPE_SHAPE_TABLE_MAX_INDEX] = 1.0;
    envelope_shapes[4][ENVELOPE_SHAPE_TABLE_MAX_INDEX] = 1.0;
    envelope_shapes[5][ENVELOPE_SHAPE_TABLE_MAX_INDEX] = 1.0;
    envelope_shapes[6][ENVELOPE_SHAPE_TABLE_MAX_INDEX] = 1.0;
    envelope_shapes[7][ENVELOPE_SHAPE_TABLE_MAX_INDEX] = 1.0;
    envelope_shapes[8][ENVELOPE_SHAPE_TABLE_MAX_INDEX] = 1.0;
    envelope_shapes[9][ENVELOPE_SHAPE_TABLE_MAX_INDEX] = 1.0;
    envelope_shapes[10][ENVELOPE_SHAPE_TABLE_MAX_INDEX] = 1.0;
    envelope_shapes[11][ENVELOPE_SHAPE_TABLE_MAX_INDEX] = 1.0;
}


Number Math::env_shape_smooth_smooth(Number const x) noexcept
{
    /*
    Antiderivative of 6 * (x - x ^ 2).

    Construction: the idea is to map [0, 1] to itself with a smooth function f
    for which all of the following properties hold:

     1. f(0) = 0.

     2. f(1) = 1.

     3. f'(0) = f'(1) = 0 (ie. connect smoothly to the constant 0 and 1
        functions on the respective ends).

     4. f'(x) > 0 for all x where 0 < x < 1.

     5. f'(x) = f'(1 - x) for all x where 0 < x < 1.

     6. f''(1 / 2) = 0.

     7. f''(x) > 0 for all x where 0 <= x < 1 / 2.

     8. f''(x) < 0 for all x where 1 / 2 < x <= 1.

    The 1 / 4 - (x - 1 / 2) ^ 2 function is almost perfect for the role of
    f', but it needs to be scaled by 6 in order to make f fit the bill.
    After expanding and simplifying, we get 6 * (x - x ^ 2).

    See also: https://en.wikipedia.org/wiki/Horner%27s_method
    */

    return ((-2.0 * x + 3.0) * x) * x;
}


Number Math::env_shape_smooth_smooth_steep(Number const x) noexcept
{
    /*
    Antiderivative of 30 * ((x - x ^ 2) ^ 2).
    Same idea as in env_shape_smooth_smooth() but steeper.
    */

    return ((((6.0 * x - 15.0) * x + 10.0) * x) * x) * x;
}


Number Math::env_shape_smooth_smooth_steeper(Number const x) noexcept
{
    /*
    Antiderivative of 2772 * ((x - x ^ 2) ^ 5).
    Same idea as in env_shape_smooth_smooth() but a lot steeper.
    */

    constexpr Number a = -252.0;
    constexpr Number b = 1386.0;
    constexpr Number c = 3080.0;
    constexpr Number d = 3465.0;
    constexpr Number e = 1980.0;
    constexpr Number f = 462.0;

    return (
        ((((((((((a * x + b) * x - c) * x + d) * x - e) * x + f) * x) * x) * x) * x) * x) * x
    );
}


Number Math::env_shape_sharp_sharp(Number const x) noexcept
{
    /*
    Antiderivative of (2 * x - 1) ^ 2.
    Same idea as in env_shape_smooth_smooth() but the derivative at the
    endpoints is positive, and 0 in the middle.
    */

    return ((4.0 * x - 6.0) * x + 3.0) * x;
}


Number Math::env_shape_sharp_sharp_steep(Number const x) noexcept
{
    /*
    Antiderivative of ((2 * x - 1) ^ 2) ^ 2.
    Same idea as in env_shape_sharp_sharp() but steeper near the endpoints.
    */

    return ((((16.0 * x - 40.0) * x + 40.0) * x - 20.0) * x + 5.0) * x;
}


Number Math::env_shape_sharp_sharp_steeper(Number const x) noexcept
{
    /*
    Antiderivative of ((2 * x - 1) ^ 2) ^ 5.
    Same idea as in env_shape_sharp_sharp() but even more steep near the
    endpoints.
    */

    constexpr Number a = 1024.0;
    constexpr Number b = 5632.0;
    constexpr Number c = 14080.0;
    constexpr Number d = 21120.0;
    constexpr Number e = 21120.0;
    constexpr Number f = 14784.0;
    constexpr Number g = 7392.0;
    constexpr Number h = 2640.0;
    constexpr Number i = 660.0;
    constexpr Number j = 110.0;
    constexpr Number k = 11.0;

    return ((((((((((a * x - b) * x + c) * x - d) * x + e) * x - f) * x + g) * x - h) * x + i) * x - j) * x + k) * x;
}


Number Math::env_shape_smooth_sharp(Number const x) noexcept
{
    return std::pow(x, 2.0);
}


Number Math::env_shape_smooth_sharp_steep(Number const x) noexcept
{
    return std::pow(x, 3.0);
}


Number Math::env_shape_smooth_sharp_steeper(Number const x) noexcept
{
    return std::pow(x, 5.0);
}


Number Math::env_shape_sharp_smooth(Number const x) noexcept
{
    return x * (1.0 - std::log(x + 0.001)) / (1.0 - log(1.001));
}


Number Math::env_shape_sharp_smooth_steep(Number const x) noexcept
{
    return std::pow(x * (1.0 - std::log(x + 0.001)) / (1.0 - log(1.001)), 2.0 / 3.0);
}


Number Math::env_shape_sharp_smooth_steeper(Number const x) noexcept
{
    return std::pow(x * (1.0 - std::log(x + 0.001)) / (1.0 - log(1.001)), 1.0 / 3.0);
}


Number Math::ratio_to_exact_log_biquad_filter_q(Number const ratio) noexcept
{
    return ratio_to_exact_log_value(
        ratio,
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        LOG_BIQUAD_FILTER_Q_VALUE_OFFSET
    );
}


Number Math::ratio_to_exact_log_value(
        Number const ratio,
        Number const min,
        Number const max,
        Number const offset
) noexcept {
    Number const min_with_offset = min + offset;
    Number const max_with_offset = max + offset;
    Number const range = max_with_offset / min_with_offset;

    return min_with_offset * std::pow(range, ratio) - offset;
}


void Math::init_linear_to_db() noexcept
{
    constexpr Number scale = LINEAR_TO_DB_MAX / (Number)LINEAR_TO_DB_TABLE_SIZE;

    for (int i = 0; i != LINEAR_TO_DB_TABLE_SIZE; ++i) {
        Number const x = LINEAR_TO_DB_MIN + scale * (Number)i;

        linear_to_dbs[i] = LINEAR_TO_DB_GAIN_SCALE * std::log10(x);
    }
}


bool Math::is_abs_small(Number const x, Number const threshold) noexcept
{
    return std::fabs(x) < threshold;
}


bool Math::is_close(Number const a, Number const b, Number const threshold) noexcept
{
    return is_abs_small(a - b, threshold);
}


Number Math::sin(Number const x) noexcept
{
    return math.sin_impl(x);
}


Number Math::cos(Number const x) noexcept
{
    return math.cos_impl(x);
}


Number Math::sin_impl(Number const x) const noexcept
{
    return math.trig(sines, x);
}


Number Math::cos_impl(Number const x) const noexcept
{
    return math.trig(cosines, x);
}


Number Math::trig(Number const* const table, Number const x) const noexcept
{
    Number const index = x * SINE_SCALE;
    Number const after_weight = index - std::floor(index);
    int const before_index = ((int)index) & SIN_TABLE_INDEX_MASK;
    int const after_index = (before_index + 1) & SIN_TABLE_INDEX_MASK;

    return combine(after_weight, table[after_index], table[before_index]);
}


void Math::sincos(Number const x, Number& sin, Number& cos) noexcept
{
    math.sincos_impl(x, sin, cos);
}


void Math::sincos_impl(Number const x, Number& sin, Number& cos) const noexcept
{
    Number const index = x * SINE_SCALE;
    Number const after_weight = index - std::floor(index);
    int const before_index = ((int)index) & SIN_TABLE_INDEX_MASK;
    int const after_index = (before_index + 1) & SIN_TABLE_INDEX_MASK;

    sin = combine(after_weight, sines[after_index], sines[before_index]);
    cos = combine(after_weight, cosines[after_index], cosines[before_index]);
}


Number Math::exp(Number const x) noexcept
{
    return iterate_exp(x, EXP_SCALE);
}


Number Math::iterate_exp(Number const x, Number const scale) noexcept
{
    /* \exp(x) = \lim_{n \to \infty} ( 1 + \frac{x}{n} ) ^ n */

    /*
    Running the approximation for a limited number of iterations can be 2-3
    times faster than the built-in std::exp() and std::pow() while the error
    remains acceptably low on the intervals that we care about. See:

      https://codingforspeed.com/using-faster-exponential-approximation/
    */

    Number value = 1.0 + x * scale;

    for (int i = 0; i != EXP_ITERATIONS; ++i) {
        value *= value;
    }

    return value;
}


Number Math::pow_10(Number const x) noexcept
{
    return iterate_exp(x, POW_10_SCALE);
}


Number Math::pow_10_inv(Number const x) noexcept
{
    return iterate_exp(x, POW_10_INV_SCALE);
}


Number Math::db_to_linear(Number const db) noexcept
{
    return pow_10(db * DB_TO_LINEAR_GAIN_SCALE);
}


Number Math::linear_to_db(Number const linear) noexcept
{
    /* LINEAR_TO_DB_MIN is considered to be approximately 0.0 */
    return (
        linear >= LINEAR_TO_DB_MIN
            ? lookup(
                math.linear_to_dbs,
                LINEAR_TO_DB_TABLE_MAX_INDEX,
                linear * LINEAR_TO_DB_SCALE
            )
            : DB_MIN
    );
}


Number const* Math::log_biquad_filter_freq_table() noexcept
{
    return math.log_biquad_filter_freq;
}


Number const* Math::log_biquad_filter_q_table() noexcept
{
    return math.log_biquad_filter_q;
}


Frequency Math::detune(Frequency const frequency, Number const cents) noexcept
{
    /*
    The approximation errors in exp() would keep piling up in oscillators (even
    with more iterations) until the oscillators go so much out of phase that it
    may produce noticable, audible problems, so we're using cmath here. Also,
    detuning with cmath doesn't seem to introduce any noticable performance
    difference compared to detuning with iterate_exp(), even with fewer
    iterations.

    Note that std::pow(2.0, c1 * x) seems to be almost twice as fast as
    std::exp(c2 * x), for constants c1 and c2 (where c2 = c1 * LN_OF_2).
    */
    return (
        frequency
        * (Frequency)std::pow(2.0, DETUNE_CENTS_TO_POWER_OF_2_SCALE * cents)
    );
}


void Math::compute_statistics(
        std::vector<Number> const& numbers,
        Statistics& statistics
) noexcept {
    std::vector<Number>::size_type const size = numbers.size();

    statistics.min = std::numeric_limits<Number>::max();
    statistics.median = 0.0;
    statistics.max = std::numeric_limits<Number>::min();
    statistics.mean = 0.0;
    statistics.standard_deviation = 0.0;
    statistics.is_valid = size >= 1;

    if (!statistics.is_valid) {
        return;
    }

    std::vector<Number>::size_type const middle = size >> 1;

    std::vector<Number> sorted(numbers);

    std::sort(sorted.begin(), sorted.end());

    if ((size & 1) == 0) {
        statistics.median = (sorted[middle - 1] + sorted[middle]) / 2.0;
    } else {
        statistics.median = sorted[middle];
    }

    for (std::vector<Number>::const_iterator it = sorted.begin(); it != sorted.end(); ++it) {
        statistics.mean += *it;

        if (*it < statistics.min) {
            statistics.min = *it;
        }

        if (*it > statistics.max) {
            statistics.max = *it;
        }
    }

    Number const size_float = (Number)size;

    statistics.mean /= size_float;

    for (std::vector<Number>::const_iterator it = sorted.begin(); it != sorted.end(); ++it) {
        Number const diff = *it - statistics.mean;
        statistics.standard_deviation += diff * diff;
    }

    statistics.standard_deviation = std::sqrt(
        statistics.standard_deviation / size_float
    );
}


Number Math::combine(
        Number const a_weight,
        Number const a,
        Number const b
) noexcept {
    /*
    One of the multiplications can be eliminated from the following formula:

        a_weight * a + (1.0 - a_weight) * b
    */
    return a_weight * (a - b) + b;
}


Number Math::distort(Number const level, Number const number) noexcept
{
    if (level < 0.0001) {
        return number;
    }

    return combine(
        level,
        lookup(
            math.distortion, DISTORTION_TABLE_MAX_INDEX, number * DISTORTION_SCALE
        ),
        number
    );
}


Number Math::distort_centered_lfo(Number const level, Number const number) noexcept
{
    if (level < 0.0001) {
        return number;
    }

    return combine(
        level,
        lookup(
            math.distortion_centered_lfo, DISTORTION_TABLE_MAX_INDEX, (number + 0.5) * DISTORTION_SCALE
        ),
        number
    );
}


Number Math::randomize(Number const level, Number const number) noexcept
{
    if (level < 0.000001) {
        return number;
    }

    Number const random = lookup(
        math.randoms, RANDOMS_MAX_INDEX, number * RANDOM_SCALE
    );

    return combine(level, random, number);
}


Number Math::randomize_centered_lfo(Number const level, Number const number) noexcept
{
    if (level < 0.000001) {
        return number;
    }

    Number const random = lookup(
        math.randoms_centered_lfo, RANDOMS_MAX_INDEX, (number + 0.5) * RANDOM_SCALE
    );

    return combine(level, random, number);
}


Number Math::apply_envelope_shape(EnvelopeShape const shape, Number const value) noexcept
{
    return lookup(
        math.envelope_shapes[(int)shape],
        ENVELOPE_SHAPE_TABLE_MAX_INDEX,
        value * ENVELOPE_SHAPE_SCALE
    );
}


Number Math::lookup(
        Number const* const table,
        int const max_index,
        Number const index
) noexcept {
    int const before_index = (int)index;

    if (before_index >= max_index) {
        return table[max_index];
    }

    Number const after_weight = index - std::floor(index);
    int const after_index = before_index + 1;

    return combine(after_weight, table[after_index], table[before_index]);
}


template<bool is_index_positive>
Number Math::lookup_periodic(
        Number const* table,
        int const table_size,
        Number const index
) noexcept {
    Number const floor_index = std::floor(index);
    Number const after_weight = index - floor_index;
    int before_index = (int)floor_index;

    if constexpr (!is_index_positive) {
        if (before_index < 0) {
            before_index -= (before_index / table_size - 1) * table_size;
        }
    }

    if (before_index >= table_size) {
        before_index %= table_size;
    }

    int after_index = before_index + 1;

    if (after_index == table_size) {
        after_index = 0;
    }

    return combine(after_weight, table[after_index], table[before_index]);
}


Number Math::lookup_periodic_2(
        Number const* table,
        int const table_size,
        int const table_index_mask,
        Number const index
) noexcept {
    Number const floor_index = std::floor(index);
    Number const after_weight = index - floor_index;
    int const before_index = (int)floor_index;
    int const after_index = before_index + 1;

    return combine(
        after_weight,
        table[after_index & table_index_mask],
        table[before_index & table_index_mask]
    );
}

}

#endif
