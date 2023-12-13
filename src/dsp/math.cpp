/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2022  Attila M. Magyar
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
    init_linear_to_db();
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

    constexpr Integer seed = 0x1705;
    constexpr Number scale = 1.0 / 65536.0;

    Integer x = seed;
    Integer c = (((~seed) >> 3) ^ 0x3cf5) & 0xffff;

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


void Math::init_log_biquad_filter_freq() noexcept
{
    Number prev_idx = 0.0;
    Number prev = Constants::BIQUAD_FILTER_FREQUENCY_MIN;

    log_biquad_filter_freq[0] = prev;

    for (int i = 1; i != LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX; ++i) {
        Number const current_idx = (Number)i;
        Number const ratio = current_idx * LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX_INV;
        Number const current = ratio_to_exact_log_biquad_filter_frequency(ratio);

        /*
        The error of the piece-wise linear interpolation of this exponential
        curve is positive on the whole domain. By slightly shifting the line
        segments downward, parts of them go below the exact curve, introducing
        negative errors which balance things out, reducing the overall,
        integrated error.

        The correction term is based on the error at the midpoint of the line
        segment, ie. the difference between the linearly interpolated value and
        the exact value. The constant scaler is picked so that the integrated
        error is sufficiently close to 0.
        */
        Number const correction = -0.6683103012188 * (
            (current + prev) * 0.5
            - ratio_to_exact_log_biquad_filter_frequency(
                (prev_idx + 0.5) * LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX_INV
            )
        );

        log_biquad_filter_freq[i] = current + correction;
        prev = current;
        prev_idx = current_idx;
    }

    log_biquad_filter_freq[LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX] = (
        Constants::BIQUAD_FILTER_FREQUENCY_MAX
    );
}


Number Math::ratio_to_exact_log_biquad_filter_frequency(Number ratio) noexcept
{
    constexpr Number min = Constants::BIQUAD_FILTER_FREQUENCY_MIN;
    constexpr Number max = Constants::BIQUAD_FILTER_FREQUENCY_MAX;
    constexpr Number range = max / min;

    return min * std::pow(range, ratio);
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
