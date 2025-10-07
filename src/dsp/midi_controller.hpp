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

#ifndef JS80P__DSP__MIDI_CONTROLLER_HPP
#define JS80P__DSP__MIDI_CONTROLLER_HPP

#include "js80p.hpp"
#include "midi.hpp"

#include "dsp/queue.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

/**
 * \brief MIDI Control change event log.
 */
class MidiController
{
    public:
        static constexpr SignalProducer::Event::Type EVT_CHANGE = 1;

        MidiController() noexcept;

        /**
         * \brief Store the new value of the controller, and also queue it as
         *        an event with a time offset for sample-exact parameters.
         */
        void change(
            Midi::Channel const channel,
            Seconds const time_offset,
            Number const new_value
        ) noexcept;

        /**
         * \brief Store the new value of the controller, and also queue it as
         *        an event with a time offset for sample-exact parameters for
         *        all channels.
         */
        void change_all_channels(Seconds const time_offset, Number const new_value) noexcept;

        Integer get_change_index(Midi::Channel const channel) const noexcept;

        Number get_value(Midi::Channel const channel) const noexcept;

        void clear() noexcept;

        void assigned() noexcept;
        void released() noexcept;
        Integer is_assigned() const noexcept;

    protected:
        void change(Midi::Channel const channel, Number const new_value) noexcept;

    private:
        Queue<SignalProducer::Event> event_queues_rw[Midi::CHANNELS];
        Integer change_indices[Midi::CHANNELS];
        Number values[Midi::CHANNELS];
        Integer assignments;

    public:
        Queue<SignalProducer::Event> const* const event_queues;
};

}

#endif
