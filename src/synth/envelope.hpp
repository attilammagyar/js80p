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

#ifndef JS80P__SYNTH__ENVELOPE_HPP
#define JS80P__SYNTH__ENVELOPE_HPP

#include <string>

#include "js80p.hpp"

#include "synth/param.hpp"


namespace JS80P
{

class FloatParam;


/**
 * \brief A collection of parameters specifying an envelope.
 */
class Envelope
{
    public:
        Envelope(std::string const name);

        FloatParam amount;
        FloatParam initial_value;
        FloatParam delay_time;
        FloatParam attack_time;
        FloatParam peak_value;
        FloatParam hold_time;
        FloatParam decay_time;
        FloatParam sustain_value;
        FloatParam release_time;
        FloatParam final_value;
};

}

#endif
