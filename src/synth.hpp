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
#include "synth/comb_filter.hpp"
#include "synth/delay.hpp"
#include "synth/distortion.hpp"
#include "synth/echo.hpp"
#include "synth/effect.hpp"
#include "synth/effects.hpp"
#include "synth/flexible_controller.hpp"
#include "synth/filter.hpp"
#include "synth/lfo.hpp"
#include "synth/math.hpp"
#include "synth/midi_controller.hpp"
#include "synth/oscillator.hpp"
#include "synth/param.hpp"
#include "synth/reverb.hpp"
#include "synth/queue.hpp"
#include "synth/signal_producer.hpp"
#include "synth/voice.hpp"


namespace JS80P
{

/**
 * \warning Calling any method of a \c Synth object or its members outside the
 *          audio thread is not safe, unless indicated otherwise.
 */
class Synth : public Midi::EventHandler, public SignalProducer
{
    friend class SignalProducer;

    public:
        typedef Voice<SignalProducer> Modulator;
        typedef Voice<Modulator::ModulationOut> Carrier;

        static constexpr Integer OUT_CHANNELS = Carrier::CHANNELS;

        static constexpr Integer ENVELOPES = 6;
        static constexpr Integer MIDI_CONTROLLERS = 128;
        static constexpr Integer FLEXIBLE_CONTROLLERS = 10;
        static constexpr Integer LFOS = 8;

        enum MessageType {
            SET_PARAM = 1,          ///< Set the given parameter's value to
                                    ///< \c number_param.

            ASSIGN_CONTROLLER = 2,  ///< Assign the controller identified by
                                    ///< \c byte_param to the given parameter.

            REFRESH_PARAM = 3,      ///< Make sure that \c get_param_ratio_atomic()
                                    ///< will return the most recent value of
                                    ///< the given parameter.

            INVALID,
        };

        enum ParamId {
            MIX = 0,         ///< Modulator Additive Volume
            PM = 1,          ///< Phase Modulation
            FM = 2,          ///< Frequency Modulation
            AM = 3,          ///< Amplitude Modulation

            MAMP = 4,        ///< Modulator Amplitude
            MVS = 5,         ///< Modulator Velocity Sensitivity
            MFLD = 6,        ///< Modulator Folding
            MPRT = 7,        ///< Modulator Portamento Length
            MPRD = 8,        ///< Modulator Portamento Depth
            MDTN = 9,        ///< Modulator Detune
            MFIN = 10,       ///< Modulator Fine Detune
            MWID = 11,       ///< Modulator Width
            MPAN = 12,       ///< Modulator Pan
            MVOL = 13,       ///< Modulator Volume

            MC1 = 14,        ///< Modulator Custom Waveform 1st Harmonic
            MC2 = 15,        ///< Modulator Custom Waveform 2nd Harmonic
            MC3 = 16,        ///< Modulator Custom Waveform 3rd Harmonic
            MC4 = 17,        ///< Modulator Custom Waveform 4th Harmonic
            MC5 = 18,        ///< Modulator Custom Waveform 5th Harmonic
            MC6 = 19,        ///< Modulator Custom Waveform 6th Harmonic
            MC7 = 20,        ///< Modulator Custom Waveform 7th Harmonic
            MC8 = 21,        ///< Modulator Custom Waveform 8th Harmonic
            MC9 = 22,        ///< Modulator Custom Waveform 9th Harmonic
            MC10 = 23,       ///< Modulator Custom Waveform 10th Harmonic

            MF1FRQ = 24,     ///< Modulator Filter 1 Frequency
            MF1Q = 25,       ///< Modulator Filter 1 Q Factor
            MF1G = 26,       ///< Modulator Filter 1 Gain

            MF2FRQ = 27,     ///< Modulator Filter 2 Frequency
            MF2Q = 28,       ///< Modulator Filter 2 Q Factor
            MF2G = 29,       ///< Modulator Filter 2 Gain

            CAMP = 30,       ///< Carrier Amplitude
            CVS = 31,        ///< Carrier Velocity Sensitivity
            CFLD = 32,       ///< Carrier Folding
            CPRT = 33,       ///< Carrier Portamento Length
            CPRD = 34,       ///< Carrier Portamento Depth
            CDTN = 35,       ///< Carrier Detune
            CFIN = 36,       ///< Carrier Fine Detune
            CWID = 37,       ///< Carrier Width
            CPAN = 38,       ///< Carrier Pan
            CVOL = 39,       ///< Carrier Volume

            CC1 = 40,        ///< Carrier Custom Waveform 1st Harmonic
            CC2 = 41,        ///< Carrier Custom Waveform 2nd Harmonic
            CC3 = 42,        ///< Carrier Custom Waveform 3rd Harmonic
            CC4 = 43,        ///< Carrier Custom Waveform 4th Harmonic
            CC5 = 44,        ///< Carrier Custom Waveform 5th Harmonic
            CC6 = 45,        ///< Carrier Custom Waveform 6th Harmonic
            CC7 = 46,        ///< Carrier Custom Waveform 7th Harmonic
            CC8 = 47,        ///< Carrier Custom Waveform 8th Harmonic
            CC9 = 48,        ///< Carrier Custom Waveform 9th Harmonic
            CC10 = 49,       ///< Carrier Custom Waveform 10th Harmonic

            CF1FRQ = 50,     ///< Carrier Filter 1 Frequency
            CF1Q = 51,       ///< Carrier Filter 1 Q Factor
            CF1G = 52,       ///< Carrier Filter 1 Gain

            CF2FRQ = 53,     ///< Carrier Filter 2 Frequency
            CF2Q = 54,       ///< Carrier Filter 2 Q Factor
            CF2G = 55,       ///< Carrier Filter 2 Gain

            EOG = 56,        ///< Effects Overdrive Gain

            EDG = 57,        ///< Effects Distortaion Gain

            EF1FRQ = 58,     ///< Effects Filter 1 Frequency
            EF1Q = 59,       ///< Effects Filter 1 Q Factor
            EF1G = 60,       ///< Effects Filter 1 Gain

            EF2FRQ = 61,     ///< Effects Filter 2 Frequency
            EF2Q = 62,       ///< Effects Filter 2 Q Factor
            EF2G = 63,       ///< Effects Filter 2 Gain

            EEDEL = 64,      ///< Effects Echo Delay
            EEFB = 65,       ///< Effects Echo Feedback
            EEDF = 66,       ///< Effects Echo Dampening Frequency
            EEDG = 67,       ///< Effects Echo Dampening Gain
            EEWID = 68,      ///< Effects Echo Stereo Width
            EEHPF = 69,      ///< Effects Echo Highpass Frequency
            EEWET = 70,      ///< Effects Echo Wet Volume
            EEDRY = 71,      ///< Effects Echo Dry Volume

            ERRS = 72,       ///< Effects Reverb Room Size
            ERDF = 73,       ///< Effects Reverb Dampening Frequency
            ERDG = 74,       ///< Effects Reverb Dampening Gain
            ERWID = 75,      ///< Effects Reverb Stereo Width
            ERHPF = 76,      ///< Effects Reverb Highpass Frequency
            ERWET = 77,      ///< Effects Reverb Wet Volume
            ERDRY = 78,      ///< Effects Reverb Dry Volume

            F1IN = 79,       ///< Flexible Controller 1 Input
            F1MIN = 80,      ///< Flexible Controller 1 Minimum Value
            F1MAX = 81,      ///< Flexible Controller 1 Maximum Value
            F1AMT = 82,      ///< Flexible Controller 1 Amount
            F1DST = 83,      ///< Flexible Controller 1 Distortion
            F1RND = 84,      ///< Flexible Controller 1 Randomness

            F2IN = 85,       ///< Flexible Controller 2 Input
            F2MIN = 86,      ///< Flexible Controller 2 Minimum Value
            F2MAX = 87,      ///< Flexible Controller 2 Maximum Value
            F2AMT = 88,      ///< Flexible Controller 2 Amount
            F2DST = 89,      ///< Flexible Controller 2 Distortion
            F2RND = 90,      ///< Flexible Controller 2 Randomness

            F3IN = 91,       ///< Flexible Controller 3 Input
            F3MIN = 92,      ///< Flexible Controller 3 Minimum Value
            F3MAX = 93,      ///< Flexible Controller 3 Maximum Value
            F3AMT = 94,      ///< Flexible Controller 3 Amount
            F3DST = 95,      ///< Flexible Controller 3 Distortion
            F3RND = 96,      ///< Flexible Controller 3 Randomness

            F4IN = 97,       ///< Flexible Controller 4 Input
            F4MIN = 98,      ///< Flexible Controller 4 Minimum Value
            F4MAX = 99,      ///< Flexible Controller 4 Maximum Value
            F4AMT = 100,     ///< Flexible Controller 4 Amount
            F4DST = 101,     ///< Flexible Controller 4 Distortion
            F4RND = 102,     ///< Flexible Controller 4 Randomness

            F5IN = 103,      ///< Flexible Controller 5 Input
            F5MIN = 104,     ///< Flexible Controller 5 Minimum Value
            F5MAX = 105,     ///< Flexible Controller 5 Maximum Value
            F5AMT = 106,     ///< Flexible Controller 5 Amount
            F5DST = 107,     ///< Flexible Controller 5 Distortion
            F5RND = 108,     ///< Flexible Controller 5 Randomness

            F6IN = 109,      ///< Flexible Controller 6 Input
            F6MIN = 110,     ///< Flexible Controller 6 Minimum Value
            F6MAX = 111,     ///< Flexible Controller 6 Maximum Value
            F6AMT = 112,     ///< Flexible Controller 6 Amount
            F6DST = 113,     ///< Flexible Controller 6 Distortion
            F6RND = 114,     ///< Flexible Controller 6 Randomness

            F7IN = 115,      ///< Flexible Controller 7 Input
            F7MIN = 116,     ///< Flexible Controller 7 Minimum Value
            F7MAX = 117,     ///< Flexible Controller 7 Maximum Value
            F7AMT = 118,     ///< Flexible Controller 7 Amount
            F7DST = 119,     ///< Flexible Controller 7 Distortion
            F7RND = 120,     ///< Flexible Controller 7 Randomness

            F8IN = 121,      ///< Flexible Controller 8 Input
            F8MIN = 122,     ///< Flexible Controller 8 Minimum Value
            F8MAX = 123,     ///< Flexible Controller 8 Maximum Value
            F8AMT = 124,     ///< Flexible Controller 8 Amount
            F8DST = 125,     ///< Flexible Controller 8 Distortion
            F8RND = 126,     ///< Flexible Controller 8 Randomness

            F9IN = 127,      ///< Flexible Controller 9 Input
            F9MIN = 128,     ///< Flexible Controller 9 Minimum Value
            F9MAX = 129,     ///< Flexible Controller 9 Maximum Value
            F9AMT = 130,     ///< Flexible Controller 9 Amount
            F9DST = 131,     ///< Flexible Controller 9 Distortion
            F9RND = 132,     ///< Flexible Controller 9 Randomness

            F10IN = 133,     ///< Flexible Controller 10 Input
            F10MIN = 134,    ///< Flexible Controller 10 Minimum Value
            F10MAX = 135,    ///< Flexible Controller 10 Maximum Value
            F10AMT = 136,    ///< Flexible Controller 10 Amount
            F10DST = 137,    ///< Flexible Controller 10 Distortion
            F10RND = 138,    ///< Flexible Controller 10 Randomness

            N1AMT = 139,     ///< Envelope 1 Amount
            N1INI = 140,     ///< Envelope 1 Initial Level
            N1DEL = 141,     ///< Envelope 1 Delay Time
            N1ATK = 142,     ///< Envelope 1 Attack Time
            N1PK = 143,      ///< Envelope 1 Peak Level
            N1HLD = 144,     ///< Envelope 1 Hold Time
            N1DEC = 145,     ///< Envelope 1 Decay Time
            N1SUS = 146,     ///< Envelope 1 Sustain Level
            N1REL = 147,     ///< Envelope 1 Release Time
            N1FIN = 148,     ///< Envelope 1 Final Level

            N2AMT = 149,     ///< Envelope 2 Amount
            N2INI = 150,     ///< Envelope 2 Initial Level
            N2DEL = 151,     ///< Envelope 2 Delay Time
            N2ATK = 152,     ///< Envelope 2 Attack Time
            N2PK = 153,      ///< Envelope 2 Peak Level
            N2HLD = 154,     ///< Envelope 2 Hold Time
            N2DEC = 155,     ///< Envelope 2 Decay Time
            N2SUS = 156,     ///< Envelope 2 Sustain Level
            N2REL = 157,     ///< Envelope 2 Release Time
            N2FIN = 158,     ///< Envelope 2 Final Level

            N3AMT = 159,     ///< Envelope 3 Amount
            N3INI = 160,     ///< Envelope 3 Initial Level
            N3DEL = 161,     ///< Envelope 3 Delay Time
            N3ATK = 162,     ///< Envelope 3 Attack Time
            N3PK = 163,      ///< Envelope 3 Peak Level
            N3HLD = 164,     ///< Envelope 3 Hold Time
            N3DEC = 165,     ///< Envelope 3 Decay Time
            N3SUS = 166,     ///< Envelope 3 Sustain Level
            N3REL = 167,     ///< Envelope 3 Release Time
            N3FIN = 168,     ///< Envelope 3 Final Level

            N4AMT = 169,     ///< Envelope 4 Amount
            N4INI = 170,     ///< Envelope 4 Initial Level
            N4DEL = 171,     ///< Envelope 4 Delay Time
            N4ATK = 172,     ///< Envelope 4 Attack Time
            N4PK = 173,      ///< Envelope 4 Peak Level
            N4HLD = 174,     ///< Envelope 4 Hold Time
            N4DEC = 175,     ///< Envelope 4 Decay Time
            N4SUS = 176,     ///< Envelope 4 Sustain Level
            N4REL = 177,     ///< Envelope 4 Release Time
            N4FIN = 178,     ///< Envelope 4 Final Level

            N5AMT = 179,     ///< Envelope 5 Amount
            N5INI = 180,     ///< Envelope 5 Initial Level
            N5DEL = 181,     ///< Envelope 5 Delay Time
            N5ATK = 182,     ///< Envelope 5 Attack Time
            N5PK = 183,      ///< Envelope 5 Peak Level
            N5HLD = 184,     ///< Envelope 5 Hold Time
            N5DEC = 185,     ///< Envelope 5 Decay Time
            N5SUS = 186,     ///< Envelope 5 Sustain Level
            N5REL = 187,     ///< Envelope 5 Release Time
            N5FIN = 188,     ///< Envelope 5 Final Level

            N6AMT = 189,     ///< Envelope 6 Amount
            N6INI = 190,     ///< Envelope 6 Initial Level
            N6DEL = 191,     ///< Envelope 6 Delay Time
            N6ATK = 192,     ///< Envelope 6 Attack Time
            N6PK = 193,      ///< Envelope 6 Peak Level
            N6HLD = 194,     ///< Envelope 6 Hold Time
            N6DEC = 195,     ///< Envelope 6 Decay Time
            N6SUS = 196,     ///< Envelope 6 Sustain Level
            N6REL = 197,     ///< Envelope 6 Release Time
            N6FIN = 198,     ///< Envelope 6 Final Level

            L1FRQ = 199,     ///< LFO 1 Frequency
            L1PHS = 200,     ///< LFO 1 Phase
            L1MIN = 201,     ///< LFO 1 Minimum Value
            L1MAX = 202,     ///< LFO 1 Maximum Value
            L1AMT = 203,     ///< LFO 1 Amount
            L1DST = 204,     ///< LFO 1 Distortion
            L1RND = 205,     ///< LFO 1 Randomness

            L2FRQ = 206,     ///< LFO 2 Frequency
            L2PHS = 207,     ///< LFO 2 Phase
            L2MIN = 208,     ///< LFO 2 Minimum Value
            L2MAX = 209,     ///< LFO 2 Maximum Value
            L2AMT = 210,     ///< LFO 2 Amount
            L2DST = 211,     ///< LFO 2 Distortion
            L2RND = 212,     ///< LFO 2 Randomness

            L3FRQ = 213,     ///< LFO 3 Frequency
            L3PHS = 214,     ///< LFO 3 Phase
            L3MIN = 215,     ///< LFO 3 Minimum Value
            L3MAX = 216,     ///< LFO 3 Maximum Value
            L3AMT = 217,     ///< LFO 3 Amount
            L3DST = 218,     ///< LFO 3 Distortion
            L3RND = 219,     ///< LFO 3 Randomness

            L4FRQ = 220,     ///< LFO 4 Frequency
            L4PHS = 221,     ///< LFO 4 Phase
            L4MIN = 222,     ///< LFO 4 Minimum Value
            L4MAX = 223,     ///< LFO 4 Maximum Value
            L4AMT = 224,     ///< LFO 4 Amount
            L4DST = 225,     ///< LFO 4 Distortion
            L4RND = 226,     ///< LFO 4 Randomness

            L5FRQ = 227,     ///< LFO 5 Frequency
            L5PHS = 228,     ///< LFO 5 Phase
            L5MIN = 229,     ///< LFO 5 Minimum Value
            L5MAX = 230,     ///< LFO 5 Maximum Value
            L5AMT = 231,     ///< LFO 5 Amount
            L5DST = 232,     ///< LFO 5 Distortion
            L5RND = 233,     ///< LFO 5 Randomness

            L6FRQ = 234,     ///< LFO 6 Frequency
            L6PHS = 235,     ///< LFO 6 Phase
            L6MIN = 236,     ///< LFO 6 Minimum Value
            L6MAX = 237,     ///< LFO 6 Maximum Value
            L6AMT = 238,     ///< LFO 6 Amount
            L6DST = 239,     ///< LFO 6 Distortion
            L6RND = 240,     ///< LFO 6 Randomness

            L7FRQ = 241,     ///< LFO 7 Frequency
            L7PHS = 242,     ///< LFO 7 Phase
            L7MIN = 243,     ///< LFO 7 Minimum Value
            L7MAX = 244,     ///< LFO 7 Maximum Value
            L7AMT = 245,     ///< LFO 7 Amount
            L7DST = 246,     ///< LFO 7 Distortion
            L7RND = 247,     ///< LFO 7 Randomness

            L8FRQ = 248,     ///< LFO 8 Frequency
            L8PHS = 249,     ///< LFO 8 Phase
            L8MIN = 250,     ///< LFO 8 Minimum Value
            L8MAX = 251,     ///< LFO 8 Maximum Value
            L8AMT = 252,     ///< LFO 8 Amount
            L8DST = 253,     ///< LFO 8 Distortion
            L8RND = 254,     ///< LFO 8 Randomness

            MODE = 255,      ///< Mode
            MWAV = 256,      ///< Modulator Waveform
            CWAV = 257,      ///< Carrier Waveform
            MF1TYP = 258,    ///< Modulator Filter 1 Type
            MF2TYP = 259,    ///< Modulator Filter 2 Type
            CF1TYP = 260,    ///< Carrier Filter 1 Type
            CF2TYP = 261,    ///< Carrier Filter 2 Type
            EF1TYP = 262,    ///< Effects Filter 1 Type
            EF2TYP = 263,    ///< Effects Filter 2 Type
            L1WAV = 264,     ///< LFO 1 Waveform
            L2WAV = 265,     ///< LFO 2 Waveform
            L3WAV = 266,     ///< LFO 3 Waveform
            L4WAV = 267,     ///< LFO 4 Waveform
            L5WAV = 268,     ///< LFO 5 Waveform
            L6WAV = 269,     ///< LFO 6 Waveform
            L7WAV = 270,     ///< LFO 7 Waveform
            L8WAV = 271,     ///< LFO 8 Waveform
            L1SYN = 272,     ///< LFO 1 Tempo Synchronization
            L2SYN = 273,     ///< LFO 2 Tempo Synchronization
            L3SYN = 274,     ///< LFO 3 Tempo Synchronization
            L4SYN = 275,     ///< LFO 4 Tempo Synchronization
            L5SYN = 276,     ///< LFO 5 Tempo Synchronization
            L6SYN = 277,     ///< LFO 6 Tempo Synchronization
            L7SYN = 278,     ///< LFO 7 Tempo Synchronization
            L8SYN = 279,     ///< LFO 8 Tempo Synchronization
            EESYN = 280,     ///< Effects Echo Tempo Synchronization

            MAX_PARAM_ID = 281
        };

        static constexpr Integer FLOAT_PARAMS = ParamId::MODE;

        enum ControllerId {
            NONE =                      Midi::NONE,                 ///< None
            MODULATION_WHEEL =          Midi::MODULATION_WHEEL,     ///< Modulation Wheel (CC 1)
            BREATH =                    Midi::BREATH,               ///< Breath (CC 2)
            UNDEFINED_1 =               Midi::UNDEFINED_1,          ///< Undefined (CC 3)
            FOOT_PEDAL =                Midi::FOOT_PEDAL,           ///< Foot Pedal (CC 4)
            PORTAMENTO_TIME =           Midi::PORTAMENTO_TIME,      ///< Portamento Time (CC 5)
            VOLUME =                    Midi::VOLUME,               ///< Volume (CC 7)
            BALANCE =                   Midi::BALANCE,              ///< Balance (CC 8)
            UNDEFINED_2 =               Midi::UNDEFINED_2,          ///< Undefined (CC 9)
            PAN =                       Midi::PAN,                  ///< Pan (CC 10)
            EXPRESSION_PEDAL =          Midi::EXPRESSION_PEDAL,     ///< Expression Pedal (CC 11)
            FX_CTL_1 =                  Midi::FX_CTL_1,             ///< Effect Control 1 (CC 12)
            FX_CTL_2 =                  Midi::FX_CTL_2,             ///< Effect Control 2 (CC 13)
            UNDEFINED_3 =               Midi::UNDEFINED_3,          ///< Undefined (CC 14)
            UNDEFINED_4 =               Midi::UNDEFINED_4,          ///< Undefined (CC 15)
            GENERAL_1 =                 Midi::GENERAL_1,            ///< General 1 (CC 16)
            GENERAL_2 =                 Midi::GENERAL_2,            ///< General 2 (CC 17)
            GENERAL_3 =                 Midi::GENERAL_3,            ///< General 3 (CC 18)
            GENERAL_4 =                 Midi::GENERAL_4,            ///< General 4 (CC 19)
            UNDEFINED_5 =               Midi::UNDEFINED_5,          ///< Undefined (CC 20)
            UNDEFINED_6 =               Midi::UNDEFINED_6,          ///< Undefined (CC 21)
            UNDEFINED_7 =               Midi::UNDEFINED_7,          ///< Undefined (CC 22)
            UNDEFINED_8 =               Midi::UNDEFINED_8,          ///< Undefined (CC 23)
            UNDEFINED_9 =               Midi::UNDEFINED_9,          ///< Undefined (CC 24)
            UNDEFINED_10 =              Midi::UNDEFINED_10,         ///< Undefined (CC 25)
            UNDEFINED_11 =              Midi::UNDEFINED_11,         ///< Undefined (CC 26)
            UNDEFINED_12 =              Midi::UNDEFINED_12,         ///< Undefined (CC 27)
            UNDEFINED_13 =              Midi::UNDEFINED_13,         ///< Undefined (CC 28)
            UNDEFINED_14 =              Midi::UNDEFINED_14,         ///< Undefined (CC 29)
            UNDEFINED_15 =              Midi::UNDEFINED_15,         ///< Undefined (CC 30)
            UNDEFINED_16 =              Midi::UNDEFINED_16,         ///< Undefined (CC 31)
            PORTAMENTO_AMOUNT =         Midi::PORTAMENTO_AMOUNT,    ///< Portamento (CC 84)
            SOUND_1 =                   Midi::SOUND_1,              ///< Sound 1 (CC 70)
            SOUND_2 =                   Midi::SOUND_2,              ///< Sound 2 (CC 71)
            SOUND_3 =                   Midi::SOUND_3,              ///< Sound 3 (CC 72)
            SOUND_4 =                   Midi::SOUND_4,              ///< Sound 4 (CC 73)
            SOUND_5 =                   Midi::SOUND_5,              ///< Sound 5 (CC 74)
            SOUND_6 =                   Midi::SOUND_6,              ///< Sound 6 (CC 75)
            SOUND_7 =                   Midi::SOUND_7,              ///< Sound 7 (CC 76)
            SOUND_8 =                   Midi::SOUND_8,              ///< Sound 8 (CC 77)
            SOUND_9 =                   Midi::SOUND_9,              ///< Sound 9 (CC 78)
            SOUND_10 =                  Midi::SOUND_10,             ///< Sound 10 (CC 79)
            UNDEFINED_17 =              Midi::UNDEFINED_17,         ///< Undefined (CC 85)
            UNDEFINED_18 =              Midi::UNDEFINED_18,         ///< Undefined (CC 86)
            UNDEFINED_19 =              Midi::UNDEFINED_19,         ///< Undefined (CC 87)
            UNDEFINED_20 =              Midi::UNDEFINED_20,         ///< Undefined (CC 89)
            UNDEFINED_21 =              Midi::UNDEFINED_21,         ///< Undefined (CC 90)
            FX_1 =                      Midi::FX_1,                 ///< Effect 1 (CC 91)
            FX_2 =                      Midi::FX_2,                 ///< Effect 2 (CC 92)
            FX_3 =                      Midi::FX_3,                 ///< Effect 3 (CC 93)
            FX_4 =                      Midi::FX_4,                 ///< Effect 4 (CC 94)
            FX_5 =                      Midi::FX_5,                 ///< Effect 5 (CC 95)
            UNDEFINED_22 =              Midi::UNDEFINED_22,         ///< Undefined (CC 102)
            UNDEFINED_23 =              Midi::UNDEFINED_23,         ///< Undefined (CC 103)
            UNDEFINED_24 =              Midi::UNDEFINED_24,         ///< Undefined (CC 104)
            UNDEFINED_25 =              Midi::UNDEFINED_25,         ///< Undefined (CC 105)
            UNDEFINED_26 =              Midi::UNDEFINED_26,         ///< Undefined (CC 106)
            UNDEFINED_27 =              Midi::UNDEFINED_27,         ///< Undefined (CC 107)
            UNDEFINED_28 =              Midi::UNDEFINED_28,         ///< Undefined (CC 108)
            UNDEFINED_29 =              Midi::UNDEFINED_29,         ///< Undefined (CC 109)
            UNDEFINED_30 =              Midi::UNDEFINED_30,         ///< Undefined (CC 110)
            UNDEFINED_31 =              Midi::UNDEFINED_31,         ///< Undefined (CC 111)
            UNDEFINED_32 =              Midi::UNDEFINED_32,         ///< Undefined (CC 112)
            UNDEFINED_33 =              Midi::UNDEFINED_33,         ///< Undefined (CC 113)
            UNDEFINED_34 =              Midi::UNDEFINED_34,         ///< Undefined (CC 114)
            UNDEFINED_35 =              Midi::UNDEFINED_35,         ///< Undefined (CC 115)
            UNDEFINED_36 =              Midi::UNDEFINED_36,         ///< Undefined (CC 116)
            UNDEFINED_37 =              Midi::UNDEFINED_37,         ///< Undefined (CC 117)
            UNDEFINED_38 =              Midi::UNDEFINED_38,         ///< Undefined (CC 118)
            UNDEFINED_39 =              Midi::UNDEFINED_39,         ///< Undefined (CC 119)
            PITCH_WHEEL =               128,                        ///< Pitch Wheel
            NOTE =                      129,                        ///< Note
            VELOCITY =                  130,                        ///< Velocity
            FLEXIBLE_CONTROLLER_1 =     131,                        ///< Flexible Controller 1
            FLEXIBLE_CONTROLLER_2 =     132,                        ///< Flexible Controller 2
            FLEXIBLE_CONTROLLER_3 =     133,                        ///< Flexible Controller 3
            FLEXIBLE_CONTROLLER_4 =     134,                        ///< Flexible Controller 4
            FLEXIBLE_CONTROLLER_5 =     135,                        ///< Flexible Controller 5
            FLEXIBLE_CONTROLLER_6 =     136,                        ///< Flexible Controller 6
            FLEXIBLE_CONTROLLER_7 =     137,                        ///< Flexible Controller 7
            FLEXIBLE_CONTROLLER_8 =     138,                        ///< Flexible Controller 8
            FLEXIBLE_CONTROLLER_9 =     139,                        ///< Flexible Controller 9
            FLEXIBLE_CONTROLLER_10 =    140,                        ///< Flexible Controller 10
            LFO_1 =                     141,                        ///< LFO 1
            LFO_2 =                     142,                        ///< LFO 2
            LFO_3 =                     143,                        ///< LFO 3
            LFO_4 =                     144,                        ///< LFO 4
            LFO_5 =                     145,                        ///< LFO 5
            LFO_6 =                     146,                        ///< LFO 6
            LFO_7 =                     147,                        ///< LFO 7
            LFO_8 =                     148,                        ///< LFO 8
            ENVELOPE_1 =                149,                        ///< Envelope 1
            ENVELOPE_2 =                150,                        ///< Envelope 2
            ENVELOPE_3 =                151,                        ///< Envelope 3
            ENVELOPE_4 =                152,                        ///< Envelope 4
            ENVELOPE_5 =                153,                        ///< Envelope 5
            ENVELOPE_6 =                154,                        ///< Envelope 6

            MAX_CONTROLLER_ID =         155,
        };

        typedef Byte Mode;

        static constexpr Mode MIX_AND_MOD = 0;
        static constexpr Mode SPLIT_AT_C3 = 1;
        static constexpr Mode SPLIT_AT_Db3 = 2;
        static constexpr Mode SPLIT_AT_D3 = 3;
        static constexpr Mode SPLIT_AT_Eb3 = 4;
        static constexpr Mode SPLIT_AT_E3 = 5;
        static constexpr Mode SPLIT_AT_F3 = 6;
        static constexpr Mode SPLIT_AT_Gb3 = 7;
        static constexpr Mode SPLIT_AT_G3 = 8;
        static constexpr Mode SPLIT_AT_Ab3 = 9;
        static constexpr Mode SPLIT_AT_A3 = 10;
        static constexpr Mode SPLIT_AT_Bb3 = 11;
        static constexpr Mode SPLIT_AT_B3 = 12;
        static constexpr Mode SPLIT_AT_C4 = 13;

        static constexpr int MODES = 14;

        class ModeParam : public Param<Mode>
        {
            public:
                ModeParam(std::string const name) noexcept;
        };

        static bool is_supported_midi_controller(
            Midi::Controller const controller
        ) noexcept;

        static bool is_controller_polyphonic(
            ControllerId const controller_id
        ) noexcept;

        Synth() noexcept;
        virtual ~Synth() override;

        bool is_lock_free() const noexcept;

        void suspend() noexcept;
        void resume() noexcept;

        Sample const* const* generate_samples(
            Integer const round, Integer const sample_count
        ) noexcept;

        /**
         * \brief Thread-safe way to change the state of the synthesizer outside
         *        the audio thread.
         */
        void push_message(
            MessageType const message,
            ParamId const param_id,
            Number const number_param,
            Byte const byte_param
        ) noexcept;

        void process_messages() noexcept;

        std::string get_param_name(ParamId const param_id) const noexcept;
        ParamId get_param_id(std::string const name) const noexcept;
        void get_param_id_hash_table_statistics(
            Integer& max_collisions,
            Number& avg_collisions,
            Number& avg_bucket_size
        ) const noexcept;

        Number float_param_ratio_to_display_value(
            ParamId const param_id,
            Number const ratio
        ) const noexcept;
        Byte int_param_ratio_to_display_value(
            ParamId const param_id,
            Number const ratio
        ) const noexcept;

        Number get_param_max_value(ParamId const param_id) const noexcept;
        Number get_param_ratio_atomic(ParamId const param_id) const noexcept;
        Number get_param_default_ratio(ParamId const param_id) const noexcept;
        ControllerId get_param_controller_id_atomic(
            ParamId const param_id
        ) const noexcept;

        void note_on(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity
        ) noexcept;

        void aftertouch(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const pressure
        ) noexcept;

        void note_off(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity
        ) noexcept;

        void control_change(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Controller const controller,
            Number const new_value
        ) noexcept;

        void pitch_wheel_change(
            Seconds const time_offset,
            Midi::Channel const channel,
            Number const new_value
        ) noexcept;

        void all_sound_off(
            Seconds const time_offset, Midi::Channel const channel
        ) noexcept;

        void reset_all_controllers(
            Seconds const time_offset, Midi::Channel const channel
        ) noexcept;

        void all_notes_off(
            Seconds const time_offset, Midi::Channel const channel
        ) noexcept;

        ModeParam mode;
        FloatParam modulator_add_volume;
        FloatParam phase_modulation_level;
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
        ) noexcept;

        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        Frequency frequencies[Midi::NOTES];

    private:
        class Message
        {
            public:
                Message() noexcept;
                Message(Message const& message) noexcept;
                Message(
                    MessageType const type,
                    ParamId const param_id,
                    Number const number_param,
                    Byte const byte_param
                ) noexcept;

                Message& operator=(Message const& message) noexcept;

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
                SingleProducerSingleConsumerMessageQueue() noexcept;

                SingleProducerSingleConsumerMessageQueue(
                    SingleProducerSingleConsumerMessageQueue const& queue
                ) = delete;

                bool is_lock_free() const noexcept;
                bool push(Message const& message) noexcept;
                bool pop(Message& message) noexcept;
                size_t size() const noexcept;

            private:
                static constexpr size_t SIZE = 0x1000; /* must be power of 2 */
                static constexpr size_t SIZE_MASK = SIZE - 1;

                size_t advance(size_t const index) const noexcept;

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
                    FloatParam& modulator_add_volume
                ) noexcept;

                Sample const* const* initialize_rendering(
                    Integer const round,
                    Integer const sample_count
                ) noexcept;

                void render(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample** buffer
                ) noexcept;

            private:
                void mix_modulators(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample** buffer
                ) const noexcept;

                void mix_carriers(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample** buffer
                ) const noexcept;

                Integer const polyphony;
                Synth::Modulator* const* const modulators;
                Synth::Carrier* const* const carriers;
                FloatParam& modulator_add_volume;
                Sample const* modulator_add_volume_buffer;
                std::vector<bool> modulators_on;
                std::vector<bool> carriers_on;
                bool is_silent;
        };

        class ParamIdHashTable
        {
            public:
                ParamIdHashTable() noexcept;
                ~ParamIdHashTable();

                void add(char const* name, ParamId const param_id) noexcept;
                ParamId lookup(char const* name) noexcept;
                void get_statistics(
                    Integer& max_collisions,
                    Number& avg_collisions,
                    Number& avg_bucket_size
                ) const noexcept;

            private:
                class Entry
                {
                    public:
                        static constexpr Integer NAME_SIZE = 8;
                        static constexpr Integer NAME_MAX_INDEX = NAME_SIZE - 1;

                        Entry() noexcept;
                        Entry(const char* name, ParamId const param_id) noexcept;
                        ~Entry();

                        void set(const char* name, ParamId const param_id) noexcept;

                        Entry *next;
                        char name[NAME_SIZE];
                        ParamId param_id;
                };

                static constexpr Integer ENTRIES = 0x80;
                static constexpr Integer MASK = 0x7f;
                static constexpr Integer MULTIPLIER = 125123;
                static constexpr Integer SHIFT = 14;

                static Integer hash(char const* name) noexcept;

                void lookup(
                    char const* name,
                    Entry** root,
                    Entry** parent,
                    Entry** entry
                ) noexcept;

                Entry entries[ENTRIES];
        };

        static constexpr Integer NEXT_VOICE_MASK = 0x0f;
        static constexpr Integer POLYPHONY = NEXT_VOICE_MASK + 1;

        static constexpr Number NOTE_TO_PARAM_SCALE = 1.0 / 127.0;

        static std::vector<bool> supported_midi_controllers;
        static bool supported_midi_controllers_initialized;

        static ParamIdHashTable param_id_hash_table;
        static std::string param_names_by_id[ParamId::MAX_PARAM_ID];

        void initialize_supported_midi_controllers() noexcept;
        void register_main_params() noexcept;
        void register_modulator_params() noexcept;
        void register_carrier_params() noexcept;
        void register_effects_params() noexcept;
        void create_voices() noexcept;
        void create_midi_controllers() noexcept;
        void create_flexible_controllers() noexcept;
        void create_envelopes() noexcept;
        void create_lfos() noexcept;

        template<class ParamClass>
        void register_param_as_child(
            ParamId const param_id,
            ParamClass& param
        ) noexcept;

        template<class ParamClass>
        void register_param(ParamId const param_id, ParamClass& param) noexcept;

        void register_float_param_as_child(
            ParamId const param_id,
            FloatParam& float_param
        ) noexcept;

        void register_float_param(
            ParamId const param_id,
            FloatParam& float_param
        ) noexcept;

        void handle_set_param(
            ParamId const param_id,
            Number const ratio
        ) noexcept;
        void handle_assign_controller(
            ParamId const param_id,
            Byte const controller_id
        ) noexcept;
        void handle_refresh_param(ParamId const param_id) noexcept;

        void assign_controller_to_param(
            ParamId const param_id,
            ControllerId const controller_id
        ) noexcept;
        void assign_controller_to_float_param(
            ParamId const param_id,
            ControllerId const controller_id
        ) noexcept;

        Number get_param_ratio(ParamId const param_id) const noexcept;

        void clear_midi_controllers() noexcept;

        void update_param_states() noexcept;

        std::string const to_string(Integer const) const noexcept;

        SingleProducerSingleConsumerMessageQueue messages;
        Bus bus;
        Effects::Effects<Bus> effects;
        Sample const* const* raw_output;
        FloatParam* float_params[FLOAT_PARAMS];
        BiquadFilterSharedCache* biquad_filter_shared_caches[4];
        std::atomic<Number> param_ratios[ParamId::MAX_PARAM_ID];
        std::atomic<Byte> controller_assignments[ParamId::MAX_PARAM_ID];
        Envelope* envelopes_rw[ENVELOPES];
        LFO* lfos_rw[LFOS];
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
        LFO* const* const lfos;
};

}

#endif
