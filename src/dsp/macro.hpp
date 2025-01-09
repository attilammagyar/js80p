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

#ifndef JS80P__DSP__MACRO_HPP
#define JS80P__DSP__MACRO_HPP

#include <string>

#include "js80p.hpp"

#include "dsp/math.hpp"
#include "dsp/midi_controller.hpp"
#include "dsp/param.hpp"


namespace JS80P
{

/**
 * \brief Adjust the value of the \c input \c FloatParamB, so that if that has a
 *        \c MidiController assigned, then the \c Macro can be used as an
 *        adjustable version of that controller.
 */
class Macro : public MidiController
{
    public:
        static constexpr Byte DIST_CURVE_SMOOTH_SMOOTH = Math::DistortionCurve::DIST_CURVE_SMOOTH_SMOOTH;
        static constexpr Byte DIST_CURVE_SMOOTH_SHARP = Math::DistortionCurve::DIST_CURVE_SMOOTH_SHARP;
        static constexpr Byte DIST_CURVE_SHARP_SMOOTH = Math::DistortionCurve::DIST_CURVE_SHARP_SMOOTH;
        static constexpr Byte DIST_CURVE_SHARP_SHARP = Math::DistortionCurve::DIST_CURVE_SHARP_SHARP;

        static constexpr Integer PARAMS = 8;

        class DistortionCurveParam : public ByteParam
        {
            public:
                explicit DistortionCurveParam(std::string const& name) noexcept;
        };

        explicit Macro(
            std::string const& name = "",
            Number const input_default_value = 0.5
        ) noexcept;

        void update() noexcept;

        FloatParamB midpoint;
        FloatParamB input;
        FloatParamB min;
        FloatParamB max;
        FloatParamB scale;
        FloatParamB distortion;
        FloatParamB randomness;
        DistortionCurveParam distortion_curve;

    private:
        bool update_change_indices() noexcept;

        template<class ParamClass>
        bool update_change_index(
            ParamClass& param,
            Integer& change_index
        ) const noexcept;

        Integer midpoint_change_index;
        Integer input_change_index;
        Integer min_change_index;
        Integer max_change_index;
        Integer scale_change_index;
        Integer distortion_change_index;
        Integer randomness_change_index;
        Integer distortion_curve_change_index;
        bool is_updating;
};

}

#endif
