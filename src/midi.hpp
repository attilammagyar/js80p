/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024  Attila M. Magyar
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

#ifndef JS80P__MIDI_HPP
#define JS80P__MIDI_HPP

#include <cstddef>
#include <cstdint>

#include "js80p.hpp"


namespace JS80P { namespace Midi
{

typedef uint8_t Byte;
typedef uint16_t Word;

typedef Byte Note;
typedef Byte Channel;
typedef Byte Controller;
typedef Byte Command;


class EventHandler
{
    public:
        EventHandler() : running_status(0) {}

        void note_off(
            Seconds const time_offset,
            Channel const channel,
            Note const note,
            Byte const velocity
        ) noexcept {}

        void note_on(
            Seconds const time_offset,
            Channel const channel,
            Note const note,
            Byte const velocity
        ) noexcept {}

        void aftertouch(
            Seconds const time_offset,
            Channel const channel,
            Note const note,
            Byte const pressure
        ) noexcept {}

        void control_change(
            Seconds const time_offset,
            Channel const channel,
            Controller const controller,
            Byte const new_value
        ) noexcept {}

        void program_change(
            Seconds const time_offset,
            Channel const channel,
            Byte const new_program
        ) noexcept {}

        void channel_pressure(
            Seconds const time_offset,
            Channel const channel,
            Byte const pressure
        ) noexcept {}

        void pitch_wheel_change(
            Seconds const time_offset,
            Channel const channel,
            Word const new_value
        ) noexcept {}

        void all_sound_off(
            Seconds const time_offset,
            Channel const channel
        ) noexcept {}

        void reset_all_controllers(
            Seconds const time_offset,
            Channel const channel
        ) noexcept {}

        void all_notes_off(
            Seconds const time_offset,
            Channel const channel
        ) noexcept {}

        void mono_mode_on(
            Seconds const time_offset,
            Channel const channel
        ) noexcept {}

        void mono_mode_off(
            Seconds const time_offset,
            Channel const channel
        ) noexcept {}

        Byte running_status;
};


template<class EventHandlerClass>
class EventDispatcher
{
    public:
        /**
         * \brief Parse and dispatch the events found in the buffer.
         *
         * \sa dispatch_event()
         */
        static size_t dispatch_events(
            EventHandlerClass& event_handler,
            Seconds const time_offset,
            Byte const* const buffer,
            size_t const buffer_size
        ) noexcept;

        /**
         * \brief Parse and dispatch the first event that can be read from the
         *        buffer.
         *
         * If the \c running_status member of the \c EventHandlerClass object
         * indicates a previously established valid running status (MSB is 1),
         * then data bytes (MSB is 0) at the beginning of the buffer are parsed
         * as if a new event with the same status byte was received. Otherwise
         * data bytes at the beginning of the buffer are skipped over.
         *
         * \return Number of bytes processed.
         */
        static size_t dispatch_event(
            EventHandlerClass& event_handler,
            Seconds const time_offset,
            Byte const* const buffer,
            size_t const buffer_size
        ) noexcept;

    private:
        static constexpr Byte STATUS_MASK = 0x80;
        static constexpr Byte MESSAGE_TYPE_MASK = 0xf0;
        static constexpr Byte CHANNEL_MASK = 0x0f;

        static bool is_status_byte(Byte const byte) noexcept;
        static bool is_data_byte(Byte const byte) noexcept;

        static size_t process_note_off(
            EventHandlerClass& event_handler,
            Seconds const time_offset,
            Byte const channel,
            Byte const* const buffer,
            size_t const buffer_size,
            size_t& next_byte
        ) noexcept;

        static size_t process_note_on(
            EventHandlerClass& event_handler,
            Seconds const time_offset,
            Byte const channel,
            Byte const* const buffer,
            size_t const buffer_size,
            size_t& next_byte
        ) noexcept;

        static size_t process_aftertouch(
            EventHandlerClass& event_handler,
            Seconds const time_offset,
            Byte const channel,
            Byte const* const buffer,
            size_t const buffer_size,
            size_t& next_byte
        ) noexcept;

        static size_t process_control_change(
            EventHandlerClass& event_handler,
            Seconds const time_offset,
            Byte const channel,
            Byte const* const buffer,
            size_t const buffer_size,
            size_t& next_byte
        ) noexcept;

        static size_t process_program_change(
            EventHandlerClass& event_handler,
            Seconds const time_offset,
            Byte const channel,
            Byte const* const buffer,
            size_t const buffer_size,
            size_t& next_byte
        ) noexcept;

        static size_t process_channel_pressure(
            EventHandlerClass& event_handler,
            Seconds const time_offset,
            Byte const channel,
            Byte const* const buffer,
            size_t const buffer_size,
            size_t& next_byte
        ) noexcept;

        static size_t process_pitch_bend_change(
            EventHandlerClass& event_handler,
            Seconds const time_offset,
            Byte const channel,
            Byte const* const buffer,
            size_t const buffer_size,
            size_t& next_byte
        ) noexcept;

        static bool parse_data_byte(
            Byte const* const buffer,
            size_t const buffer_size,
            size_t& next_byte,
            Byte& byte
        ) noexcept;

        static bool parse_data_bytes(
            Byte const* const buffer,
            size_t const buffer_size,
            size_t& next_byte,
            Byte& byte_1,
            Byte& byte_2
        ) noexcept;

        static size_t skip_remaining_data_bytes(
            Byte const* const buffer,
            size_t const buffer_size,
            size_t& next_byte
        ) noexcept;
};


constexpr Channel CHANNEL_MAX                       = 15;
constexpr Channel CHANNELS                          = CHANNEL_MAX + 1;
constexpr Channel INVALID_CHANNEL                   = 255;

constexpr Note NOTE_MAX                             = 127;
constexpr Note NOTES                                = NOTE_MAX + 1;
constexpr Note INVALID_NOTE                         = 255;

constexpr Note NOTE_G_9                             = 127;
constexpr Note NOTE_F_SHARP_9                       = 126;
constexpr Note NOTE_G_FLAT_9                        = 126;
constexpr Note NOTE_F_9                             = 125;
constexpr Note NOTE_E_9                             = 124;
constexpr Note NOTE_D_SHARP_9                       = 123;
constexpr Note NOTE_E_FLAT_9                        = 123;
constexpr Note NOTE_D_9                             = 122;
constexpr Note NOTE_C_SHARP_9                       = 121;
constexpr Note NOTE_D_FLAT_9                        = 121;
constexpr Note NOTE_C_9                             = 120;
constexpr Note NOTE_B_8                             = 119;
constexpr Note NOTE_A_SHARP_8                       = 118;
constexpr Note NOTE_B_FLAT_8                        = 118;
constexpr Note NOTE_A_8                             = 117;
constexpr Note NOTE_G_SHARP_8                       = 116;
constexpr Note NOTE_A_FLAT_8                        = 116;
constexpr Note NOTE_G_8                             = 115;
constexpr Note NOTE_F_SHARP_8                       = 114;
constexpr Note NOTE_G_FLAT_8                        = 114;
constexpr Note NOTE_F_8                             = 113;
constexpr Note NOTE_E_8                             = 112;
constexpr Note NOTE_D_SHARP_8                       = 111;
constexpr Note NOTE_E_FLAT_8                        = 111;
constexpr Note NOTE_D_8                             = 110;
constexpr Note NOTE_C_SHARP_8                       = 109;
constexpr Note NOTE_D_FLAT_8                        = 109;
constexpr Note NOTE_C_8                             = 108;
constexpr Note NOTE_B_7                             = 107;
constexpr Note NOTE_A_SHARP_7                       = 106;
constexpr Note NOTE_B_FLAT_7                        = 106;
constexpr Note NOTE_A_7                             = 105;
constexpr Note NOTE_G_SHARP_7                       = 104;
constexpr Note NOTE_A_FLAT_7                        = 104;
constexpr Note NOTE_G_7                             = 103;
constexpr Note NOTE_F_SHARP_7                       = 102;
constexpr Note NOTE_G_FLAT_7                        = 102;
constexpr Note NOTE_F_7                             = 101;
constexpr Note NOTE_E_7                             = 100;
constexpr Note NOTE_D_SHARP_7                       = 99;
constexpr Note NOTE_E_FLAT_7                        = 99;
constexpr Note NOTE_D_7                             = 98;
constexpr Note NOTE_C_SHARP_7                       = 97;
constexpr Note NOTE_D_FLAT_7                        = 97;
constexpr Note NOTE_C_7                             = 96;
constexpr Note NOTE_B_6                             = 95;
constexpr Note NOTE_A_SHARP_6                       = 94;
constexpr Note NOTE_B_FLAT_6                        = 94;
constexpr Note NOTE_A_6                             = 93;
constexpr Note NOTE_G_SHARP_6                       = 92;
constexpr Note NOTE_A_FLAT_6                        = 92;
constexpr Note NOTE_G_6                             = 91;
constexpr Note NOTE_F_SHARP_6                       = 90;
constexpr Note NOTE_G_FLAT_6                        = 90;
constexpr Note NOTE_F_6                             = 89;
constexpr Note NOTE_E_6                             = 88;
constexpr Note NOTE_D_SHARP_6                       = 87;
constexpr Note NOTE_E_FLAT_6                        = 87;
constexpr Note NOTE_D_6                             = 86;
constexpr Note NOTE_C_SHARP_6                       = 85;
constexpr Note NOTE_D_FLAT_6                        = 85;
constexpr Note NOTE_C_6                             = 84;
constexpr Note NOTE_B_5                             = 83;
constexpr Note NOTE_A_SHARP_5                       = 82;
constexpr Note NOTE_B_FLAT_5                        = 82;
constexpr Note NOTE_A_5                             = 81;
constexpr Note NOTE_G_SHARP_5                       = 80;
constexpr Note NOTE_A_FLAT_5                        = 80;
constexpr Note NOTE_G_5                             = 79;
constexpr Note NOTE_F_SHARP_5                       = 78;
constexpr Note NOTE_G_FLAT_5                        = 78;
constexpr Note NOTE_F_5                             = 77;
constexpr Note NOTE_E_5                             = 76;
constexpr Note NOTE_D_SHARP_5                       = 75;
constexpr Note NOTE_E_FLAT_5                        = 75;
constexpr Note NOTE_D_5                             = 74;
constexpr Note NOTE_C_SHARP_5                       = 73;
constexpr Note NOTE_D_FLAT_5                        = 73;
constexpr Note NOTE_C_5                             = 72;
constexpr Note NOTE_B_4                             = 71;
constexpr Note NOTE_A_SHARP_4                       = 70;
constexpr Note NOTE_B_FLAT_4                        = 70;
constexpr Note NOTE_A_4                             = 69;
constexpr Note NOTE_G_SHARP_4                       = 68;
constexpr Note NOTE_A_FLAT_4                        = 68;
constexpr Note NOTE_G_4                             = 67;
constexpr Note NOTE_F_SHARP_4                       = 66;
constexpr Note NOTE_G_FLAT_4                        = 66;
constexpr Note NOTE_F_4                             = 65;
constexpr Note NOTE_E_4                             = 64;
constexpr Note NOTE_D_SHARP_4                       = 63;
constexpr Note NOTE_E_FLAT_4                        = 63;
constexpr Note NOTE_D_4                             = 62;
constexpr Note NOTE_C_SHARP_4                       = 61;
constexpr Note NOTE_D_FLAT_4                        = 61;
constexpr Note NOTE_C_4                             = 60;
constexpr Note NOTE_B_3                             = 59;
constexpr Note NOTE_A_SHARP_3                       = 58;
constexpr Note NOTE_B_FLAT_3                        = 58;
constexpr Note NOTE_A_3                             = 57;
constexpr Note NOTE_G_SHARP_3                       = 56;
constexpr Note NOTE_A_FLAT_3                        = 56;
constexpr Note NOTE_G_3                             = 55;
constexpr Note NOTE_F_SHARP_3                       = 54;
constexpr Note NOTE_G_FLAT_3                        = 54;
constexpr Note NOTE_F_3                             = 53;
constexpr Note NOTE_E_3                             = 52;
constexpr Note NOTE_D_SHARP_3                       = 51;
constexpr Note NOTE_E_FLAT_3                        = 51;
constexpr Note NOTE_D_3                             = 50;
constexpr Note NOTE_C_SHARP_3                       = 49;
constexpr Note NOTE_D_FLAT_3                        = 49;
constexpr Note NOTE_C_3                             = 48;
constexpr Note NOTE_B_2                             = 47;
constexpr Note NOTE_A_SHARP_2                       = 46;
constexpr Note NOTE_B_FLAT_2                        = 46;
constexpr Note NOTE_A_2                             = 45;
constexpr Note NOTE_G_SHARP_2                       = 44;
constexpr Note NOTE_A_FLAT_2                        = 44;
constexpr Note NOTE_G_2                             = 43;
constexpr Note NOTE_F_SHARP_2                       = 42;
constexpr Note NOTE_G_FLAT_2                        = 42;
constexpr Note NOTE_F_2                             = 41;
constexpr Note NOTE_E_2                             = 40;
constexpr Note NOTE_D_SHARP_2                       = 39;
constexpr Note NOTE_E_FLAT_2                        = 39;
constexpr Note NOTE_D_2                             = 38;
constexpr Note NOTE_C_SHARP_2                       = 37;
constexpr Note NOTE_D_FLAT_2                        = 37;
constexpr Note NOTE_C_2                             = 36;
constexpr Note NOTE_B_1                             = 35;
constexpr Note NOTE_A_SHARP_1                       = 34;
constexpr Note NOTE_B_FLAT_1                        = 34;
constexpr Note NOTE_A_1                             = 33;
constexpr Note NOTE_G_SHARP_1                       = 32;
constexpr Note NOTE_A_FLAT_1                        = 32;
constexpr Note NOTE_G_1                             = 31;
constexpr Note NOTE_F_SHARP_1                       = 30;
constexpr Note NOTE_G_FLAT_1                        = 30;
constexpr Note NOTE_F_1                             = 29;
constexpr Note NOTE_E_1                             = 28;
constexpr Note NOTE_D_SHARP_1                       = 27;
constexpr Note NOTE_E_FLAT_1                        = 27;
constexpr Note NOTE_D_1                             = 26;
constexpr Note NOTE_C_SHARP_1                       = 25;
constexpr Note NOTE_D_FLAT_1                        = 25;
constexpr Note NOTE_C_1                             = 24;
constexpr Note NOTE_B_0                             = 23;
constexpr Note NOTE_A_SHARP_0                       = 22;
constexpr Note NOTE_B_FLAT_0                        = 22;
constexpr Note NOTE_A_0                             = 21;

constexpr Note NOTE_127                             = 127;
constexpr Note NOTE_126                             = 126;
constexpr Note NOTE_125                             = 125;
constexpr Note NOTE_124                             = 124;
constexpr Note NOTE_123                             = 123;
constexpr Note NOTE_122                             = 122;
constexpr Note NOTE_121                             = 121;
constexpr Note NOTE_120                             = 120;
constexpr Note NOTE_119                             = 119;
constexpr Note NOTE_118                             = 118;
constexpr Note NOTE_117                             = 117;
constexpr Note NOTE_116                             = 116;
constexpr Note NOTE_115                             = 115;
constexpr Note NOTE_114                             = 114;
constexpr Note NOTE_113                             = 113;
constexpr Note NOTE_112                             = 112;
constexpr Note NOTE_111                             = 111;
constexpr Note NOTE_110                             = 110;
constexpr Note NOTE_109                             = 109;
constexpr Note NOTE_108                             = 108;
constexpr Note NOTE_107                             = 107;
constexpr Note NOTE_106                             = 106;
constexpr Note NOTE_105                             = 105;
constexpr Note NOTE_104                             = 104;
constexpr Note NOTE_103                             = 103;
constexpr Note NOTE_102                             = 102;
constexpr Note NOTE_101                             = 101;
constexpr Note NOTE_100                             = 100;
constexpr Note NOTE_99                              = 99;
constexpr Note NOTE_98                              = 98;
constexpr Note NOTE_97                              = 97;
constexpr Note NOTE_96                              = 96;
constexpr Note NOTE_95                              = 95;
constexpr Note NOTE_94                              = 94;
constexpr Note NOTE_93                              = 93;
constexpr Note NOTE_92                              = 92;
constexpr Note NOTE_91                              = 91;
constexpr Note NOTE_90                              = 90;
constexpr Note NOTE_89                              = 89;
constexpr Note NOTE_88                              = 88;
constexpr Note NOTE_87                              = 87;
constexpr Note NOTE_86                              = 86;
constexpr Note NOTE_85                              = 85;
constexpr Note NOTE_84                              = 84;
constexpr Note NOTE_83                              = 83;
constexpr Note NOTE_82                              = 82;
constexpr Note NOTE_81                              = 81;
constexpr Note NOTE_80                              = 80;
constexpr Note NOTE_79                              = 79;
constexpr Note NOTE_78                              = 78;
constexpr Note NOTE_77                              = 77;
constexpr Note NOTE_76                              = 76;
constexpr Note NOTE_75                              = 75;
constexpr Note NOTE_74                              = 74;
constexpr Note NOTE_73                              = 73;
constexpr Note NOTE_72                              = 72;
constexpr Note NOTE_71                              = 71;
constexpr Note NOTE_70                              = 70;
constexpr Note NOTE_69                              = 69;
constexpr Note NOTE_68                              = 68;
constexpr Note NOTE_67                              = 67;
constexpr Note NOTE_66                              = 66;
constexpr Note NOTE_65                              = 65;
constexpr Note NOTE_64                              = 64;
constexpr Note NOTE_63                              = 63;
constexpr Note NOTE_62                              = 62;
constexpr Note NOTE_61                              = 61;
constexpr Note NOTE_60                              = 60;
constexpr Note NOTE_59                              = 59;
constexpr Note NOTE_58                              = 58;
constexpr Note NOTE_57                              = 57;
constexpr Note NOTE_56                              = 56;
constexpr Note NOTE_55                              = 55;
constexpr Note NOTE_54                              = 54;
constexpr Note NOTE_53                              = 53;
constexpr Note NOTE_52                              = 52;
constexpr Note NOTE_51                              = 51;
constexpr Note NOTE_50                              = 50;
constexpr Note NOTE_49                              = 49;
constexpr Note NOTE_48                              = 48;
constexpr Note NOTE_47                              = 47;
constexpr Note NOTE_46                              = 46;
constexpr Note NOTE_45                              = 45;
constexpr Note NOTE_44                              = 44;
constexpr Note NOTE_43                              = 43;
constexpr Note NOTE_42                              = 42;
constexpr Note NOTE_41                              = 41;
constexpr Note NOTE_40                              = 40;
constexpr Note NOTE_39                              = 39;
constexpr Note NOTE_38                              = 38;
constexpr Note NOTE_37                              = 37;
constexpr Note NOTE_36                              = 36;
constexpr Note NOTE_35                              = 35;
constexpr Note NOTE_34                              = 34;
constexpr Note NOTE_33                              = 33;
constexpr Note NOTE_32                              = 32;
constexpr Note NOTE_31                              = 31;
constexpr Note NOTE_30                              = 30;
constexpr Note NOTE_29                              = 29;
constexpr Note NOTE_28                              = 28;
constexpr Note NOTE_27                              = 27;
constexpr Note NOTE_26                              = 26;
constexpr Note NOTE_25                              = 25;
constexpr Note NOTE_24                              = 24;
constexpr Note NOTE_23                              = 23;
constexpr Note NOTE_22                              = 22;
constexpr Note NOTE_21                              = 21;
constexpr Note NOTE_20                              = 20;
constexpr Note NOTE_19                              = 19;
constexpr Note NOTE_18                              = 18;
constexpr Note NOTE_17                              = 17;
constexpr Note NOTE_16                              = 16;
constexpr Note NOTE_15                              = 15;
constexpr Note NOTE_14                              = 14;
constexpr Note NOTE_13                              = 13;
constexpr Note NOTE_12                              = 12;
constexpr Note NOTE_11                              = 11;
constexpr Note NOTE_10                              = 10;
constexpr Note NOTE_9                               = 9;
constexpr Note NOTE_8                               = 8;
constexpr Note NOTE_7                               = 7;
constexpr Note NOTE_6                               = 6;
constexpr Note NOTE_5                               = 5;
constexpr Note NOTE_4                               = 4;
constexpr Note NOTE_3                               = 3;
constexpr Note NOTE_2                               = 2;
constexpr Note NOTE_1                               = 1;
constexpr Note NOTE_0                               = 0;

constexpr Controller NONE                           = 0;
constexpr Controller MODULATION_WHEEL               = 1;
constexpr Controller BREATH                         = 2;
constexpr Controller UNDEFINED_1                    = 3;
constexpr Controller FOOT_PEDAL                     = 4;
constexpr Controller PORTAMENTO_TIME                = 5;
constexpr Controller DATA_ENTRY                     = 6;
constexpr Controller VOLUME                         = 7;
constexpr Controller BALANCE                        = 8;
constexpr Controller UNDEFINED_2                    = 9;
constexpr Controller PAN                            = 10;
constexpr Controller EXPRESSION_PEDAL               = 11;
constexpr Controller FX_CTL_1                       = 12;
constexpr Controller FX_CTL_2                       = 13;
constexpr Controller UNDEFINED_3                    = 14;
constexpr Controller UNDEFINED_4                    = 15;
constexpr Controller GENERAL_1                      = 16;
constexpr Controller GENERAL_2                      = 17;
constexpr Controller GENERAL_3                      = 18;
constexpr Controller GENERAL_4                      = 19;
constexpr Controller UNDEFINED_5                    = 20;
constexpr Controller UNDEFINED_6                    = 21;
constexpr Controller UNDEFINED_7                    = 22;
constexpr Controller UNDEFINED_8                    = 23;
constexpr Controller UNDEFINED_9                    = 24;
constexpr Controller UNDEFINED_10                   = 25;
constexpr Controller UNDEFINED_11                   = 26;
constexpr Controller UNDEFINED_12                   = 27;
constexpr Controller UNDEFINED_13                   = 28;
constexpr Controller UNDEFINED_14                   = 29;
constexpr Controller UNDEFINED_15                   = 30;
constexpr Controller UNDEFINED_16                   = 31;
constexpr Controller SUSTAIN_PEDAL                  = 64;
constexpr Controller SOUND_1                        = 70;
constexpr Controller SOUND_2                        = 71;
constexpr Controller SOUND_3                        = 72;
constexpr Controller SOUND_4                        = 73;
constexpr Controller SOUND_5                        = 74;
constexpr Controller SOUND_6                        = 75;
constexpr Controller SOUND_7                        = 76;
constexpr Controller SOUND_8                        = 77;
constexpr Controller SOUND_9                        = 78;
constexpr Controller SOUND_10                       = 79;
constexpr Controller PORTAMENTO                     = 84;
constexpr Controller UNDEFINED_17                   = 85;
constexpr Controller UNDEFINED_18                   = 86;
constexpr Controller UNDEFINED_19                   = 87;
constexpr Controller UNDEFINED_20                   = 89;
constexpr Controller UNDEFINED_21                   = 90;
constexpr Controller FX_1                           = 91;
constexpr Controller FX_2                           = 92;
constexpr Controller FX_3                           = 93;
constexpr Controller FX_4                           = 94;
constexpr Controller FX_5                           = 95;
constexpr Controller UNDEFINED_22                   = 102;
constexpr Controller UNDEFINED_23                   = 103;
constexpr Controller UNDEFINED_24                   = 104;
constexpr Controller UNDEFINED_25                   = 105;
constexpr Controller UNDEFINED_26                   = 106;
constexpr Controller UNDEFINED_27                   = 107;
constexpr Controller UNDEFINED_28                   = 108;
constexpr Controller UNDEFINED_29                   = 109;
constexpr Controller UNDEFINED_30                   = 110;
constexpr Controller UNDEFINED_31                   = 111;
constexpr Controller UNDEFINED_32                   = 112;
constexpr Controller UNDEFINED_33                   = 113;
constexpr Controller UNDEFINED_34                   = 114;
constexpr Controller UNDEFINED_35                   = 115;
constexpr Controller UNDEFINED_36                   = 116;
constexpr Controller UNDEFINED_37                   = 117;
constexpr Controller UNDEFINED_38                   = 118;
constexpr Controller UNDEFINED_39                   = 119;

constexpr Controller MAX_CONTROLLER_ID              = 127;

constexpr Command NOTE_OFF                          = 0x80;
constexpr Command NOTE_ON                           = 0x90;
constexpr Command AFTERTOUCH                        = 0xa0;
constexpr Command CONTROL_CHANGE                    = 0xb0;
constexpr Command PROGRAM_CHANGE                    = 0xc0;
constexpr Command CHANNEL_PRESSURE                  = 0xd0;
constexpr Command PITCH_BEND_CHANGE                 = 0xe0;

constexpr Command CONTROL_CHANGE_ALL_SOUND_OFF          = 0x78;
constexpr Command CONTROL_CHANGE_RESET_ALL_CONTROLLERS  = 0x79;
constexpr Command CONTROL_CHANGE_ALL_NOTES_OFF          = 0x7b;
constexpr Command CONTROL_CHANGE_OMNI_MODE_OFF          = 0x7c;
constexpr Command CONTROL_CHANGE_OMNI_MODE_ON           = 0x7d;
constexpr Command CONTROL_CHANGE_MONO_MODE_ON           = 0x7e;
constexpr Command CONTROL_CHANGE_MONO_MODE_OFF          = 0x7f;


template<class EventHandlerClass>
size_t EventDispatcher<EventHandlerClass>::dispatch_events(
        EventHandlerClass& event_handler,
        Seconds const time_offset,
        Byte const* const buffer,
        size_t const buffer_size
) noexcept {
    size_t next_byte = 0;

    while (next_byte != buffer_size) {
        next_byte += dispatch_event(
            event_handler, time_offset, &buffer[next_byte], buffer_size - next_byte
        );
    }

    return next_byte;
}


template<class EventHandlerClass>
size_t EventDispatcher<EventHandlerClass>::dispatch_event(
        EventHandlerClass& event_handler,
        Seconds const time_offset,
        Byte const* const buffer,
        size_t const buffer_size
) noexcept {
    size_t next_byte = 0;

    if (buffer_size < 1) {
        return next_byte;
    }

    Byte status = buffer[next_byte];

    if (is_status_byte(status)) {
        ++next_byte;
        event_handler.running_status = status;
    } else {
        status = event_handler.running_status;

        if (!is_status_byte(status)) {
            return skip_remaining_data_bytes(buffer, buffer_size, next_byte);
        }
    }

    Command msg_type = status & MESSAGE_TYPE_MASK;
    Channel channel = status & CHANNEL_MAX;

    switch (msg_type) {
        case NOTE_OFF:
            return process_note_off(
                event_handler, time_offset, channel, buffer, buffer_size, next_byte
            );

        case NOTE_ON:
            return process_note_on(
                event_handler, time_offset, channel, buffer, buffer_size, next_byte
            );

        case AFTERTOUCH:
            return process_aftertouch(
                event_handler, time_offset, channel, buffer, buffer_size, next_byte
            );

        case CONTROL_CHANGE:
            return process_control_change(
                event_handler, time_offset, channel, buffer, buffer_size, next_byte
            );

        case PROGRAM_CHANGE:
            return process_program_change(
                event_handler, time_offset, channel, buffer, buffer_size, next_byte
            );

        case CHANNEL_PRESSURE:
            return process_channel_pressure(
                event_handler, time_offset, channel, buffer, buffer_size, next_byte
            );

        case PITCH_BEND_CHANGE:
            return process_pitch_bend_change(
                event_handler, time_offset, channel, buffer, buffer_size, next_byte
            );

        default:
            return skip_remaining_data_bytes(buffer, buffer_size, next_byte);
    }

    return next_byte;
}


template<class EventHandlerClass>
bool EventDispatcher<EventHandlerClass>::is_status_byte(Byte const byte) noexcept
{
    return (byte & STATUS_MASK) != 0;
}


template<class EventHandlerClass>
bool EventDispatcher<EventHandlerClass>::is_data_byte(Byte const byte) noexcept
{
    return (byte & STATUS_MASK) == 0;
}


template<class EventHandlerClass>
size_t EventDispatcher<EventHandlerClass>::process_note_off(
        EventHandlerClass& event_handler,
        Seconds const time_offset,
        Byte const channel,
        Byte const* const buffer,
        size_t const buffer_size,
        size_t& next_byte
) noexcept {
    Byte note;
    Byte velocity;

    if (!parse_data_bytes(buffer, buffer_size, next_byte, note, velocity)) {
        return next_byte;
    }

    event_handler.note_off(time_offset, channel, (Note)note, velocity);

    return next_byte;
}


template<class EventHandlerClass>
size_t EventDispatcher<EventHandlerClass>::process_note_on(
        EventHandlerClass& event_handler,
        Seconds const time_offset,
        Byte const channel,
        Byte const* const buffer,
        size_t const buffer_size,
        size_t& next_byte
) noexcept {
    Byte note;
    Byte velocity;

    if (!parse_data_bytes(buffer, buffer_size, next_byte, note, velocity)) {
        return next_byte;
    }

    event_handler.note_on(time_offset, channel, (Note)note, velocity);

    return next_byte;
}


template<class EventHandlerClass>
size_t EventDispatcher<EventHandlerClass>::process_aftertouch(
        EventHandlerClass& event_handler,
        Seconds const time_offset,
        Byte const channel,
        Byte const* const buffer,
        size_t const buffer_size,
        size_t& next_byte
) noexcept {
    Byte note;
    Byte pressure;

    if (!parse_data_bytes(buffer, buffer_size, next_byte, note, pressure)) {
        return next_byte;
    }

    event_handler.aftertouch(time_offset, channel, (Note)note, pressure);

    return next_byte;
}


template<class EventHandlerClass>
size_t EventDispatcher<EventHandlerClass>::process_control_change(
        EventHandlerClass& event_handler,
        Seconds const time_offset,
        Byte const channel,
        Byte const* const buffer,
        size_t const buffer_size,
        size_t& next_byte
) noexcept {
    Byte d1;
    Byte d2;

    if (!parse_data_bytes(buffer, buffer_size, next_byte, d1, d2)) {
        return next_byte;
    }

    if (d1 < CONTROL_CHANGE_ALL_SOUND_OFF) {
        /*
        Interpretation of MIDI CC messages that belong to special controllers
        (e.g. sustain pedal) is left for the event handler. This aligns with the
        restrictions that are imposed by hosts which swallow most of the raw CC
        messages and instead, require plugins to export parameters that can be
        assigned to MIDI controllers (for example, FL Studio 21).
        */

        event_handler.control_change(time_offset, channel, (Controller)d1, d2);
    } else {
        switch ((Command)d1) {
            case CONTROL_CHANGE_ALL_SOUND_OFF:
                event_handler.all_sound_off(time_offset, channel);
                break;

            case CONTROL_CHANGE_RESET_ALL_CONTROLLERS:
                event_handler.reset_all_controllers(time_offset, channel);
                break;

            case CONTROL_CHANGE_ALL_NOTES_OFF:
            case CONTROL_CHANGE_OMNI_MODE_OFF:
            case CONTROL_CHANGE_OMNI_MODE_ON:
                event_handler.all_notes_off(time_offset, channel);
                break;

            case CONTROL_CHANGE_MONO_MODE_ON:
                event_handler.all_notes_off(time_offset, channel);
                event_handler.mono_mode_on(time_offset, channel);
                break;

            case CONTROL_CHANGE_MONO_MODE_OFF:
                event_handler.all_notes_off(time_offset, channel);
                event_handler.mono_mode_off(time_offset, channel);
                break;

            default:
                break;
        }
    }

    return next_byte;
}


template<class EventHandlerClass>
size_t EventDispatcher<EventHandlerClass>::process_program_change(
        EventHandlerClass& event_handler,
        Seconds const time_offset,
        Byte const channel,
        Byte const* const buffer,
        size_t const buffer_size,
        size_t& next_byte
) noexcept {
    Byte new_program;

    if (!parse_data_byte(buffer, buffer_size, next_byte, new_program)) {
        return next_byte;
    }

    event_handler.program_change(time_offset, channel, new_program);

    return next_byte;
}


template<class EventHandlerClass>
size_t EventDispatcher<EventHandlerClass>::process_channel_pressure(
        EventHandlerClass& event_handler,
        Seconds const time_offset,
        Byte const channel,
        Byte const* const buffer,
        size_t const buffer_size,
        size_t& next_byte
) noexcept {
    Byte pressure;

    if (!parse_data_byte(buffer, buffer_size, next_byte, pressure)) {
        return next_byte;
    }

    event_handler.channel_pressure(time_offset, channel, pressure);

    return next_byte;
}


template<class EventHandlerClass>
size_t EventDispatcher<EventHandlerClass>::process_pitch_bend_change(
        EventHandlerClass& event_handler,
        Seconds const time_offset,
        Byte const channel,
        Byte const* const buffer,
        size_t const buffer_size,
        size_t& next_byte
) noexcept {
    Byte lsb;
    Byte msb;

    if (!parse_data_bytes(buffer, buffer_size, next_byte, lsb, msb)) {
        return next_byte;
    }

    event_handler.pitch_wheel_change(time_offset, channel, (msb << 7) | lsb);

    return next_byte;
}


template<class EventHandlerClass>
bool EventDispatcher<EventHandlerClass>::parse_data_bytes(
        Byte const* const buffer,
        size_t const buffer_size,
        size_t& next_byte,
        Byte& byte_1,
        Byte& byte_2
) noexcept {
    return (
        parse_data_byte(buffer, buffer_size, next_byte, byte_1)
        && parse_data_byte(buffer, buffer_size, next_byte, byte_2)
    );
}


template<class EventHandlerClass>
bool EventDispatcher<EventHandlerClass>::parse_data_byte(
        Byte const* const buffer,
        size_t const buffer_size,
        size_t& next_byte,
        Byte& byte
) noexcept {
    if (next_byte >= buffer_size || !is_data_byte(buffer[next_byte])) {
        return false;
    }

    byte = buffer[next_byte];
    ++next_byte;

    return true;
}


template<class EventHandlerClass>
size_t EventDispatcher<EventHandlerClass>::skip_remaining_data_bytes(
        Byte const* const buffer,
        size_t const buffer_size,
        size_t& next_byte
) noexcept {
    while (next_byte < buffer_size && is_data_byte(buffer[next_byte])) {
        ++next_byte;
    }

    return next_byte;
}

} }

#endif
