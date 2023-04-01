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

#ifndef JS80P__SYNTH__EFFECTS_CPP
#define JS80P__SYNTH__EFFECTS_CPP

#include "synth/effects.hpp"


namespace JS80P { namespace Effects
{

template<class InputSignalProducerClass>
Effects<InputSignalProducerClass>::Effects(
        std::string const name,
        InputSignalProducerClass& input
) : Filter< Reverb<InputSignalProducerClass> >(reverb, 8, input.get_channels()),
    overdrive(name + "O", 3.0, input),
    distortion(name + "D", 10.0, overdrive),
    filter_1_type(name + "F1TYP"),
    filter_2_type(name + "F2TYP"),
    filter_1(name + "F1", distortion, filter_1_type),
    filter_2(name + "F2", filter_1, filter_2_type),
    echo(name + "E", filter_2),
    reverb(name + "R", echo)
{
    this->register_child(overdrive);
    this->register_child(distortion);
    this->register_child(filter_1_type);
    this->register_child(filter_2_type);
    this->register_child(filter_1);
    this->register_child(filter_2);
    this->register_child(echo);
    this->register_child(reverb);
}

} }

#endif
