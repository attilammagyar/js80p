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

#include "synth/math.hpp"


namespace JS80P
{

Math const Math::math;


Math::Math()
{
    constexpr Number scale = PI_DOUBLE / (Number)TABLE_SIZE;

    for (Integer i = 0; i < TABLE_SIZE; ++i) {
        sines[i] = std::sin((Number)i * scale);
    }
}


Number Math::sin(Number const x)
{
    return Math::math.sin_impl(x);
}


Number Math::cos(Number const x)
{
    return Math::sin(x + PI_HALF);
}


Number Math::sin_impl(Number const x) const
{
    return lookup(sines, x * SINE_SCALE);
}


Number Math::lookup(Number const* table, Number const index) const
{
    Number const after_weight = index - std::floor(index);
    Number const before_weight = 1.0 - after_weight;
    int const before_index = ((int)index) & TABLE_MASK;
    int const after_index = (before_index + 1) & TABLE_MASK;

    return (
        before_weight * table[before_index] + after_weight * table[after_index]
    );
}


Number Math::exp(Number const x)
{
    return iterate_exp(x, EXP_SCALE);
}


Number Math::iterate_exp(Number const x, Number const scale)
{
    // \exp(x) = \lim_{n \to \infty} ( 1 + \frac{x}{n} ) ^ n

    /*
     * Running the approximation for a limited number of iterations
     * can be 2-3 times faster than the built-in std::exp() and
     * std::pow() while the error remains acceptably low on the
     * intervals that we care about. See:
     *
     *   https://codingforspeed.com/using-faster-exponential-approximation/
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
     * The approximation errors in exp() would keep piling up in oscillators
     * (even with more iterations) until the oscillators go so much out of phase
     * that it may produce noticable, even audible problems, so we're using
     * cmath here.
     *
     * Also, std::pow(2.0, c1 * x) seems to be almost twice as fast as
     * std::exp(c2 * x), for constants c1 and c2 (where c2 = c1 * LN_OF_2).
     */
    return frequency * (Frequency)std::pow(
        2.0, Constants::DETUNE_CENTS_TO_POWER_OF_2_SCALE * cents
    );
}

}

#endif
