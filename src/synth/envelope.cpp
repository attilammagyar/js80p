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

#ifndef JS80P__SYNTH__ENVELOPE_CPP
#define JS80P__SYNTH__ENVELOPE_CPP

#include "envelope.hpp"


namespace JS80P
{

Envelope::Envelope(std::string const name)
    : SignalProducer(0, 10),
    amount(name + "AMT",            0.0,    1.0,  1.0),
    initial_value(name + "INI",     0.0,    1.0,  0.0),
    delay_time(name + "DEL",        0.0,    6.0,  0.0),
    attack_time(name + "ATK",       0.0,    6.0,  0.02),
    peak_value(name + "PK",         0.0,    1.0,  1.0),
    hold_time(name + "HLD",         0.0,   12.0,  0.3),
    decay_time(name + "DEC",        0.001, 15.0,  0.6),
    sustain_value(name + "SUS",     0.0,    1.0,  0.7),
    release_time(name + "REL",      0.0,    6.0,  0.1),
    final_value(name + "FIN",       0.0,    1.0,  0.0)
{
    register_child(amount);
    register_child(initial_value);
    register_child(delay_time);
    register_child(attack_time);
    register_child(peak_value);
    register_child(hold_time);
    register_child(decay_time);
    register_child(sustain_value);
    register_child(release_time);
    register_child(final_value);
}

}

#endif
