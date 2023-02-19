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

#include "test.cpp"
#include "utils.cpp"

#include <cmath>
#include <vector>

#include "js80p.hpp"

#include "synth/math.cpp"


using namespace JS80P;


constexpr Integer RESOLUTION = 1000000;
constexpr Number DELTA = 1.0 / (Number)RESOLUTION;
constexpr Number TOLERANCE_TRIG = 0.00001;
constexpr Number TOLERANCE_EXP = 3.0 / 100.0;

constexpr Number PI_HALF = Math::PI_HALF;
constexpr Number PI = Math::PI;
constexpr Number PI_HALF_3 = 3.0 * Math::PI_HALF;
constexpr Number PI_DOUBLE = Math::PI_DOUBLE;


TEST(sin, {
    for (Number x = 0.0, limit = PI_DOUBLE * 2.0; x < limit; x += DELTA) {
        assert_eq(std::sin(x), Math::sin(x), TOLERANCE_TRIG, "x=%f", x);
    }

    assert_eq(
        std::sin(PI_HALF), Math::sin(PI_HALF), TOLERANCE_TRIG, "x=%f", PI_HALF
    );
    assert_eq(
        std::sin(PI), Math::sin(PI), TOLERANCE_TRIG, "x=%f", PI
    );
    assert_eq(
        std::sin(PI_HALF_3), Math::sin(PI_HALF_3), TOLERANCE_TRIG, "x=%f", PI_HALF_3
    );
    assert_eq(
        std::sin(PI_DOUBLE), Math::sin(PI_DOUBLE), TOLERANCE_TRIG, "x=%f", PI_DOUBLE
    );
})


TEST(cos, {
    for (Number x = 0.0, limit = PI_DOUBLE * 2.0; x < limit; x += DELTA) {
        assert_eq(std::cos(x), Math::cos(x), TOLERANCE_TRIG, "x=%f", x);
    }

    assert_eq(
        std::cos(PI_HALF), Math::cos(PI_HALF), TOLERANCE_TRIG, "x=%f", PI_HALF
    );
    assert_eq(
        std::cos(PI), Math::cos(PI), TOLERANCE_TRIG, "x=%f", PI
    );
    assert_eq(
        std::cos(PI_HALF_3), Math::cos(PI_HALF_3), TOLERANCE_TRIG, "x=%f", PI_HALF_3
    );
    assert_eq(
        std::cos(PI_DOUBLE), Math::cos(PI_DOUBLE), TOLERANCE_TRIG, "x=%f", PI_DOUBLE
    );
})


TEST(exp_limits_sanity, {
    std::vector<Number> values;
    Number min = 999999.0;
    Number max = -999999.0;

    values.push_back(Math::LN_OF_10 * Math::POW_10_MIN);
    values.push_back(Math::LN_OF_10 * Math::POW_10_MAX);
    values.push_back(-1.0 * Math::LN_OF_10 * Math::POW_10_INV_MIN);
    values.push_back(-1.0 * Math::LN_OF_10 * Math::POW_10_INV_MAX);

    for (std::vector<Number>::iterator it = values.begin(); it != values.end(); ++it) {
        if (min > *it) {
            min = *it;
        }

        if (max < *it) {
            max = *it;
        }
    }

    assert_gte(Math::EXP_MAX, max);
    assert_lte(Math::EXP_MIN, min);
})


TEST(exp, {
    constexpr Number min = Math::EXP_MIN - 0.125;
    constexpr Number max = Math::EXP_MAX + 0.125;

    for (Number x = min; x < max; x += DELTA) {
        Number const expected = std::exp(x);

        assert_eq(expected, Math::exp(x), expected * TOLERANCE_EXP, "x=%f", x);
    }
})


TEST(pow_10, {
    constexpr Number min = Math::POW_10_MIN - 0.125;
    constexpr Number max = Math::POW_10_MAX + 0.125;

    for (Number x = min; x < max; x += DELTA) {
        Number const expected = std::pow(10.0, x);

        assert_eq(expected, Math::pow_10(x), expected * TOLERANCE_EXP, "x=%f", x);
    }
})


TEST(pow_10_inv, {
    constexpr Number min = Math::POW_10_INV_MIN - 0.125;
    constexpr Number max = Math::POW_10_INV_MAX + 0.125;

    for (Number x = min; x < max; x += DELTA) {
        Number const expected = 1.0 / std::pow(10.0, x);

        assert_eq(
            expected, Math::pow_10_inv(x), expected * TOLERANCE_EXP, "x=%f", x
        );
    }
})


TEST(detune, {
    assert_eq(110.0, Math::detune(440.0, -2400.0), DOUBLE_DELTA);
    assert_eq(220.0, Math::detune(440.0, -1200.0), DOUBLE_DELTA);
    assert_eq(415.304698, Math::detune(440.0, -100.0), DOUBLE_DELTA);
    assert_eq(440.0, Math::detune(440.0, 0.0), DOUBLE_DELTA);
    assert_eq(466.163762, Math::detune(440.0, 100.0), DOUBLE_DELTA);
    assert_eq(880.0, Math::detune(440.0, 1200.0), DOUBLE_DELTA);
    assert_eq(1760.0, Math::detune(440.0, 2400.0), DOUBLE_DELTA);
})


TEST(lookup, {
    Number const table[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
    Integer const max_index = 6;

    assert_eq(1.0, Math::lookup(table, max_index, 0.0), DOUBLE_DELTA);
    assert_eq(1.5, Math::lookup(table, max_index, 0.5), DOUBLE_DELTA);
    assert_eq(1.7, Math::lookup(table, max_index, 0.7), DOUBLE_DELTA);
    assert_eq(6.0, Math::lookup(table, max_index, 5.0), DOUBLE_DELTA);
    assert_eq(6.3, Math::lookup(table, max_index, 5.3), DOUBLE_DELTA);
    assert_eq(6.999, Math::lookup(table, max_index, 5.999), DOUBLE_DELTA);
    assert_eq(7.0, Math::lookup(table, max_index, 6.0), DOUBLE_DELTA);
    assert_eq(7.0, Math::lookup(table, max_index, 6.1), DOUBLE_DELTA);
    assert_eq(7.0, Math::lookup(table, max_index, 7.0), DOUBLE_DELTA);
})
