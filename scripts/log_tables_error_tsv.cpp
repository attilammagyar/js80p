/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023  Attila M. Magyar
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


Number* build_log_freq_lookup_table_without_correction()
{
    Number* table = new Number[Math::LOG_BIQUAD_FILTER_FREQ_TABLE_SIZE];

    table[0] = Constants::BIQUAD_FILTER_FREQUENCY_MIN;

    for (int i = 1; i != Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX; ++i) {
        Number const idx = (Number)i;

        table[i] = Math::ratio_to_exact_log_biquad_filter_frequency(
            idx * Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX_INV
        );
    }

    table[Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX] = (
        Constants::BIQUAD_FILTER_FREQUENCY_MAX
    );

    return table;
}


Number* build_log_q_lookup_table_without_correction()
{
    Number* table = new Number[Math::LOG_BIQUAD_FILTER_Q_TABLE_SIZE];

    table[0] = Constants::BIQUAD_FILTER_Q_MIN;

    for (int i = 1; i != Math::LOG_BIQUAD_FILTER_Q_TABLE_MAX_INDEX; ++i) {
        Number const idx = (Number)i;

        table[i] = Math::ratio_to_exact_log_biquad_filter_q(
            idx * Math::LOG_BIQUAD_FILTER_Q_TABLE_MAX_INDEX_INV
        );
    }

    table[Math::LOG_BIQUAD_FILTER_Q_TABLE_MAX_INDEX] = (
        Constants::BIQUAD_FILTER_Q_MAX
    );

    return table;
}


int main(int argc, char* argv[])
{
    constexpr int resolution = 5000000;
    constexpr Number scale = 1.0 / (Number)(resolution - 1);

    constexpr int freq_max_index = Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX;
    constexpr int q_max_index = Math::LOG_BIQUAD_FILTER_Q_TABLE_MAX_INDEX;

    fprintf(
        stdout,
        "ratio"
        "\tfreq-error-without-correction\tfreq-error-with-correction\tfreq-abs-error-with-correction"
        "\tq-error-without-correction\tq-error-with-correction\tq-abs-error-with-correction"
        "\n"
    );

    Number const* freq_with_correction = Math::log_biquad_filter_freq_table();
    Number const* q_with_correction = Math::log_biquad_filter_q_table();

    Number sum_freq_error_with_correction = 0.0;
    Number sum_freq_error_without_correction = 0.0;
    Number sum_freq_abs_error_with_correction = 0.0;

    Number sum_q_error_without_correction = 0.0;
    Number sum_q_error_with_correction = 0.0;
    Number sum_q_abs_error_with_correction = 0.0;

    Number* freq_without_correction = build_log_freq_lookup_table_without_correction();
    Number* q_without_correction = build_log_q_lookup_table_without_correction();

    for (int i = 0; i != resolution; ++i) {
        Number const ratio = scale * (Number)i;

        Number const freq_index = ratio * Math::LOG_BIQUAD_FILTER_FREQ_SCALE;
        Number const freq_exact = Math::ratio_to_exact_log_biquad_filter_frequency(ratio);
        Number const freq_error_without_correction = (
            Math::lookup(freq_without_correction, freq_max_index, freq_index) - freq_exact
        );
        Number const freq_error_with_correction = (
            Math::lookup(freq_with_correction, freq_max_index, freq_index) - freq_exact
        );
        Number const freq_abs_error_with_correction = (
            std::abs(freq_error_with_correction)
        );

        Number const q_index = ratio * Math::LOG_BIQUAD_FILTER_Q_SCALE;
        Number const q_exact = Math::ratio_to_exact_log_biquad_filter_q(ratio);
        Number const q_error_without_correction = (
            Math::lookup(q_without_correction, q_max_index, q_index) - q_exact
        );
        Number const q_error_with_correction = (
            Math::lookup(q_with_correction, q_max_index, q_index) - q_exact
        );
        Number const q_abs_error_with_correction = (
            std::abs(q_error_with_correction)
        );

        fprintf(
            stdout,
            "%.15f\t%.15f\t%.15f\t%.15f\t%.15f\t%.15f\t%.15f\n",
            ratio,
            freq_error_without_correction,
            freq_error_with_correction,
            freq_abs_error_with_correction,
            q_error_without_correction,
            q_error_with_correction,
            q_abs_error_with_correction
        );

        sum_freq_error_without_correction += freq_error_without_correction;
        sum_freq_error_with_correction += freq_error_with_correction;
        sum_freq_abs_error_with_correction += freq_abs_error_with_correction;

        sum_q_error_without_correction += q_error_without_correction;
        sum_q_error_with_correction += q_error_with_correction;
        sum_q_abs_error_with_correction += q_abs_error_with_correction;
    }

    fprintf(
        stdout,
        "sum:\t%.15f\t%.15f\t%.15f\t%.15f\t%.15f\t%.15f\n",
        sum_freq_error_without_correction,
        sum_freq_error_with_correction,
        sum_freq_abs_error_with_correction,
        sum_q_error_without_correction,
        sum_q_error_with_correction,
        sum_q_abs_error_with_correction
    );

    delete[] freq_without_correction;
    delete[] q_without_correction;

    return 0;
}
