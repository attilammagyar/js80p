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

#ifndef JS80P__DSP__FLEXIBLE_CONTROLLER_HPP
#define JS80P__DSP__FLEXIBLE_CONTROLLER_HPP

#include <string>

#include "js80p.hpp"

#include "dsp/midi_controller.hpp"
#include "dsp/param.hpp"


namespace JS80P
{

class FloatParam;


/**
 * \brief Adjust the value of the \c input \c FloatParam, so that if that has a
 *        \c MidiController assigned, then the \c FlexibleController can be used
 *        as an adjustable version of that controller.
 */
class FlexibleController : public MidiController
{
    public:
        FlexibleController(std::string const name = "") noexcept;

        void update() noexcept;

        FloatParam input;
        FloatParam min;
        FloatParam max;
        FloatParam amount;
        FloatParam distortion;
        FloatParam randomness;

    private:
        bool update_change_indices() noexcept;
        bool update_change_index(
            FloatParam& param, Integer& change_index
        ) const noexcept;

        Integer input_change_index;
        Integer min_change_index;
        Integer max_change_index;
        Integer amount_change_index;
        Integer distortion_change_index;
        Integer randomness_change_index;
        bool is_updating;
};

}

#endif
