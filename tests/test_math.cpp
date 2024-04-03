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

#include "test.cpp"
#include "utils.cpp"

#include <cmath>
#include <vector>

#include "js80p.hpp"

#include "dsp/math.cpp"


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


TEST(sincos, {
    for (Number x = 0.0, limit = PI_DOUBLE * 2.0; x < limit; x += DELTA) {
        Number sin = 0.0;
        Number cos = 0.0;

        Math::sincos(x, sin, cos);

        assert_eq(std::sin(x), sin, TOLERANCE_TRIG, "x=%f", x);
        assert_eq(std::cos(x), cos, TOLERANCE_TRIG, "x=%f", x);
    }
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


TEST(db_to_linear, {
    assert_eq(2.0, Math::db_to_linear(6.0), 0.01);
    assert_eq(1.0, Math::db_to_linear(0.0), DOUBLE_DELTA);
    assert_eq(1.0 / 2.0, Math::db_to_linear(-6.0), 0.001);
    assert_eq(1.0 / 4.0, Math::db_to_linear(-12.0), 0.001);
    assert_eq(1.0 / 8.0, Math::db_to_linear(-18.0), 0.001);
    assert_eq(1.0 / 16.0, Math::db_to_linear(-24.0), 0.001);
    assert_eq(1.0 / 32.0, Math::db_to_linear(-30.0), 0.001);
    assert_eq(1.0 / 64.0, Math::db_to_linear(-36.0), 0.001);
    assert_eq(1.0 / 128.0, Math::db_to_linear(-42.0), 0.001);
    assert_eq(1.0 / 256.0, Math::db_to_linear(-48.0), 0.001);
    assert_eq(0.0, Math::db_to_linear(Math::DB_MIN), DOUBLE_DELTA);
})


TEST(linear_to_db, {
    assert_eq(13.98, Math::linear_to_db(10e10), 0.03);
    assert_eq(13.98, Math::linear_to_db(10.0), 0.03);
    assert_eq(13.98, Math::linear_to_db(5.0), 0.03);
    assert_eq(6.0, Math::linear_to_db(2.0), 0.03);
    assert_eq(0.0, Math::linear_to_db(1.0), 0.03);
    assert_eq(-6.0, Math::linear_to_db(1.0 / 2.0), 0.03);
    assert_eq(-12.0, Math::linear_to_db(1.0 / 4.0), 0.05);
    assert_eq(-18.0, Math::linear_to_db(1.0 / 8.0), 0.07);
    assert_eq(-24.0, Math::linear_to_db(1.0 / 16.0), 0.09);
    assert_eq(-30.0, Math::linear_to_db(1.0 / 32.0), 0.11);
    assert_eq(-36.0, Math::linear_to_db(1.0 / 64.0), 0.15);
    assert_eq(-42.0, Math::linear_to_db(1.0 / 128.0), 0.21);
    assert_eq(-48.0, Math::linear_to_db(1.0 / 256.0), 0.70);
    assert_eq(Math::DB_MIN, Math::linear_to_db(0.0), 0.05);
    assert_eq(Math::DB_MIN, Math::linear_to_db(-0.1), 0.05);
    assert_eq(Math::DB_MIN, Math::linear_to_db(-1.0), 0.05);
})


TEST(converting_back_and_forth_between_linear_and_db_reproduces_the_original_value, {
    constexpr int resolution = 50000;
    constexpr Number scale = Math::LINEAR_TO_DB_MAX / (Number)resolution;

    /*
    Skipping values below around -36 dB, because they tend to have larger
    errors, but in practice, these aren't noticable.
    */
    for (int i = 150; i != resolution; ++i) {
        Number const linear = Math::LINEAR_TO_DB_MIN + scale * (Number)i;
        Number const db = Math::linear_to_db(linear);
        Number const db_tolerance = std::max(0.001, std::fabs(db * 0.01));

        assert_eq(linear, Math::db_to_linear(db), 0.03, "i=%d", i);
        assert_eq(db, Math::linear_to_db(Math::db_to_linear(db)), db_tolerance, "i=%d", i);
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


TEST(combine, {
    assert_eq(42.0, Math::combine(1.0, 42.0, 123.0), DOUBLE_DELTA);
    assert_eq(123.0, Math::combine(0.0, 42.0, 123.0), DOUBLE_DELTA);
    assert_eq(
        0.3 * 42.0 + 0.7 * 123.0, Math::combine(0.3, 42.0, 123.0), DOUBLE_DELTA
    );
})


TEST(lookup, {
    constexpr Integer max_index = 6;
    Number const table[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};

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


TEST(lookup_periodic, {
    constexpr Integer table_size = 7;
    Number const table[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};

    assert_eq(7.0, Math::lookup_periodic<true>(table, table_size, 6.0), DOUBLE_DELTA);
    assert_eq(6.4, Math::lookup_periodic<true>(table, table_size, 6.1), DOUBLE_DELTA);
    assert_eq(1.0, Math::lookup_periodic<true>(table, table_size, 7.0), DOUBLE_DELTA);
    assert_eq(1.7, Math::lookup_periodic<true>(table, table_size, 7.7), DOUBLE_DELTA);
    assert_eq(6.0, Math::lookup_periodic<true>(table, table_size, 12.0), DOUBLE_DELTA);
    assert_eq(6.3, Math::lookup_periodic<true>(table, table_size, 12.3), DOUBLE_DELTA);
    assert_eq(6.99, Math::lookup_periodic<true>(table, table_size, 12.99), DOUBLE_DELTA);
    assert_eq(6.94, Math::lookup_periodic<true>(table, table_size, 13.01), DOUBLE_DELTA);
    assert_eq(2.7, Math::lookup_periodic<true>(table, table_size, 15.7), DOUBLE_DELTA);
    assert_eq(6.3, Math::lookup_periodic<false>(table, table_size, -15.7), DOUBLE_DELTA);
    assert_eq(3.8, Math::lookup_periodic<false>(table, table_size, -11.2), DOUBLE_DELTA);
    assert_eq(7.0, Math::lookup_periodic<false>(table, table_size, -8.0), DOUBLE_DELTA);
    assert_eq(2.8, Math::lookup_periodic<false>(table, table_size, -7.3), DOUBLE_DELTA);
    assert_eq(1.0, Math::lookup_periodic<false>(table, table_size, -7.0), DOUBLE_DELTA);
    assert_eq(2.0, Math::lookup_periodic<false>(table, table_size, -6.0), DOUBLE_DELTA);
})


TEST(lookup_periodic_2, {
    constexpr Integer table_size = 8;
    constexpr Integer table_mask = 7;
    Number const table[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};

    assert_eq(8.0, Math::lookup_periodic_2(table, table_size, table_mask, 7.0), DOUBLE_DELTA);
    assert_eq(7.3, Math::lookup_periodic_2(table, table_size, table_mask, 7.1), DOUBLE_DELTA);
    assert_eq(1.0, Math::lookup_periodic_2(table, table_size, table_mask, 8.0), DOUBLE_DELTA);
    assert_eq(1.7, Math::lookup_periodic_2(table, table_size, table_mask, 8.7), DOUBLE_DELTA);
    assert_eq(6.0, Math::lookup_periodic_2(table, table_size, table_mask, 13.0), DOUBLE_DELTA);
    assert_eq(6.3, Math::lookup_periodic_2(table, table_size, table_mask, 13.3), DOUBLE_DELTA);
    assert_eq(7.99, Math::lookup_periodic_2(table, table_size, table_mask, 14.99), DOUBLE_DELTA);
    assert_eq(7.93, Math::lookup_periodic_2(table, table_size, table_mask, 15.01), DOUBLE_DELTA);
    assert_eq(1.7, Math::lookup_periodic_2(table, table_size, table_mask, 16.7), DOUBLE_DELTA);
    assert_eq(5.9, Math::lookup_periodic_2(table, table_size, table_mask, -16.7), DOUBLE_DELTA);
    assert_eq(6.8, Math::lookup_periodic_2(table, table_size, table_mask, -10.2), DOUBLE_DELTA);
    assert_eq(8.0, Math::lookup_periodic_2(table, table_size, table_mask, -1.0), DOUBLE_DELTA);
    assert_eq(2.7, Math::lookup_periodic_2(table, table_size, table_mask, -6.3), DOUBLE_DELTA);
    assert_eq(1.0, Math::lookup_periodic_2(table, table_size, table_mask, -8.0), DOUBLE_DELTA);
    assert_eq(2.0, Math::lookup_periodic_2(table, table_size, table_mask, -7.0), DOUBLE_DELTA);
})


TEST(statistics, {
    std::vector<Number> empty;
    std::vector<Number> one_element{1.0};
    std::vector<Number> two_elements{2.0, 1.0};
    std::vector<Number> three_elements{2.0, 3.0, 1.0};
    std::vector<Number> four_elements{3.0, 1.0, 4.0, 2.0};
    std::vector<Number> five_elements{2.0, 5.0, 3.0, 4.0, 0.0};

    Math::Statistics empty_stats;
    Math::Statistics one_stats;
    Math::Statistics two_stats;
    Math::Statistics three_stats;
    Math::Statistics four_stats;
    Math::Statistics five_stats;

    Math::compute_statistics(empty, empty_stats);
    Math::compute_statistics(one_element, one_stats);
    Math::compute_statistics(two_elements, two_stats);
    Math::compute_statistics(three_elements, three_stats);
    Math::compute_statistics(four_elements, four_stats);
    Math::compute_statistics(five_elements, five_stats);

    assert_statistics(false, 0.0, 0.0, 0.0, 0.0, 0.0, empty_stats);
    assert_statistics(true, 1.0, 1.0, 1.0, 1.0, 0.0, one_stats);
    assert_statistics(true, 1.0, 1.5, 2.0, 1.5, 0.5, two_stats);
    assert_statistics(
        true, 1.0, 2.0, 3.0, 2.0, std::sqrt(2.0 / 3.0), three_stats
    );
    assert_statistics(
        true,
        1.0,
        2.5,
        4.0,
        2.5,
        std::sqrt((1.5 * 1.5 * 2.0 + 0.5 * 0.5 * 2.0) / 4.0),
        four_stats
    );
    assert_statistics(
        true,
        0.0,
        3.0,
        5.0,
        2.8,
        std::sqrt(
            (2.2 * 2.2 + 1.2 * 1.2 + 0.2 * 0.2 + 0.8 * 0.8 + 2.8 * 2.8) / 5.0
        ),
        five_stats
    );
})


void assert_distorted(
        Number const expected,
        Number const level,
        Number const number,
        Number const tolerance
) {
    assert_eq(
        expected,
        Math::distort(level, number),
        tolerance,
        "level=%f, number=%f",
        level,
        number
    );
    assert_eq(
        expected - 0.5,
        Math::distort_centered_lfo(level, number - 0.5),
        tolerance,
        "level=%f, number=%f",
        level,
        number
    );
}


TEST(distort, {
    constexpr Number tolerance = 0.01;

    assert_distorted(0.0, 1.0, 0.0, DOUBLE_DELTA);
    assert_distorted(1.0, 1.0, 1.0, tolerance);
    assert_distorted(0.0, 1.0, 0.1, tolerance);
    assert_distorted(0.0, 1.0, 0.2, tolerance);
    assert_distorted(0.5, 1.0, 0.5, tolerance);
    assert_distorted(1.0, 1.0, 0.8, tolerance);
    assert_distorted(1.0, 1.0, 0.9, tolerance);

    assert_distorted(0.0, 0.5, 0.0, DOUBLE_DELTA);
    assert_gt(0.1, Math::distort(0.5, 0.1));
    assert_gt(0.2, Math::distort(0.5, 0.2));
    assert_distorted(0.5, 0.5, 0.5, tolerance);
    assert_lt(0.8, Math::distort(0.5, 0.8));
    assert_lt(0.9, Math::distort(0.5, 0.9));
    assert_distorted(1.0, 0.5, 1.0, DOUBLE_DELTA);

    assert_distorted(0.0, 0.0, 0.0, DOUBLE_DELTA);
    assert_distorted(0.1, 0.0, 0.1, DOUBLE_DELTA);
    assert_distorted(0.2, 0.0, 0.2, DOUBLE_DELTA);
    assert_distorted(0.5, 0.0, 0.5, DOUBLE_DELTA);
    assert_distorted(0.8, 0.0, 0.8, DOUBLE_DELTA);
    assert_distorted(0.9, 0.0, 0.9, DOUBLE_DELTA);
    assert_distorted(1.0, 0.0, 1.0, DOUBLE_DELTA);
})


TEST(randomize, {
    constexpr Integer last_probe = 500;
    std::vector<Number> numbers(last_probe + 1);
    Math::Statistics statistics;

    for (Integer i = 0; i != last_probe; ++i) {
        Number const number = (Number)i / (Number)last_probe;
        numbers[i] = Math::randomize(1.0, number);
        assert_eq(number, Math::randomize(0.2, number), 0.21);
    }

    numbers[last_probe] = Math::randomize(1.0, 1.0);

    Math::compute_statistics(numbers, statistics);
    assert_statistics(true, 0.0, 0.5, 1.0, 0.5, 0.25, statistics, 0.02);
    assert_eq(Math::randomize(1.0, 1.0), Math::randomize(1.0, 99999.0));
})


TEST(randomize_centered_lfo, {
    constexpr Integer last_probe = 500;
    std::vector<Number> numbers(last_probe + 1);
    Math::Statistics statistics;

    for (Integer i = 0; i != last_probe; ++i) {
        Number const number = (Number)i / (Number)last_probe - 0.5;
        numbers[i] = Math::randomize_centered_lfo(1.0, number);
        assert_eq(number, Math::randomize_centered_lfo(0.2, number), 0.21);
    }

    numbers[last_probe] = Math::randomize_centered_lfo(1.0, 0.5);

    Math::compute_statistics(numbers, statistics);
    assert_statistics(true, -0.5, 0.0, 0.5, 0.0, 0.25, statistics, 0.02);
    assert_eq(
        Math::randomize_centered_lfo(1.0, 0.5),
        Math::randomize_centered_lfo(1.0, 99999.0)
    );
})


TEST(ratio_to_exact_log_biquad_filter_frequency, {
    constexpr Number min = Constants::BIQUAD_FILTER_FREQUENCY_MIN;
    constexpr Number max = Constants::BIQUAD_FILTER_FREQUENCY_MAX;

    assert_eq(min, Math::ratio_to_exact_log_biquad_filter_frequency(0.0), DOUBLE_DELTA);
    assert_eq(max, Math::ratio_to_exact_log_biquad_filter_frequency(1.0), DOUBLE_DELTA);
    assert_eq(
        std::sqrt(min * (max / min)),
        Math::ratio_to_exact_log_biquad_filter_frequency(0.5),
        DOUBLE_DELTA
    );
})


TEST(ratio_to_exact_log_biquad_filter_q, {
    constexpr Number min = Constants::BIQUAD_FILTER_Q_MIN;
    constexpr Number max = Constants::BIQUAD_FILTER_Q_MAX;
    constexpr Number min_p1 = min + 1.0;
    constexpr Number max_p1 = max + 1.0;

    assert_eq(min, Math::ratio_to_exact_log_biquad_filter_q(0.0), DOUBLE_DELTA);
    assert_eq(max, Math::ratio_to_exact_log_biquad_filter_q(1.0), DOUBLE_DELTA);
    assert_eq(
        std::sqrt(min_p1 * (max_p1 / min_p1)) - 1.0,
        Math::ratio_to_exact_log_biquad_filter_q(0.5),
        DOUBLE_DELTA
    );
})


TEST(is_abs_small, {
    assert_true(Math::is_abs_small(0.01, 0.1));
    assert_true(Math::is_abs_small(-0.01, 0.1));
    assert_false(Math::is_abs_small(0.2, 0.1));
    assert_false(Math::is_abs_small(-0.2, 0.1));
})


TEST(is_close, {
    assert_true(Math::is_close(1.0, 1.05, 0.1));
    assert_true(Math::is_close(-1.0, -1.05, 0.1));
    assert_false(Math::is_close(1.0, 1.2, 0.1));
    assert_false(Math::is_close(-1.0, -1.2, 0.1));
})
