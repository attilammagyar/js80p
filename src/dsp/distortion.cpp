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


TypeParam::TypeParam(std::string const& name, Byte const default_type) noexcept
    : ByteParam(name, TYPE_TANH_3, TYPE_DELAY_FEEDBACK, default_type)
{
}


/*
The initialize_tanh_tables(), the initialize_bit_crush_tables(),
initialize_harmonic_tables(), and the initialize_delay_feedback_tables()
methods will initialize both f_tables and F0_tables.
*/
// cppcheck-suppress uninitMemberVar
Tables::Tables()
{
    initialize_tanh_tables(TYPE_TANH_3, 3.0);
    initialize_tanh_tables(TYPE_TANH_5, 5.0);
    initialize_tanh_tables(TYPE_TANH_10, 10.0);

    /* Parametrizations generated with scripts/plot_distortion_func.py */

    initialize_harmonic_tables(
        TYPE_HARMONIC_13,
        1.1,
        1.1,
        0.0,
        0.9,
        0.8,
        56.89999999999904,
        -168.99999999999977,
        167.39999999999952,
        -54.39999999999995,
        13.874999999999957,
        1.1603333333333254
    );

    initialize_harmonic_tables(
        TYPE_HARMONIC_15,
        0.67,
        0.0,
        0.67,
        0.9,
        0.8,
        -249.35359999999906,
        672.888319999996,
        -597.6158399999965,
        174.9811199999989,
        -37.064239999999714,
        0.606781226666696
    );

    initialize_harmonic_tables(
        TYPE_HARMONIC_135,
        1.2,
        1.2,
        1.2,
        0.9,
        0.8,
        -235.79599999999957,
        621.7551999999969,
        -536.0223999999973,
        150.96319999999906,
        -30.18806666666643,
        0.9979216000000265
    );

    initialize_harmonic_tables(
        TYPE_HARMONIC_SQR,
        1.400563499208679,
        0.4668544997362264,
        0.2801126998417358,
        0.9,
        0.8,
        31.91329334025542,
        -94.4369203478766,
        93.23396067498527,
        -29.810333667364745,
        7.760670110766895,
        0.6364113417460288
    );

    initialize_harmonic_tables(
        TYPE_HARMONIC_TRI,
        0.9726833629664426,
        -0.10807592921849361,
        0.03890733451865771,
        0.9,
        0.8,
        -17.09400151409264,
        45.29805764102855,
        -39.214110739780836,
        11.910054612844277,
        -2.0621847447737487,
        0.5172748779374402
    );

    initialize_bit_crush_tables(
        TYPE_BIT_CRUSH_1,
        1,
        0.98,
        0.72,
        10.213328331155054,
        -28.176465314939634,
        25.732945636414115,
        -6.789808652629539,
        1.7754921899469307,
        0.4849657534427936
    );

    initialize_bit_crush_tables(
        TYPE_BIT_CRUSH_2,
        2,
        0.98,
        0.63,
        -10.79648657612005,
        25.063439194479884,
        -17.717418660599705,
        4.430466042239825,
        -0.21378146606993798,
        0.5273294574249775
    );

    initialize_bit_crush_tables(
        TYPE_BIT_CRUSH_3,
        4,
        0.95,
        0.81,
        -28.376716953283704,
        74.55217884166359,
        -63.92420682347711,
        18.698744935096414,
        -3.459855232258499,
        0.5404833888859787
    );

    initialize_bit_crush_tables(
        TYPE_BIT_CRUSH_4,
        8,
        0.97,
        0.903,
        -99.2936384096713,
        278.45751678735814,
        -259.00411834570605,
        80.81023996801468,
        -18.283943455199896,
        0.5219661504200304
    );

    initialize_bit_crush_tables(
        TYPE_BIT_CRUSH_4_6,
        12,
        0.975,
        0.936,
        -191.45504581730165,
        548.3849709584974,
        -522.3798044650903,
        166.42487932389304,
        -39.149539289855944,
        0.5177086676141855
    );

    initialize_bit_crush_tables(
        TYPE_BIT_CRUSH_5,
        16,
        0.98,
        0.953,
        -332.16796107779373,
        962.6624097085587,
        -928.8009361837321,
        299.28648755297036,
        -71.71816576117315,
        0.5139551031103028
    );

    initialize_bit_crush_tables(
        TYPE_BIT_CRUSH_5_6,
        24,
        0.986,
        0.968,
        -683.9655485837357,
        2003.676754824235,
        -1955.4428638972458,
        636.7176576567654,
        -154.88775683694445,
        0.5096168036697888
    );

    initialize_bit_crush_tables(
        TYPE_BIT_CRUSH_6,
        32,
        0.99,
        0.976,
        -1294.7571214204654,
        3817.0518132952275,
        -3749.822262329166,
        1228.5175704543653,
        -301.26109669976125,
        0.5068199139012783
    );

    initialize_bit_crush_tables(
        TYPE_BIT_CRUSH_6_6,
        48,
        0.99,
        0.984,
        -1284.5552687983727,
        3791.915685349377,
        -3730.155564303277,
        1223.7851477524382,
        -300.533776850877,
        0.5067614375135758
    );

    initialize_bit_crush_tables(
        TYPE_BIT_CRUSH_7,
        64,
        0.993,
        0.988,
        -2882.2627702876925,
        8553.549286403693,
        -8460.303261944093,
        2790.009745827876,
        -689.4708510853023,
        0.5047169376644367
    );

    initialize_bit_crush_tables(
        TYPE_BIT_CRUSH_7_6,
        96,
        0.997,
        0.992,
        -13055.919684820808,
        38948.79222364724,
        -38729.82239283435,
        12837.946854007896,
        -3190.984477600759,
        0.5020156539557239
    );

    initialize_bit_crush_tables(
        TYPE_BIT_CRUSH_8,
        128,
        0.999,
        0.994,
        -34838.297230400145,
        104118.11372153088,
        -103721.33475185931,
        34442.51726073027,
        -8577.312817710561,
        0.5006717158226137
    );

    initialize_bit_crush_tables(
        TYPE_BIT_CRUSH_8_6,
        192,
        0.999,
        0.993,
        -8808.22024447564,
        26260.77017473802,
        -26096.87861603871,
        8645.32768578548,
        -2147.4227082224024,
        0.5006631888235299
    );

    initialize_bit_crush_tables(
        TYPE_BIT_CRUSH_9,
        256,
        0.999,
        0.995,
        -9994.78312556073,
        29809.47186112404,
        -29634.593345575035,
        9820.903610004112,
        -2440.4011095381493,
        0.5006650075082928
    );

    initialize_delay_feedback_tables();
}


void Tables::initialize_tanh_tables(
        Byte const type,
        Number const steepness
) noexcept {
    Sample const steepness_inv_double = 2.0 / steepness;
    Sample const c = (
        - steepness_inv_double * std::log1p(std::exp(-steepness * INPUT_MAX))
    );

    Table& f_table = f_tables[type];
    Table& F0_table = F0_tables[type];

    for (Integer i = 0; i != SIZE; ++i) {
        Number const x = INPUT_MAX * ((Sample)i * SIZE_INV);

        f_table[i] = std::tanh(steepness * x * 0.5);
        F0_table[i] = (
            x + steepness_inv_double * std::log1p(std::exp(-steepness * x)) + c
        );
    }
}


template<class H>
void Tables::initialize_spline_tables(
        Byte const type,
        H const& h,
        Number const alpha,
        Number const gamma,
        Number const A,
        Number const B,
        Number const C,
        Number const D,
        Number const cf
) noexcept {
    /*
    Given a shaping function h(x), its derivative h'(x), its antiderivative
    H(x), and alpha and gamma parameters (0 < alpha < 1 and 0 < gamma < 1), we
    will construct a spline s(x) which:

     * connects smoothly to h(x) and its antiderivative connects smoothly to
       H(x) at x = gamma,

     * and s(1) = alpha,

     * and for 1 < x < 3, it applies soft clipping to the signal.

    Note: to connect the antiderivatives, we will adjust the integration
    constant in H(x) as well.

    The h(x) function does not have to be a a polynomial, but it must be an odd
    function in order to satisfy the assumptions of the Distortion class:
    h(x) = -h(-x).

    Let f and g be polynomials so that:

        h : [0, gamma] --> R
        f : [gamma, 1] --> R
        g : [1, 3] --> R

                { g(x)  if 1 <= x <= 3
        s(x) := { f(x)  if gamma <= x
                { h(x)  if 0 <= x < gamma

    Let F and G denote some antiderivatives, and let f' and g' denote the
    derivatives of f and g respectively.

    The equations:

        1. h'(gamma) = f'(gamma)
        2. h(gamma) = f(gamma)
        3. H(gamma) + ch = F(gamma)
        4. f'(1) = g'(1)
        5. f(1) = g(1)
        6. F(1) = G(1)
        7. g'(3) = 0
        8. g(3) = 1
        9. G(3) = 3
        10. g(1) = alpha

    To satisfy requirements 7 and 8, g can be defined as (for some E real
    number):

        g(x) := E * (x - 3)^2 + 1 = E * x^2 - 6 * E * x + 9 * E + 1
        G(x)  = (E/3) * x^3 - 3 * E * x^2 + (9 * E + 1) * x + cg
        g'(x) = 2 * E * x - 6 * E

    Requirement 10:

        g(1) = E - 6 * E + 9 * E + 1 = 4 * E + 1 = alpha
           E = (alpha - 1) / 4

    Requirement 9:

        G(3)  = (E/3) * 27 - 3 * E * 9 + 9 * E * 3 + 3 + cg = 3
            --> 9 * E - 27 * E + 27 * E + cg = 0
                9 * E + cg = 0
                cg = -9 * E

    There are 6 more equations to satisfy, and one of them constrains the
    integration constant ch that will be used for H(x), so we have 5 degrees of
    freedom for f and its antiderivative. Thus, let's define f as a third degree
    polynomial:

        f(x) := A * x^3 + B * x^2 + C * x + D
        F(x)  = (A/4) * x^4 + (B/3) * x^3 + (C/2) * x^2 + D * x + cf
        f'(x) = 3 * A * x^2 + 2 * B * x + C

    Expanding the equations:

        1.  h'(gamma) = 3 * gamma^2 * A + 2 * gamma * B + C
        2.  h(gamma) = gamma^3 * A + gamma^2 * B + gamma * C + D
        3   12 * H(gamma) =   3 * gamma^4 * A
                            + 4 * gamma^3 * B
                            + 6 * gamma^2 * C
                            + 12 * gamma * D
                            + 12 * cf
                            - 12 * ch
        4.  3 * A + 2 * B + C = 2 * E - 6 * E = -4 * E = 1 - alpha
        5.  A + B + C + D = alpha
        6.  3 * A + 4 * B + 6 * C + 12 * D + 12 * cf = 12 * G(1)

    Matrix form:

        L1 := gamma
        L2 := gamma^2
        L3 := gamma^3
        L4 := gamma^4

        V1 = h'(gamma)
        V2 = h(gamma)
        V3 = 12 * H(gamma)
        V4 = 1 - alpha
        V5 = alpha
        V6 = 12 * G(1)

                A           B           C           D       cf      ch |
        ---------------------------------------------------------------+----
           3 * L2      2 * L1           1           0        0       0 | V1
               L3          L2          L1           1        0       0 | V2
           3 * L4      4 * L3      6 * L2     12 * L1       12     -12 | V3
                3           2           1           0        0       0 | V4
                1           1           1           1        0       0 | V5
                3           4           6          12       12       0 | V6

    Multiplying the vector on the right (V1, ..., V6) with the inverse of the
    matrix on the left produces a vector that contains the values for
    A, B, C, D, cf, and ch.
    */

    Number const E = (alpha - 1.0) / 4.0;
    Number const cg = -9.0 * E;

    Number const Em3 = -3.0 * E;
    Number const Em6 = -6.0 * E;
    Number const Eo3 = E / 3.0;
    Number const E9p1 = 9.0 * E + 1.0;

    Number const Ao4 = A / 4.0;
    Number const Bo3 = B / 3.0;
    Number const Co2 = C / 2.0;

    Table& f_table = f_tables[type];
    Table& F0_table = F0_tables[type];

    for (Integer i = 0; i != SIZE; ++i) {
        Number const x = INPUT_MAX * ((Sample)i * SIZE_INV);

        if (x >= 1.0) {
            /* g(x) and G(x) */
            f_table[i] = (E * x + Em6) * x + E9p1;
            F0_table[i] = ((Eo3 * x + Em3) * x + E9p1) * x + cg;

            JS80P_ASSERT(alpha <= f_table[i] && f_table[i] <= 1.0);
        } else if (x >= gamma) {
            /* f(x) and F(x) */
            f_table[i] = ((A * x + B) * x + C) * x + D;
            F0_table[i] = (((Ao4 * x + Bo3) * x + Co2) * x + D) * x + cf;

            JS80P_ASSERT(-1.0 < f_table[i] && f_table[i] < 1.0);
        } else {
            /* h(x) and H(x) */
            h(x, f_table[i], F0_table[i]);

            JS80P_ASSERT(-1.0 <= f_table[i] && f_table[i] <= 1.0);

#ifdef JS80P_ASSERTIONS
            Number minus_y;
            Number minus_antiderivative;

            h(-x, minus_y, minus_antiderivative);
            JS80P_ASSERT(Math::is_close(- minus_y, f_table[i]));
            JS80P_ASSERT(Math::is_close(minus_antiderivative, F0_table[i]));
#endif
        }
    }

    JS80P_ASSERT(Math::is_close(f_table[0], 0.0));
}


void Tables::initialize_harmonic_tables(
        Byte const type,
        Number const w1,
        Number const w3,
        Number const w5,
        Number const alpha,
        Number const gamma,
        Number const A,
        Number const B,
        Number const C,
        Number const D,
        Number const cf,
        Number const ch
) noexcept {
    /*
    These shaping functions mix various harmonics of the original signal using
    Chebyshev polynomials. The w1, w3, and w5 parameters are the weights of the
    fundamental, the third, and the fifth harmonic. The reason for only using
    the odd harmonics is that the implementation of the Distortion class
    requires the shaping function to be odd and to disappear at 0, and
    unfortunately, the even degree Chebyshev polynomials are even functions.
    */

    constexpr Number t3_a =   4.0;
    constexpr Number t3_b =  -3.0;
    constexpr Number t5_a =  16.0;
    constexpr Number t5_b = -20.0;
    constexpr Number t5_c =   5.0;

    constexpr Number T1_a =  1.0 / 2.0;
    constexpr Number T3_b = -3.0 / 2.0;
    constexpr Number T5_a =  8.0 / 3.0;
    constexpr Number T5_b = -5.0;
    constexpr Number T5_c =  5.0 / 2.0;

    Number const t3_a_w3 = t3_a * w3;
    Number const t3_b_w3 = t3_b * w3;
    Number const t5_a_w5 = t5_a * w5;
    Number const t5_b_w5 = t5_b * w5;
    Number const t5_c_w5 = t5_c * w5;
    Number const sum_w1_t3bw3_t5cw5 = w1 + t3_b_w3 + t5_c_w5;

    Number const T1_a_w1 = T1_a * w1;
    Number const T3_b_w3 = T3_b * w3;
    Number const T5_a_w5 = T5_a * w5;
    Number const T5_b_w5 = T5_b * w5;
    Number const T5_c_w5 = T5_c * w5;
    Number const sum_T1aw1_T3bw3_T5cw5 = T1_a_w1 + T3_b_w3 + T5_c_w5;

    initialize_spline_tables(
        type,
        [&](Number const x, Number& y, Number& antiderivative) {
            Number const x2 = std::pow(x, 2.0);

            y = x * (
                sum_w1_t3bw3_t5cw5 + (t3_a_w3 + t5_a_w5 * x2 + t5_b_w5) * x2
            );

            antiderivative = ch + x2 * (
                sum_T1aw1_T3bw3_T5cw5 + (w3 + T5_a_w5 * x2 + T5_b_w5) * x2
            );
        },
        alpha,
        gamma,
        A,
        B,
        C,
        D,
        cf
    );
}


void Tables::initialize_bit_crush_tables(
        Byte const type,
        Integer const k,
        Number const alpha,
        Number const gamma,
        Number const A,
        Number const B,
        Number const C,
        Number const D,
        Number const cf,
        Number const ch
) noexcept {
    /*
    For a bit-crusher effect, we need a staircase function with a parametric
    number of steps. The function should also be smooth and we will need its
    antiderivative as well for the ADAA waveshaping.

    The slope of the staircase function will be provided by the y = x line, and
    the staircase shape will be achieved by alternating between overshooting
    this line and lagging behind. For this, we can subtract a sinusoid from the
    y = x line with some scaling so that the resulting function remains more or
    less monotonically increasing. For some k integer (this will control the
    number of steps via sine's periodicity), let:

        h(x) := x - W * sin(2 * pi * k * x)

    If we wanted to preserve monotonicity, we'd need to pick a value for W which
    ensures that h'(x) remains positive:

        h'(x) = 1 - W * 2 * pi * k * cos(2 * pi * k * x) > 0
            1 > W * 2 * pi * k * cos(2 * pi * k * x)

    Since the maximum value of the cosine term is 1, we obtain the following
    limit for W:

            W < 1 / (2 * pi * k)

    In practice, we're going to use a slightly greater value than that, for
    increased emphasis of the effect.

    With these, a staircase function with k steps can be written as:

        h_k(x) := x - W * sin(2 * pi * k * x)
        h_k'(x) = 1 - W * cos(2 * pi * k * x)
        H_k(x)  = (1/2) * x^2 + W * cos(2 * pi * k * x) / (2 * pi * k) + ch
    */

    Number const pik = Math::PI * (Number)k;
    Number const pi2k = 2.0 * pik;
    Number const pi2k_inv = 1.0 / pi2k;
    Number const W = 1.0 / (1.4 * pik);

    initialize_spline_tables(
        type,
        [&](Number const x, Number& y, Number& antiderivative) {
            Number const pi2k_x = pi2k * x;

            y = x - W * std::sin(pi2k_x);

            antiderivative = (
                std::pow(x, 2.0) * 0.5 + W * std::cos(pi2k_x) * pi2k_inv + ch
            );
        },
        alpha,
        gamma,
        A,
        B,
        C,
        D,
        cf
    );
}


void Tables::initialize_delay_feedback_tables() noexcept
{
    /*
    A tanh(steepness * x) distortion does not play nice with delay feedback: if
    the steepness is low, then it doesn't have a chance to add noticable
    distortion before the signal decays, but greater steepness values (which
    produce any significant distortion) prevent lower signals from ringing down
    completely, since

        d/dx tanh(steepness * x) = steepness * sech(steepness * x)^2

    which is equal to the steepness parameter itself at x = 0. This, and the
    interpolation and floating point errors, and the error of the numerical
    differentiation in the ADAA wave shaping algorithm, are enough to boost
    signal levels near zero back to a level where the gain reduction on the
    delay feedback path is undone for any sensible level of reduction.

    To prevent this, we need a soft clipping function which stays below the
    y = x line so that the distortion will never be able to win over the gain
    reduction, but still manages to achieve noticable saturation. In other
    words, repeated applications of the shaping function must not converge to
    any other fixed points than 0 (at least, not on the interval that is used by
    the Distortion class).

    The shaping function must also satisfy the assumptions of the Distortion
    class:

     - it must stay between -1.0 and 1.0, and it must be equal or very close to
       these at the respective boundaries of the [INPUT_MIN, INPUT_MAX]
       interval,

     - its antiderivative must connect to the y = x line at x = INPUT_MAX and to
       the y = -x line at x = INPUT_MIN smoothly, in order to avoid glitches
       being produced by the ADAA algorithm,

     - the function must be odd so that it can be flipped around zero with the
       -f(-x) formula.

    For a given 0 < beta < 1, let:

        h(x) := beta * x
        H(x) := (1/2) * beta * x^2
        h'(x) = beta

    We will use initialize_spline_tables() to construct the shaping function.
    The alpha, beta, and gamma parameters must be picked so that for all 0 < x,
    s(x) < x.

    The coefficients and integration constants below are calculated in
    scripts/distortion_delay_spline.py.
    */

    constexpr Number alpha = 899.0 / 1024.0;
    constexpr Number beta = 5.0 / 8.0;
    constexpr Number gamma = 1.0 / 16.0;

    constexpr Number A = -1.186148148148148;
    constexpr Number B = 1.6221944444444443;
    constexpr Number C = 0.43612586805555564;
    constexpr Number D = 0.005757523148148036;
    constexpr Number cf = 0.613365306712963;
    constexpr Number ch = 0.613483746846517;

    initialize_spline_tables(
        TYPE_DELAY_FEEDBACK,
        [&](Number const x, Number& y, Number& antiderivative) {
            y = beta * x;
            antiderivative = y * x * 0.5 + ch;
        },
        alpha,
        gamma,
        A,
        B,
        C,
        D,
        cf
    );

    Table& f_table = f_tables[TYPE_DELAY_FEEDBACK];
    Table& F0_table = F0_tables[TYPE_DELAY_FEEDBACK];

    /*
    Floating point errors and interpolation errors become relatively large
    compared to the signal level near 0. To prevent errors from increasing the
    signal level, the first few entries of the table are forced to be 0.
    */
    f_table[0] = 0.0;
    f_table[1] = 0.0;
    F0_table[0] = ch;
    F0_table[1] = ch;
}


Table const& Tables::get_f_table(Byte const type) const noexcept
{
    return f_tables[type];
}


Table const& Tables::get_F0_table(Byte const type) const noexcept
{
    return F0_tables[type];
}


template<class InputSignalProducerClass>
Distortion<InputSignalProducerClass>::Distortion(
        std::string const& name,
        TypeParam const& type,
        InputSignalProducerClass& input,
        SignalProducer* const buffer_owner
) noexcept
    : Filter<InputSignalProducerClass>(input, 1, 0, buffer_owner),
    level(name + "G", 0.0, 1.0, 0.0),
    type(type)
{
    initialize_instance();
}


template<class InputSignalProducerClass>
Distortion<InputSignalProducerClass>::Distortion(
        std::string const& name,
        TypeParam const& type,
        InputSignalProducerClass& input,
        FloatParamS& level_leader,
        SignalProducer* const buffer_owner
) noexcept
    : Filter<InputSignalProducerClass>(input, 1, 0, buffer_owner),
    level(level_leader),
    type(type)
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
        current_type = type.get_value();
        previous_type = current_type;
        Table const& F0_table = tables.get_F0_table(current_type);

        for (Integer c = 0; c != this->channels; ++c) {
            previous_input_sample[c] = 0.0;
            F0_previous_input_sample[c] = F0(F0_table, 0.0);
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

    current_type = type.get_value();
    previous_type = current_type;

    Table const& F0_table = tables.get_F0_table(current_type);

    for (Integer c = 0; c != this->channels; ++c) {
        previous_input_sample[c] = 0.0;
        F0_previous_input_sample[c] = F0(F0_table, 0.0);
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

    current_type = type.get_value();

    if (current_type != previous_type) {
        previous_type = current_type;

        Table const& F0_table = tables.get_F0_table(current_type);

        for (Integer c = 0; c != this->channels; ++c) {
            F0_previous_input_sample[c] = F0(F0_table, previous_input_sample[c]);
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
    Table const& f_table = tables.get_f_table(current_type);
    Table const& F0_table = tables.get_F0_table(current_type);

    Sample* previous_input_sample = this->previous_input_sample;
    Sample* F0_previous_input_sample = this->F0_previous_input_sample;

    if (level_buffer == NULL) {
        for (Integer c = 0; c != channels; ++c) {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const input_sample = input_buffer[c][i];

                buffer[c][i] = Math::combine(
                    level_value,
                    distort(
                        f_table,
                        F0_table,
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
                        f_table,
                        F0_table,
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
        Table const& f_table,
        Table const& F0_table,
        Sample const input_sample,
        Sample& previous_input_sample,
        Sample& F0_previous_input_sample
) noexcept {
    Sample const delta = input_sample - previous_input_sample;

    if (JS80P_UNLIKELY(Math::is_abs_small(delta, 0.00000001))) {
        previous_input_sample = input_sample;
        F0_previous_input_sample = F0(F0_table, input_sample);

        /*
        We're supposed to calculate the average of the current and the previous
        input sample here, but since we only do this when their difference is
        very small or zero, we can probably get away with just using one of
        them.
        */
        return f(f_table, input_sample);
    }

    Sample const F0_input_sample = F0(F0_table, input_sample);
    Sample const ret = (F0_input_sample - F0_previous_input_sample) / delta;

    previous_input_sample = input_sample;
    F0_previous_input_sample = F0_input_sample;

    return ret;
}


template<class InputSignalProducerClass>
Sample Distortion<InputSignalProducerClass>::f(
        Table const& f_table,
        Sample const x
) const noexcept {
    if (x < 0.0) {
        return -lookup(f_table, -x);
    } else {
        return lookup(f_table, x);
    }
}


template<class InputSignalProducerClass>
Sample Distortion<InputSignalProducerClass>::F0(
        Table const& F0_table,
        Sample const x
) const noexcept {
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
