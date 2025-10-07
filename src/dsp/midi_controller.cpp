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

#ifndef JS80P__DSP__MIDI_CONTROLLER_CPP
#define JS80P__DSP__MIDI_CONTROLLER_CPP

#include <algorithm>

#include "dsp/midi_controller.hpp"


namespace JS80P
{

MidiController::MidiController() noexcept
    : event_queues_rw{
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
        Queue<SignalProducer::Event>{32},
    },
    change_indices{},
    assignments(0),
    event_queues(event_queues_rw)
{
    std::fill_n(values, Midi::CHANNELS, 0.5);
}


void MidiController::change(
        Midi::Channel const channel,
        Seconds const time_offset,
        Number const new_value
) noexcept {
    SignalProducer::Event event(EVT_CHANGE, time_offset, 0, new_value, 0.0);

    event_queues_rw[channel].push(event);
    change(channel, new_value);
}


void MidiController::change_all_channels(
        Seconds const time_offset,
        Number const new_value
) noexcept {
    SignalProducer::Event event(EVT_CHANGE, time_offset, 0, new_value, 0.0);

    for (Midi::Channel channel = 0; channel != Midi::CHANNELS; ++channel) {
        event_queues_rw[channel].push(event);
        change(channel, new_value);
    }
}


Integer MidiController::get_change_index(Midi::Channel const channel) const noexcept
{
    return change_indices[channel];
}


void MidiController::change(Midi::Channel const channel, Number const new_value) noexcept
{
    values[channel] = new_value;
    ++change_indices[channel];
    change_indices[channel] &= 0x7fffffff;
}


Number MidiController::get_value(Midi::Channel const channel) const noexcept
{
    return values[channel];
}


void MidiController::clear() noexcept
{
    for (Midi::Channel channel = 0; channel != Midi::CHANNELS; ++channel) {
        event_queues_rw[channel].drop(0);
    }
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
