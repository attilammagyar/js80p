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

#ifndef JS80P__SYNTH_HPP
#define JS80P__SYNTH_HPP

#include <atomic>
#include <cstddef>
#include <string>
#include <vector>

#include "js80p.hpp"
#include "midi.hpp"
#include "note_stack.hpp"
#include "spscqueue.hpp"
#include "voice.hpp"

#include "dsp/envelope.hpp"
#include "dsp/biquad_filter.hpp"
#include "dsp/chorus.hpp"
#include "dsp/delay.hpp"
#include "dsp/distortion.hpp"
#include "dsp/echo.hpp"
#include "dsp/effect.hpp"
#include "dsp/effects.hpp"
#include "dsp/filter.hpp"
#include "dsp/gain.hpp"
#include "dsp/lfo.hpp"
#include "dsp/macro.hpp"
#include "dsp/math.hpp"
#include "dsp/midi_controller.hpp"
#include "dsp/mixer.hpp"
#include "dsp/oscillator.hpp"
#include "dsp/param.hpp"
#include "dsp/peak_tracker.hpp"
#include "dsp/reverb.hpp"
#include "dsp/queue.hpp"
#include "dsp/side_chain_compressable_effect.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

/**
 * \warning Calling any method of a \c Synth object or its members outside the
 *          audio thread is not safe, unless indicated otherwise.
 */
class Synth : public Midi::EventHandler, public SignalProducer
{
    friend class SignalProducer;

    private:
        static constexpr Integer VOICE_INDEX_MASK = 0x3f;

    public:
        static constexpr Integer POLYPHONY = VOICE_INDEX_MASK + 1;

        static constexpr Integer OUT_CHANNELS = Carrier::CHANNELS;

        static constexpr Integer ENVELOPE_FLOAT_PARAMS = 12;
        static constexpr Integer ENVELOPE_DISCRETE_PARAMS = 5;

        static constexpr Integer MIDI_CONTROLLERS = 128;

        static constexpr Integer MACROS = 20;
        static constexpr Integer MACRO_FLOAT_PARAMS = 6;

        static constexpr Integer LFO_FLOAT_PARAMS = 7;

        enum MessageType {
            SET_PARAM = 1,          ///< Set the given parameter's ratio to
                                    ///< \c number_param.

            ASSIGN_CONTROLLER = 2,  ///< Assign the controller identified by
                                    ///< \c byte_param to the given parameter.

            REFRESH_PARAM = 3,      ///< Make sure that \c get_param_ratio_atomic()
                                    ///< will return the most recent value of
                                    ///< the given parameter.

            CLEAR = 4,              ///< Clear all buffers, release all
                                    ///< controller assignments, and reset all
                                    ///< parameters to their default values.

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
            MSUB = 14,       ///< Modulator Subharmonic Amplitude
            MC1 = 15,        ///< Modulator Custom Waveform 1st Harmonic
            MC2 = 16,        ///< Modulator Custom Waveform 2nd Harmonic
            MC3 = 17,        ///< Modulator Custom Waveform 3rd Harmonic
            MC4 = 18,        ///< Modulator Custom Waveform 4th Harmonic
            MC5 = 19,        ///< Modulator Custom Waveform 5th Harmonic
            MC6 = 20,        ///< Modulator Custom Waveform 6th Harmonic
            MC7 = 21,        ///< Modulator Custom Waveform 7th Harmonic
            MC8 = 22,        ///< Modulator Custom Waveform 8th Harmonic
            MC9 = 23,        ///< Modulator Custom Waveform 9th Harmonic
            MC10 = 24,       ///< Modulator Custom Waveform 10th Harmonic
            MF1FRQ = 25,     ///< Modulator Filter 1 Frequency
            MF1Q = 26,       ///< Modulator Filter 1 Q Factor
            MF1G = 27,       ///< Modulator Filter 1 Gain
            MF1FIA = 28,     ///< Modulator Filter 1 Frequency Inaccuracy
            MF1QIA = 29,     ///< Modulator Filter 1 Q Factor Inaccuracy
            MF2FRQ = 30,     ///< Modulator Filter 2 Frequency
            MF2Q = 31,       ///< Modulator Filter 2 Q Factor
            MF2G = 32,       ///< Modulator Filter 2 Gain
            MF2FIA = 33,     ///< Modulator Filter 2 Frequency Inaccuracy
            MF2QIA = 34,     ///< Modulator Filter 2 Q Factor Inaccuracy

            CAMP = 35,       ///< Carrier Amplitude
            CVS = 36,        ///< Carrier Velocity Sensitivity
            CFLD = 37,       ///< Carrier Folding
            CPRT = 38,       ///< Carrier Portamento Length
            CPRD = 39,       ///< Carrier Portamento Depth
            CDTN = 40,       ///< Carrier Detune
            CFIN = 41,       ///< Carrier Fine Detune
            CWID = 42,       ///< Carrier Width
            CPAN = 43,       ///< Carrier Pan
            CVOL = 44,       ///< Carrier Volume
            CDG = 45,        ///< Carrier Distortion Gain
            CC1 = 46,        ///< Carrier Custom Waveform 1st Harmonic
            CC2 = 47,        ///< Carrier Custom Waveform 2nd Harmonic
            CC3 = 48,        ///< Carrier Custom Waveform 3rd Harmonic
            CC4 = 49,        ///< Carrier Custom Waveform 4th Harmonic
            CC5 = 50,        ///< Carrier Custom Waveform 5th Harmonic
            CC6 = 51,        ///< Carrier Custom Waveform 6th Harmonic
            CC7 = 52,        ///< Carrier Custom Waveform 7th Harmonic
            CC8 = 53,        ///< Carrier Custom Waveform 8th Harmonic
            CC9 = 54,        ///< Carrier Custom Waveform 9th Harmonic
            CC10 = 55,       ///< Carrier Custom Waveform 10th Harmonic
            CF1FRQ = 56,     ///< Carrier Filter 1 Frequency
            CF1Q = 57,       ///< Carrier Filter 1 Q Factor
            CF1G = 58,       ///< Carrier Filter 1 Gain
            CF1FIA = 59,     ///< Carrier Filter 1 Frequency Inaccuracy
            CF1QIA = 60,     ///< Carrier Filter 1 Q Factor Inaccuracy
            CF2FRQ = 61,     ///< Carrier Filter 2 Frequency
            CF2Q = 62,       ///< Carrier Filter 2 Q Factor
            CF2G = 63,       ///< Carrier Filter 2 Gain
            CF2FIA = 64,     ///< Carrier Filter 2 Frequency Inaccuracy
            CF2QIA = 65,     ///< Carrier Filter 2 Q Factor Inaccuracy

            EV1V = 66,       ///< Effects Volume 1
            EOG = 67,        ///< Effects Overdrive Gain
            EDG = 68,        ///< Effects Distortion Gain
            EF1FRQ = 69,     ///< Effects Filter 1 Frequency
            EF1Q = 70,       ///< Effects Filter 1 Q Factor
            EF1G = 71,       ///< Effects Filter 1 Gain
            EF2FRQ = 72,     ///< Effects Filter 2 Frequency
            EF2Q = 73,       ///< Effects Filter 2 Q Factor
            EF2G = 74,       ///< Effects Filter 2 Gain
            EV2V = 75,       ///< Effects Volume 2
            ECDEL = 76,      ///< Effects Chorus Delay
            ECFRQ = 77,      ///< Effects Chorus LFO Frequency
            ECDPT = 78,      ///< Effects Chorus Depth
            ECFB = 79,       ///< Effects Chorus Feedback
            ECDF = 80,       ///< Effects Chorus Dampening Frequency
            ECDG = 81,       ///< Effects Chorus Dampening Gain
            ECWID = 82,      ///< Effects Chorus Stereo Width
            ECHPF = 83,      ///< Effects Chorus Highpass Frequency
            ECWET = 84,      ///< Effects Chorus Wet Volume
            ECDRY = 85,      ///< Effects Chorus Dry Volume
            EEDEL = 86,      ///< Effects Echo Delay
            EEINV = 87,      ///< Effects Echo Input Volume
            EEFB = 88,       ///< Effects Echo Feedback
            EEDST = 89,      ///< Effects Echo Distortion
            EEDF = 90,       ///< Effects Echo Dampening Frequency
            EEDG = 91,       ///< Effects Echo Dampening Gain
            EEWID = 92,      ///< Effects Echo Stereo Width
            EEHPF = 93,      ///< Effects Echo Highpass Frequency
            EECTH = 94,      ///< Effects Echo Side-Chain Compression Threshold
            EECAT = 95,      ///< Effects Echo Side-Chain Compression Attack Time
            EECRL = 96,      ///< Effects Echo Side-Chain Compression Release Time
            EECR = 97,       ///< Effects Echo Side-Chain Compression Ratio
            EEWET = 98,      ///< Effects Echo Wet Volume
            EEDRY = 99,      ///< Effects Echo Dry Volume
            ERRS = 100,      ///< Effects Reverb Room Size
            ERDST = 101,     ///< Effects Reverb Distortion
            ERDF = 102,      ///< Effects Reverb Dampening Frequency
            ERDG = 103,      ///< Effects Reverb Dampening Gain
            ERWID = 104,     ///< Effects Reverb Stereo Width
            ERHPF = 105,     ///< Effects Reverb Highpass Frequency
            ERCTH = 106,     ///< Effects Reverb Side-Chain Compression Threshold
            ERCAT = 107,     ///< Effects Reverb Side-Chain Compression Attack Time
            ERCRL = 108,     ///< Effects Reverb Side-Chain Compression Release Time
            ERCR = 109,      ///< Effects Reverb Side-Chain Compression Ratio
            ERWET = 110,     ///< Effects Reverb Wet Volume
            ERDRY = 111,     ///< Effects Reverb Dry Volume
            EV3V = 112,      ///< Effects Volume 3

            M1IN = 113,      ///< Macro 1 Input
            M1MIN = 114,     ///< Macro 1 Minimum Value
            M1MAX = 115,     ///< Macro 1 Maximum Value
            M1AMT = 116,     ///< Macro 1 Amount
            M1DST = 117,     ///< Macro 1 Distortion
            M1RND = 118,     ///< Macro 1 Randomness

            M2IN = 119,      ///< Macro 2 Input
            M2MIN = 120,     ///< Macro 2 Minimum Value
            M2MAX = 121,     ///< Macro 2 Maximum Value
            M2AMT = 122,     ///< Macro 2 Amount
            M2DST = 123,     ///< Macro 2 Distortion
            M2RND = 124,     ///< Macro 2 Randomness

            M3IN = 125,      ///< Macro 3 Input
            M3MIN = 126,     ///< Macro 3 Minimum Value
            M3MAX = 127,     ///< Macro 3 Maximum Value
            M3AMT = 128,     ///< Macro 3 Amount
            M3DST = 129,     ///< Macro 3 Distortion
            M3RND = 130,     ///< Macro 3 Randomness

            M4IN = 131,      ///< Macro 4 Input
            M4MIN = 132,     ///< Macro 4 Minimum Value
            M4MAX = 133,     ///< Macro 4 Maximum Value
            M4AMT = 134,     ///< Macro 4 Amount
            M4DST = 135,     ///< Macro 4 Distortion
            M4RND = 136,     ///< Macro 4 Randomness

            M5IN = 137,      ///< Macro 5 Input
            M5MIN = 138,     ///< Macro 5 Minimum Value
            M5MAX = 139,     ///< Macro 5 Maximum Value
            M5AMT = 140,     ///< Macro 5 Amount
            M5DST = 141,     ///< Macro 5 Distortion
            M5RND = 142,     ///< Macro 5 Randomness

            M6IN = 143,      ///< Macro 6 Input
            M6MIN = 144,     ///< Macro 6 Minimum Value
            M6MAX = 145,     ///< Macro 6 Maximum Value
            M6AMT = 146,     ///< Macro 6 Amount
            M6DST = 147,     ///< Macro 6 Distortion
            M6RND = 148,     ///< Macro 6 Randomness

            M7IN = 149,      ///< Macro 7 Input
            M7MIN = 150,     ///< Macro 7 Minimum Value
            M7MAX = 151,     ///< Macro 7 Maximum Value
            M7AMT = 152,     ///< Macro 7 Amount
            M7DST = 153,     ///< Macro 7 Distortion
            M7RND = 154,     ///< Macro 7 Randomness

            M8IN = 155,      ///< Macro 8 Input
            M8MIN = 156,     ///< Macro 8 Minimum Value
            M8MAX = 157,     ///< Macro 8 Maximum Value
            M8AMT = 158,     ///< Macro 8 Amount
            M8DST = 159,     ///< Macro 8 Distortion
            M8RND = 160,     ///< Macro 8 Randomness

            M9IN = 161,      ///< Macro 9 Input
            M9MIN = 162,     ///< Macro 9 Minimum Value
            M9MAX = 163,     ///< Macro 9 Maximum Value
            M9AMT = 164,     ///< Macro 9 Amount
            M9DST = 165,     ///< Macro 9 Distortion
            M9RND = 166,     ///< Macro 9 Randomness

            M10IN = 167,     ///< Macro 10 Input
            M10MIN = 168,    ///< Macro 10 Minimum Value
            M10MAX = 169,    ///< Macro 10 Maximum Value
            M10AMT = 170,    ///< Macro 10 Amount
            M10DST = 171,    ///< Macro 10 Distortion
            M10RND = 172,    ///< Macro 10 Randomness

            M11IN = 173,     ///< Macro 11 Input
            M11MIN = 174,    ///< Macro 11 Minimum Value
            M11MAX = 175,    ///< Macro 11 Maximum Value
            M11AMT = 176,    ///< Macro 11 Amount
            M11DST = 177,    ///< Macro 11 Distortion
            M11RND = 178,    ///< Macro 11 Randomness

            M12IN = 179,     ///< Macro 12 Input
            M12MIN = 180,    ///< Macro 12 Minimum Value
            M12MAX = 181,    ///< Macro 12 Maximum Value
            M12AMT = 182,    ///< Macro 12 Amount
            M12DST = 183,    ///< Macro 12 Distortion
            M12RND = 184,    ///< Macro 12 Randomness

            M13IN = 185,     ///< Macro 13 Input
            M13MIN = 186,    ///< Macro 13 Minimum Value
            M13MAX = 187,    ///< Macro 13 Maximum Value
            M13AMT = 188,    ///< Macro 13 Amount
            M13DST = 189,    ///< Macro 13 Distortion
            M13RND = 190,    ///< Macro 13 Randomness

            M14IN = 191,     ///< Macro 14 Input
            M14MIN = 192,    ///< Macro 14 Minimum Value
            M14MAX = 193,    ///< Macro 14 Maximum Value
            M14AMT = 194,    ///< Macro 14 Amount
            M14DST = 195,    ///< Macro 14 Distortion
            M14RND = 196,    ///< Macro 14 Randomness

            M15IN = 197,     ///< Macro 15 Input
            M15MIN = 198,    ///< Macro 15 Minimum Value
            M15MAX = 199,    ///< Macro 15 Maximum Value
            M15AMT = 200,    ///< Macro 15 Amount
            M15DST = 201,    ///< Macro 15 Distortion
            M15RND = 202,    ///< Macro 15 Randomness

            M16IN = 203,     ///< Macro 16 Input
            M16MIN = 204,    ///< Macro 16 Minimum Value
            M16MAX = 205,    ///< Macro 16 Maximum Value
            M16AMT = 206,    ///< Macro 16 Amount
            M16DST = 207,    ///< Macro 16 Distortion
            M16RND = 208,    ///< Macro 16 Randomness

            M17IN = 209,     ///< Macro 17 Input
            M17MIN = 210,    ///< Macro 17 Minimum Value
            M17MAX = 211,    ///< Macro 17 Maximum Value
            M17AMT = 212,    ///< Macro 17 Amount
            M17DST = 213,    ///< Macro 17 Distortion
            M17RND = 214,    ///< Macro 17 Randomness

            M18IN = 215,     ///< Macro 18 Input
            M18MIN = 216,    ///< Macro 18 Minimum Value
            M18MAX = 217,    ///< Macro 18 Maximum Value
            M18AMT = 218,    ///< Macro 18 Amount
            M18DST = 219,    ///< Macro 18 Distortion
            M18RND = 220,    ///< Macro 18 Randomness

            M19IN = 221,     ///< Macro 19 Input
            M19MIN = 222,    ///< Macro 19 Minimum Value
            M19MAX = 223,    ///< Macro 19 Maximum Value
            M19AMT = 224,    ///< Macro 19 Amount
            M19DST = 225,    ///< Macro 19 Distortion
            M19RND = 226,    ///< Macro 19 Randomness

            M20IN = 227,     ///< Macro 20 Input
            M20MIN = 228,    ///< Macro 20 Minimum Value
            M20MAX = 229,    ///< Macro 20 Maximum Value
            M20AMT = 230,    ///< Macro 20 Amount
            M20DST = 231,    ///< Macro 20 Distortion
            M20RND = 232,    ///< Macro 20 Randomness

            N1AMT = 233,     ///< Envelope 1 Amount
            N1INI = 234,     ///< Envelope 1 Initial Level
            N1DEL = 235,     ///< Envelope 1 Delay Time
            N1ATK = 236,     ///< Envelope 1 Attack Time
            N1PK = 237,      ///< Envelope 1 Peak Level
            N1HLD = 238,     ///< Envelope 1 Hold Time
            N1DEC = 239,     ///< Envelope 1 Decay Time
            N1SUS = 240,     ///< Envelope 1 Sustain Level
            N1REL = 241,     ///< Envelope 1 Release Time
            N1FIN = 242,     ///< Envelope 1 Final Level
            N1TIN = 243,     ///< Envelope 1 Time Inaccuracy
            N1VIN = 244,     ///< Envelope 1 Level Inaccuracy

            N2AMT = 245,     ///< Envelope 2 Amount
            N2INI = 246,     ///< Envelope 2 Initial Level
            N2DEL = 247,     ///< Envelope 2 Delay Time
            N2ATK = 248,     ///< Envelope 2 Attack Time
            N2PK = 249,      ///< Envelope 2 Peak Level
            N2HLD = 250,     ///< Envelope 2 Hold Time
            N2DEC = 251,     ///< Envelope 2 Decay Time
            N2SUS = 252,     ///< Envelope 2 Sustain Level
            N2REL = 253,     ///< Envelope 2 Release Time
            N2FIN = 254,     ///< Envelope 2 Final Level
            N2TIN = 255,     ///< Envelope 2 Time Inaccuracy
            N2VIN = 256,     ///< Envelope 2 Level Inaccuracy

            N3AMT = 257,     ///< Envelope 3 Amount
            N3INI = 258,     ///< Envelope 3 Initial Level
            N3DEL = 259,     ///< Envelope 3 Delay Time
            N3ATK = 260,     ///< Envelope 3 Attack Time
            N3PK = 261,      ///< Envelope 3 Peak Level
            N3HLD = 262,     ///< Envelope 3 Hold Time
            N3DEC = 263,     ///< Envelope 3 Decay Time
            N3SUS = 264,     ///< Envelope 3 Sustain Level
            N3REL = 265,     ///< Envelope 3 Release Time
            N3FIN = 266,     ///< Envelope 3 Final Level
            N3TIN = 267,     ///< Envelope 3 Time Inaccuracy
            N3VIN = 268,     ///< Envelope 3 Level Inaccuracy

            N4AMT = 269,     ///< Envelope 4 Amount
            N4INI = 270,     ///< Envelope 4 Initial Level
            N4DEL = 271,     ///< Envelope 4 Delay Time
            N4ATK = 272,     ///< Envelope 4 Attack Time
            N4PK = 273,      ///< Envelope 4 Peak Level
            N4HLD = 274,     ///< Envelope 4 Hold Time
            N4DEC = 275,     ///< Envelope 4 Decay Time
            N4SUS = 276,     ///< Envelope 4 Sustain Level
            N4REL = 277,     ///< Envelope 4 Release Time
            N4FIN = 278,     ///< Envelope 4 Final Level
            N4TIN = 279,     ///< Envelope 4 Time Inaccuracy
            N4VIN = 280,     ///< Envelope 4 Level Inaccuracy

            N5AMT = 281,     ///< Envelope 5 Amount
            N5INI = 282,     ///< Envelope 5 Initial Level
            N5DEL = 283,     ///< Envelope 5 Delay Time
            N5ATK = 284,     ///< Envelope 5 Attack Time
            N5PK = 285,      ///< Envelope 5 Peak Level
            N5HLD = 286,     ///< Envelope 5 Hold Time
            N5DEC = 287,     ///< Envelope 5 Decay Time
            N5SUS = 288,     ///< Envelope 5 Sustain Level
            N5REL = 289,     ///< Envelope 5 Release Time
            N5FIN = 290,     ///< Envelope 5 Final Level
            N5TIN = 291,     ///< Envelope 5 Time Inaccuracy
            N5VIN = 292,     ///< Envelope 5 Level Inaccuracy

            N6AMT = 293,     ///< Envelope 6 Amount
            N6INI = 294,     ///< Envelope 6 Initial Level
            N6DEL = 295,     ///< Envelope 6 Delay Time
            N6ATK = 296,     ///< Envelope 6 Attack Time
            N6PK = 297,      ///< Envelope 6 Peak Level
            N6HLD = 298,     ///< Envelope 6 Hold Time
            N6DEC = 299,     ///< Envelope 6 Decay Time
            N6SUS = 300,     ///< Envelope 6 Sustain Level
            N6REL = 301,     ///< Envelope 6 Release Time
            N6FIN = 302,     ///< Envelope 6 Final Level
            N6TIN = 303,     ///< Envelope 6 Time Inaccuracy
            N6VIN = 304,     ///< Envelope 6 Level Inaccuracy

            N7AMT = 305,     ///< Envelope 7 Amount
            N7INI = 306,     ///< Envelope 7 Initial Level
            N7DEL = 307,     ///< Envelope 7 Delay Time
            N7ATK = 308,     ///< Envelope 7 Attack Time
            N7PK = 309,      ///< Envelope 7 Peak Level
            N7HLD = 310,     ///< Envelope 7 Hold Time
            N7DEC = 311,     ///< Envelope 7 Decay Time
            N7SUS = 312,     ///< Envelope 7 Sustain Level
            N7REL = 313,     ///< Envelope 7 Release Time
            N7FIN = 314,     ///< Envelope 7 Final Level
            N7TIN = 315,     ///< Envelope 7 Time Inaccuracy
            N7VIN = 316,     ///< Envelope 7 Level Inaccuracy

            N8AMT = 317,     ///< Envelope 8 Amount
            N8INI = 318,     ///< Envelope 8 Initial Level
            N8DEL = 319,     ///< Envelope 8 Delay Time
            N8ATK = 320,     ///< Envelope 8 Attack Time
            N8PK = 321,      ///< Envelope 8 Peak Level
            N8HLD = 322,     ///< Envelope 8 Hold Time
            N8DEC = 323,     ///< Envelope 8 Decay Time
            N8SUS = 324,     ///< Envelope 8 Sustain Level
            N8REL = 325,     ///< Envelope 8 Release Time
            N8FIN = 326,     ///< Envelope 8 Final Level
            N8TIN = 327,     ///< Envelope 8 Time Inaccuracy
            N8VIN = 328,     ///< Envelope 8 Level Inaccuracy

            N9AMT = 329,     ///< Envelope 9 Amount
            N9INI = 330,     ///< Envelope 9 Initial Level
            N9DEL = 331,     ///< Envelope 9 Delay Time
            N9ATK = 332,     ///< Envelope 9 Attack Time
            N9PK = 333,      ///< Envelope 9 Peak Level
            N9HLD = 334,     ///< Envelope 9 Hold Time
            N9DEC = 335,     ///< Envelope 9 Decay Time
            N9SUS = 336,     ///< Envelope 9 Sustain Level
            N9REL = 337,     ///< Envelope 9 Release Time
            N9FIN = 338,     ///< Envelope 9 Final Level
            N9TIN = 339,     ///< Envelope 9 Time Inaccuracy
            N9VIN = 340,     ///< Envelope 9 Level Inaccuracy

            N10AMT = 341,     ///< Envelope 10 Amount
            N10INI = 342,     ///< Envelope 10 Initial Level
            N10DEL = 343,     ///< Envelope 10 Delay Time
            N10ATK = 344,     ///< Envelope 10 Attack Time
            N10PK = 345,      ///< Envelope 10 Peak Level
            N10HLD = 346,     ///< Envelope 10 Hold Time
            N10DEC = 347,     ///< Envelope 10 Decay Time
            N10SUS = 348,     ///< Envelope 10 Sustain Level
            N10REL = 349,     ///< Envelope 10 Release Time
            N10FIN = 350,     ///< Envelope 10 Final Level
            N10TIN = 351,     ///< Envelope 10 Time Inaccuracy
            N10VIN = 352,     ///< Envelope 10 Level Inaccuracy

            N11AMT = 353,     ///< Envelope 11 Amount
            N11INI = 354,     ///< Envelope 11 Initial Level
            N11DEL = 355,     ///< Envelope 11 Delay Time
            N11ATK = 356,     ///< Envelope 11 Attack Time
            N11PK = 357,      ///< Envelope 11 Peak Level
            N11HLD = 358,     ///< Envelope 11 Hold Time
            N11DEC = 359,     ///< Envelope 11 Decay Time
            N11SUS = 360,     ///< Envelope 11 Sustain Level
            N11REL = 361,     ///< Envelope 11 Release Time
            N11FIN = 362,     ///< Envelope 11 Final Level
            N11TIN = 363,     ///< Envelope 11 Time Inaccuracy
            N11VIN = 364,     ///< Envelope 11 Level Inaccuracy

            N12AMT = 365,     ///< Envelope 12 Amount
            N12INI = 366,     ///< Envelope 12 Initial Level
            N12DEL = 367,     ///< Envelope 12 Delay Time
            N12ATK = 368,     ///< Envelope 12 Attack Time
            N12PK = 369,      ///< Envelope 12 Peak Level
            N12HLD = 370,     ///< Envelope 12 Hold Time
            N12DEC = 371,     ///< Envelope 12 Decay Time
            N12SUS = 372,     ///< Envelope 12 Sustain Level
            N12REL = 373,     ///< Envelope 12 Release Time
            N12FIN = 374,     ///< Envelope 12 Final Level
            N12TIN = 375,     ///< Envelope 12 Time Inaccuracy
            N12VIN = 376,     ///< Envelope 12 Level Inaccuracy

            L1FRQ = 377,     ///< LFO 1 Frequency
            L1PHS = 378,     ///< LFO 1 Phase
            L1MIN = 379,     ///< LFO 1 Minimum Value
            L1MAX = 380,     ///< LFO 1 Maximum Value
            L1AMT = 381,     ///< LFO 1 Amount
            L1DST = 382,     ///< LFO 1 Distortion
            L1RND = 383,     ///< LFO 1 Randomness

            L2FRQ = 384,     ///< LFO 2 Frequency
            L2PHS = 385,     ///< LFO 2 Phase
            L2MIN = 386,     ///< LFO 2 Minimum Value
            L2MAX = 387,     ///< LFO 2 Maximum Value
            L2AMT = 388,     ///< LFO 2 Amount
            L2DST = 389,     ///< LFO 2 Distortion
            L2RND = 390,     ///< LFO 2 Randomness

            L3FRQ = 391,     ///< LFO 3 Frequency
            L3PHS = 392,     ///< LFO 3 Phase
            L3MIN = 393,     ///< LFO 3 Minimum Value
            L3MAX = 394,     ///< LFO 3 Maximum Value
            L3AMT = 395,     ///< LFO 3 Amount
            L3DST = 396,     ///< LFO 3 Distortion
            L3RND = 397,     ///< LFO 3 Randomness

            L4FRQ = 398,     ///< LFO 4 Frequency
            L4PHS = 399,     ///< LFO 4 Phase
            L4MIN = 400,     ///< LFO 4 Minimum Value
            L4MAX = 401,     ///< LFO 4 Maximum Value
            L4AMT = 402,     ///< LFO 4 Amount
            L4DST = 403,     ///< LFO 4 Distortion
            L4RND = 404,     ///< LFO 4 Randomness

            L5FRQ = 405,     ///< LFO 5 Frequency
            L5PHS = 406,     ///< LFO 5 Phase
            L5MIN = 407,     ///< LFO 5 Minimum Value
            L5MAX = 408,     ///< LFO 5 Maximum Value
            L5AMT = 409,     ///< LFO 5 Amount
            L5DST = 410,     ///< LFO 5 Distortion
            L5RND = 411,     ///< LFO 5 Randomness

            L6FRQ = 412,     ///< LFO 6 Frequency
            L6PHS = 413,     ///< LFO 6 Phase
            L6MIN = 414,     ///< LFO 6 Minimum Value
            L6MAX = 415,     ///< LFO 6 Maximum Value
            L6AMT = 416,     ///< LFO 6 Amount
            L6DST = 417,     ///< LFO 6 Distortion
            L6RND = 418,     ///< LFO 6 Randomness

            L7FRQ = 419,     ///< LFO 7 Frequency
            L7PHS = 420,     ///< LFO 7 Phase
            L7MIN = 421,     ///< LFO 7 Minimum Value
            L7MAX = 422,     ///< LFO 7 Maximum Value
            L7AMT = 423,     ///< LFO 7 Amount
            L7DST = 424,     ///< LFO 7 Distortion
            L7RND = 425,     ///< LFO 7 Randomness

            L8FRQ = 426,     ///< LFO 8 Frequency
            L8PHS = 427,     ///< LFO 8 Phase
            L8MIN = 428,     ///< LFO 8 Minimum Value
            L8MAX = 429,     ///< LFO 8 Maximum Value
            L8AMT = 430,     ///< LFO 8 Amount
            L8DST = 431,     ///< LFO 8 Distortion
            L8RND = 432,     ///< LFO 8 Randomness

            MODE = 433,      ///< Mode
            MWAV = 434,      ///< Modulator Waveform
            CWAV = 435,      ///< Carrier Waveform
            MF1TYP = 436,    ///< Modulator Filter 1 Type
            MF2TYP = 437,    ///< Modulator Filter 2 Type
            CF1TYP = 438,    ///< Carrier Filter 1 Type
            CF2TYP = 439,    ///< Carrier Filter 2 Type
            EF1TYP = 440,    ///< Effects Filter 1 Type
            EF2TYP = 441,    ///< Effects Filter 2 Type
            L1WAV = 442,     ///< LFO 1 Waveform
            L2WAV = 443,     ///< LFO 2 Waveform
            L3WAV = 444,     ///< LFO 3 Waveform
            L4WAV = 445,     ///< LFO 4 Waveform
            L5WAV = 446,     ///< LFO 5 Waveform
            L6WAV = 447,     ///< LFO 6 Waveform
            L7WAV = 448,     ///< LFO 7 Waveform
            L8WAV = 449,     ///< LFO 8 Waveform
            L1LOG = 450,     ///< LFO 1 Logarithmic Frequency
            L2LOG = 451,     ///< LFO 2 Logarithmic Frequency
            L3LOG = 452,     ///< LFO 3 Logarithmic Frequency
            L4LOG = 453,     ///< LFO 4 Logarithmic Frequency
            L5LOG = 454,     ///< LFO 5 Logarithmic Frequency
            L6LOG = 455,     ///< LFO 6 Logarithmic Frequency
            L7LOG = 456,     ///< LFO 7 Logarithmic Frequency
            L8LOG = 457,     ///< LFO 8 Logarithmic Frequency
            L1CEN = 458,     ///< LFO 1 Center
            L2CEN = 459,     ///< LFO 2 Center
            L3CEN = 460,     ///< LFO 3 Center
            L4CEN = 461,     ///< LFO 4 Center
            L5CEN = 462,     ///< LFO 5 Center
            L6CEN = 463,     ///< LFO 6 Center
            L7CEN = 464,     ///< LFO 7 Center
            L8CEN = 465,     ///< LFO 8 Center
            L1SYN = 466,     ///< LFO 1 Tempo Synchronization
            L2SYN = 467,     ///< LFO 2 Tempo Synchronization
            L3SYN = 468,     ///< LFO 3 Tempo Synchronization
            L4SYN = 469,     ///< LFO 4 Tempo Synchronization
            L5SYN = 470,     ///< LFO 5 Tempo Synchronization
            L6SYN = 471,     ///< LFO 6 Tempo Synchronization
            L7SYN = 472,     ///< LFO 7 Tempo Synchronization
            L8SYN = 473,     ///< LFO 8 Tempo Synchronization
            ECSYN = 474,     ///< Effects Chorus Tempo Synchronization
            EESYN = 475,     ///< Effects Echo Tempo Synchronization
            MF1LOG = 476,    ///< Modulator Filter 1 Logarithmic Frequency
            MF2LOG = 477,    ///< Modulator Filter 2 Logarithmic Frequency
            CF1LOG = 478,    ///< Carrier Filter 1 Logarithmic Frequency
            CF2LOG = 479,    ///< Carrier Filter 2 Logarithmic Frequency
            EF1LOG = 480,    ///< Effects Filter 1 Logarithmic Frequency
            EF2LOG = 481,    ///< Effects Filter 2 Logarithmic Frequency
            ECLOG = 482,     ///< Effects Chorus Logarithmic Filter Frequencies
            ECLLG = 483,     ///< Effects Chorus Logarithmic LFO Frequency
            EELOG = 484,     ///< Effects Echo Logarithmic Filter Frequencies
            ERLOG = 485,     ///< Effects Reverb Logarithmic Filter Frequencies
            N1DYN = 486,     ///< Envelope 1 Dynamic
            N2DYN = 487,     ///< Envelope 2 Dynamic
            N3DYN = 488,     ///< Envelope 3 Dynamic
            N4DYN = 489,     ///< Envelope 4 Dynamic
            N5DYN = 490,     ///< Envelope 5 Dynamic
            N6DYN = 491,     ///< Envelope 6 Dynamic
            N7DYN = 492,     ///< Envelope 7 Dynamic
            N8DYN = 493,     ///< Envelope 8 Dynamic
            N9DYN = 494,     ///< Envelope 9 Dynamic
            N10DYN = 495,     ///< Envelope 10 Dynamic
            N11DYN = 496,     ///< Envelope 11 Dynamic
            N12DYN = 497,     ///< Envelope 12 Dynamic
            POLY = 498,      ///< Polyphonic
            ERTYP = 499,     ///< Effects Reverb Type
            ECTYP = 500,     ///< Effects Chorus Type
            MTUN = 501,      ///< Modulator Tuning
            CTUN = 502,      ///< Carrier Tuning
            MOIA = 503,      ///< Modulator Oscillator Inaccuracy
            MOIS = 504,      ///< Modulator Oscillator Instability
            COIA = 505,      ///< Carrier Oscillator Inaccuracy
            COIS = 506,      ///< Carrier Oscillator Instability
            MF1QLG = 507,    ///< Modulator Filter 1 Logarithmic Q Factor
            MF2QLG = 508,    ///< Modulator Filter 2 Logarithmic Q Factor
            CF1QLG = 509,    ///< Carrier Filter 1 Logarithmic Q Factor
            CF2QLG = 510,    ///< Carrier Filter 2 Logarithmic Q Factor
            EF1QLG = 511,    ///< Effects Filter 1 Logarithmic Q Factor
            EF2QLG = 512,    ///< Effects Filter 2 Logarithmic Q Factor
            L1AEN = 513,     ///< LFO 1 Amount Envelope
            L2AEN = 514,     ///< LFO 2 Amount Envelope
            L3AEN = 515,     ///< LFO 3 Amount Envelope
            L4AEN = 516,     ///< LFO 4 Amount Envelope
            L5AEN = 517,     ///< LFO 5 Amount Envelope
            L6AEN = 518,     ///< LFO 6 Amount Envelope
            L7AEN = 519,     ///< LFO 7 Amount Envelope
            L8AEN = 520,     ///< LFO 8 Amount Envelope
            N1SYN = 521,     ///< Envelope 1 Tempo Synchronization
            N2SYN = 522,     ///< Envelope 2 Tempo Synchronization
            N3SYN = 523,     ///< Envelope 3 Tempo Synchronization
            N4SYN = 524,     ///< Envelope 4 Tempo Synchronization
            N5SYN = 525,     ///< Envelope 5 Tempo Synchronization
            N6SYN = 526,     ///< Envelope 6 Tempo Synchronization
            N7SYN = 527,     ///< Envelope 7 Tempo Synchronization
            N8SYN = 528,     ///< Envelope 8 Tempo Synchronization
            N9SYN = 529,     ///< Envelope 9 Tempo Synchronization
            N10SYN = 530,     ///< Envelope 10 Tempo Synchronization
            N11SYN = 531,     ///< Envelope 11 Tempo Synchronization
            N12SYN = 532,     ///< Envelope 12 Tempo Synchronization
            N1ASH = 533,     ///< Envelope 1 Attack Shape
            N2ASH = 534,     ///< Envelope 2 Attack Shape
            N3ASH = 535,     ///< Envelope 3 Attack Shape
            N4ASH = 536,     ///< Envelope 4 Attack Shape
            N5ASH = 537,     ///< Envelope 5 Attack Shape
            N6ASH = 538,     ///< Envelope 6 Attack Shape
            N7ASH = 539,     ///< Envelope 7 Attack Shape
            N8ASH = 540,     ///< Envelope 8 Attack Shape
            N9ASH = 541,     ///< Envelope 9 Attack Shape
            N10ASH = 542,     ///< Envelope 10 Attack Shape
            N11ASH = 543,     ///< Envelope 11 Attack Shape
            N12ASH = 544,     ///< Envelope 12 Attack Shape
            N1DSH = 545,     ///< Envelope 1 Decay Shape
            N2DSH = 546,     ///< Envelope 2 Decay Shape
            N3DSH = 547,     ///< Envelope 3 Decay Shape
            N4DSH = 548,     ///< Envelope 4 Decay Shape
            N5DSH = 549,     ///< Envelope 5 Decay Shape
            N6DSH = 550,     ///< Envelope 6 Decay Shape
            N7DSH = 551,     ///< Envelope 7 Decay Shape
            N8DSH = 552,     ///< Envelope 8 Decay Shape
            N9DSH = 553,     ///< Envelope 9 Decay Shape
            N10DSH = 554,     ///< Envelope 10 Decay Shape
            N11DSH = 555,     ///< Envelope 11 Decay Shape
            N12DSH = 556,     ///< Envelope 12 Decay Shape
            N1RSH = 557,     ///< Envelope 1 Release Shape
            N2RSH = 558,     ///< Envelope 2 Release Shape
            N3RSH = 559,     ///< Envelope 3 Release Shape
            N4RSH = 560,     ///< Envelope 4 Release Shape
            N5RSH = 561,     ///< Envelope 5 Release Shape
            N6RSH = 562,     ///< Envelope 6 Release Shape
            N7RSH = 563,     ///< Envelope 7 Release Shape
            N8RSH = 564,     ///< Envelope 8 Release Shape
            N9RSH = 565,     ///< Envelope 9 Release Shape
            N10RSH = 566,     ///< Envelope 10 Release Shape
            N11RSH = 567,     ///< Envelope 11 Release Shape
            N12RSH = 568,     ///< Envelope 12 Release Shape

            PARAM_ID_COUNT = 569,
            INVALID_PARAM_ID = PARAM_ID_COUNT,
        };

        static constexpr Integer FLOAT_PARAMS = ParamId::MODE;

        enum ControllerId {
            NONE =                      Midi::NONE,                 ///< None
            MODULATION_WHEEL =          Midi::MODULATION_WHEEL,     ///< Modulation Wheel (CC 1)
            BREATH =                    Midi::BREATH,               ///< Breath (CC 2)
            UNDEFINED_1 =               Midi::UNDEFINED_1,          ///< Undefined (CC 3)
            FOOT_PEDAL =                Midi::FOOT_PEDAL,           ///< Foot Pedal (CC 4)
            PORTAMENTO_TIME =           Midi::PORTAMENTO_TIME,      ///< Portamento Time (CC 5)
            DATA_ENTRY =                Midi::DATA_ENTRY,           ///< Data Entry (CC 6)
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
            SUSTAIN_PEDAL =             Midi::SUSTAIN_PEDAL,        ///< Sustain Pedal (CC 64)
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

            MACRO_1 =                   131,                        ///< Macro 1
            MACRO_2 =                   132,                        ///< Macro 2
            MACRO_3 =                   133,                        ///< Macro 3
            MACRO_4 =                   134,                        ///< Macro 4
            MACRO_5 =                   135,                        ///< Macro 5
            MACRO_6 =                   136,                        ///< Macro 6
            MACRO_7 =                   137,                        ///< Macro 7
            MACRO_8 =                   138,                        ///< Macro 8
            MACRO_9 =                   139,                        ///< Macro 9
            MACRO_10 =                  140,                        ///< Macro 10

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

            CHANNEL_PRESSURE =          155,                        ///< Channel Pressure

            MIDI_LEARN =                156,                        ///< MIDI Learn

            MACRO_11 =                  157,                        ///< Macro 11
            MACRO_12 =                  158,                        ///< Macro 12
            MACRO_13 =                  159,                        ///< Macro 13
            MACRO_14 =                  160,                        ///< Macro 14
            MACRO_15 =                  161,                        ///< Macro 15
            MACRO_16 =                  162,                        ///< Macro 16
            MACRO_17 =                  163,                        ///< Macro 17
            MACRO_18 =                  164,                        ///< Macro 18
            MACRO_19 =                  165,                        ///< Macro 19
            MACRO_20 =                  166,                        ///< Macro 20

            OSC_1_PEAK =                167,                        ///< Oscillator 1 Peak
            OSC_2_PEAK =                168,                        ///< Oscillator 1 Peak
            VOL_1_PEAK =                169,                        ///< Volume 1 Peak
            VOL_2_PEAK =                170,                        ///< Volume 2 Peak
            VOL_3_PEAK =                171,                        ///< Volume 3 Peak

            ENVELOPE_7 =                172,                        ///< Envelope 7
            ENVELOPE_8 =                173,                        ///< Envelope 8
            ENVELOPE_9 =                174,                        ///< Envelope 9
            ENVELOPE_10 =               175,                        ///< Envelope 10
            ENVELOPE_11 =               176,                        ///< Envelope 11
            ENVELOPE_12 =               177,                        ///< Envelope 12

            CONTROLLER_ID_COUNT =       178,
            INVALID_CONTROLLER_ID =     CONTROLLER_ID_COUNT,
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

        class Message
        {
            public:
                Message() noexcept;
                Message(Message const& message) noexcept = default;
                Message(Message&& message) noexcept = default;

                Message(
                    MessageType const type,
                    ParamId const param_id,
                    Number const number_param,
                    Byte const byte_param
                ) noexcept;

                Message& operator=(Message const& message) noexcept = default;
                Message& operator=(Message&& message) noexcept = default;

                MessageType type;
                ParamId param_id;
                Number number_param;
                Byte byte_param;
        };

        class ModeParam : public Param<Mode, ParamEvaluation::BLOCK>
        {
            public:
                explicit ModeParam(std::string const& name) noexcept;
        };

        class NoteTuning
        {
            public:
                NoteTuning() noexcept
                    : frequency(0.0),
                    channel(Midi::INVALID_CHANNEL),
                    note(Midi::INVALID_NOTE)
                {
                }

                NoteTuning(NoteTuning const& note_tuning) noexcept = default;
                NoteTuning(NoteTuning&& note_tuning) noexcept = default;

                NoteTuning(
                    Midi::Channel const channel,
                    Midi::Note const note,
                    Frequency const frequency = 0.0
                ) noexcept
                    : frequency(frequency),
                    channel(channel),
                    note(note)
                {
                }

                NoteTuning& operator=(NoteTuning const& note_tuning) noexcept = default;
                NoteTuning& operator=(NoteTuning&& note_tuning) noexcept = default;

                bool is_valid() const noexcept
                {
                    return channel <= Midi::CHANNEL_MAX && note <= Midi::NOTE_MAX;
                }

                Frequency frequency;
                Midi::Channel channel;
                Midi::Note note;
        };

        typedef NoteTuning NoteTunings[POLYPHONY];

        static bool is_supported_midi_controller(
            Midi::Controller const controller
        ) noexcept;

        static bool is_controller_polyphonic(
            ControllerId const controller_id
        ) noexcept;

        static Number calculate_inaccuracy_seed(Integer const voice) noexcept;

        explicit Synth(Integer const samples_between_gc = 8000) noexcept;
        virtual ~Synth() override;

        virtual void set_sample_rate(Frequency const new_sample_rate) noexcept override;
        virtual void set_block_size(Integer const new_block_size) noexcept override;
        virtual void reset() noexcept override;

        bool is_lock_free() const noexcept;

        bool is_dirty() const noexcept;
        void clear_dirty_flag() noexcept;

        void suspend() noexcept;
        void resume() noexcept;

        bool has_mts_esp_tuning() const noexcept;
        bool has_continuous_mts_esp_tuning() const noexcept;
        bool is_mts_esp_connected() const noexcept;
        void mts_esp_connected() noexcept;
        void mts_esp_disconnected() noexcept;
        NoteTunings& collect_active_notes(Integer& active_notes_count) noexcept;
        void update_note_tuning(NoteTuning const& note_tuning) noexcept;
        void update_note_tunings(NoteTunings const& note_tunings, Integer const count) noexcept;

        Sample const* const* generate_samples(
            Integer const round, Integer const sample_count
        ) noexcept;

        /**
         * \brief Thread-safe way to change the state of the synthesizer outside
         *        the audio thread.
         */
        void push_message(
            MessageType const type,
            ParamId const param_id,
            Number const number_param,
            Byte const byte_param
        ) noexcept;

        /**
         * \brief Thread-safe way to change the state of the synthesizer outside
         *        the audio thread.
         */
        void push_message(Message const& message) noexcept;

        void process_messages() noexcept;

        /**
         * \brief Process a state changing message inside the audio thread.
         */
        void process_message(
            MessageType const type,
            ParamId const param_id,
            Number const number_param,
            Byte const byte_param
        ) noexcept;

        /**
         * \brief Process a state changing message inside the audio thread.
         */
        void process_message(Message const& message) noexcept;

        std::string const& get_param_name(ParamId const param_id) const noexcept;
        ParamId get_param_id(std::string const& name) const noexcept;

#ifdef JS80P_ASSERTIONS
        void get_param_id_hash_table_statistics(
            Integer& max_collisions,
            Number& avg_collisions,
            Number& avg_bucket_size
        ) const noexcept;
#endif

        Number float_param_ratio_to_display_value(
            ParamId const param_id,
            Number const ratio
        ) const noexcept;

        Byte int_param_ratio_to_display_value(
            ParamId const param_id,
            Number const ratio
        ) const noexcept;

        bool is_toggle_param(ParamId const param_id) const noexcept;

        Number get_param_max_value(ParamId const param_id) const noexcept;
        Number get_param_ratio_atomic(ParamId const param_id) const noexcept;
        Number get_param_default_ratio(ParamId const param_id) const noexcept;

        ControllerId get_param_controller_id_atomic(
            ParamId const param_id
        ) const noexcept;

        void note_off(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Midi::Byte const velocity
        ) noexcept;

        void note_on(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Midi::Byte const velocity
        ) noexcept;

        void aftertouch(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Midi::Byte const pressure
        ) noexcept;

        void control_change(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Controller const controller,
            Midi::Byte const new_value
        ) noexcept;

        void channel_pressure(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Byte const pressure
        ) noexcept;

        void pitch_wheel_change(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Word const new_value
        ) noexcept;

        void all_sound_off(
            Seconds const time_offset,
            Midi::Channel const channel
        ) noexcept;

        void reset_all_controllers(
            Seconds const time_offset,
            Midi::Channel const channel
        ) noexcept;

        void all_notes_off(
            Seconds const time_offset,
            Midi::Channel const channel
        ) noexcept;

        void mono_mode_on(
            Seconds const time_offset,
            Midi::Channel const channel
        ) noexcept;

        void mono_mode_off(
            Seconds const time_offset,
            Midi::Channel const channel
        ) noexcept;

        ToggleParam polyphonic;
        ModeParam mode;
        FloatParamS modulator_add_volume;
        FloatParamS phase_modulation_level;
        FloatParamS frequency_modulation_level;
        FloatParamS amplitude_modulation_level;

        Modulator::Params modulator_params;
        Carrier::Params carrier_params;

        MidiController pitch_wheel;
        MidiController note;
        MidiController velocity;
        MidiController channel_pressure_ctl;
        MidiController osc_1_peak;
        MidiController osc_2_peak;
        MidiController vol_1_peak;
        MidiController vol_2_peak;
        MidiController vol_3_peak;

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

        void finalize_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        FrequencyTable frequencies;
        PerChannelFrequencyTable per_channel_frequencies;

    private:
        class Bus : public SignalProducer
        {
            friend class SignalProducer;

            public:
                Bus(
                    Integer const channels,
                    Modulator* const* const modulators,
                    Modulator::Params const& modulator_params,
                    Carrier* const* const carriers,
                    Carrier::Params const& carrier_params,
                    Integer const polyphony,
                    FloatParamS& modulator_add_volume
                ) noexcept;

                virtual ~Bus();

                virtual void set_block_size(
                    Integer const new_block_size
                ) noexcept override;

                void find_modulators_peak(
                    Integer const sample_count,
                    Sample& peak,
                    Integer& peak_index
                ) noexcept;

                void find_carriers_peak(
                    Integer const sample_count,
                    Sample& peak,
                    Integer& peak_index
                ) noexcept;

                void collect_active_notes(
                    NoteTunings& note_tunings,
                    Integer& note_tunings_count
                ) noexcept;

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

            private:
                void allocate_buffers() noexcept;
                void free_buffers() noexcept;
                void reallocate_buffers() noexcept;

                void collect_active_voices() noexcept;

                template<class VoiceClass, bool should_sync_oscillator_inaccuracy, bool should_sync_oscillator_instability>
                void render_voices(
                    VoiceClass* (&voices)[POLYPHONY],
                    size_t const voices_count,
                    typename VoiceClass::Params const& params,
                    Integer const round,
                    Integer const sample_count
                ) noexcept;

                void mix_modulators(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index
                ) noexcept;

                template<bool is_additive_volume_constant>
                void mix_modulators(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample const add_volume_value,
                    Sample const* add_volume_buffer
                ) noexcept;

                void mix_carriers(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index
                ) noexcept;

                Integer const polyphony;
                Modulator* const* const modulators;
                Carrier* const* const carriers;
                Modulator::Params const& modulator_params;
                Carrier::Params const& carrier_params;
                Modulator* active_modulators[POLYPHONY];
                Carrier* active_carriers[POLYPHONY];
                size_t active_modulators_count;
                size_t active_carriers_count;
                FloatParamS& modulator_add_volume;
                Sample const* modulator_add_volume_buffer;
                Sample** modulators_buffer;
                Sample** carriers_buffer;
        };

        class ParamIdHashTable
        {
            public:
                ParamIdHashTable() noexcept;
                ~ParamIdHashTable();

                void add(std::string const& name, ParamId const param_id) noexcept;
                ParamId lookup(std::string const& name) noexcept;

#ifdef JS80P_ASSERTIONS
                void get_statistics(
                    Integer& max_collisions,
                    Number& avg_collisions,
                    Number& avg_bucket_size
                ) const noexcept;
#endif

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

                static constexpr Integer ENTRIES = 0x100;
                static constexpr Integer MASK = ENTRIES - 1;
                static constexpr Integer MULTIPLIER = 3189;
                static constexpr Integer SHIFT = 11;

                static Integer hash(std::string const& name) noexcept;

                void lookup(
                    std::string const& name,
                    Entry** root,
                    Entry** parent,
                    Entry** entry
                ) noexcept;

                Entry entries[ENTRIES];
        };

        class MidiControllerMessage
        {
            public:
                MidiControllerMessage();
                MidiControllerMessage(MidiControllerMessage const& message) = default;
                MidiControllerMessage(MidiControllerMessage&& message) = default;

                MidiControllerMessage(Seconds const time_offset, Midi::Word const value);

                bool operator==(MidiControllerMessage const& message) const noexcept;
                MidiControllerMessage& operator=(MidiControllerMessage const& message) noexcept = default;
                MidiControllerMessage& operator=(MidiControllerMessage&& message) noexcept = default;

            private:
                Seconds time_offset;
                Midi::Word value;
        };

        class DeferredNoteOff
        {
            public:
                DeferredNoteOff();
                DeferredNoteOff(DeferredNoteOff const& deferred_note_off) = default;
                DeferredNoteOff(DeferredNoteOff&& deferred_note_off) = default;

                DeferredNoteOff(
                    Integer const note_id,
                    Midi::Channel const channel,
                    Midi::Note const note,
                    Midi::Byte const velocity,
                    Integer const voice
                );

                DeferredNoteOff& operator=(DeferredNoteOff const& deferred_note_off) noexcept = default;
                DeferredNoteOff& operator=(DeferredNoteOff&& deferred_note_off) noexcept = default;

                Integer get_note_id() const noexcept;
                Midi::Channel get_channel() const noexcept;
                Midi::Note get_note() const noexcept;
                Midi::Byte get_velocity() const noexcept;
                Integer get_voice() const noexcept;

            private:
                Integer voice;
                Integer note_id;
                Midi::Channel channel;
                Midi::Note note;
                Midi::Byte velocity;
        };

        static constexpr SPSCQueue<Message>::SizeType MESSAGE_QUEUE_SIZE = 8192;

        static constexpr Number MIDI_WORD_SCALE = 1.0 / 16384.0;
        static constexpr Number MIDI_BYTE_SCALE = 1.0 / 127.0;

        static constexpr Integer INVALID_VOICE = -1;

        static constexpr Integer NOTE_ID_MASK = 0x7fffffff;

        static constexpr Integer BIQUAD_FILTER_SHARED_BUFFERS = 6;

        static std::vector<bool> supported_midi_controllers;
        static bool supported_midi_controllers_initialized;

        static ParamIdHashTable param_id_hash_table;
        static std::string param_names_by_id[ParamId::PARAM_ID_COUNT];

        static bool should_sync_oscillator_inaccuracy(
            Modulator::Params const& modulator_params,
            Carrier::Params const& carrier_params
        ) noexcept;

        static bool should_sync_oscillator_instability(
            Modulator::Params const& modulator_params,
            Carrier::Params const& carrier_params
        ) noexcept;

        void initialize_supported_midi_controllers() noexcept;

        void build_frequency_table() noexcept;
        void register_main_params() noexcept;
        void register_modulator_params() noexcept;
        void register_carrier_params() noexcept;
        void register_effects_params() noexcept;
        void create_voices() noexcept;
        void create_midi_controllers() noexcept;
        void create_macros() noexcept;
        void create_envelopes() noexcept;
        void create_lfos() noexcept;

        void allocate_buffers() noexcept;
        void free_buffers() noexcept;
        void reallocate_buffers() noexcept;

        template<class ParamClass>
        void register_param_as_child(
            ParamId const param_id,
            ParamClass& param
        ) noexcept;

        template<class ParamClass>
        void register_param(ParamId const param_id, ParamClass const& param) noexcept;

        Number midi_byte_to_float(Midi::Byte const midi_byte) const noexcept;
        Number midi_word_to_float(Midi::Word const midi_word) const noexcept;

        void sustain_on(Seconds const time_offset) noexcept;
        void sustain_off(Seconds const time_offset) noexcept;

        bool is_repeated_midi_controller_message(
            ControllerId const controller_id,
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Word const value
        ) noexcept;

        void stop_lfos() noexcept;
        void start_lfos() noexcept;

        void handle_set_param(
            ParamId const param_id,
            Number const ratio
        ) noexcept;

        void handle_assign_controller(
            ParamId const param_id,
            Byte const controller_id
        ) noexcept;

        void handle_refresh_param(ParamId const param_id) noexcept;

        void handle_clear() noexcept;

        bool assign_controller_to_discrete_param(
            ParamId const param_id,
            ControllerId const controller_id
        ) noexcept;

        template<class FloatParamClass>
        bool assign_controller(
            FloatParamClass& param,
            ControllerId const controller_id
        ) noexcept;

        Number get_param_ratio(ParamId const param_id) const noexcept;

        void clear_midi_controllers() noexcept;

        void clear_midi_note_to_voice_assignments() noexcept;

        void clear_sustain() noexcept;

        bool should_sync_oscillator_inaccuracy() const noexcept;
        bool should_sync_oscillator_instability() const noexcept;

        void note_on_polyphonic(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity
        ) noexcept;

        void note_on_monophonic(
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity,
            bool const trigger_if_off
        ) noexcept;

        void trigger_note_on_voice(
            Integer const voice,
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity
        ) noexcept;

        template<class VoiceClass>
        void trigger_note_on_voice_monophonic(
            VoiceClass& voice,
            bool const is_off,
            Seconds const time_offset,
            Midi::Channel const channel,
            Midi::Note const note,
            Number const velocity,
            bool const should_sync_oscillator_inaccuracy
        ) noexcept;

        void assign_voice_and_note_id(
            Integer const voice,
            Midi::Channel const channel,
            Midi::Note const note
        ) noexcept;

        void stop_polyphonic_notes() noexcept;

        void update_param_states() noexcept;

        void garbage_collect_voices() noexcept;

        std::string const to_string(Integer const) const noexcept;

        std::vector<DeferredNoteOff> deferred_note_offs;
        SPSCQueue<Message> messages;
        Bus bus;
        NoteStack note_stack;
        PeakTracker osc_1_peak_tracker;
        PeakTracker osc_2_peak_tracker;
        PeakTracker vol_1_peak_tracker;
        PeakTracker vol_2_peak_tracker;
        PeakTracker vol_3_peak_tracker;

        Sample const* const* raw_output;
        MidiControllerMessage previous_controller_message[ControllerId::CONTROLLER_ID_COUNT];
        BiquadFilterSharedBuffers biquad_filter_shared_buffers[BIQUAD_FILTER_SHARED_BUFFERS];
        std::atomic<Number> param_ratios[ParamId::PARAM_ID_COUNT];
        std::atomic<Byte> controller_assignments[ParamId::PARAM_ID_COUNT];
        Envelope* envelopes_rw[Constants::ENVELOPES];
        LFO* lfos_rw[Constants::LFOS];
        Macro* macros_rw[MACROS];
        MidiController* midi_controllers_rw[MIDI_CONTROLLERS];
        Integer midi_note_to_voice_assignments[Midi::CHANNELS][Midi::NOTES];
        OscillatorInaccuracy* synced_oscillator_inaccuracies[POLYPHONY];
        Modulator* modulators[POLYPHONY];
        Carrier* carriers[POLYPHONY];
        NoteTunings active_note_tunings;
        Integer samples_since_gc;
        Integer samples_between_gc;
        Integer next_voice;
        Integer next_note_id;
        Midi::Note previous_note;
        bool is_learning;
        bool is_sustaining;
        bool is_polyphonic;
        bool was_polyphonic;
        bool is_dirty_;
        std::atomic<bool> is_mts_esp_connected_;

    public:
        Effects::Effects<Bus> effects;
        MidiController* const* const midi_controllers;
        Macro* const* const macros;
        Envelope* const* const envelopes;
        LFO* const* const lfos;
};

}

#endif
