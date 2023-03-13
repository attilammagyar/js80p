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

#ifndef JS80P__SYNTH_HPP
#define JS80P__SYNTH_HPP

#include <atomic>
#include <string>
#include <vector>

#include "js80p.hpp"
#include "midi.hpp"

#include "synth/envelope.hpp"
#include "synth/biquad_filter.hpp"
#include "synth/distortion.hpp"
#include "synth/flexible_controller.hpp"
#include "synth/filter.hpp"
#include "synth/math.hpp"
#include "synth/midi_controller.hpp"
#include "synth/oscillator.hpp"
#include "synth/param.hpp"
#include "synth/queue.hpp"
#include "synth/signal_producer.hpp"
#include "synth/voice.hpp"


namespace JS80P
{

class Synth : public Midi::EventHandler, public SignalProducer
{
    friend class SignalProducer;

    public:
        typedef Voice<SignalProducer> Modulator;
        typedef Voice<Modulator::ModulationOut> Carrier;

        static constexpr Integer OUT_CHANNELS = Carrier::CHANNELS;

        static constexpr Integer ENVELOPES = 6;
        static constexpr Integer MIDI_CONTROLLERS = 120;
        static constexpr Integer FLEXIBLE_CONTROLLERS = 10;
        static constexpr Integer LFOS = 8;

        static char const* const PARAM_NAMES[];

        enum MessageType {
            SET_PARAM = 1,
            ASSIGN_CONTROLLER = 2,
            REFRESH_PARAM = 3,
            INVALID,
        };

        enum ParamId {
            VOL = 0,         // Volume

            ADD = 1,         // Modulator Additive Volume
            FM = 2,          // Frequency Modulation
            AM = 3,          // Amplitude Modulation

            MAMP = 4,        // Modulator Amplitude
            MVS = 5,         // Modulator Velocity Sensitivity
            MFLD = 6,        // Modulator Folding
            MPRT = 7,        // Modulator Portamento Length
            MPRD = 8,        // Modulator Portamento Depth
            MDTN = 9,        // Modulator Detune
            MFIN = 10,       // Modulator Fine Detune
            MWID = 11,       // Modulator Width
            MPAN = 12,       // Modulator Pan
            MVOL = 13,       // Modulator Volume

            MC1 = 14,        // Modulator Custom Waveform 1st Harmonic
            MC2 = 15,        // Modulator Custom Waveform 2nd Harmonic
            MC3 = 16,        // Modulator Custom Waveform 3rd Harmonic
            MC4 = 17,        // Modulator Custom Waveform 4th Harmonic
            MC5 = 18,        // Modulator Custom Waveform 5th Harmonic
            MC6 = 19,        // Modulator Custom Waveform 6th Harmonic
            MC7 = 20,        // Modulator Custom Waveform 7th Harmonic
            MC8 = 21,        // Modulator Custom Waveform 8th Harmonic
            MC9 = 22,        // Modulator Custom Waveform 9th Harmonic
            MC10 = 23,       // Modulator Custom Waveform 10th Harmonic

            MF1FRQ = 24,     // Modulator Filter 1 Frequency
            MF1Q = 25,       // Modulator Filter 1 Q Factor
            MF1G = 26,       // Modulator Filter 1 Gain

            MF2FRQ = 27,     // Modulator Filter 2 Frequency
            MF2Q = 28,       // Modulator Filter 2 Q Factor
            MF2G = 29,       // Modulator Filter 2 Gain

            CAMP = 30,       // Carrier Amplitude
            CVS = 31,        // Carrier Velocity Sensitivity
            CFLD = 32,       // Carrier Folding
            CPRT = 33,       // Carrier Portamento Length
            CPRD = 34,       // Carrier Portamento Depth
            CDTN = 35,       // Carrier Detune
            CFIN = 36,       // Carrier Fine Detune
            CWID = 37,       // Carrier Width
            CPAN = 38,       // Carrier Pan
            CVOL = 39,       // Carrier Volume

            CC1 = 40,        // Carrier Custom Waveform 1st Harmonic
            CC2 = 41,        // Carrier Custom Waveform 2nd Harmonic
            CC3 = 42,        // Carrier Custom Waveform 3rd Harmonic
            CC4 = 43,        // Carrier Custom Waveform 4th Harmonic
            CC5 = 44,        // Carrier Custom Waveform 5th Harmonic
            CC6 = 45,        // Carrier Custom Waveform 6th Harmonic
            CC7 = 46,        // Carrier Custom Waveform 7th Harmonic
            CC8 = 47,        // Carrier Custom Waveform 8th Harmonic
            CC9 = 48,        // Carrier Custom Waveform 9th Harmonic
            CC10 = 49,       // Carrier Custom Waveform 10th Harmonic

            CF1FRQ = 50,     // Carrier Filter 1 Frequency
            CF1Q = 51,       // Carrier Filter 1 Q Factor
            CF1G = 52,       // Carrier Filter 1 Gain

            CF2FRQ = 53,     // Carrier Filter 2 Frequency
            CF2Q = 54,       // Carrier Filter 2 Q Factor
            CF2G = 55,       // Carrier Filter 2 Gain

            EOG = 56,        // Effects Overdrive Gain

            EDG = 57,        // Effects Distortaion Gain

            EF1FRQ = 58,     // Effects Filter 1 Frequency
            EF1Q = 59,       // Effects Filter 1 Q Factor
            EF1G = 60,       // Effects Filter 1 Gain

            EF2FRQ = 61,     // Effects Filter 2 Frequency
            EF2Q = 62,       // Effects Filter 2 Q Factor
            EF2G = 63,       // Effects Filter 2 Gain

            EEDEL = 64,      // Effects Echo Delay
            EEFB = 65,       // Effects Echo Feedback
            EEDF = 66,       // Effects Echo Dampening Frequency
            EEDG = 67,       // Effects Echo Dampening Gain
            EEWID = 68,      // Effects Echo Stereo Width
            EEHPF = 69,      // Effects Echo Highpass Frequency
            EEWET = 70,      // Effects Echo Wet Volume
            EEDRY = 71,      // Effects Echo Dry Volume

            ERRS = 72,       // Effects Reverb Room Size
            ERDF = 73,       // Effects Reverb Dampening Frequency
            ERDG = 74,       // Effects Reverb Dampening Gain
            ERWID = 75,      // Effects Reverb Stereo Width
            ERHPF = 76,      // Effects Reverb Highpass Frequency
            ERWET = 77,      // Effects Reverb Wet Volume
            ERDRY = 78,      // Effects Reverb Dry Volume

            F1IN = 79,       // Flexible Controller 1 Input
            F1MIN = 80,      // Flexible Controller 1 Minimum Value
            F1MAX = 81,      // Flexible Controller 1 Maximum Value
            F1AMT = 82,      // Flexible Controller 1 Amount
            F1DST = 83,      // Flexible Controller 1 Distortion
            F1RND = 84,      // Flexible Controller 1 Randomness

            F2IN = 85,       // Flexible Controller 2 Input
            F2MIN = 86,      // Flexible Controller 2 Minimum Value
            F2MAX = 87,      // Flexible Controller 2 Maximum Value
            F2AMT = 88,      // Flexible Controller 2 Amount
            F2DST = 89,      // Flexible Controller 2 Distortion
            F2RND = 90,      // Flexible Controller 2 Randomness

            F3IN = 91,       // Flexible Controller 3 Input
            F3MIN = 92,      // Flexible Controller 3 Minimum Value
            F3MAX = 93,      // Flexible Controller 3 Maximum Value
            F3AMT = 94,      // Flexible Controller 3 Amount
            F3DST = 95,      // Flexible Controller 3 Distortion
            F3RND = 96,      // Flexible Controller 3 Randomness

            F4IN = 97,       // Flexible Controller 4 Input
            F4MIN = 98,      // Flexible Controller 4 Minimum Value
            F4MAX = 99,      // Flexible Controller 4 Maximum Value
            F4AMT = 100,     // Flexible Controller 4 Amount
            F4DST = 101,     // Flexible Controller 4 Distortion
            F4RND = 102,     // Flexible Controller 4 Randomness

            F5IN = 103,      // Flexible Controller 5 Input
            F5MIN = 104,     // Flexible Controller 5 Minimum Value
            F5MAX = 105,     // Flexible Controller 5 Maximum Value
            F5AMT = 106,     // Flexible Controller 5 Amount
            F5DST = 107,     // Flexible Controller 5 Distortion
            F5RND = 108,     // Flexible Controller 5 Randomness

            F6IN = 109,      // Flexible Controller 6 Input
            F6MIN = 110,     // Flexible Controller 6 Minimum Value
            F6MAX = 111,     // Flexible Controller 6 Maximum Value
            F6AMT = 112,     // Flexible Controller 6 Amount
            F6DST = 113,     // Flexible Controller 6 Distortion
            F6RND = 114,     // Flexible Controller 6 Randomness

            F7IN = 115,      // Flexible Controller 7 Input
            F7MIN = 116,     // Flexible Controller 7 Minimum Value
            F7MAX = 117,     // Flexible Controller 7 Maximum Value
            F7AMT = 118,     // Flexible Controller 7 Amount
            F7DST = 119,     // Flexible Controller 7 Distortion
            F7RND = 120,     // Flexible Controller 7 Randomness

            F8IN = 121,      // Flexible Controller 8 Input
            F8MIN = 122,     // Flexible Controller 8 Minimum Value
            F8MAX = 123,     // Flexible Controller 8 Maximum Value
            F8AMT = 124,     // Flexible Controller 8 Amount
            F8DST = 125,     // Flexible Controller 8 Distortion
            F8RND = 126,     // Flexible Controller 8 Randomness

            F9IN = 127,      // Flexible Controller 9 Input
            F9MIN = 128,     // Flexible Controller 9 Minimum Value
            F9MAX = 129,     // Flexible Controller 9 Maximum Value
            F9AMT = 130,     // Flexible Controller 9 Amount
            F9DST = 131,     // Flexible Controller 9 Distortion
            F9RND = 132,     // Flexible Controller 9 Randomness

            F10IN = 133,     // Flexible Controller 10 Input
            F10MIN = 134,    // Flexible Controller 10 Minimum Value
            F10MAX = 135,    // Flexible Controller 10 Maximum Value
            F10AMT = 136,    // Flexible Controller 10 Amount
            F10DST = 137,    // Flexible Controller 10 Distortion
            F10RND = 138,    // Flexible Controller 10 Randomness

            N1AMT = 139,     // Envelope 1 Amount
            N1INI = 140,     // Envelope 1 Initial Level
            N1DEL = 141,     // Envelope 1 Delay Time
            N1ATK = 142,     // Envelope 1 Attack Time
            N1PK = 143,      // Envelope 1 Peak Level
            N1HLD = 144,     // Envelope 1 Hold Time
            N1DEC = 145,     // Envelope 1 Decay Time
            N1SUS = 146,     // Envelope 1 Sustain Level
            N1REL = 147,     // Envelope 1 Release Time
            N1FIN = 148,     // Envelope 1 Final Level

            N2AMT = 149,     // Envelope 2 Amount
            N2INI = 150,     // Envelope 2 Initial Level
            N2DEL = 151,     // Envelope 2 Delay Time
            N2ATK = 152,     // Envelope 2 Attack Time
            N2PK = 153,      // Envelope 2 Peak Level
            N2HLD = 154,     // Envelope 2 Hold Time
            N2DEC = 155,     // Envelope 2 Decay Time
            N2SUS = 156,     // Envelope 2 Sustain Level
            N2REL = 157,     // Envelope 2 Release Time
            N2FIN = 158,     // Envelope 2 Final Level

            N3AMT = 159,     // Envelope 3 Amount
            N3INI = 160,     // Envelope 3 Initial Level
            N3DEL = 161,     // Envelope 3 Delay Time
            N3ATK = 162,     // Envelope 3 Attack Time
            N3PK = 163,      // Envelope 3 Peak Level
            N3HLD = 164,     // Envelope 3 Hold Time
            N3DEC = 165,     // Envelope 3 Decay Time
            N3SUS = 166,     // Envelope 3 Sustain Level
            N3REL = 167,     // Envelope 3 Release Time
            N3FIN = 168,     // Envelope 3 Final Level

            N4AMT = 169,     // Envelope 4 Amount
            N4INI = 170,     // Envelope 4 Initial Level
            N4DEL = 171,     // Envelope 4 Delay Time
            N4ATK = 172,     // Envelope 4 Attack Time
            N4PK = 173,      // Envelope 4 Peak Level
            N4HLD = 174,     // Envelope 4 Hold Time
            N4DEC = 175,     // Envelope 4 Decay Time
            N4SUS = 176,     // Envelope 4 Sustain Level
            N4REL = 177,     // Envelope 4 Release Time
            N4FIN = 178,     // Envelope 4 Final Level

            N5AMT = 179,     // Envelope 5 Amount
            N5INI = 180,     // Envelope 5 Initial Level
            N5DEL = 181,     // Envelope 5 Delay Time
            N5ATK = 182,     // Envelope 5 Attack Time
            N5PK = 183,      // Envelope 5 Peak Level
            N5HLD = 184,     // Envelope 5 Hold Time
            N5DEC = 185,     // Envelope 5 Decay Time
            N5SUS = 186,     // Envelope 5 Sustain Level
            N5REL = 187,     // Envelope 5 Release Time
            N5FIN = 188,     // Envelope 5 Final Level

            N6AMT = 189,     // Envelope 6 Amount
            N6INI = 190,     // Envelope 6 Initial Level
            N6DEL = 191,     // Envelope 6 Delay Time
            N6ATK = 192,     // Envelope 6 Attack Time
            N6PK = 193,      // Envelope 6 Peak Level
            N6HLD = 194,     // Envelope 6 Hold Time
            N6DEC = 195,     // Envelope 6 Decay Time
            N6SUS = 196,     // Envelope 6 Sustain Level
            N6REL = 197,     // Envelope 6 Release Time
            N6FIN = 198,     // Envelope 6 Final Level

            L1FRQ = 199,     // LFO 1 Frequency
            L1AMT = 200,     // LFO 1 Amount
            L1MIN = 201,     // LFO 1 Minimum Value
            L1MAX = 202,     // LFO 1 Maximum Value
            L1DST = 203,     // LFO 1 Distortion
            L1RND = 204,     // LFO 1 Randomness

            L2FRQ = 205,     // LFO 2 Frequency
            L2AMT = 206,     // LFO 2 Amount
            L2MIN = 207,     // LFO 2 Minimum Value
            L2MAX = 208,     // LFO 2 Maximum Value
            L2DST = 209,     // LFO 2 Distortion
            L2RND = 210,     // LFO 2 Randomness

            L3FRQ = 211,     // LFO 3 Frequency
            L3AMT = 212,     // LFO 3 Amount
            L3MIN = 213,     // LFO 3 Minimum Value
            L3MAX = 214,     // LFO 3 Maximum Value
            L3DST = 215,     // LFO 3 Distortion
            L3RND = 216,     // LFO 3 Randomness

            L4FRQ = 217,     // LFO 4 Frequency
            L4AMT = 218,     // LFO 4 Amount
            L4MIN = 219,     // LFO 4 Minimum Value
            L4MAX = 220,     // LFO 4 Maximum Value
            L4DST = 221,     // LFO 4 Distortion
            L4RND = 222,     // LFO 4 Randomness

            L5FRQ = 223,     // LFO 5 Frequency
            L5AMT = 224,     // LFO 5 Amount
            L5MIN = 225,     // LFO 5 Minimum Value
            L5MAX = 226,     // LFO 5 Maximum Value
            L5DST = 227,     // LFO 5 Distortion
            L5RND = 228,     // LFO 5 Randomness

            L6FRQ = 229,     // LFO 6 Frequency
            L6AMT = 230,     // LFO 6 Amount
            L6MIN = 231,     // LFO 6 Minimum Value
            L6MAX = 232,     // LFO 6 Maximum Value
            L6DST = 233,     // LFO 6 Distortion
            L6RND = 234,     // LFO 6 Randomness

            L7FRQ = 235,     // LFO 7 Frequency
            L7AMT = 236,     // LFO 7 Amount
            L7MIN = 237,     // LFO 7 Minimum Value
            L7MAX = 238,     // LFO 7 Maximum Value
            L7DST = 239,     // LFO 7 Distortion
            L7RND = 240,     // LFO 7 Randomness

            L8FRQ = 241,     // LFO 8 Frequency
            L8AMT = 242,     // LFO 8 Amount
            L8MIN = 243,     // LFO 8 Minimum Value
            L8MAX = 244,     // LFO 8 Maximum Value
            L8DST = 245,     // LFO 8 Distortion
            L8RND = 246,     // LFO 8 Randomness

            MODE = 247,      // Mode

            MWAV = 248,      // Modulator Waveform
            CWAV = 249,      // Carrier Waveform

            MF1TYP = 250,    // Modulator Filter 1 Type
            MF2TYP = 251,    // Modulator Filter 2 Type

            CF1TYP = 252,    // Carrier Filter 1 Type
            CF2TYP = 253,    // Carrier Filter 2 Type

            EF1TYP = 254,    // Effects Filter 1 Type
            EF2TYP = 255,    // Effects Filter 2 Type

            L1WAV = 256,     // LFO 1 Waveform
            L2WAV = 257,     // LFO 2 Waveform
            L3WAV = 258,     // LFO 3 Waveform
            L4WAV = 259,     // LFO 4 Waveform
            L5WAV = 260,     // LFO 5 Waveform
            L6WAV = 261,     // LFO 6 Waveform
            L7WAV = 262,     // LFO 7 Waveform
            L8WAV = 263,     // LFO 8 Waveform

            MAX_PARAM_ID = 264,
        };

        enum ControllerId {
            NONE = 0,                       // None
            MODULATION_WHEEL = 1,           // Modulation Wheel (CC 1)
            BREATH = 2,                     // Breath (CC 2)
            UNDEFINED_1 = 3,                // Undefined (CC 3)
            FOOT_PEDAL = 4,                 // Foot Pedal (CC 4)
            PORTAMENTO_TIME = 5,            // Portamento Time (CC 5)
            VOLUME = 7,                     // Volume (CC 7)
            BALANCE = 8,                    // Balance (CC 8)
            UNDEFINED_2 = 9,                // Undefined (CC 9)
            PAN = 10,                       // Pan (CC 10)
            EXPRESSION_PEDAL = 11,          // Expression Pedal (CC 11)
            FX_CTL_1 = 12,                  // Effect Control 1 (CC 12)
            FX_CTL_2 = 13,                  // Effect Control 2 (CC 13)
            UNDEFINED_3 = 14,               // Undefined (CC 14)
            UNDEFINED_4 = 15,               // Undefined (CC 15)
            GENERAL_1 = 16,                 // General (CC 16)
            GENERAL_2 = 17,                 // General (CC 17)
            GENERAL_3 = 18,                 // General (CC 18)
            GENERAL_4 = 19,                 // General (CC 19)
            UNDEFINED_5 = 20,               // Undefined (CC 20)
            UNDEFINED_6 = 21,               // Undefined (CC 21)
            UNDEFINED_7 = 22,               // Undefined (CC 22)
            UNDEFINED_8 = 23,               // Undefined (CC 23)
            UNDEFINED_9 = 24,               // Undefined (CC 24)
            UNDEFINED_10 = 25,              // Undefined (CC 25)
            UNDEFINED_11 = 26,              // Undefined (CC 26)
            UNDEFINED_12 = 27,              // Undefined (CC 27)
            UNDEFINED_13 = 28,              // Undefined (CC 28)
            UNDEFINED_14 = 29,              // Undefined (CC 29)
            UNDEFINED_15 = 30,              // Undefined (CC 30)
            UNDEFINED_16 = 31,              // Undefined (CC 31)
            PORTAMENTO_AMOUNT = 84,         // Portamento (CC 84)
            SOUND_1 = 70,                   // Sound 1 (CC 70)
            SOUND_2 = 71,                   // Sound 2 (CC 71)
            SOUND_3 = 72,                   // Sound 3 (CC 72)
            SOUND_4 = 73,                   // Sound 4 (CC 73)
            SOUND_5 = 74,                   // Sound 5 (CC 74)
            SOUND_6 = 75,                   // Sound 6 (CC 75)
            SOUND_7 = 76,                   // Sound 7 (CC 76)
            SOUND_8 = 77,                   // Sound 8 (CC 77)
            SOUND_9 = 78,                   // Sound 9 (CC 78)
            SOUND_10 = 79,                  // Sound 10 (CC 79)
            UNDEFINED_17 = 85,              // Undefined (CC 85)
            UNDEFINED_18 = 86,              // Undefined (CC 86)
            UNDEFINED_19 = 87,              // Undefined (CC 87)
            UNDEFINED_20 = 89,              // Undefined (CC 89)
            UNDEFINED_21 = 90,              // Undefined (CC 90)
            FX_1 = 91,                      // Effect 1 (CC 91)
            FX_2 = 92,                      // Effect 2 (CC 92)
            FX_3 = 93,                      // Effect 3 (CC 93)
            FX_4 = 94,                      // Effect 4 (CC 94)
            FX_5 = 95,                      // Effect 5 (CC 95)
            UNDEFINED_22 = 102,             // Undefined (CC 102)
            UNDEFINED_23 = 103,             // Undefined (CC 103)
            UNDEFINED_24 = 104,             // Undefined (CC 104)
            UNDEFINED_25 = 105,             // Undefined (CC 105)
            UNDEFINED_26 = 106,             // Undefined (CC 106)
            UNDEFINED_27 = 107,             // Undefined (CC 107)
            UNDEFINED_28 = 108,             // Undefined (CC 108)
            UNDEFINED_29 = 109,             // Undefined (CC 109)
            UNDEFINED_30 = 110,             // Undefined (CC 110)
            UNDEFINED_31 = 111,             // Undefined (CC 111)
            UNDEFINED_32 = 112,             // Undefined (CC 112)
            UNDEFINED_33 = 113,             // Undefined (CC 113)
            UNDEFINED_34 = 114,             // Undefined (CC 114)
            UNDEFINED_35 = 115,             // Undefined (CC 115)
            UNDEFINED_36 = 116,             // Undefined (CC 116)
            UNDEFINED_37 = 117,             // Undefined (CC 117)
            UNDEFINED_38 = 118,             // Undefined (CC 118)
            UNDEFINED_39 = 119,             // Undefined (CC 119)
            PITCH_WHEEL = 128,              // Pitch Wheel
            NOTE = 129,                     // Note
            VELOCITY = 130,                 // Velocity
            FLEXIBLE_CONTROLLER_1 = 131,    // Flexible Controller 1
            FLEXIBLE_CONTROLLER_2 = 132,    // Flexible Controller 2
            FLEXIBLE_CONTROLLER_3 = 133,    // Flexible Controller 3
            FLEXIBLE_CONTROLLER_4 = 134,    // Flexible Controller 4
            FLEXIBLE_CONTROLLER_5 = 135,    // Flexible Controller 5
            FLEXIBLE_CONTROLLER_6 = 136,    // Flexible Controller 6
            FLEXIBLE_CONTROLLER_7 = 137,    // Flexible Controller 7
            FLEXIBLE_CONTROLLER_8 = 138,    // Flexible Controller 8
            FLEXIBLE_CONTROLLER_9 = 139,    // Flexible Controller 9
            FLEXIBLE_CONTROLLER_10 = 140,   // Flexible Controller 10
            LFO_1 = 141,                    // LFO 1
            LFO_2 = 142,                    // LFO 2
            LFO_3 = 143,                    // LFO 3
            LFO_4 = 144,                    // LFO 4
            LFO_5 = 145,                    // LFO 5
            LFO_6 = 146,                    // LFO 6
            LFO_7 = 147,                    // LFO 7
            LFO_8 = 148,                    // LFO 8
            ENVELOPE_1 = 149,               // Envelope 1
            ENVELOPE_2 = 150,               // Envelope 2
            ENVELOPE_3 = 151,               // Envelope 3
            ENVELOPE_4 = 152,               // Envelope 4
            ENVELOPE_5 = 153,               // Envelope 5
            ENVELOPE_6 = 154,               // Envelope 6

            MAX_CONTROLLER_ID = 155,
        };

        Synth();
        virtual ~Synth() override;

        bool is_lock_free() const;

        void suspend();
        void resume();

        Sample const* const* generate_samples(
            Integer const round, Integer const sample_count
        );

        void push_message(
            MessageType const message,
            ParamId const param_id,
            Number const number_param,
            Byte const byte_param
        );

        std::string get_param_name(ParamId const param_id) const;
        ParamId get_param_id(std::string const name) const;
        void get_param_id_hash_table_statistics(
            Integer& max_collisions,
            Number& avg_collisions,
            Number& avg_bucket_size
        ) const;

        Number float_param_ratio_to_display_value(
            ParamId const param_id,
            Number const ratio
        ) const;
        Byte int_param_ratio_to_display_value(
            ParamId const param_id,
            Number const ratio
        ) const;

        Number get_param_ratio_atomic(ParamId const param_id) const;
        Number get_param_default_ratio(ParamId const param_id) const;
        ControllerId get_param_controller_id_atomic(ParamId const param_id) const;

        void note_on(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity
        );

        void aftertouch(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const pressure
        );

        void note_off(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity
        );

        void control_change(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Controller const controller,
            Number const new_value
        );

        void pitch_wheel_change(
            Seconds const time_offset,
            Midi::Channel const channel,
            Number const new_value
        );

        void all_sound_off(
            Seconds const time_offset, Midi::Channel const channel
        );

        void reset_all_controllers(
            Seconds const time_offset, Midi::Channel const channel
        );

        void all_notes_off(
            Seconds const time_offset, Midi::Channel const channel
        );

        // TODO: operating mode: mix&mod (same as add + AM + FM in JS-80) vs. splits

        FloatParam volume;
        FloatParam modulator_add_volume;
        FloatParam frequency_modulation_level;
        FloatParam amplitude_modulation_level;

        Modulator::Params modulator_params;
        Carrier::Params carrier_params;

        MidiController pitch_wheel;
        MidiController note;
        MidiController velocity;

    protected:
        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        );

        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        );

        Frequency frequencies[Midi::NOTES];

    private:
        class Message
        {
            public:
                Message();
                Message(Message const& message);
                Message(
                    MessageType const type,
                    ParamId const param_id,
                    Number const number_param,
                    Byte const byte_param
                );

                Message& operator=(Message const& message);

                MessageType type;
                ParamId param_id;
                Number number_param;
                Byte byte_param;
        };

        /*
        See Timur Doumler [ACCU 2017]: Lock-free programming with modern C++
          https://www.youtube.com/watch?v=qdrp6k4rcP4
        */
        class SingleProducerSingleConsumerMessageQueue
        {
            public:
                SingleProducerSingleConsumerMessageQueue();

                SingleProducerSingleConsumerMessageQueue(
                    SingleProducerSingleConsumerMessageQueue const& queue
                ) = delete;

                bool is_lock_free() const;
                bool push(Message const& message);
                bool pop(Message& message);
                size_t size() const;

            private:
                static constexpr size_t SIZE = 0x1000; /* must be power of 2 */
                static constexpr size_t SIZE_MASK = SIZE - 1;

                size_t advance(size_t const index) const;

                Message messages[SIZE];

                std::atomic<size_t> next_push;
                std::atomic<size_t> next_pop;
        };

        class Bus : public SignalProducer
        {
            friend class SignalProducer;

            public:
                Bus(
                    Integer const channels,
                    Modulator* const* const modulators,
                    Carrier* const* const carriers,
                    Integer const polyphony,
                    FloatParam& volume,
                    FloatParam& modulator_add_volume
                );

                Sample const* const* initialize_rendering(
                    Integer const round,
                    Integer const sample_count
                );

                void render(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample** buffer
                );

            private:
                void mix_modulators(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample** buffer
                ) const;

                void mix_carriers(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample** buffer
                ) const;

                void apply_volume(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample** buffer
                ) const;

                Integer const polyphony;
                Synth::Modulator* const* const modulators;
                Synth::Carrier* const* const carriers;
                FloatParam& volume;
                FloatParam& modulator_add_volume;
                Sample const* volume_buffer;
                Sample const* modulator_add_volume_buffer;
                std::vector<bool> modulators_on;
                std::vector<bool> carriers_on;
                bool is_silent;
        };

        class ParamIdHashTable
        {
            public:
                ParamIdHashTable();
                ~ParamIdHashTable();

                void add(char const* name, ParamId const param_id);
                ParamId lookup(char const* name);
                void get_statistics(
                    Integer& max_collisions,
                    Number& avg_collisions,
                    Number& avg_bucket_size
                ) const;

            private:
                class Entry
                {
                    public:
                        static constexpr Integer NAME_SIZE = 8;
                        static constexpr Integer NAME_MAX_INDEX = NAME_SIZE - 1;

                        Entry();
                        Entry(const char* name, ParamId const param_id);
                        ~Entry();

                        void set(const char* name, ParamId const param_id);

                        Entry *next;
                        char name[NAME_SIZE];
                        ParamId param_id;
                };

                static constexpr Integer ENTRIES = 0x80;
                static constexpr Integer MASK = 0x7f;
                static constexpr Integer MULTIPLIER = 23781;
                static constexpr Integer SHIFT = 9;

                static Integer hash(char const* name);

                void lookup(
                    char const* name,
                    Entry** root,
                    Entry** parent,
                    Entry** entry
                );

                Entry entries[ENTRIES];
        };

        typedef Distortion<Bus> Overdrive;
        typedef Distortion<Overdrive> Distortion_;
        typedef BiquadFilter<Distortion_> Filter1;
        typedef BiquadFilter<Filter1> Filter2;

        static constexpr Integer NEXT_VOICE_MASK = 0x0f;
        static constexpr Integer POLYPHONY = NEXT_VOICE_MASK + 1;

        static constexpr Integer FLOAT_PARAMS = 247;

        static constexpr Number NOTE_TO_PARAM_SCALE = 1.0 / 127.0;

        static ParamIdHashTable param_id_hash_table;

        template<class ParamClass>
        void register_param_as_child(
            ParamId const param_id,
            ParamClass& param
        );

        void register_float_param_as_child(
            ParamId const param_id,
            FloatParam& float_param
        );

        void register_float_param(
            ParamId const param_id,
            FloatParam& float_param
        );

        void process_messages();
        void handle_set_param(
            ParamId const param_id,
            Number const ratio
        );
        void handle_assign_controller(
            ParamId const param_id,
            Byte const controller_id
        );
        void handle_refresh_param(ParamId const param_id);

        void assign_controller_to_param(
            ParamId const param_id,
            ControllerId const controller_id
        );
        void assign_controller_to_float_param(
            ParamId const param_id,
            ControllerId const controller_id
        );

        Number get_param_ratio(ParamId const param_id) const;

        void clear_midi_controllers();

        void update_param_states();

        std::string const to_string(Integer const) const;

        SingleProducerSingleConsumerMessageQueue messages;
        Bus bus;
        Overdrive overdrive;
        Distortion_ distortion;
        typename Filter1::TypeParam filter_1_type;
        typename Filter2::TypeParam filter_2_type;
        Filter1 filter_1;
        Filter2 filter_2;
        FloatParam* float_params[FLOAT_PARAMS];
        std::atomic<Number> param_ratios[ParamId::MAX_PARAM_ID];
        std::atomic<Byte> controller_assignments[ParamId::MAX_PARAM_ID];
        Envelope* envelopes_rw[ENVELOPES];
        FlexibleController* flexible_controllers_rw[FLEXIBLE_CONTROLLERS];
        MidiController* midi_controllers_rw[MIDI_CONTROLLERS];
        Integer midi_note_to_voice_assignments[Midi::CHANNELS][Midi::NOTES];
        Modulator* modulators[POLYPHONY];
        Carrier* carriers[POLYPHONY];
        Integer next_voice;
        Midi::Note previous_note;

    public:
        MidiController* const* const midi_controllers;
        FlexibleController* const* const flexible_controllers;
        Envelope* const* const envelopes;
};

}

#endif
