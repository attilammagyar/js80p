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

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "dsp/envelope.cpp"
#include "dsp/lfo.cpp"
#include "dsp/macro.cpp"
#include "dsp/math.cpp"
#include "dsp/midi_controller.cpp"
#include "dsp/oscillator.cpp"
#include "dsp/param.cpp"
#include "dsp/queue.cpp"
#include "dsp/signal_producer.cpp"
#include "dsp/wavetable.cpp"


using namespace JS80P;


void assert_error_of_repeated_ratio_to_log_scale_value_and_back_conversion_is_low_and_stable(
        Number const* table,
        int const max_index,
        Number const index_scale,
        Number const min,
        Number const max,
        Number const default_value,
        Number const value_offset,
        Number const tolerance_percent,
        Number const min_tolerance
) {
    constexpr int resolution = 20000;
    constexpr int iterations = 50000;
    constexpr Number resolution_inv = 1.0 / (Number)(resolution - 1);

    Number const range = max - min;

    ToggleParam toggle("log", ToggleParam::OFF);
    FloatParamS param(
        "p",
        min,
        max,
        default_value,
        0.0,
        &toggle,
        table,
        max_index,
        index_scale,
        value_offset
    );

    toggle.set_value(ToggleParam::ON);

    for (int i = 0; i != resolution; ++i) {
        Number const value = min + range * ((Number)i) * resolution_inv;

        param.set_value(value);

        for (int j = 0; j != iterations; ++j) {
            param.set_ratio(param.get_ratio());

            assert_eq(
                value,
                param.get_value(),
                std::max(min_tolerance, value * tolerance_percent),
                "i=%d (%d), j=%d (%d)",
                i,
                resolution,
                j,
                iterations
            );
        }
    }
}


TEST(error_of_repeated_ratio_to_log_scale_value_and_back_conversion_is_low_and_stable, {
    assert_error_of_repeated_ratio_to_log_scale_value_and_back_conversion_is_low_and_stable(
        Math::log_biquad_filter_freq_table(),
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_INDEX_SCALE,
        Constants::BIQUAD_FILTER_FREQUENCY_MIN,
        Constants::BIQUAD_FILTER_FREQUENCY_MAX,
        Constants::BIQUAD_FILTER_FREQUENCY_DEFAULT,
        0.0,
        0.0021, /* 0.21% */
        0.0
    );
    assert_error_of_repeated_ratio_to_log_scale_value_and_back_conversion_is_low_and_stable(
        Math::log_biquad_filter_q_table(),
        Math::LOG_BIQUAD_FILTER_Q_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_Q_TABLE_INDEX_SCALE,
        Constants::BIQUAD_FILTER_Q_MIN,
        Constants::BIQUAD_FILTER_Q_MAX,
        Constants::BIQUAD_FILTER_Q_DEFAULT,
        1.0,
        0.0029,
        0.0054
    );
})
