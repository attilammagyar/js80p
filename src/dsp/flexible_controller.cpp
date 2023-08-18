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

#ifndef JS80P__DSP__FLEXIBLE_CONTROLLER_CPP
#define JS80P__DSP__FLEXIBLE_CONTROLLER_CPP

#include "dsp/flexible_controller.hpp"


namespace JS80P
{

FlexibleController::FlexibleController(std::string const name) noexcept
    : MidiController(),
    input(name + "IN", 0.0, 1.0, 0.5),
    min(name + "MIN", 0.0, 1.0, 0.0),
    max(name + "MAX", 0.0, 1.0, 1.0),
    amount(name + "AMT", 0.0, 1.0, 1.0),
    distortion(name + "DST", 0.0, 1.0, 0.0),
    randomness(name + "RND", 0.0, 1.0, 0.0),
    input_change_index(0),
    min_change_index(0),
    max_change_index(0),
    amount_change_index(0),
    distortion_change_index(0),
    randomness_change_index(0),
    is_updating(false)
{
}


void FlexibleController::update() noexcept
{
    if (is_updating) {
        return;
    }

    is_updating = true;

    if (!update_change_indices()) {
        is_updating = false;

        return;
    }

    Number const min_value = min.get_value();
    Number const computed_value = Math::randomize(
        randomness.get_value(),
        Math::distort(distortion.get_value(), input.get_value())
    );

    MidiController::change(
        min_value
        + computed_value * amount.get_value() * (max.get_value() - min_value)
    );

    is_updating = false;
}


bool FlexibleController::update_change_indices() noexcept
{
    bool is_dirty;

    is_dirty = update_change_index(input, input_change_index);
    is_dirty |= update_change_index(min, min_change_index);
    is_dirty |= update_change_index(max, max_change_index);
    is_dirty |= update_change_index(amount, amount_change_index);
    is_dirty |= update_change_index(distortion, distortion_change_index);
    is_dirty |= update_change_index(randomness, randomness_change_index);

    return is_dirty;
}


bool FlexibleController::update_change_index(
        FloatParamB& param,
        Integer& change_index
) const noexcept {
    Integer const new_change_index = param.get_change_index();

    if (new_change_index == change_index) {
        return false;
    }

    change_index = new_change_index;

    return true;
}

}

#endif
