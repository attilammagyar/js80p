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

#ifndef JS80P__DSP__DISTORTION_CPP
#define JS80P__DSP__DISTORTION_CPP

#include <cmath>

#include "dsp/distortion.hpp"

#include "dsp/math.hpp"


namespace JS80P { namespace Distortion
{

Tables tables;


/*
The initialize_tables() and initialize_delay_feedback_tables() methods will initialize
both f_tables and F0_tables.
*/
// cppcheck-suppress uninitMemberVar
Tables::Tables()
{
    initialize_tables(Type::SOFT, 3.0);
    initialize_tables(Type::HEAVY, 10.0);
    initialize_delay_feedback_tables();
}


void Tables::initialize_tables(Type const type, Number const steepness) noexcept
{
    Sample const steepness_inv_double = 2.0 / steepness;

    Table& f_table = f_tables[(int)type];
    Table& F0_table = F0_tables[(int)type];

    for (Integer i = 0; i != SIZE; ++i) {
        Number const x = INPUT_MAX * ((Sample)i * SIZE_INV);

        f_table[i] = std::tanh(steepness * x * 0.5);
        F0_table[i] = (
            x + steepness_inv_double * std::log1p(std::exp(-steepness * x))
        );
    }
}


void Tables::initialize_delay_feedback_tables() noexcept
{
    /*
    A tanh(steepness * x) distortion does not play nice with feedback: if the
    steepness is low, then it doesn't have a chance to add noticable distortion
    before the signal fades away, but greater steepness values which produce any
    significant distortion prevent lower signals from ringing down, because no
    matter how much gain reduction is applied on the feedback path, there will
    be a small, non-zero signal level which will be amplified by the distortion
    more than it can be reduced by the feedback gain.

    To overcome these problems, we need a shaping function which stays below
    the y = x line so that the distortion will never be able to undo the
    gain reduction on the feedback path, but still manages to treat louder
    signals differently from quieter ones. In other words, repeated applications
    of the shaping function must not converge to any fixed points other than 0.

    The shaping function must also satisfy the assumptions of the Distortion
    class:

     - it must stay between -1.0 and 1.0, and it must be equal or very close
       these at the respective boundaries of the [INPUT_MIN, INPUT_MAX]
       interval,

     - and its antiderivative must connect to the y = x line as smoothly as
       possible in order to avoid glitches in the ADAA algorithm,

     - and both the function and its antiderivative must play nicely when the
       function is flipped around 0.0 using the -f(-x) formula.

    With these in mind, a spline seems to be a usable choice. The Distortion
    class already takes care of the negative half of the function's domain, so
    it's enough to construct the shaper function for only positive numbers. The
    spline is composed of two polynomials:

        f : [0, 1] --> [0, 1]
        g : [1, 3] --> [0, 1]

    These are connected at x = 1, and let alpha denote the desired value of the
    shaper function there:

        f(1) = g(1) = alpha

    Let f'(x) and F(x) denote the derivative and the antiderivative of the f
    function respectively, let g'(x) and G(x) denote the derivative and the
    antiderivative of the g function respectively.

    In order to join these functions and their antiderivatives smoothly, and to
    satisfy the requirements of the Distortion class, the following properties
    must hold:

        1. f(0) = 0                 (f can be flipped around 0)
        2. f(1) = g(1) = alpha      (f and g join continuously)
        3. f'(1) = g'(1)            (f and g join smoothly)
        4. F(1) = G(1)              (F and G join continuously)
        5. g(3) = 1                 (g joins continuously to y = 1 at x = 3)
        6. g'(3) = 0                (g joins smoothly to y = 1 at x = 3)
        7. G(3) = 3                 (G joins continuously to y = x at x = 3)

    Loud signals should decay slowly, so alpha should be relatively high, but
    the effect shouldn't immediately start to hard clip at 1 either, so alpha
    should stay below 1 as well. This motivates adding the following
    restriction on requirement 3:

        3. f'(1) = g'(1) = 1 - alpha

    Signals which get close to 0 should eventually decay completely:

        8. f'(0) = 0

    The requirements for g can be satisfied by an upside down parabola which has
    a single root at x = 3, and is squeezed and shifted so that requirement 2
    and 5 are satisfied:

        g(x) := ((alpha - 1) / 4) * (x - 3)^2 + 1

    Then, with cg denoting the constant of integration, G becomes:

        G(x) = ((alpha - 1) / 4) * (1/3 * x^3 - 3 * x^2 + 9 * x) + x + cg

    To satisfy requirement 7, cg must be:

        cg = -9 * (alpha - 1) / 4

    With this, g and G are complete.

    There are 4 requirements for f, and one more for F, so a third degree
    polynomial should be enough. For some constants A, B, C, and D, and with cf
    being the constant of integration, let:

        f(x) := A * x^3 + B * x^2 + C * x + D

    Then:

        f'(x) = 3 * A * x^2 + 2 * B * x + C

    and

        F(x) = (1/4) * A * x^4 + (1/3) * B * x^3 + (1/2) * C * x^2 + D * x + cf

    From requirement 1 and 8, C = 0 and D = 0 are obtained.

    Requirement 2 and 3 poses the follwing system of equations after
    expanding f(1) and f'(1):

            A +     B = alpha
        3 * A + 2 * B = 1 - alpha

    From these we obtain:

        A = 1 - 3 * alpha
        B = 4 * alpha - 1

    Now cf can be obtained from requirement 4:

        cf = (-5 * alpha + 7) / 4

    With this, f and F are also complete.

    As for the choice of the alpha, it should be as great as possible while
    satisfying the following requirements for 0 < x < 1:

        a) f(x) != x        (f does not touch or cross the y = x line)
        b) f'(x) > 0        (f is strictly monotonically increasing)
        c) alpha < 1        (avoid hard-clipping at x = 1)

    The f function touching or crossing the y = x line is described by the
    following equation:

        f(x) - x = 0

    Expanding:

        f(x) - x = (A * x^2 + B * x - 1) * x = 0

    Since 0 < x < 1, the only way the equality can hold is:

        A * x^2 + B * x - 1 = 0

    Expanding and solving this quadratic equation for x > 0 yields:

        x1, x2 = (-B +/- sqrt(B^2 + 4 * A)) / (2 * A)

    Touching at a single point occurs when there's only a single root:

        B^2 + 4 * A = 0

    Substituting and simplifying:

        4 * alpha^2 - 5 * alpha + 5/4 = 0

    Solving this for alpha:

        alpha = (5 +/- sqrt(5)) / 8

    In order to eliminate the real-valued solutions of the f(x) - x = 0
    equation, and thus, to prevent f from having a fixed point for any x > 0,
    the following inequality must hold:

        4 * alpha^2 - 5 * alpha + 5/4 < 0

    The coefficient of alpha^2 is positive, therefore the parabola is convex,
    and the alpha values which satisfy the inequality are located between the
    roots. From that interval, a sufficiently high value should be picked for
    alpha, give or take for interpolation and floating point errors.

    Note: an alpha which does not eliminate the real-valued solutions of
    f(x) = x = 0, but pushes them outside the (0, 1) interval may also be a
    usable choice, but actually, the rest of the requirements will discard
    solutions from this approach.

    Solving f'(x) > 0 with 0 < x < 1 for alpha yields 3 choices:

    1. 1/4 < alpha < 1/3
    2. alpha = 1/3 (f becomes quadratic)
    3. 1/3 < alpha < 1

    These boil down to 1/4 < alpha < 1. Though values between 1/4 and
    (5 - sqrt(5)) / 8 do satisfy all the requirements, the higher values are
    preferred in order to minimize the loss of loudness caused by the
    distortion effect on the delay feedback line.

    In conclusion, the best choice for alpha is a value a little bit below
    (5 + sqrt(5)) / 8.
    */
    constexpr Number alpha = (5.0 + std::sqrt(5.0)) / 8.0 - 0.001;

    constexpr Number alpha_m_1 = alpha - 1.0;
    constexpr Number alpha_m_1_o_4 = alpha_m_1 / 4.0;

    constexpr Number A = 1.0 - 3.0 * alpha;
    constexpr Number B = 4.0 * alpha - 1.0;
    constexpr Number cf = (-5.0 * alpha + 7.0) / 4.0;

    constexpr Number cg = -9.0 * alpha_m_1_o_4;

    Table& f_table = f_tables[(int)Type::DELAY_FEEDBACK];
    Table& F0_table = F0_tables[(int)Type::DELAY_FEEDBACK];

    /*
    Floating point errors and interpolation errors become relatively large
    compared to the signal level near 0. To prevent errors from increasing the
    signal level, the first few entries of the table are forced to be 0.
    */
    f_table[0] = 0.0;
    f_table[1] = 0.0;
    f_table[2] = 0.0;
    F0_table[0] = cf;
    F0_table[1] = cf;
    F0_table[2] = cf;

    for (Integer i = 3; i != SIZE; ++i) {
        Number const x = INPUT_MAX * ((Sample)i * SIZE_INV);

        if (x <= 1.0) {
            f_table[i] = (A * x + B) * std::pow(x, 2.0);
            F0_table[i] = ((A / 4.0) * x + B / 3.0) * std::pow(x, 3.0) + cf;
        } else {
            f_table[i] = (
                (alpha_m_1_o_4 * x - 6.0 * alpha_m_1_o_4) * x
                + 9.0 * alpha_m_1_o_4 + 1.0
            );
            F0_table[i] = (
                (
                    ((alpha_m_1_o_4 / 3.0) * x - 3.0 * alpha_m_1_o_4) * x
                    + 9.0 * alpha_m_1_o_4 + 1.0
                ) * x
                + cg
            );
        }
    }
}


Table const& Tables::get_f_table(Type const type) const noexcept
{
    return f_tables[type];
}


Table const& Tables::get_F0_table(Type const type) const noexcept
{
    return F0_tables[type];
}


template<class InputSignalProducerClass>
Distortion<InputSignalProducerClass>::Distortion(
        std::string const& name,
        Type const type,
        InputSignalProducerClass& input,
        SignalProducer* const buffer_owner
) noexcept
    : Filter<InputSignalProducerClass>(input, 1, 0, buffer_owner),
    level(name + "G", 0.0, 1.0, 0.0),
    f_table(tables.get_f_table(type)),
    F0_table(tables.get_F0_table(type))
{
    initialize_instance();
}


template<class InputSignalProducerClass>
Distortion<InputSignalProducerClass>::Distortion(
        std::string const& name,
        Type const type,
        InputSignalProducerClass& input,
        FloatParamS& level_leader,
        SignalProducer* const buffer_owner
) noexcept
    : Filter<InputSignalProducerClass>(input, 1, 0, buffer_owner),
    level(level_leader),
    f_table(tables.get_f_table(type)),
    F0_table(tables.get_F0_table(type))
{
    initialize_instance();
}


template<class InputSignalProducerClass>
void Distortion<InputSignalProducerClass>::initialize_instance() noexcept
{
    this->register_child(level);

    if (this->channels > 0) {
        previous_input_sample = new Sample[this->channels];
        F0_previous_input_sample = new Sample[this->channels];

        for (Integer c = 0; c != this->channels; ++c) {
            previous_input_sample[c] = 0.0;
            F0_previous_input_sample[c] = F0(0.0);
        }
    } else {
        previous_input_sample = NULL;
        F0_previous_input_sample = NULL;
    }
}


template<class InputSignalProducerClass>
Distortion<InputSignalProducerClass>::~Distortion()
{
    if (previous_input_sample != NULL) {
        delete[] previous_input_sample;
        delete[] F0_previous_input_sample;
    }

    previous_input_sample = NULL;
    F0_previous_input_sample = NULL;
}


template<class InputSignalProducerClass>
void Distortion<InputSignalProducerClass>::reset() noexcept
{
    Filter<InputSignalProducerClass>::reset();

    for (Integer c = 0; c != this->channels; ++c) {
        previous_input_sample[c] = 0.0;
        F0_previous_input_sample[c] = F0(0.0);
    }
}


template<class InputSignalProducerClass>
Sample const* const* Distortion<InputSignalProducerClass>::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    Filter<InputSignalProducerClass>::initialize_rendering(round, sample_count);

    level_buffer = FloatParamS::produce_if_not_constant(level, round, sample_count);

    if (this->input.is_silent(round, sample_count)) {
        return this->input_was_silent(round);
    }

    if (level_buffer == NULL)
    {
        level_value = level.get_value();

        if (level_value < 0.000001) {
            return this->input_buffer;
        }
    }

    return NULL;
}


template<class InputSignalProducerClass>
void Distortion<InputSignalProducerClass>::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    Integer const channels = this->channels;
    Sample const* const level_buffer = this->level_buffer;
    Sample const* const* const input_buffer = this->input_buffer;

    Sample* previous_input_sample = this->previous_input_sample;
    Sample* F0_previous_input_sample = this->F0_previous_input_sample;

    if (level_buffer == NULL) {
        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const input_sample = input_buffer[c][i];

                buffer[c][i] = Math::combine(
                    level_value,
                    distort(
                        input_sample,
                        previous_input_sample[c],
                        F0_previous_input_sample[c]
                    ),
                    input_sample
                );
            }
        }
    } else {
        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const input_sample = input_buffer[c][i];

                buffer[c][i] = Math::combine(
                    level_buffer[i],
                    distort(
                        input_sample,
                        previous_input_sample[c],
                        F0_previous_input_sample[c]
                    ),
                    input_sample
                );
            }
        }
    }
}


template<class InputSignalProducerClass>
Sample Distortion<InputSignalProducerClass>::distort(
        Sample const input_sample,
        Sample& previous_input_sample,
        Sample& F0_previous_input_sample
) noexcept {
    Sample const delta = input_sample - previous_input_sample;

    if (JS80P_UNLIKELY(Math::is_abs_small(delta, 0.00000001))) {
        previous_input_sample = input_sample;
        F0_previous_input_sample = F0(input_sample);

        /*
        We're supposed to calculate the average of the current and the previous
        input sample here, but since we only do this when their difference is
        very small or zero, we can probably get away with just using one of
        them.
        */
        return f(input_sample);
    }

    Sample const F0_input_sample = F0(input_sample);
    Sample const ret = (F0_input_sample - F0_previous_input_sample) / delta;

    previous_input_sample = input_sample;
    F0_previous_input_sample = F0_input_sample;

    return ret;
}


template<class InputSignalProducerClass>
Sample Distortion<InputSignalProducerClass>::f(Sample const x) const noexcept
{
    if (x < 0.0) {
        return -lookup(f_table, -x);
    } else {
        return lookup(f_table, x);
    }
}


template<class InputSignalProducerClass>
Sample Distortion<InputSignalProducerClass>::F0(Sample const x) const noexcept
{
    if (x < 0.0) {
        if (x < INPUT_MIN) {
            return -x;
        }

        return lookup(F0_table, -x);
    } else {
        if (x > INPUT_MAX) {
            return x;
        }

        return lookup(F0_table, x);
    }
}


template<class InputSignalProducerClass>
Sample Distortion<InputSignalProducerClass>::lookup(
        Table const& table,
        Sample const x
) const noexcept {
    return Math::lookup(&(table[0]), MAX_INDEX, x * SCALE);
}

} }

#endif
