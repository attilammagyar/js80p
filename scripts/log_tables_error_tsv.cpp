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

#include <cstdio>

#include "js80p.hpp"
#include "dsp/math.cpp"

using namespace JS80P;


Number* build_log_chorus_lfo_freq_lookup_table_without_correction()
{
    Number* table = new Number[Math::LOG_CHORUS_LFO_FREQ_TABLE_SIZE];

    Math::init_log_table(
        table,
        Math::LOG_CHORUS_LFO_FREQ_TABLE_MAX_INDEX,
        Math::LOG_CHORUS_LFO_FREQ_TABLE_MAX_INDEX_INV,
        Constants::CHORUS_LFO_FREQUENCY_MIN,
        Constants::CHORUS_LFO_FREQUENCY_MAX,
        0.0,
        [](Number const ratio) -> Number {
            return Math::ratio_to_exact_log_chorus_lfo_frequency(ratio);
        }
    );

    return table;
}


Number* build_log_lfo_freq_lookup_table_without_correction()
{
    Number* table = new Number[Math::LOG_LFO_FREQ_TABLE_SIZE];

    Math::init_log_table(
        table,
        Math::LOG_LFO_FREQ_TABLE_MAX_INDEX,
        Math::LOG_LFO_FREQ_TABLE_MAX_INDEX_INV,
        Constants::LFO_FREQUENCY_MIN,
        Constants::LFO_FREQUENCY_MAX,
        0.0,
        [](Number const ratio) -> Number {
            return Math::ratio_to_exact_log_lfo_frequency(ratio);
        }
    );

    return table;
}


Number* build_log_biquad_filter_freq_lookup_table_without_correction()
{
    Number* table = new Number[Math::LOG_BIQUAD_FILTER_FREQ_TABLE_SIZE];

    Math::init_log_table(
        table,
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX_INV,
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        0.0,
        [](Number const ratio) -> Number {
            return Math::ratio_to_exact_log_biquad_filter_frequency(ratio);
        }
    );

    return table;
}


Number* build_log_biquad_filter_q_lookup_table_without_correction()
{
    Number* table = new Number[Math::LOG_BIQUAD_FILTER_Q_TABLE_SIZE];

    Math::init_log_table(
        table,
        Math::LOG_BIQUAD_FILTER_Q_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_Q_TABLE_MAX_INDEX_INV,
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        0.0,
        [](Number const ratio) -> Number {
            return Math::ratio_to_exact_log_biquad_filter_q(ratio);
        }
    );

    return table;
}


int main(int argc, char* argv[])
{
    constexpr int resolution = 5000000;
    constexpr Number scale = 1.0 / (Number)(resolution - 1);

    constexpr int chorus_lfo_freq_max_index = Math::LOG_CHORUS_LFO_FREQ_TABLE_MAX_INDEX;
    constexpr int lfo_freq_max_index = Math::LOG_LFO_FREQ_TABLE_MAX_INDEX;
    constexpr int filter_freq_max_index = Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX;
    constexpr int filter_q_max_index = Math::LOG_BIQUAD_FILTER_Q_TABLE_MAX_INDEX;

    fprintf(
        stdout,
        "ratio"
        "\tchorus-lfo-freq-error-without-correction\tchorus-lfo-freq-error-with-correction\tchorus-lfo-freq-abs-error-with-correction"
        "\tlfo-freq-error-without-correction\tlfo-freq-error-with-correction\tlfo-freq-abs-error-with-correction"
        "\tfilter-freq-error-without-correction\tfilter-freq-error-with-correction\tfilter-freq-abs-error-with-correction"
        "\tfilter-q-error-without-correction\tfilter-q-error-with-correction\tfilter-q-abs-error-with-correction"
        "\n"
    );

    Number const* chorus_lfo_freq_with_correction = Math::log_chorus_lfo_freq_table();
    Number const* lfo_freq_with_correction = Math::log_lfo_freq_table();
    Number const* filter_freq_with_correction = Math::log_biquad_filter_freq_table();
    Number const* filter_q_with_correction = Math::log_biquad_filter_q_table();

    Number sum_chorus_lfo_freq_error_with_correction = 0.0;
    Number sum_chorus_lfo_freq_error_without_correction = 0.0;
    Number sum_chorus_lfo_freq_abs_error_with_correction = 0.0;

    Number sum_lfo_freq_error_with_correction = 0.0;
    Number sum_lfo_freq_error_without_correction = 0.0;
    Number sum_lfo_freq_abs_error_with_correction = 0.0;

    Number sum_filter_freq_error_with_correction = 0.0;
    Number sum_filter_freq_error_without_correction = 0.0;
    Number sum_filter_freq_abs_error_with_correction = 0.0;

    Number sum_filter_q_error_without_correction = 0.0;
    Number sum_filter_q_error_with_correction = 0.0;
    Number sum_filter_q_abs_error_with_correction = 0.0;

    Number* chorus_lfo_freq_without_correction = build_log_chorus_lfo_freq_lookup_table_without_correction();
    Number* lfo_freq_without_correction = build_log_lfo_freq_lookup_table_without_correction();
    Number* filter_freq_without_correction = build_log_biquad_filter_freq_lookup_table_without_correction();
    Number* filter_q_without_correction = build_log_biquad_filter_q_lookup_table_without_correction();

    for (int i = 0; i != resolution; ++i) {
        Number const ratio = scale * (Number)i;

        Number const chorus_lfo_freq_index = ratio * Math::LOG_CHORUS_LFO_FREQ_TABLE_INDEX_SCALE;
        Number const chorus_lfo_freq_exact = Math::ratio_to_exact_log_chorus_lfo_frequency(ratio);
        Number const chorus_lfo_freq_error_without_correction = (
            Math::lookup(
                chorus_lfo_freq_without_correction,
                chorus_lfo_freq_max_index,
                chorus_lfo_freq_index
            ) - chorus_lfo_freq_exact
        );
        Number const chorus_lfo_freq_error_with_correction = (
            Math::lookup(
                chorus_lfo_freq_with_correction,
                chorus_lfo_freq_max_index,
                chorus_lfo_freq_index
            ) - chorus_lfo_freq_exact
        );
        Number const chorus_lfo_freq_abs_error_with_correction = (
            std::abs(chorus_lfo_freq_error_with_correction)
        );

        Number const lfo_freq_index = ratio * Math::LOG_LFO_FREQ_TABLE_INDEX_SCALE;
        Number const lfo_freq_exact = Math::ratio_to_exact_log_lfo_frequency(ratio);
        Number const lfo_freq_error_without_correction = (
            Math::lookup(
                lfo_freq_without_correction,
                lfo_freq_max_index,
                lfo_freq_index
            ) - lfo_freq_exact
        );
        Number const lfo_freq_error_with_correction = (
            Math::lookup(
                lfo_freq_with_correction,
                lfo_freq_max_index,
                lfo_freq_index
            ) - lfo_freq_exact
        );
        Number const lfo_freq_abs_error_with_correction = (
            std::abs(lfo_freq_error_with_correction)
        );

        Number const filter_freq_index = ratio * Math::LOG_BIQUAD_FILTER_FREQ_TABLE_INDEX_SCALE;
        Number const filter_freq_exact = Math::ratio_to_exact_log_biquad_filter_frequency(ratio);
        Number const filter_freq_error_without_correction = (
            Math::lookup(
                filter_freq_without_correction,
                filter_freq_max_index,
                filter_freq_index
            ) - filter_freq_exact
        );
        Number const filter_freq_error_with_correction = (
            Math::lookup(
                filter_freq_with_correction,
                filter_freq_max_index,
                filter_freq_index
            ) - filter_freq_exact
        );
        Number const filter_freq_abs_error_with_correction = (
            std::abs(filter_freq_error_with_correction)
        );

        Number const filter_q_index = ratio * Math::LOG_BIQUAD_FILTER_Q_TABLE_INDEX_SCALE;
        Number const filter_q_exact = Math::ratio_to_exact_log_biquad_filter_q(ratio);
        Number const filter_q_error_without_correction = (
            Math::lookup(
                filter_q_without_correction,
                filter_q_max_index,
                filter_q_index
            ) - filter_q_exact
        );
        Number const filter_q_error_with_correction = (
            Math::lookup(
                filter_q_with_correction,
                filter_q_max_index,
                filter_q_index
            ) - filter_q_exact
        );
        Number const filter_q_abs_error_with_correction = (
            std::abs(filter_q_error_with_correction)
        );

        fprintf(
            stdout,
            (
                "%.15f\t"
                "%.15f\t%.15f\t%.15f\t"
                "%.15f\t%.15f\t%.15f\t"
                "%.15f\t%.15f\t%.15f\t"
                "%.15f\t%.15f\t%.15f\n"
            ),
            ratio,
            chorus_lfo_freq_error_without_correction,
            chorus_lfo_freq_error_with_correction,
            chorus_lfo_freq_abs_error_with_correction,
            lfo_freq_error_without_correction,
            lfo_freq_error_with_correction,
            lfo_freq_abs_error_with_correction,
            filter_freq_error_without_correction,
            filter_freq_error_with_correction,
            filter_freq_abs_error_with_correction,
            filter_q_error_without_correction,
            filter_q_error_with_correction,
            filter_q_abs_error_with_correction
        );

        sum_chorus_lfo_freq_error_without_correction += chorus_lfo_freq_error_without_correction;
        sum_chorus_lfo_freq_error_with_correction += chorus_lfo_freq_error_with_correction;
        sum_chorus_lfo_freq_abs_error_with_correction += chorus_lfo_freq_abs_error_with_correction;

        sum_lfo_freq_error_without_correction += lfo_freq_error_without_correction;
        sum_lfo_freq_error_with_correction += lfo_freq_error_with_correction;
        sum_lfo_freq_abs_error_with_correction += lfo_freq_abs_error_with_correction;

        sum_filter_freq_error_without_correction += filter_freq_error_without_correction;
        sum_filter_freq_error_with_correction += filter_freq_error_with_correction;
        sum_filter_freq_abs_error_with_correction += filter_freq_abs_error_with_correction;

        sum_filter_q_error_without_correction += filter_q_error_without_correction;
        sum_filter_q_error_with_correction += filter_q_error_with_correction;
        sum_filter_q_abs_error_with_correction += filter_q_abs_error_with_correction;
    }

    fprintf(
        stdout,
        (
            "sum:\t"
            "%.15f\t%.15f\t%.15f\t"
            "%.15f\t%.15f\t%.15f\t"
            "%.15f\t%.15f\t%.15f\t"
            "%.15f\t%.15f\t%.15f\n"
        ),
        sum_chorus_lfo_freq_error_without_correction,
        sum_chorus_lfo_freq_error_with_correction,
        sum_chorus_lfo_freq_abs_error_with_correction,
        sum_lfo_freq_error_without_correction,
        sum_lfo_freq_error_with_correction,
        sum_lfo_freq_abs_error_with_correction,
        sum_filter_freq_error_without_correction,
        sum_filter_freq_error_with_correction,
        sum_filter_freq_abs_error_with_correction,
        sum_filter_q_error_without_correction,
        sum_filter_q_error_with_correction,
        sum_filter_q_abs_error_with_correction
    );

    delete[] chorus_lfo_freq_without_correction;
    delete[] lfo_freq_without_correction;
    delete[] filter_freq_without_correction;
    delete[] filter_q_without_correction;

    return 0;
}
