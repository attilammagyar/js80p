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

#include "synth/envelope.hpp"


namespace JS80P
{

Envelope::Envelope(std::string const name) noexcept
    : dynamic(name + "DYN", ToggleParam::OFF),
    amount(name + "AMT",            0.0,    1.0,  1.0),
    initial_value(name + "INI",     0.0,    1.0,  0.0),
    delay_time(name + "DEL",        0.0,    6.0,  0.0),
    attack_time(name + "ATK",       0.0,    6.0,  0.02),
    peak_value(name + "PK",         0.0,    1.0,  1.0),
    hold_time(name + "HLD",         0.0,   12.0,  0.3),
    decay_time(name + "DEC",        0.001, 15.0,  0.6),
    sustain_value(name + "SUS",     0.0,    1.0,  0.7),
    release_time(name + "REL",      0.0,    6.0,  0.1),
    final_value(name + "FIN",       0.0,    1.0,  0.0),
    dynamic_change_index(-1),
    amount_change_index(-1),
    initial_value_change_index(-1),
    delay_time_change_index(-1),
    attack_time_change_index(-1),
    peak_value_change_index(-1),
    hold_time_change_index(-1),
    decay_time_change_index(-1),
    sustain_value_change_index(-1),
    release_time_change_index(-1),
    final_value_change_index(-1),
    change_index(-1)
{
    update();
}


void Envelope::update() noexcept
{
    bool is_dirty;

    is_dirty = update_change_index(delay_time, delay_time_change_index);
    is_dirty |= update_change_index(attack_time, attack_time_change_index);
    is_dirty |= update_change_index(hold_time, hold_time_change_index);
    is_dirty |= update_change_index(decay_time, decay_time_change_index);

    if (is_dirty) {
        dahd_length = (
            delay_time.get_value()
            + attack_time.get_value()
            + hold_time.get_value()
            + decay_time.get_value()
        );
    }

    is_dirty |= update_change_index<ToggleParam>(dynamic, dynamic_change_index);
    is_dirty |= update_change_index(amount, amount_change_index);
    is_dirty |= update_change_index(initial_value, initial_value_change_index);
    is_dirty |= update_change_index(peak_value, peak_value_change_index);
    is_dirty |= update_change_index(sustain_value, sustain_value_change_index);
    is_dirty |= update_change_index(release_time, release_time_change_index);
    is_dirty |= update_change_index(final_value, final_value_change_index);

    if (is_dirty) {
        ++change_index;
        change_index &= 0x7fffffff;
    }
}


Integer Envelope::get_change_index() const noexcept
{
    return change_index;
}


Seconds Envelope::get_dahd_length() const noexcept
{
    return dahd_length;
}


template<class ParamType = FloatParam>
bool Envelope::update_change_index(ParamType const& param, Integer& change_index)
{
    Integer const new_change_index = param.get_change_index();

    if (new_change_index != change_index) {
        change_index = new_change_index;

        return true;
    }

    return false;
}

}

#endif
