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


TEST(error_of_repeated_ratio_to_value_back_and_forth_conversion_of_logarithmic_param_is_small_and_stable, {
    constexpr int resolution = 20000;
    constexpr int iterations = 50000;
    constexpr Number tolerance_percent = 0.0021; /* 0.21% */
    constexpr Number min = Constants::BIQUAD_FILTER_FREQUENCY_MIN;
    constexpr Number max = Constants::BIQUAD_FILTER_FREQUENCY_MAX;
    constexpr Number range = max - min;
    constexpr Number resolution_inv = 1.0 / (Number)(resolution - 1);

    ToggleParam toggle("log", ToggleParam::OFF);
    FloatParamS param(
        "freq",
        min,
        max,
        Constants::BIQUAD_FILTER_FREQUENCY_DEFAULT,
        0.0,
        &toggle,
        Math::log_biquad_filter_freq_table(),
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_MAX_INDEX,
        Math::LOG_BIQUAD_FILTER_FREQ_TABLE_INDEX_SCALE
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
                value * tolerance_percent,
                "i=%d (%d), j=%d (%d)",
                i,
                resolution,
                j,
                iterations
            );
        }
    }
})
