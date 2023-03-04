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

#ifndef JS80P__SYNTH__MATH_CPP
#define JS80P__SYNTH__MATH_CPP

#include <limits>

#include "synth/math.hpp"


namespace JS80P
{

Math const Math::math;


Math::Math()
{
    init_sines();
    init_randoms();
    init_distortion();
}


void Math::init_sines()
{
    constexpr Number scale = PI_DOUBLE / (Number)SIN_TABLE_SIZE;

    for (int i = 0; i != SIN_TABLE_SIZE; ++i) {
        sines[i] = std::sin((Number)i * scale);
    }
}


void Math::init_randoms()
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
    }
}


void Math::init_distortion()
{
    Number const max_inv = 1.0 / (Number)DISTORTION_TABLE_MAX_INDEX;

    for (int i = 0; i != DISTORTION_TABLE_SIZE; ++i) {
        Number const x = 2.0 * ((Number)i * max_inv) - 1.0;
        distortion[i] = std::tanh(8.0 * x) * 0.5 + 0.5;
    }
}


Number Math::sin(Number const x)
{
    return math.sin_impl(x);
}


Number Math::cos(Number const x)
{
    return sin(x + PI_HALF);
}


Number Math::sin_impl(Number const x) const
{
    Number const index = x * SINE_SCALE;
    Number const after_weight = index - std::floor(index);
    int const before_index = ((int)index) & SIN_TABLE_MASK;
    int const after_index = (before_index + 1) & SIN_TABLE_MASK;

    return combine(after_weight, sines[after_index], sines[before_index]);
}


Number Math::exp(Number const x)
{
    return iterate_exp(x, EXP_SCALE);
}


Number Math::iterate_exp(Number const x, Number const scale)
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


Number Math::pow_10(Number const x)
{
    return iterate_exp(x, POW_10_SCALE);
}


Number Math::pow_10_inv(Number const x)
{
    return iterate_exp(x, POW_10_INV_SCALE);
}


Frequency Math::detune(Frequency const frequency, Number const cents)
{
    /*
    The approximation errors in exp() would keep piling up in oscillators (even
    with more iterations) until the oscillators go so much out of phase that it
    may produce noticable, even audible problems, so we're using cmath here.

    Also, std::pow(2.0, c1 * x) seems to be almost twice as fast as
    std::exp(c2 * x), for constants c1 and c2 (where c2 = c1 * LN_OF_2).
    */
    return frequency * (Frequency)std::pow(
        2.0, Constants::DETUNE_CENTS_TO_POWER_OF_2_SCALE * cents
    );
}


void Math::compute_statistics(
        std::vector<Number> const numbers,
        Statistics& statistics
) {
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
) {
    /*
    One of the multiplications can be eliminated from the following formula:

        a_weight * a + (1.0 - a_weight) * b
    */
    return a_weight * (a - b) + b;
}


Number Math::distort(Number const level, Number const number)
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


Number Math::randomize(Number const level, Number const number)
{
    if (level < 0.0001) {
        return number;
    }

    Number const random = lookup(
        math.randoms, RANDOMS_MAX_INDEX, number * RANDOM_SCALE
    );

    return combine(level, random, number);
}


Number Math::lookup(
        Number const* const table,
        int const max_index,
        Number const index
) {
    int const before_index = (int)index;

    if (before_index >= max_index) {
        return table[max_index];
    }

    Number const after_weight = index - std::floor(index);
    int const after_index = before_index + 1;

    return combine(after_weight, table[after_index], table[before_index]);
}


Number Math::lookup_periodic(
        Number const* table,
        int const table_size,
        Number const index
) {
    Number const after_weight = index - std::floor(index);
    int before_index = (int)std::floor(index);

    if (before_index < 0) {
        before_index -= (before_index / table_size - 1) * table_size;
    }

    if (before_index >= table_size) {
        before_index %= table_size;
    }

    int after_index = before_index + 1;

    if (after_index == table_size) {
        after_index = 0;
    }

    return combine(
        after_weight,
        table[after_index],
        table[before_index]
    );
}

}

#endif
