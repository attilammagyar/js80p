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

#include "test.cpp"
#include "utils.hpp"

#include <cstddef>

#include "js80p.hpp"

#include "serializer.hpp"
#include "synth.hpp"

#include "gui/gui.hpp"


using namespace JS80P;


void assert_ratio_as_str(
        Synth const& synth,
        char const* const expected,
        Synth::ParamId param_id,
        Number const ratio,
        Number const scale,
        char const* const format,
        char const* const* const options = NULL,
        size_t const number_of_options = 0
) {
    constexpr size_t buffer_size = 16;
    char buffer[buffer_size];

    GUI::param_ratio_to_str(
        synth,
        param_id,
        ratio,
        scale,
        format,
        options,
        number_of_options,
        buffer,
        buffer_size
    );

    assert_eq(
        expected,
        buffer,
        "ratio=%f, format=\"%s\", number_of_options=%d",
        ratio,
        format == NULL ? "<NULL>" : format,
        (int)number_of_options
    );
}


TEST(param_ratio_to_str, {
    constexpr size_t number_of_options = 6;
    constexpr Synth::ParamId mc1 = Synth::ParamId::MC1;
    constexpr Synth::ParamId mwav = Synth::ParamId::MWAV;
    char const* const options[number_of_options] = {
        "first",
        "second",
        "third",
        "fourth",
        "fifth",
        "sixth",
    };

    Synth synth;

    assert_ratio_as_str(synth, "-1.000", mc1, 0.0, 1.0, "%.3f", NULL, 0);
    assert_ratio_as_str(synth, "1.000", mc1, 1.0, 1.0, "%.3f", NULL, 0);
    assert_ratio_as_str(synth, "-10.000", mc1, 0.0, 10.0, "%.3f", NULL, 0);
    assert_ratio_as_str(synth, "10.000", mc1, 1.0, 10.0, "%.3f", NULL, 0);
    assert_ratio_as_str(synth, "-5.000", mc1, 0.25, 10.0, "%.3f", NULL, 0);
    assert_ratio_as_str(synth, "5.000", mc1, 0.75, 10.0, "%.3f", NULL, 0);
    assert_ratio_as_str(synth, "0.000", mc1, 0.5, 10.0, "%.3f", NULL, 0);
    assert_ratio_as_str(synth, "0.00", mc1, 0.4999999, 10.0, "%.2f", NULL, 0);

    assert_ratio_as_str(synth, "first", mwav, 0.0, 0.0, NULL, options, number_of_options);
    assert_ratio_as_str(synth, "second", mwav, 1.0 / 10.0, 0.0, NULL, options, number_of_options);
    assert_ratio_as_str(synth, "third", mwav, 2.0 / 10.0, 0.0, NULL, options, number_of_options);
    assert_ratio_as_str(synth, "", mwav, 1.0, 0.0, NULL, options, number_of_options);
    assert_ratio_as_str(synth, "", mwav, -1.0, 0.0, NULL, options, number_of_options);
})


TEST(clamp_ratio, {
    assert_eq(0.0, GUI::clamp_ratio(-0.1), DOUBLE_DELTA);
    assert_eq(0.0, GUI::clamp_ratio(-0.0), DOUBLE_DELTA);
    assert_eq(0.0, GUI::clamp_ratio(0.0), DOUBLE_DELTA);
    assert_eq(0.1, GUI::clamp_ratio(0.1), DOUBLE_DELTA);
    assert_eq(1.0, GUI::clamp_ratio(1.0), DOUBLE_DELTA);
    assert_eq(1.0, GUI::clamp_ratio(1.1), DOUBLE_DELTA);
})


TEST(gui_initialization, {
    Synth synth;

    GUI gui(NULL, NULL, NULL, synth, false);
    gui.show();
})
