/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024, 2025  Attila M. Magyar
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

#ifndef JS80P__DSP__MACRO_CPP
#define JS80P__DSP__MACRO_CPP

#include "dsp/macro.hpp"


namespace JS80P
{

Macro::DistortionCurveParam::DistortionCurveParam(
        std::string const& name
) noexcept
    : ByteParam(
        name,
        DIST_CURVE_SMOOTH_SMOOTH,
        DIST_CURVE_SHARP_SHARP,
        DIST_CURVE_SMOOTH_SMOOTH
    )
{
}


Macro::Macro(std::string const& name, Number const input_default_value) noexcept
    : MidiController(),
    midpoint(name + "MID", 0.0, 1.0, 0.5),
    input(name + "IN", 0.0, 1.0, input_default_value),
    min(name + "MIN", 0.0, 1.0, 0.0),
    max(name + "MAX", 0.0, 1.0, 1.0),
    scale(name + "AMT", 0.0, 1.0, 1.0),
    distortion(name + "DST", 0.0, 1.0, 0.0),
    randomness(name + "RND", 0.0, 1.0, 0.0),
    distortion_curve(name + "DSH"),
    midpoint_change_index(0),
    input_change_index(0),
    min_change_index(0),
    max_change_index(0),
    scale_change_index(0),
    distortion_change_index(0),
    randomness_change_index(0),
    distortion_curve_change_index(0),
    is_updating(false)
{
}


void Macro::update() noexcept
{
    if (is_updating) {
        return;
    }

    is_updating = true;

    if (!update_change_indices()) {
        is_updating = false;

        return;
    }

    Number const midpoint_value = midpoint.get_value();
    Number const input_value = input.get_value();
    Number const shifted_input_value = (
        input_value < 0.5
            ? 2.0 * input_value * midpoint_value
            : (midpoint_value + (2.0 * input_value - 1.0) * (1.0 - midpoint_value))
    );
    Number const min_value = min.get_value();
    Number const computed_value = Math::randomize(
        randomness.get_value(),
        Math::distort(
            distortion.get_value(),
            shifted_input_value,
            (Math::DistortionCurve)distortion_curve.get_value()
        )
    );

    MidiController::change(
        min_value
        + computed_value * scale.get_value() * (max.get_value() - min_value)
    );

    is_updating = false;
}


bool Macro::update_change_indices() noexcept
{
    bool is_dirty;

    is_dirty = update_change_index<FloatParamB>(midpoint, midpoint_change_index);
    is_dirty = update_change_index<FloatParamB>(input, input_change_index) || is_dirty;
    is_dirty = update_change_index<FloatParamB>(min, min_change_index) || is_dirty;
    is_dirty = update_change_index<FloatParamB>(max, max_change_index) || is_dirty;
    is_dirty = update_change_index<FloatParamB>(scale, scale_change_index) || is_dirty;
    is_dirty = update_change_index<FloatParamB>(distortion, distortion_change_index) || is_dirty;
    is_dirty = update_change_index<FloatParamB>(randomness, randomness_change_index) || is_dirty;
    is_dirty = update_change_index<DistortionCurveParam>(distortion_curve, distortion_curve_change_index) || is_dirty;

    return is_dirty;
}


template<class ParamClass>
bool Macro::update_change_index(
        ParamClass& param,
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
