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
    gain reduction on the feedback path, but still manages to achieve noticable
    distortion. In other words, repeated applications of the shaping function
    must not converge to any fixed points other than 0.

    The shaping function must also satisfy the assumptions of the Distortion
    class:

     - it must stay between -1.0 and 1.0, and it must be equal or very close
       these at the respective boundaries of the [INPUT_MIN, INPUT_MAX]
       interval,

     - its antiderivative must connect to the y = x line at INPUT_MAX and to the
       y = -x at INPUT_MIN as smoothly as possible in order to avoid glitches
       being produced by the ADAA algorithm,

     - both the function and its antiderivative must play nicely near 0 when the
       function is flipped using the -f(-x) formula.

    With these in mind, a spline seems to be a usable choice. The Distortion
    class already takes care of the negative half of the function's domain, so
    it's enough to construct the shaper function for only positive numbers. The
    spline is composed of three polynomials, for some gamma parameter in (0, 1):

        h : [0, gamma] --> [0, 1]
        f : [gamma, 1] --> [0, 1]
        g : [1, 3] --> [0, 1]o


                { g(x)  if x >= 1
        s(x) := { f(x)  if x >= gamma
                { h(x)  if gamma > x >= 0

    Let f'(x) and F(x) denote the derivative and the antiderivative of the f
    function respectively, let g'(x) and G(x) denote the derivative and the
    antiderivative of the g function respectively, and let h'(x) and H(x) denote
    the derivative and the antiderivative of the h function respectively. For
    the antiderivatives, let cf, cg, and ch denote the constants of integration
    for f, g, and h respectively.

    The purpose of the h function is to make the shaper function linear for
    quiet signals so they tail of the feedback delay will decay into silence
    without any more distortion. For some beta parameter in (0, 1]:

        h(x) := beta * x
        H(x) := (1/2) * beta * x^2 + ch
        h'(x) = beta

    The three functions are connected at x = gamma and x = 1 respectively. Let
    alpha denote a parameter which controls the value of the shaping function
    at x = 1:

        f(1) = g(1) = alpha

    In order to join these functions and their antiderivatives smoothly, and to
    satisfy the requirements of the Distortion class, the following properties
    must hold:

         1. h(0) = 0                (h can be flipped around 0)
         2. f(gamma) = h(gamma)     (f and h join continuously)
         3. f'(gamma) = h'(gamma)   (f and h join smoothly)
         4. F(gamma) = H(gamma)     (F and H join continuously)
         5. f(1) = g(1) = alpha     (f and g join continuously)
         6. f'(1) = g(1)            (f and g join smoothly)
         7. F(1) = G(1)             (F and G join continuously)
         8. g(3) = 1                (g joins continuously to y = 1 at x = 3)
         9. g'(3) = 0               (g joins smoothly to y = 1 at x = 3)
        10. G(3) = 3                (G joins continuously to y = x at x = 3)

    Loud signals should decay slowly, so alpha should be relatively high, but
    the effect shouldn't immediately start to hard clip at 1 either, so alpha
    should stay below 1 as well. This motivates adding the following
    restriction on requirement 5:

        5. f'(1) = g'(1) = 1 - alpha

    The requirements for g can be satisfied by an upside down parabola which has
    a single root at x = 3, and is squeezed and shifted so that requirement 2
    and 5 are satisfied:

        g(x) := ((alpha - 1) / 4) * (x - 3)^2 + 1
        G(x) = ((alpha - 1) / 4) * (1/3 * x^3 - 3 * x^2 + 9 * x) + x + cg

    To satisfy requirement 10, cg must be:

        cg = -9 * (alpha - 1) / 4

    With this, g and G are complete.

    There are six requirements which f, f', F, and H must fulfill. Requirement 4
    is a constraint on ch, and the remaining five requirements constrain f and
    cf. This suggests a third degree polynomial:

        f(x) := A * x^3 + B * x^2 + C * x + D
        f'(x) = 3 * A * x^2 + 2 * B * x + C
        F(x) = (1/4) * A * x^4 + (1/3) * B * x^3 + (1/2) * C * x^2 + D * x + cf

    The requirements can be written as a system of equation:

        1. From f(1) = g(1) = alpha:

               A + B + C + D = alpha

        2. From f'(1) = g'(1) = 1 - alpha:

               3 * A + 2 * B + C = 1 - alpha

        3. From f(beta) = h(beta):

               gamma^3 * A + gamma^2 * B + gamma * C + D = beta * gamma

        4. From f'(gamma) = h'(gamma):

               3 * gamma^2 * A + 2 * gamma * B + C = beta

        5. From F(1) = G(1):

               (1/4) * A + (1/3) * B + (1/2) * C + D + cf = (20 - 8 * alpha) / 12

        6. From F(gamma) = H(gamma):

                 (1/4) * gamma^4 * A
               + (1/3) * gamma^3 * B
               + (1/2) * gamma^2 * C
               +           gamma * D
               + cf
               = (20 - 8 * alpha) / 12

    After rearranging and scaling, the following table can be constructed:

          ch      cf       D       A       B       C |
        ---------------------------------------------+--------------------
         -12      12      D1      A1      B1      C1 | 6 * gamma^2 * beta
           0      12      12       3       4       6 | 20 - 8 * alpha
           0       0       1       1       1       1 | alpha
           0       0       0       3       2       1 | 1 - alpha
           0       0       0      A5      B5       1 | beta
           0       0       0      A6      B6      C6 | beta * gamma - alpha

    Where:

        A1 :=  3 * gamma^4
        B1 :=  4 * gamma^3
        C1 :=  6 * gamma^2
        D1 := 12 * gamma

        A5 := 3 * gamma^2
        B5 := 2 * gamma

        A6 := gamma^3 - 1
        B6 := gamma^2 - 1
        C6 := gamma   - 1

    Multiplying the vector on the right with the inverse of the matrix on the
    left produces a vector that contains the values for ch, cf, D, A, B, and C.

    Note: alpha, beta, and gamma must be chosen so that the following equation
    does not have any other real solutions on the [0, 3] interval than 0:

        s(x) - x = 0
    */

    constexpr Number alpha = 899.0 / 1024.0;
    constexpr Number beta = 5.0 / 8.0;
    constexpr Number gamma = 1.0 / 16.0;

    constexpr Number alpha_m1o4 = (alpha - 1.0) / 4.0;

    constexpr Number A = -1.186148148148148;
    constexpr Number B = 1.6221944444444443;
    constexpr Number C = 0.43612586805555564;
    constexpr Number D = 0.005757523148148036;
    constexpr Number cf = 0.613365306712963;

    constexpr Number cg = -9.0 * alpha_m1o4;

    constexpr Number ch = 0.613483746846517;

    Table& f_table = f_tables[(int)Type::DELAY_FEEDBACK];
    Table& F0_table = F0_tables[(int)Type::DELAY_FEEDBACK];

    /*
    Floating point errors and interpolation errors become relatively large
    compared to the signal level near 0. To prevent errors from increasing the
    signal level, the first few entries of the table are forced to be 0.
    */
    f_table[0] = 0.0;
    f_table[1] = 0.0;
    F0_table[0] = ch;
    F0_table[1] = ch;

    for (Integer i = 2; i != SIZE; ++i) {
        Number const x = INPUT_MAX * ((Sample)i * SIZE_INV);

        if (x >= 1.0) {
            /* g(x) and G(x) */
            f_table[i] = (
                (alpha_m1o4 * x - 6.0 * alpha_m1o4) * x + 9.0 * alpha_m1o4 + 1.0
            );
            F0_table[i] = (
                (
                    ((alpha_m1o4 / 3.0) * x - 3.0 * alpha_m1o4) * x
                    + 9.0 * alpha_m1o4 + 1.0
                ) * x
                + cg
            );

            JS80P_ASSERT(alpha <= f_table[i] && f_table[i] <= 1.0);
        } else if (x >= gamma) {
            /* f(x) and F(x) */
            f_table[i] = ((A * x + B) * x + C) * x + D;
            F0_table[i] = (
                ((((A / 4.0) * x + (B / 3.0)) * x + (C / 2.0)) * x + D) * x + cf
            );

            JS80P_ASSERT(0.0 < f_table[i] && f_table[i] < x);
        } else {
            /* h(x) and H(x) */
            f_table[i] = beta * x;
            F0_table[i] = beta * std::pow(x, 2.0) / 2.0 + ch;

            JS80P_ASSERT(0.0 <= f_table[i] && f_table[i] <= x);
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
