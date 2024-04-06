/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2024  Attila M. Magyar
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

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"
#include "midi.hpp"


using namespace JS80P;


class MidiEventLogger : public Midi::EventHandler
{
    public:
        void note_off(
                Seconds const time_offset,
                Midi::Channel const channel,
                Midi::Note const note,
                Midi::Byte const velocity
        ) noexcept {
            log_event(
                "NOTE_OFF", time_offset, channel, (Byte)note, (Byte)velocity
            );
        }

        void note_on(
                Seconds const time_offset,
                Midi::Channel const channel,
                Midi::Note const note,
                Midi::Byte const velocity
        ) noexcept {
            log_event(
                "NOTE_ON", time_offset, channel, (Byte)note, (Byte)velocity
            );
        }

        void aftertouch(
                Seconds const time_offset,
                Midi::Channel const channel,
                Midi::Note const note,
                Midi::Byte const pressure
        ) noexcept {
            log_event(
                "AFTERTOUCH", time_offset, channel, (Byte)note, (Byte)pressure
            );
        }

        void control_change(
                Seconds const time_offset,
                Midi::Channel const channel,
                Midi::Controller const controller,
                Midi::Byte const new_value
        ) noexcept {
            log_event(
                "CONTROL_CHANGE",
                time_offset,
                channel,
                (Byte)controller,
                (Byte)new_value
            );
        }

        void program_change(
                Seconds const time_offset,
                Midi::Channel const channel,
                Midi::Byte const new_program
        ) noexcept {
            log_event(
                "PROGRAM_CHANGE", time_offset, channel, (Byte)new_program
            );
        }

        void channel_pressure(
                Seconds const time_offset,
                Midi::Channel const channel,
                Midi::Byte const pressure
        ) noexcept {
            log_event("CHANNEL_PRESSURE", time_offset, channel, (Byte)pressure);
        }

        void pitch_wheel_change(
                Seconds const time_offset,
                Midi::Channel const channel,
                Midi::Word const new_value
        ) noexcept {
            log_event("PITCH_WHEEL", time_offset, channel, new_value);
        }

        void all_sound_off(
                Seconds const time_offset,
                Midi::Channel const channel
        ) noexcept {
            log_event("ALL_SOUND_OFF", time_offset, channel);
        }

        void reset_all_controllers(
                Seconds const time_offset,
                Midi::Channel const channel
        ) noexcept {
            log_event("RESET_ALL_CONTROLLERS", time_offset, channel);
        }

        void all_notes_off(
                Seconds const time_offset,
                Midi::Channel const channel
        ) noexcept {
            log_event("ALL_NOTES_OFF", time_offset, channel);
        }

        void mono_mode_on(
                Seconds const time_offset,
                Midi::Channel const channel
        ) noexcept {
            log_event("MONO_MODE_ON", time_offset, channel);
        }

        void mono_mode_off(
                Seconds const time_offset,
                Midi::Channel const channel
        ) noexcept {
            log_event("MONO_MODE_OFF", time_offset, channel);
        }

        std::string events;

    private:
        void log_event(
                char const* const event_name,
                Seconds const time_offset,
                Midi::Channel const channel
        ) {
            char buffer[128];

            snprintf(
                buffer, 128, "%s %.1f 0x%02hhx\n", event_name, time_offset, channel
            );

            events += buffer;
        }

        void log_event(
                char const* const event_name,
                Seconds const time_offset,
                Midi::Channel const channel,
                Midi::Byte const byte
        ) {
            char buffer[128];

            snprintf(
                buffer,
                128,
                "%s %.1f 0x%02hhx 0x%02hhx\n",
                event_name,
                time_offset,
                channel,
                byte
            );

            events += buffer;
        }

        void log_event(
                char const* const event_name,
                Seconds const time_offset,
                Midi::Channel const channel,
                Midi::Byte const byte_1,
                Midi::Byte const byte_2
        ) {
            char buffer[128];

            snprintf(
                buffer,
                128,
                "%s %.1f 0x%02hhx 0x%02hhx 0x%02hhx\n",
                event_name,
                time_offset,
                channel,
                byte_1,
                byte_2
            );

            events += buffer;
        }

        void log_event(
                char const* const event_name,
                Seconds const time_offset,
                Midi::Channel const channel,
                Midi::Word const word
        ) {
            char buffer[128];

            snprintf(
                buffer,
                128,
                "%s %.1f 0x%02hhx 0x%04hx\n",
                event_name,
                time_offset,
                channel,
                word
            );

            events += buffer;
        }
};


void assert_all_bytes_were_processed(
        size_t const buffer_size,
        size_t const processed_bytes
) {
    assert_eq((int)buffer_size, (int)processed_bytes);
}


std::string parse_midi(
        Seconds const time_offset,
        char const* const buffer,
        size_t buffer_size = 0
) {
    MidiEventLogger logger;
    size_t const size = buffer_size == 0 ? strlen(buffer) : buffer_size;
    size_t const processed_bytes = (
        Midi::EventDispatcher<MidiEventLogger>::dispatch_events(
            logger, time_offset, (Midi::Byte const*)buffer, size
        )
    );

    assert_all_bytes_were_processed(size, processed_bytes);

    return logger.events;
}


TEST(parses_known_midi_messages_and_ignores_unknown_and_invalid_ones, {
    assert_eq("NOTE_OFF 1.0 0x06 0x42 0x70\n", parse_midi(1.0, "\x86\x42\x70"));
    assert_eq("NOTE_ON 2.0 0x06 0x42 0x70\n", parse_midi(2.0, "\x96\x42\x70"));
    assert_eq("AFTERTOUCH 3.0 0x06 0x42 0x70\n", parse_midi(3.0, "\xa6\x42\x70"));
    assert_eq("CONTROL_CHANGE 4.0 0x06 0x01 0x70\n", parse_midi(4.0, "\xb6\x01\x70"));
    assert_eq("PROGRAM_CHANGE 5.0 0x06 0x01\n", parse_midi(5.0, "\xc6\x01"));
    assert_eq("CHANNEL_PRESSURE 6.0 0x06 0x42\n", parse_midi(6.0, "\xd6\x42"));
    assert_eq("PITCH_WHEEL 7.0 0x06 0x0abc\n", parse_midi(7.0, "\xe6\x3c\x15"));
    assert_eq("ALL_SOUND_OFF 8.0 0x06\n", parse_midi(8.0, "\xb6\x78\x00", 3));
    assert_eq("RESET_ALL_CONTROLLERS 9.0 0x06\n", parse_midi(9.0, "\xb6\x79\x00", 3));
    assert_eq("ALL_NOTES_OFF 10.0 0x06\n", parse_midi(10.0, "\xb6\x7b\x00", 3));
    assert_eq("ALL_NOTES_OFF 11.0 0x06\n", parse_midi(11.0, "\xb6\x7c\x00", 3));
    assert_eq("ALL_NOTES_OFF 12.0 0x06\n", parse_midi(12.0, "\xb6\x7d\x00", 3));
    assert_eq(
        (
            "ALL_NOTES_OFF 13.0 0x06\n"
            "MONO_MODE_ON 13.0 0x06\n"
        ),
        parse_midi(13.0, "\xb6\x7e\x00", 3)
    );
    assert_eq(
        (
            "ALL_NOTES_OFF 14.0 0x06\n"
            "MONO_MODE_OFF 14.0 0x06\n"
        ),
        parse_midi(14.0, "\xb6\x7f\x00", 3)
    );
    assert_eq(
        "NOTE_ON 15.0 0x06 0x42 0x70\n",
        parse_midi(15.0, "\x01\xff\x7f\x7f\x86\x99\xff\x96\x42\x70\xff")
    );
})


TEST(running_status, {
    assert_eq(
        (
            "NOTE_ON 1.0 0x07 0x61 0x70\n"
            "NOTE_ON 1.0 0x07 0x61 0x00\n"
            "NOTE_ON 1.0 0x07 0x62 0x71\n"
            "NOTE_ON 1.0 0x07 0x63 0x72\n"
            "NOTE_ON 1.0 0x07 0x64 0x73\n"
            "CONTROL_CHANGE 1.0 0x07 0x01 0x60\n"
            "CONTROL_CHANGE 1.0 0x07 0x01 0x61\n"
            "CONTROL_CHANGE 1.0 0x07 0x01 0x62\n"
        ),
        parse_midi(
            1.0,
            (
                "\x97\x61\x70"
                    "\x61\x00"
                    "\x62\x71"
                    "\x63\x72"
                    "\x64\x73"
                "\xb7\x01\x60"
                    "\x01\x61"
                    "\x01\x62"
            ),
            18
        )
    );
})
