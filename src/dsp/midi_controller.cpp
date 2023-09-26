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

#ifndef JS80P__DSP__MIDI_CONTROLLER_CPP
#define JS80P__DSP__MIDI_CONTROLLER_CPP

#include "dsp/midi_controller.hpp"


namespace JS80P
{

MidiController::MidiController() noexcept
    : change_index(0),
    assignments(0),
    value(0.5),
    events(events_rw)
{
}


void MidiController::change(
        Seconds const time_offset,
        Number const new_value
) noexcept {
    SignalProducer::Event event(EVT_CHANGE, time_offset, 0, new_value, 0.0);

    events_rw.push(event);
    change(new_value);
}


Integer MidiController::get_change_index() const noexcept
{
    return change_index;
}


void MidiController::change(Number const new_value) noexcept
{
    value = new_value;
    ++change_index;
    change_index &= 0x7fffffff;
}


Number MidiController::get_value() const noexcept
{
    return value;
}


void MidiController::clear() noexcept
{
    events_rw.drop(0);
}


void MidiController::assigned() noexcept
{
    ++assignments;
}


void MidiController::released() noexcept
{
    if (is_assigned()) {
        --assignments;
    }
}


Integer MidiController::is_assigned() const noexcept
{
    return assignments != 0;
}

}

#endif
