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
#include "dsp/lfo_envelope_list.hpp"
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
#include "dsp/tape.hpp"


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
        static constexpr Integer IN_CHANNELS = OUT_CHANNELS;

        static constexpr Integer ENVELOPE_FLOAT_PARAMS = 12;
        static constexpr Integer ENVELOPE_DISCRETE_PARAMS = 5;

        static constexpr Integer MIDI_CONTROLLERS = 128;

        static constexpr Integer MACROS = 30;

        enum MessageType {
            SET_PARAM = 1,          ///< Set the given parameter's ratio to
                                    ///< \c number_param.

            SET_PARAM_SMOOTHLY = 2, ///< Set the given parameter's ratio to
                                    ///< \c number_param smoothly.

            ASSIGN_CONTROLLER = 3,  ///< Assign the controller identified by
                                    ///< \c byte_param to the given parameter.

            REFRESH_PARAM = 4,      ///< Make sure that \c get_param_ratio_atomic()
                                    ///< will return the most recent value of
                                    ///< the given parameter.

            CLEAR = 5,              ///< Clear all buffers, release all
                                    ///< controller assignments, and reset all
                                    ///< parameters to their default values.

            CLEAR_DIRTY_FLAG = 6,   ///< Clear the dirty flag.

            INVALID_MESSAGE_TYPE,
        };

        enum ParamId {
            MIX = 0,         ///< Modulator Additive Volume
            PM = 1,          ///< Phase Modulation
            FM = 2,          ///< Frequency Modulation
            AM = 3,          ///< Amplitude Modulation
            INVOL = 4,       ///< Auxiliary Input Volume

            MAMP = 5,        ///< Modulator Amplitude
            MVS = 6,         ///< Modulator Velocity Sensitivity
            MFLD = 7,        ///< Modulator Folding
            MPRT = 8,        ///< Modulator Portamento Length
            MPRD = 9,        ///< Modulator Portamento Depth
            MDTN = 10,       ///< Modulator Detune
            MFIN = 11,       ///< Modulator Fine Detune
            MWID = 12,       ///< Modulator Width
            MPAN = 13,       ///< Modulator Pan
            MVOL = 14,       ///< Modulator Volume
            MSUB = 15,       ///< Modulator Subharmonic Amplitude
            MC1 = 16,        ///< Modulator Custom Waveform 1st Harmonic
            MC2 = 17,        ///< Modulator Custom Waveform 2nd Harmonic
            MC3 = 18,        ///< Modulator Custom Waveform 3rd Harmonic
            MC4 = 19,        ///< Modulator Custom Waveform 4th Harmonic
            MC5 = 20,        ///< Modulator Custom Waveform 5th Harmonic
            MC6 = 21,        ///< Modulator Custom Waveform 6th Harmonic
            MC7 = 22,        ///< Modulator Custom Waveform 7th Harmonic
            MC8 = 23,        ///< Modulator Custom Waveform 8th Harmonic
            MC9 = 24,        ///< Modulator Custom Waveform 9th Harmonic
            MC10 = 25,       ///< Modulator Custom Waveform 10th Harmonic
            MF1FRQ = 26,     ///< Modulator Filter 1 Frequency
            MF1Q = 27,       ///< Modulator Filter 1 Q Factor
            MF1G = 28,       ///< Modulator Filter 1 Gain
            MF1FIA = 29,     ///< Modulator Filter 1 Frequency Inaccuracy
            MF1QIA = 30,     ///< Modulator Filter 1 Q Factor Inaccuracy
            MF2FRQ = 31,     ///< Modulator Filter 2 Frequency
            MF2Q = 32,       ///< Modulator Filter 2 Q Factor
            MF2G = 33,       ///< Modulator Filter 2 Gain
            MF2FIA = 34,     ///< Modulator Filter 2 Frequency Inaccuracy
            MF2QIA = 35,     ///< Modulator Filter 2 Q Factor Inaccuracy

            CAMP = 36,       ///< Carrier Amplitude
            CVS = 37,        ///< Carrier Velocity Sensitivity
            CFLD = 38,       ///< Carrier Folding
            CPRT = 39,       ///< Carrier Portamento Length
            CPRD = 40,       ///< Carrier Portamento Depth
            CDTN = 41,       ///< Carrier Detune
            CFIN = 42,       ///< Carrier Fine Detune
            CWID = 43,       ///< Carrier Width
            CPAN = 44,       ///< Carrier Pan
            CVOL = 45,       ///< Carrier Volume
            CDL = 46,        ///< Carrier Distortion Level
            CC1 = 47,        ///< Carrier Custom Waveform 1st Harmonic
            CC2 = 48,        ///< Carrier Custom Waveform 2nd Harmonic
            CC3 = 49,        ///< Carrier Custom Waveform 3rd Harmonic
            CC4 = 50,        ///< Carrier Custom Waveform 4th Harmonic
            CC5 = 51,        ///< Carrier Custom Waveform 5th Harmonic
            CC6 = 52,        ///< Carrier Custom Waveform 6th Harmonic
            CC7 = 53,        ///< Carrier Custom Waveform 7th Harmonic
            CC8 = 54,        ///< Carrier Custom Waveform 8th Harmonic
            CC9 = 55,        ///< Carrier Custom Waveform 9th Harmonic
            CC10 = 56,       ///< Carrier Custom Waveform 10th Harmonic
            CF1FRQ = 57,     ///< Carrier Filter 1 Frequency
            CF1Q = 58,       ///< Carrier Filter 1 Q Factor
            CF1G = 59,       ///< Carrier Filter 1 Gain
            CF1FIA = 60,     ///< Carrier Filter 1 Frequency Inaccuracy
            CF1QIA = 61,     ///< Carrier Filter 1 Q Factor Inaccuracy
            CF2FRQ = 62,     ///< Carrier Filter 2 Frequency
            CF2Q = 63,       ///< Carrier Filter 2 Q Factor
            CF2G = 64,       ///< Carrier Filter 2 Gain
            CF2FIA = 65,     ///< Carrier Filter 2 Frequency Inaccuracy
            CF2QIA = 66,     ///< Carrier Filter 2 Q Factor Inaccuracy

            EV1V = 67,       ///< Effects Volume 1
            ED1L = 68,       ///< Effects Distortion 1 Level
            ED2L = 69,       ///< Effects Distortion 2 Level
            EF1FRQ = 70,     ///< Effects Filter 1 Frequency
            EF1Q = 71,       ///< Effects Filter 1 Q Factor
            EF1G = 72,       ///< Effects Filter 1 Gain
            EF2FRQ = 73,     ///< Effects Filter 2 Frequency
            EF2Q = 74,       ///< Effects Filter 2 Q Factor
            EF2G = 75,       ///< Effects Filter 2 Gain
            EV2V = 76,       ///< Effects Volume 2
            ETSTP = 77,      ///< Effects Tape Stop / Start
            ETWFA = 78,      ///< Effects Tape Wow and Flutter Amplitude
            ETDST = 79,      ///< Effects Tape Distortion
            ETCLR = 80,      ///< Effects Tape Color
            ETHSS = 81,      ///< Effects Tape Hiss Level
            ETWFS = 82,      ///< Effects Tape Wow and Flutter Speed
            ECDEL = 83,      ///< Effects Chorus Delay Time
            ECFRQ = 84,      ///< Effects Chorus LFO Frequency
            ECDPT = 85,      ///< Effects Chorus Depth
            ECFB = 86,       ///< Effects Chorus Feedback
            ECDF = 87,       ///< Effects Chorus Dampening Frequency
            ECDG = 88,       ///< Effects Chorus Dampening Gain
            ECWID = 89,      ///< Effects Chorus Stereo Width
            ECHPF = 90,      ///< Effects Chorus High-pass Frequency
            ECHPQ = 91,      ///< Effects Chorus High-pass Q Factor
            ECWET = 92,      ///< Effects Chorus Wet Volume
            ECDRY = 93,      ///< Effects Chorus Dry Volume
            EEDEL = 94,      ///< Effects Echo Delay Time
            EEINV = 95,      ///< Effects Echo Input Volume
            EEFB = 96,       ///< Effects Echo Feedback
            EEDST = 97,      ///< Effects Echo Distortion
            EEDF = 98,       ///< Effects Echo Dampening Frequency
            EEDG = 99,       ///< Effects Echo Dampening Gain
            EEWID = 100,     ///< Effects Echo Stereo Width
            EEHPF = 101,     ///< Effects Echo High-pass Frequency
            EEHPQ = 102,     ///< Effects Echo High-pass Q Factor
            EECTH = 103,     ///< Effects Echo Side-Chain Compression Threshold
            EECAT = 104,     ///< Effects Echo Side-Chain Compression Attack Time
            EECRL = 105,     ///< Effects Echo Side-Chain Compression Release Time
            EECR = 106,      ///< Effects Echo Side-Chain Compression Ratio
            EEWET = 107,     ///< Effects Echo Wet Volume
            EEDRY = 108,     ///< Effects Echo Dry Volume
            ERRS = 109,      ///< Effects Reverb Room Size
            ERRR = 110,      ///< Effects Reverb Room Reflectivity
            ERDST = 111,     ///< Effects Reverb Distortion
            ERDF = 112,      ///< Effects Reverb Dampening Frequency
            ERDG = 113,      ///< Effects Reverb Dampening Gain
            ERWID = 114,     ///< Effects Reverb Stereo Width
            ERHPF = 115,     ///< Effects Reverb High-pass Frequency
            ERHPQ = 116,     ///< Effects Reverb High-pass Q Factor
            ERCTH = 117,     ///< Effects Reverb Side-Chain Compression Threshold
            ERCAT = 118,     ///< Effects Reverb Side-Chain Compression Attack Time
            ERCRL = 119,     ///< Effects Reverb Side-Chain Compression Release Time
            ERCR = 120,      ///< Effects Reverb Side-Chain Compression Ratio
            ERWET = 121,     ///< Effects Reverb Wet Volume
            ERDRY = 122,     ///< Effects Reverb Dry Volume
            EV3V = 123,      ///< Effects Volume 3

            M1MID = 124,     ///< Macro 1 Midpoint
            M1IN = 125,      ///< Macro 1 Input
            M1MIN = 126,     ///< Macro 1 Minimum Value
            M1MAX = 127,     ///< Macro 1 Maximum Value
            M1SCL = 128,     ///< Macro 1 Scale
            M1DST = 129,     ///< Macro 1 Distortion
            M1RND = 130,     ///< Macro 1 Randomness

            M2MID = 131,     ///< Macro 2 Midpoint
            M2IN = 132,      ///< Macro 2 Input
            M2MIN = 133,     ///< Macro 2 Minimum Value
            M2MAX = 134,     ///< Macro 2 Maximum Value
            M2SCL = 135,     ///< Macro 2 Scale
            M2DST = 136,     ///< Macro 2 Distortion
            M2RND = 137,     ///< Macro 2 Randomness

            M3MID = 138,     ///< Macro 3 Midpoint
            M3IN = 139,      ///< Macro 3 Input
            M3MIN = 140,     ///< Macro 3 Minimum Value
            M3MAX = 141,     ///< Macro 3 Maximum Value
            M3SCL = 142,     ///< Macro 3 Scale
            M3DST = 143,     ///< Macro 3 Distortion
            M3RND = 144,     ///< Macro 3 Randomness

            M4MID = 145,     ///< Macro 4 Midpoint
            M4IN = 146,      ///< Macro 4 Input
            M4MIN = 147,     ///< Macro 4 Minimum Value
            M4MAX = 148,     ///< Macro 4 Maximum Value
            M4SCL = 149,     ///< Macro 4 Scale
            M4DST = 150,     ///< Macro 4 Distortion
            M4RND = 151,     ///< Macro 4 Randomness

            M5MID = 152,     ///< Macro 5 Midpoint
            M5IN = 153,      ///< Macro 5 Input
            M5MIN = 154,     ///< Macro 5 Minimum Value
            M5MAX = 155,     ///< Macro 5 Maximum Value
            M5SCL = 156,     ///< Macro 5 Scale
            M5DST = 157,     ///< Macro 5 Distortion
            M5RND = 158,     ///< Macro 5 Randomness

            M6MID = 159,     ///< Macro 6 Midpoint
            M6IN = 160,      ///< Macro 6 Input
            M6MIN = 161,     ///< Macro 6 Minimum Value
            M6MAX = 162,     ///< Macro 6 Maximum Value
            M6SCL = 163,     ///< Macro 6 Scale
            M6DST = 164,     ///< Macro 6 Distortion
            M6RND = 165,     ///< Macro 6 Randomness

            M7MID = 166,     ///< Macro 7 Midpoint
            M7IN = 167,      ///< Macro 7 Input
            M7MIN = 168,     ///< Macro 7 Minimum Value
            M7MAX = 169,     ///< Macro 7 Maximum Value
            M7SCL = 170,     ///< Macro 7 Scale
            M7DST = 171,     ///< Macro 7 Distortion
            M7RND = 172,     ///< Macro 7 Randomness

            M8MID = 173,     ///< Macro 8 Midpoint
            M8IN = 174,      ///< Macro 8 Input
            M8MIN = 175,     ///< Macro 8 Minimum Value
            M8MAX = 176,     ///< Macro 8 Maximum Value
            M8SCL = 177,     ///< Macro 8 Scale
            M8DST = 178,     ///< Macro 8 Distortion
            M8RND = 179,     ///< Macro 8 Randomness

            M9MID = 180,     ///< Macro 9 Midpoint
            M9IN = 181,      ///< Macro 9 Input
            M9MIN = 182,     ///< Macro 9 Minimum Value
            M9MAX = 183,     ///< Macro 9 Maximum Value
            M9SCL = 184,     ///< Macro 9 Scale
            M9DST = 185,     ///< Macro 9 Distortion
            M9RND = 186,     ///< Macro 9 Randomness

            M10MID = 187,    ///< Macro 10 Midpoint
            M10IN = 188,     ///< Macro 10 Input
            M10MIN = 189,    ///< Macro 10 Minimum Value
            M10MAX = 190,    ///< Macro 10 Maximum Value
            M10SCL = 191,    ///< Macro 10 Scale
            M10DST = 192,    ///< Macro 10 Distortion
            M10RND = 193,    ///< Macro 10 Randomness

            M11MID = 194,    ///< Macro 11 Midpoint
            M11IN = 195,     ///< Macro 11 Input
            M11MIN = 196,    ///< Macro 11 Minimum Value
            M11MAX = 197,    ///< Macro 11 Maximum Value
            M11SCL = 198,    ///< Macro 11 Scale
            M11DST = 199,    ///< Macro 11 Distortion
            M11RND = 200,    ///< Macro 11 Randomness

            M12MID = 201,    ///< Macro 12 Midpoint
            M12IN = 202,     ///< Macro 12 Input
            M12MIN = 203,    ///< Macro 12 Minimum Value
            M12MAX = 204,    ///< Macro 12 Maximum Value
            M12SCL = 205,    ///< Macro 12 Scale
            M12DST = 206,    ///< Macro 12 Distortion
            M12RND = 207,    ///< Macro 12 Randomness

            M13MID = 208,    ///< Macro 13 Midpoint
            M13IN = 209,     ///< Macro 13 Input
            M13MIN = 210,    ///< Macro 13 Minimum Value
            M13MAX = 211,    ///< Macro 13 Maximum Value
            M13SCL = 212,    ///< Macro 13 Scale
            M13DST = 213,    ///< Macro 13 Distortion
            M13RND = 214,    ///< Macro 13 Randomness

            M14MID = 215,    ///< Macro 14 Midpoint
            M14IN = 216,     ///< Macro 14 Input
            M14MIN = 217,    ///< Macro 14 Minimum Value
            M14MAX = 218,    ///< Macro 14 Maximum Value
            M14SCL = 219,    ///< Macro 14 Scale
            M14DST = 220,    ///< Macro 14 Distortion
            M14RND = 221,    ///< Macro 14 Randomness

            M15MID = 222,    ///< Macro 15 Midpoint
            M15IN = 223,     ///< Macro 15 Input
            M15MIN = 224,    ///< Macro 15 Minimum Value
            M15MAX = 225,    ///< Macro 15 Maximum Value
            M15SCL = 226,    ///< Macro 15 Scale
            M15DST = 227,    ///< Macro 15 Distortion
            M15RND = 228,    ///< Macro 15 Randomness

            M16MID = 229,    ///< Macro 16 Midpoint
            M16IN = 230,     ///< Macro 16 Input
            M16MIN = 231,    ///< Macro 16 Minimum Value
            M16MAX = 232,    ///< Macro 16 Maximum Value
            M16SCL = 233,    ///< Macro 16 Scale
            M16DST = 234,    ///< Macro 16 Distortion
            M16RND = 235,    ///< Macro 16 Randomness

            M17MID = 236,    ///< Macro 17 Midpoint
            M17IN = 237,     ///< Macro 17 Input
            M17MIN = 238,    ///< Macro 17 Minimum Value
            M17MAX = 239,    ///< Macro 17 Maximum Value
            M17SCL = 240,    ///< Macro 17 Scale
            M17DST = 241,    ///< Macro 17 Distortion
            M17RND = 242,    ///< Macro 17 Randomness

            M18MID = 243,    ///< Macro 18 Midpoint
            M18IN = 244,     ///< Macro 18 Input
            M18MIN = 245,    ///< Macro 18 Minimum Value
            M18MAX = 246,    ///< Macro 18 Maximum Value
            M18SCL = 247,    ///< Macro 18 Scale
            M18DST = 248,    ///< Macro 18 Distortion
            M18RND = 249,    ///< Macro 18 Randomness

            M19MID = 250,    ///< Macro 19 Midpoint
            M19IN = 251,     ///< Macro 19 Input
            M19MIN = 252,    ///< Macro 19 Minimum Value
            M19MAX = 253,    ///< Macro 19 Maximum Value
            M19SCL = 254,    ///< Macro 19 Scale
            M19DST = 255,    ///< Macro 19 Distortion
            M19RND = 256,    ///< Macro 19 Randomness

            M20MID = 257,    ///< Macro 20 Midpoint
            M20IN = 258,     ///< Macro 20 Input
            M20MIN = 259,    ///< Macro 20 Minimum Value
            M20MAX = 260,    ///< Macro 20 Maximum Value
            M20SCL = 261,    ///< Macro 20 Scale
            M20DST = 262,    ///< Macro 20 Distortion
            M20RND = 263,    ///< Macro 20 Randomness

            M21MID = 264,    ///< Macro 21 Midpoint
            M21IN = 265,     ///< Macro 21 Input
            M21MIN = 266,    ///< Macro 21 Minimum Value
            M21MAX = 267,    ///< Macro 21 Maximum Value
            M21SCL = 268,    ///< Macro 21 Scale
            M21DST = 269,    ///< Macro 21 Distortion
            M21RND = 270,    ///< Macro 21 Randomness

            M22MID = 271,    ///< Macro 22 Midpoint
            M22IN = 272,     ///< Macro 22 Input
            M22MIN = 273,    ///< Macro 22 Minimum Value
            M22MAX = 274,    ///< Macro 22 Maximum Value
            M22SCL = 275,    ///< Macro 22 Scale
            M22DST = 276,    ///< Macro 22 Distortion
            M22RND = 277,    ///< Macro 22 Randomness

            M23MID = 278,    ///< Macro 23 Midpoint
            M23IN = 279,     ///< Macro 23 Input
            M23MIN = 280,    ///< Macro 23 Minimum Value
            M23MAX = 281,    ///< Macro 23 Maximum Value
            M23SCL = 282,    ///< Macro 23 Scale
            M23DST = 283,    ///< Macro 23 Distortion
            M23RND = 284,    ///< Macro 23 Randomness

            M24MID = 285,    ///< Macro 24 Midpoint
            M24IN = 286,     ///< Macro 24 Input
            M24MIN = 287,    ///< Macro 24 Minimum Value
            M24MAX = 288,    ///< Macro 24 Maximum Value
            M24SCL = 289,    ///< Macro 24 Scale
            M24DST = 290,    ///< Macro 24 Distortion
            M24RND = 291,    ///< Macro 24 Randomness

            M25MID = 292,    ///< Macro 25 Midpoint
            M25IN = 293,     ///< Macro 25 Input
            M25MIN = 294,    ///< Macro 25 Minimum Value
            M25MAX = 295,    ///< Macro 25 Maximum Value
            M25SCL = 296,    ///< Macro 25 Scale
            M25DST = 297,    ///< Macro 25 Distortion
            M25RND = 298,    ///< Macro 25 Randomness

            M26MID = 299,    ///< Macro 26 Midpoint
            M26IN = 300,     ///< Macro 26 Input
            M26MIN = 301,    ///< Macro 26 Minimum Value
            M26MAX = 302,    ///< Macro 26 Maximum Value
            M26SCL = 303,    ///< Macro 26 Scale
            M26DST = 304,    ///< Macro 26 Distortion
            M26RND = 305,    ///< Macro 26 Randomness

            M27MID = 306,    ///< Macro 27 Midpoint
            M27IN = 307,     ///< Macro 27 Input
            M27MIN = 308,    ///< Macro 27 Minimum Value
            M27MAX = 309,    ///< Macro 27 Maximum Value
            M27SCL = 310,    ///< Macro 27 Scale
            M27DST = 311,    ///< Macro 27 Distortion
            M27RND = 312,    ///< Macro 27 Randomness

            M28MID = 313,    ///< Macro 28 Midpoint
            M28IN = 314,     ///< Macro 28 Input
            M28MIN = 315,    ///< Macro 28 Minimum Value
            M28MAX = 316,    ///< Macro 28 Maximum Value
            M28SCL = 317,    ///< Macro 28 Scale
            M28DST = 318,    ///< Macro 28 Distortion
            M28RND = 319,    ///< Macro 28 Randomness

            M29MID = 320,    ///< Macro 29 Midpoint
            M29IN = 321,     ///< Macro 29 Input
            M29MIN = 322,    ///< Macro 29 Minimum Value
            M29MAX = 323,    ///< Macro 29 Maximum Value
            M29SCL = 324,    ///< Macro 29 Scale
            M29DST = 325,    ///< Macro 29 Distortion
            M29RND = 326,    ///< Macro 29 Randomness

            M30MID = 327,    ///< Macro 30 Midpoint
            M30IN = 328,     ///< Macro 30 Input
            M30MIN = 329,    ///< Macro 30 Minimum Value
            M30MAX = 330,    ///< Macro 30 Maximum Value
            M30SCL = 331,    ///< Macro 30 Scale
            M30DST = 332,    ///< Macro 30 Distortion
            M30RND = 333,    ///< Macro 30 Randomness

            N1SCL = 334,     ///< Envelope 1 Scale
            N1INI = 335,     ///< Envelope 1 Initial Level
            N1DEL = 336,     ///< Envelope 1 Delay Time
            N1ATK = 337,     ///< Envelope 1 Attack Time
            N1PK = 338,      ///< Envelope 1 Peak Level
            N1HLD = 339,     ///< Envelope 1 Hold Time
            N1DEC = 340,     ///< Envelope 1 Decay Time
            N1SUS = 341,     ///< Envelope 1 Sustain Level
            N1REL = 342,     ///< Envelope 1 Release Time
            N1FIN = 343,     ///< Envelope 1 Final Level
            N1TIN = 344,     ///< Envelope 1 Time Inaccuracy
            N1VIN = 345,     ///< Envelope 1 Level Inaccuracy

            N2SCL = 346,     ///< Envelope 2 Scale
            N2INI = 347,     ///< Envelope 2 Initial Level
            N2DEL = 348,     ///< Envelope 2 Delay Time
            N2ATK = 349,     ///< Envelope 2 Attack Time
            N2PK = 350,      ///< Envelope 2 Peak Level
            N2HLD = 351,     ///< Envelope 2 Hold Time
            N2DEC = 352,     ///< Envelope 2 Decay Time
            N2SUS = 353,     ///< Envelope 2 Sustain Level
            N2REL = 354,     ///< Envelope 2 Release Time
            N2FIN = 355,     ///< Envelope 2 Final Level
            N2TIN = 356,     ///< Envelope 2 Time Inaccuracy
            N2VIN = 357,     ///< Envelope 2 Level Inaccuracy

            N3SCL = 358,     ///< Envelope 3 Scale
            N3INI = 359,     ///< Envelope 3 Initial Level
            N3DEL = 360,     ///< Envelope 3 Delay Time
            N3ATK = 361,     ///< Envelope 3 Attack Time
            N3PK = 362,      ///< Envelope 3 Peak Level
            N3HLD = 363,     ///< Envelope 3 Hold Time
            N3DEC = 364,     ///< Envelope 3 Decay Time
            N3SUS = 365,     ///< Envelope 3 Sustain Level
            N3REL = 366,     ///< Envelope 3 Release Time
            N3FIN = 367,     ///< Envelope 3 Final Level
            N3TIN = 368,     ///< Envelope 3 Time Inaccuracy
            N3VIN = 369,     ///< Envelope 3 Level Inaccuracy

            N4SCL = 370,     ///< Envelope 4 Scale
            N4INI = 371,     ///< Envelope 4 Initial Level
            N4DEL = 372,     ///< Envelope 4 Delay Time
            N4ATK = 373,     ///< Envelope 4 Attack Time
            N4PK = 374,      ///< Envelope 4 Peak Level
            N4HLD = 375,     ///< Envelope 4 Hold Time
            N4DEC = 376,     ///< Envelope 4 Decay Time
            N4SUS = 377,     ///< Envelope 4 Sustain Level
            N4REL = 378,     ///< Envelope 4 Release Time
            N4FIN = 379,     ///< Envelope 4 Final Level
            N4TIN = 380,     ///< Envelope 4 Time Inaccuracy
            N4VIN = 381,     ///< Envelope 4 Level Inaccuracy

            N5SCL = 382,     ///< Envelope 5 Scale
            N5INI = 383,     ///< Envelope 5 Initial Level
            N5DEL = 384,     ///< Envelope 5 Delay Time
            N5ATK = 385,     ///< Envelope 5 Attack Time
            N5PK = 386,      ///< Envelope 5 Peak Level
            N5HLD = 387,     ///< Envelope 5 Hold Time
            N5DEC = 388,     ///< Envelope 5 Decay Time
            N5SUS = 389,     ///< Envelope 5 Sustain Level
            N5REL = 390,     ///< Envelope 5 Release Time
            N5FIN = 391,     ///< Envelope 5 Final Level
            N5TIN = 392,     ///< Envelope 5 Time Inaccuracy
            N5VIN = 393,     ///< Envelope 5 Level Inaccuracy

            N6SCL = 394,     ///< Envelope 6 Scale
            N6INI = 395,     ///< Envelope 6 Initial Level
            N6DEL = 396,     ///< Envelope 6 Delay Time
            N6ATK = 397,     ///< Envelope 6 Attack Time
            N6PK = 398,      ///< Envelope 6 Peak Level
            N6HLD = 399,     ///< Envelope 6 Hold Time
            N6DEC = 400,     ///< Envelope 6 Decay Time
            N6SUS = 401,     ///< Envelope 6 Sustain Level
            N6REL = 402,     ///< Envelope 6 Release Time
            N6FIN = 403,     ///< Envelope 6 Final Level
            N6TIN = 404,     ///< Envelope 6 Time Inaccuracy
            N6VIN = 405,     ///< Envelope 6 Level Inaccuracy

            N7SCL = 406,     ///< Envelope 7 Scale
            N7INI = 407,     ///< Envelope 7 Initial Level
            N7DEL = 408,     ///< Envelope 7 Delay Time
            N7ATK = 409,     ///< Envelope 7 Attack Time
            N7PK = 410,      ///< Envelope 7 Peak Level
            N7HLD = 411,     ///< Envelope 7 Hold Time
            N7DEC = 412,     ///< Envelope 7 Decay Time
            N7SUS = 413,     ///< Envelope 7 Sustain Level
            N7REL = 414,     ///< Envelope 7 Release Time
            N7FIN = 415,     ///< Envelope 7 Final Level
            N7TIN = 416,     ///< Envelope 7 Time Inaccuracy
            N7VIN = 417,     ///< Envelope 7 Level Inaccuracy

            N8SCL = 418,     ///< Envelope 8 Scale
            N8INI = 419,     ///< Envelope 8 Initial Level
            N8DEL = 420,     ///< Envelope 8 Delay Time
            N8ATK = 421,     ///< Envelope 8 Attack Time
            N8PK = 422,      ///< Envelope 8 Peak Level
            N8HLD = 423,     ///< Envelope 8 Hold Time
            N8DEC = 424,     ///< Envelope 8 Decay Time
            N8SUS = 425,     ///< Envelope 8 Sustain Level
            N8REL = 426,     ///< Envelope 8 Release Time
            N8FIN = 427,     ///< Envelope 8 Final Level
            N8TIN = 428,     ///< Envelope 8 Time Inaccuracy
            N8VIN = 429,     ///< Envelope 8 Level Inaccuracy

            N9SCL = 430,     ///< Envelope 9 Scale
            N9INI = 431,     ///< Envelope 9 Initial Level
            N9DEL = 432,     ///< Envelope 9 Delay Time
            N9ATK = 433,     ///< Envelope 9 Attack Time
            N9PK = 434,      ///< Envelope 9 Peak Level
            N9HLD = 435,     ///< Envelope 9 Hold Time
            N9DEC = 436,     ///< Envelope 9 Decay Time
            N9SUS = 437,     ///< Envelope 9 Sustain Level
            N9REL = 438,     ///< Envelope 9 Release Time
            N9FIN = 439,     ///< Envelope 9 Final Level
            N9TIN = 440,     ///< Envelope 9 Time Inaccuracy
            N9VIN = 441,     ///< Envelope 9 Level Inaccuracy

            N10SCL = 442,    ///< Envelope 10 Scale
            N10INI = 443,    ///< Envelope 10 Initial Level
            N10DEL = 444,    ///< Envelope 10 Delay Time
            N10ATK = 445,    ///< Envelope 10 Attack Time
            N10PK = 446,     ///< Envelope 10 Peak Level
            N10HLD = 447,    ///< Envelope 10 Hold Time
            N10DEC = 448,    ///< Envelope 10 Decay Time
            N10SUS = 449,    ///< Envelope 10 Sustain Level
            N10REL = 450,    ///< Envelope 10 Release Time
            N10FIN = 451,    ///< Envelope 10 Final Level
            N10TIN = 452,    ///< Envelope 10 Time Inaccuracy
            N10VIN = 453,    ///< Envelope 10 Level Inaccuracy

            N11SCL = 454,    ///< Envelope 11 Scale
            N11INI = 455,    ///< Envelope 11 Initial Level
            N11DEL = 456,    ///< Envelope 11 Delay Time
            N11ATK = 457,    ///< Envelope 11 Attack Time
            N11PK = 458,     ///< Envelope 11 Peak Level
            N11HLD = 459,    ///< Envelope 11 Hold Time
            N11DEC = 460,    ///< Envelope 11 Decay Time
            N11SUS = 461,    ///< Envelope 11 Sustain Level
            N11REL = 462,    ///< Envelope 11 Release Time
            N11FIN = 463,    ///< Envelope 11 Final Level
            N11TIN = 464,    ///< Envelope 11 Time Inaccuracy
            N11VIN = 465,    ///< Envelope 11 Level Inaccuracy

            N12SCL = 466,    ///< Envelope 12 Scale
            N12INI = 467,    ///< Envelope 12 Initial Level
            N12DEL = 468,    ///< Envelope 12 Delay Time
            N12ATK = 469,    ///< Envelope 12 Attack Time
            N12PK = 470,     ///< Envelope 12 Peak Level
            N12HLD = 471,    ///< Envelope 12 Hold Time
            N12DEC = 472,    ///< Envelope 12 Decay Time
            N12SUS = 473,    ///< Envelope 12 Sustain Level
            N12REL = 474,    ///< Envelope 12 Release Time
            N12FIN = 475,    ///< Envelope 12 Final Level
            N12TIN = 476,    ///< Envelope 12 Time Inaccuracy
            N12VIN = 477,    ///< Envelope 12 Level Inaccuracy

            L1FRQ = 478,     ///< LFO 1 Frequency
            L1PHS = 479,     ///< LFO 1 Phase
            L1MIN = 480,     ///< LFO 1 Minimum Value
            L1MAX = 481,     ///< LFO 1 Maximum Value
            L1AMP = 482,     ///< LFO 1 Amplitude
            L1DST = 483,     ///< LFO 1 Distortion
            L1RND = 484,     ///< LFO 1 Randomness

            L2FRQ = 485,     ///< LFO 2 Frequency
            L2PHS = 486,     ///< LFO 2 Phase
            L2MIN = 487,     ///< LFO 2 Minimum Value
            L2MAX = 488,     ///< LFO 2 Maximum Value
            L2AMP = 489,     ///< LFO 2 Amplitude
            L2DST = 490,     ///< LFO 2 Distortion
            L2RND = 491,     ///< LFO 2 Randomness

            L3FRQ = 492,     ///< LFO 3 Frequency
            L3PHS = 493,     ///< LFO 3 Phase
            L3MIN = 494,     ///< LFO 3 Minimum Value
            L3MAX = 495,     ///< LFO 3 Maximum Value
            L3AMP = 496,     ///< LFO 3 Amplitude
            L3DST = 497,     ///< LFO 3 Distortion
            L3RND = 498,     ///< LFO 3 Randomness

            L4FRQ = 499,     ///< LFO 4 Frequency
            L4PHS = 500,     ///< LFO 4 Phase
            L4MIN = 501,     ///< LFO 4 Minimum Value
            L4MAX = 502,     ///< LFO 4 Maximum Value
            L4AMP = 503,     ///< LFO 4 Amplitude
            L4DST = 504,     ///< LFO 4 Distortion
            L4RND = 505,     ///< LFO 4 Randomness

            L5FRQ = 506,     ///< LFO 5 Frequency
            L5PHS = 507,     ///< LFO 5 Phase
            L5MIN = 508,     ///< LFO 5 Minimum Value
            L5MAX = 509,     ///< LFO 5 Maximum Value
            L5AMP = 510,     ///< LFO 5 Amplitude
            L5DST = 511,     ///< LFO 5 Distortion
            L5RND = 512,     ///< LFO 5 Randomness

            L6FRQ = 513,     ///< LFO 6 Frequency
            L6PHS = 514,     ///< LFO 6 Phase
            L6MIN = 515,     ///< LFO 6 Minimum Value
            L6MAX = 516,     ///< LFO 6 Maximum Value
            L6AMP = 517,     ///< LFO 6 Amplitude
            L6DST = 518,     ///< LFO 6 Distortion
            L6RND = 519,     ///< LFO 6 Randomness

            L7FRQ = 520,     ///< LFO 7 Frequency
            L7PHS = 521,     ///< LFO 7 Phase
            L7MIN = 522,     ///< LFO 7 Minimum Value
            L7MAX = 523,     ///< LFO 7 Maximum Value
            L7AMP = 524,     ///< LFO 7 Amplitude
            L7DST = 525,     ///< LFO 7 Distortion
            L7RND = 526,     ///< LFO 7 Randomness

            L8FRQ = 527,     ///< LFO 8 Frequency
            L8PHS = 528,     ///< LFO 8 Phase
            L8MIN = 529,     ///< LFO 8 Minimum Value
            L8MAX = 530,     ///< LFO 8 Maximum Value
            L8AMP = 531,     ///< LFO 8 Amplitude
            L8DST = 532,     ///< LFO 8 Distortion
            L8RND = 533,     ///< LFO 8 Randomness

            MODE = 534,      ///< Mode
            MWAV = 535,      ///< Modulator Waveform
            CWAV = 536,      ///< Carrier Waveform
            MF1TYP = 537,    ///< Modulator Filter 1 Type
            MF2TYP = 538,    ///< Modulator Filter 2 Type
            CF1TYP = 539,    ///< Carrier Filter 1 Type
            CDTYP = 540,     ///< Carrier Distortion Type
            CF2TYP = 541,    ///< Carrier Filter 2 Type
            ED1TYP = 542,    ///< Effects Distortion 1 Type
            ED2TYP = 543,    ///< Effects Distortion 2 Type
            EF1TYP = 544,    ///< Effects Filter 1 Type
            EF2TYP = 545,    ///< Effects Filter 2 Type
            L1WAV = 546,     ///< LFO 1 Waveform
            L2WAV = 547,     ///< LFO 2 Waveform
            L3WAV = 548,     ///< LFO 3 Waveform
            L4WAV = 549,     ///< LFO 4 Waveform
            L5WAV = 550,     ///< LFO 5 Waveform
            L6WAV = 551,     ///< LFO 6 Waveform
            L7WAV = 552,     ///< LFO 7 Waveform
            L8WAV = 553,     ///< LFO 8 Waveform
            L1LOG = 554,     ///< LFO 1 Logarithmic Frequency
            L2LOG = 555,     ///< LFO 2 Logarithmic Frequency
            L3LOG = 556,     ///< LFO 3 Logarithmic Frequency
            L4LOG = 557,     ///< LFO 4 Logarithmic Frequency
            L5LOG = 558,     ///< LFO 5 Logarithmic Frequency
            L6LOG = 559,     ///< LFO 6 Logarithmic Frequency
            L7LOG = 560,     ///< LFO 7 Logarithmic Frequency
            L8LOG = 561,     ///< LFO 8 Logarithmic Frequency
            L1CEN = 562,     ///< LFO 1 Center
            L2CEN = 563,     ///< LFO 2 Center
            L3CEN = 564,     ///< LFO 3 Center
            L4CEN = 565,     ///< LFO 4 Center
            L5CEN = 566,     ///< LFO 5 Center
            L6CEN = 567,     ///< LFO 6 Center
            L7CEN = 568,     ///< LFO 7 Center
            L8CEN = 569,     ///< LFO 8 Center
            L1SYN = 570,     ///< LFO 1 Tempo Synchronization
            L2SYN = 571,     ///< LFO 2 Tempo Synchronization
            L3SYN = 572,     ///< LFO 3 Tempo Synchronization
            L4SYN = 573,     ///< LFO 4 Tempo Synchronization
            L5SYN = 574,     ///< LFO 5 Tempo Synchronization
            L6SYN = 575,     ///< LFO 6 Tempo Synchronization
            L7SYN = 576,     ///< LFO 7 Tempo Synchronization
            L8SYN = 577,     ///< LFO 8 Tempo Synchronization
            ECSYN = 578,     ///< Effects Chorus Tempo Synchronization
            EESYN = 579,     ///< Effects Echo Tempo Synchronization
            MF1LOG = 580,    ///< Modulator Filter 1 Logarithmic Frequency
            MF2LOG = 581,    ///< Modulator Filter 2 Logarithmic Frequency
            CF1LOG = 582,    ///< Carrier Filter 1 Logarithmic Frequency
            CF2LOG = 583,    ///< Carrier Filter 2 Logarithmic Frequency
            EF1LOG = 584,    ///< Effects Filter 1 Logarithmic Frequency
            EF2LOG = 585,    ///< Effects Filter 2 Logarithmic Frequency
            ECLOG = 586,     ///< Effects Chorus Logarithmic Filter Frequencies
            ECLHQ = 587,     ///< Effects Chorus Logarithmic High-pass Filter Q Factor
            ECLLG = 588,     ///< Effects Chorus Logarithmic LFO Frequency
            EELOG = 589,     ///< Effects Echo Logarithmic Filter Frequencies
            EELHQ = 590,     ///< Effects Echo Logarithmic High-pass Filter Q Factor
            ERLOG = 591,     ///< Effects Reverb Logarithmic Filter Frequencies
            ERLHQ = 592,     ///< Effects Reverb Logarithmic High-pass Filter Q Factor
            N1UPD = 593,     ///< Envelope 1 Update Mode
            N2UPD = 594,     ///< Envelope 2 Update Mode
            N3UPD = 595,     ///< Envelope 3 Update Mode
            N4UPD = 596,     ///< Envelope 4 Update Mode
            N5UPD = 597,     ///< Envelope 5 Update Mode
            N6UPD = 598,     ///< Envelope 6 Update Mode
            N7UPD = 599,     ///< Envelope 7 Update Mode
            N8UPD = 600,     ///< Envelope 8 Update Mode
            N9UPD = 601,     ///< Envelope 9 Update Mode
            N10UPD = 602,    ///< Envelope 10 Update Mode
            N11UPD = 603,    ///< Envelope 11 Update Mode
            N12UPD = 604,    ///< Envelope 12 Update Mode
            NH = 605,        ///< Note Handling
            ERTYP = 606,     ///< Effects Reverb Type
            ECTYP = 607,     ///< Effects Chorus Type
            MTUN = 608,      ///< Modulator Tuning
            CTUN = 609,      ///< Carrier Tuning
            MOIA = 610,      ///< Modulator Oscillator Inaccuracy
            MOIS = 611,      ///< Modulator Oscillator Instability
            COIA = 612,      ///< Carrier Oscillator Inaccuracy
            COIS = 613,      ///< Carrier Oscillator Instability
            MF1QLG = 614,    ///< Modulator Filter 1 Logarithmic Q Factor
            MF2QLG = 615,    ///< Modulator Filter 2 Logarithmic Q Factor
            CF1QLG = 616,    ///< Carrier Filter 1 Logarithmic Q Factor
            CF2QLG = 617,    ///< Carrier Filter 2 Logarithmic Q Factor
            EF1QLG = 618,    ///< Effects Filter 1 Logarithmic Q Factor
            EF2QLG = 619,    ///< Effects Filter 2 Logarithmic Q Factor
            L1AEN = 620,     ///< LFO 1 Amplitude Envelope
            L2AEN = 621,     ///< LFO 2 Amplitude Envelope
            L3AEN = 622,     ///< LFO 3 Amplitude Envelope
            L4AEN = 623,     ///< LFO 4 Amplitude Envelope
            L5AEN = 624,     ///< LFO 5 Amplitude Envelope
            L6AEN = 625,     ///< LFO 6 Amplitude Envelope
            L7AEN = 626,     ///< LFO 7 Amplitude Envelope
            L8AEN = 627,     ///< LFO 8 Amplitude Envelope
            N1SYN = 628,     ///< Envelope 1 Tempo Synchronization
            N2SYN = 629,     ///< Envelope 2 Tempo Synchronization
            N3SYN = 630,     ///< Envelope 3 Tempo Synchronization
            N4SYN = 631,     ///< Envelope 4 Tempo Synchronization
            N5SYN = 632,     ///< Envelope 5 Tempo Synchronization
            N6SYN = 633,     ///< Envelope 6 Tempo Synchronization
            N7SYN = 634,     ///< Envelope 7 Tempo Synchronization
            N8SYN = 635,     ///< Envelope 8 Tempo Synchronization
            N9SYN = 636,     ///< Envelope 9 Tempo Synchronization
            N10SYN = 637,    ///< Envelope 10 Tempo Synchronization
            N11SYN = 638,    ///< Envelope 11 Tempo Synchronization
            N12SYN = 639,    ///< Envelope 12 Tempo Synchronization
            N1ASH = 640,     ///< Envelope 1 Attack Shape
            N2ASH = 641,     ///< Envelope 2 Attack Shape
            N3ASH = 642,     ///< Envelope 3 Attack Shape
            N4ASH = 643,     ///< Envelope 4 Attack Shape
            N5ASH = 644,     ///< Envelope 5 Attack Shape
            N6ASH = 645,     ///< Envelope 6 Attack Shape
            N7ASH = 646,     ///< Envelope 7 Attack Shape
            N8ASH = 647,     ///< Envelope 8 Attack Shape
            N9ASH = 648,     ///< Envelope 9 Attack Shape
            N10ASH = 649,    ///< Envelope 10 Attack Shape
            N11ASH = 650,    ///< Envelope 11 Attack Shape
            N12ASH = 651,    ///< Envelope 12 Attack Shape
            N1DSH = 652,     ///< Envelope 1 Decay Shape
            N2DSH = 653,     ///< Envelope 2 Decay Shape
            N3DSH = 654,     ///< Envelope 3 Decay Shape
            N4DSH = 655,     ///< Envelope 4 Decay Shape
            N5DSH = 656,     ///< Envelope 5 Decay Shape
            N6DSH = 657,     ///< Envelope 6 Decay Shape
            N7DSH = 658,     ///< Envelope 7 Decay Shape
            N8DSH = 659,     ///< Envelope 8 Decay Shape
            N9DSH = 660,     ///< Envelope 9 Decay Shape
            N10DSH = 661,    ///< Envelope 10 Decay Shape
            N11DSH = 662,    ///< Envelope 11 Decay Shape
            N12DSH = 663,    ///< Envelope 12 Decay Shape
            N1RSH = 664,     ///< Envelope 1 Release Shape
            N2RSH = 665,     ///< Envelope 2 Release Shape
            N3RSH = 666,     ///< Envelope 3 Release Shape
            N4RSH = 667,     ///< Envelope 4 Release Shape
            N5RSH = 668,     ///< Envelope 5 Release Shape
            N6RSH = 669,     ///< Envelope 6 Release Shape
            N7RSH = 670,     ///< Envelope 7 Release Shape
            N8RSH = 671,     ///< Envelope 8 Release Shape
            N9RSH = 672,     ///< Envelope 9 Release Shape
            N10RSH = 673,    ///< Envelope 10 Release Shape
            N11RSH = 674,    ///< Envelope 11 Release Shape
            N12RSH = 675,    ///< Envelope 12 Release Shape
            M1DSH = 676,     ///< Macro 1 Distortion Shape
            M2DSH = 677,     ///< Macro 2 Distortion Shape
            M3DSH = 678,     ///< Macro 3 Distortion Shape
            M4DSH = 679,     ///< Macro 4 Distortion Shape
            M5DSH = 680,     ///< Macro 5 Distortion Shape
            M6DSH = 681,     ///< Macro 6 Distortion Shape
            M7DSH = 682,     ///< Macro 7 Distortion Shape
            M8DSH = 683,     ///< Macro 8 Distortion Shape
            M9DSH = 684,     ///< Macro 9 Distortion Shape
            M10DSH = 685,    ///< Macro 10 Distortion Shape
            M11DSH = 686,    ///< Macro 11 Distortion Shape
            M12DSH = 687,    ///< Macro 12 Distortion Shape
            M13DSH = 688,    ///< Macro 13 Distortion Shape
            M14DSH = 689,    ///< Macro 14 Distortion Shape
            M15DSH = 690,    ///< Macro 15 Distortion Shape
            M16DSH = 691,    ///< Macro 16 Distortion Shape
            M17DSH = 692,    ///< Macro 17 Distortion Shape
            M18DSH = 693,    ///< Macro 18 Distortion Shape
            M19DSH = 694,    ///< Macro 19 Distortion Shape
            M20DSH = 695,    ///< Macro 20 Distortion Shape
            M21DSH = 696,    ///< Macro 21 Distortion Shape
            M22DSH = 697,    ///< Macro 22 Distortion Shape
            M23DSH = 698,    ///< Macro 23 Distortion Shape
            M24DSH = 699,    ///< Macro 24 Distortion Shape
            M25DSH = 700,    ///< Macro 25 Distortion Shape
            M26DSH = 701,    ///< Macro 26 Distortion Shape
            M27DSH = 702,    ///< Macro 27 Distortion Shape
            M28DSH = 703,    ///< Macro 28 Distortion Shape
            M29DSH = 704,    ///< Macro 29 Distortion Shape
            M30DSH = 705,    ///< Macro 30 Distortion Shape
            MFX4 = 706,      ///< Modulator Fine Detune x4
            CFX4 = 707,      ///< Carrier Fine Detune x4
            EER1 = 708,      ///< Effects Echo Delay 1 Reversed
            EER2 = 709,      ///< Effects Echo Delay 2 Reversed
            ETDTYP = 710,    ///< Effects Tape Distortion Type
            ETEND = 711,     ///< Effects Tape Move to End of Chain

            PARAM_ID_COUNT = 712,
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
            UNDEFINED_20 =              Midi::UNDEFINED_20,         ///< Undefined (CC 88)
            UNDEFINED_21 =              Midi::UNDEFINED_21,         ///< Undefined (CC 89)
            UNDEFINED_22 =              Midi::UNDEFINED_22,         ///< Undefined (CC 90)
            FX_1 =                      Midi::FX_1,                 ///< Effect 1 (CC 91)
            FX_2 =                      Midi::FX_2,                 ///< Effect 2 (CC 92)
            FX_3 =                      Midi::FX_3,                 ///< Effect 3 (CC 93)
            FX_4 =                      Midi::FX_4,                 ///< Effect 4 (CC 94)
            FX_5 =                      Midi::FX_5,                 ///< Effect 5 (CC 95)
            UNDEFINED_23 =              Midi::UNDEFINED_23,         ///< Undefined (CC 102)
            UNDEFINED_24 =              Midi::UNDEFINED_24,         ///< Undefined (CC 103)
            UNDEFINED_25 =              Midi::UNDEFINED_25,         ///< Undefined (CC 104)
            UNDEFINED_26 =              Midi::UNDEFINED_26,         ///< Undefined (CC 105)
            UNDEFINED_27 =              Midi::UNDEFINED_27,         ///< Undefined (CC 106)
            UNDEFINED_28 =              Midi::UNDEFINED_28,         ///< Undefined (CC 107)
            UNDEFINED_29 =              Midi::UNDEFINED_29,         ///< Undefined (CC 108)
            UNDEFINED_30 =              Midi::UNDEFINED_30,         ///< Undefined (CC 109)
            UNDEFINED_31 =              Midi::UNDEFINED_31,         ///< Undefined (CC 110)
            UNDEFINED_32 =              Midi::UNDEFINED_32,         ///< Undefined (CC 111)
            UNDEFINED_33 =              Midi::UNDEFINED_33,         ///< Undefined (CC 112)
            UNDEFINED_34 =              Midi::UNDEFINED_34,         ///< Undefined (CC 113)
            UNDEFINED_35 =              Midi::UNDEFINED_35,         ///< Undefined (CC 114)
            UNDEFINED_36 =              Midi::UNDEFINED_36,         ///< Undefined (CC 115)
            UNDEFINED_37 =              Midi::UNDEFINED_37,         ///< Undefined (CC 116)
            UNDEFINED_38 =              Midi::UNDEFINED_38,         ///< Undefined (CC 117)
            UNDEFINED_39 =              Midi::UNDEFINED_39,         ///< Undefined (CC 118)
            UNDEFINED_40 =              Midi::UNDEFINED_40,         ///< Undefined (CC 119)

            PITCH_WHEEL =               128,                        ///< Pitch Wheel

            TRIGGERED_NOTE =            129,                        ///< Triggered Note
            TRIGGERED_VELOCITY =        130,                        ///< Triggered Note's Velocity

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

            RELEASED_NOTE =             178,                        ///< Released Note
            RELEASED_VELOCITY =         179,                        ///< Released Note's Velocity

            MACRO_21 =                  180,                        ///< Macro 21
            MACRO_22 =                  181,                        ///< Macro 22
            MACRO_23 =                  182,                        ///< Macro 23
            MACRO_24 =                  183,                        ///< Macro 24
            MACRO_25 =                  184,                        ///< Macro 25
            MACRO_26 =                  185,                        ///< Macro 26
            MACRO_27 =                  186,                        ///< Macro 27
            MACRO_28 =                  187,                        ///< Macro 28
            MACRO_29 =                  188,                        ///< Macro 29
            MACRO_30 =                  189,                        ///< Macro 30

            CONTROLLER_ID_COUNT =       190,
            INVALID_CONTROLLER_ID =     CONTROLLER_ID_COUNT,
        };

        static constexpr Byte MODE_MIX_AND_MOD = 0;
        static constexpr Byte MODE_SPLIT_AT_C3 = 1;
        static constexpr Byte MODE_SPLIT_AT_Db3 = 2;
        static constexpr Byte MODE_SPLIT_AT_D3 = 3;
        static constexpr Byte MODE_SPLIT_AT_Eb3 = 4;
        static constexpr Byte MODE_SPLIT_AT_E3 = 5;
        static constexpr Byte MODE_SPLIT_AT_F3 = 6;
        static constexpr Byte MODE_SPLIT_AT_Gb3 = 7;
        static constexpr Byte MODE_SPLIT_AT_G3 = 8;
        static constexpr Byte MODE_SPLIT_AT_Ab3 = 9;
        static constexpr Byte MODE_SPLIT_AT_A3 = 10;
        static constexpr Byte MODE_SPLIT_AT_Bb3 = 11;
        static constexpr Byte MODE_SPLIT_AT_B3 = 12;
        static constexpr Byte MODE_SPLIT_AT_C4 = 13;

        static constexpr int MODES = 14;

        static constexpr Byte NOTE_HANDLING_MONOPHONIC                      = 0b0000;
        static constexpr Byte NOTE_HANDLING_MONOPHONIC_HOLD                 = 0b0001;
        static constexpr Byte NOTE_HANDLING_MONOPHONIC_IGSUS                = 0b0010;
        static constexpr Byte NOTE_HANDLING_MONOPHONIC_HOLD_IGSUS           = 0b0011;
        static constexpr Byte NOTE_HANDLING_POLYPHONIC                      = 0b0100;
        static constexpr Byte NOTE_HANDLING_POLYPHONIC_HOLD                 = 0b0101;
        static constexpr Byte NOTE_HANDLING_POLYPHONIC_IGSUS                = 0b0110;
        static constexpr Byte NOTE_HANDLING_POLYPHONIC_HOLD_IGSUS           = 0b0111;
        static constexpr Byte NOTE_HANDLING_POLYPHONIC_RETRIGGER            = 0b1000;
        static constexpr Byte NOTE_HANDLING_POLYPHONIC_RETRIGGER_HOLD       = 0b1001;
        static constexpr Byte NOTE_HANDLING_POLYPHONIC_RETRIGGER_HOLD_IGSUS = 0b1010;

    private:
        static constexpr Byte NOTE_HANDLING_MASK_HOLD                       = 0b0001;
        static constexpr Byte NOTE_HANDLING_MASK_IGSUS                      = 0b0010;
        static constexpr Byte NOTE_HANDLING_MASK_POLYPHONIC                 = 0b0100;
        static constexpr Byte NOTE_HANDLING_MASK_RETRIGGER                  = 0b1000;
        static constexpr Byte NOTE_HANDLING_MASK_POLY_OR_RETRIG             = 0b1100;

    public:
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

        class ModeParam : public ByteParam
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

        bool is_dirty() const noexcept;
        void clear_dirty_flag() noexcept;

        void suspend() noexcept;
        void resume() noexcept;

        Integer get_active_voices_count() const noexcept;

        bool has_mts_esp_tuning() const noexcept;
        bool has_continuous_mts_esp_tuning() const noexcept;
        bool is_mts_esp_connected() const noexcept;
        void mts_esp_connected() noexcept;
        void mts_esp_disconnected() noexcept;
        NoteTunings& collect_active_notes(Integer& active_notes_count) noexcept;
        void update_note_tuning(NoteTuning const& note_tuning) noexcept;
        void update_note_tunings(NoteTunings const& note_tunings, Integer const count) noexcept;

        bool is_polyphonic() const noexcept;
        bool is_monophonic() const noexcept;
        bool is_holding() const noexcept;
        bool is_retriggering() const noexcept;
        bool is_ignoring_sustain_pedal() const noexcept;

        Sample const* const* generate_samples(
            Integer const round,
            Integer const sample_count,
            Sample const* const* const input = NULL
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
        bool is_lock_free() const noexcept;

        void get_param_id_hash_table_statistics(
            Integer& max_collisions,
            Number& avg_collisions,
            Number& avg_bucket_size
        ) const noexcept;

        Number get_param_value(ParamId const param_id) const noexcept;
#endif

        Number float_param_ratio_to_display_value(
            ParamId const param_id,
            Number const ratio
        ) const noexcept;

        Byte byte_param_ratio_to_display_value(
            ParamId const param_id,
            Number const ratio
        ) const noexcept;

        bool is_discrete_param(ParamId const param_id) const noexcept;

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

        ByteParam note_handling;
        ModeParam mode;
        FloatParamS modulator_add_volume;
        FloatParamS phase_modulation_level;
        FloatParamS frequency_modulation_level;
        FloatParamS amplitude_modulation_level;

        Modulator::Params modulator_params;
        Carrier::Params carrier_params;

        FloatParamS input_volume;

        MidiController pitch_wheel;
        MidiController triggered_note;
        MidiController released_note;
        MidiController triggered_velocity;
        MidiController released_velocity;
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
                    FloatParamS& modulator_add_volume,
                    FloatParamS& input_volume
                ) noexcept;

                virtual ~Bus();

                virtual void set_block_size(
                    Integer const new_block_size
                ) noexcept override;

                void set_input(Sample const* const* const input) noexcept;

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
                ) const noexcept;

                size_t get_active_voices_count() const noexcept;

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
                static constexpr Sample MODULATOR_VOLUME_THRESHOLD = 0.000001;

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

                void mix_modulators_with_additive_volume(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index
                ) noexcept;

                void mix_modulators(
                    Integer const round,
                    Integer const first_sample_index,
                    Integer const last_sample_index
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
                Sample const* const* input;
                Modulator* active_modulators[POLYPHONY];
                Carrier* active_carriers[POLYPHONY];
                size_t active_modulators_count;
                size_t active_carriers_count;
                size_t active_voices_count;
                FloatParamS& modulator_add_volume;
                FloatParamS& input_volume;
                Sample const* modulator_add_volume_buffer;
                Sample const* input_volume_buffer;
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
                static constexpr Integer MULTIPLIER = 805;
                static constexpr Integer SHIFT = 7;

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

        enum ParamType {
            OTHER = 0,
            SAMPLE_EVALUATED_FLOAT = 1,
            BLOCK_EVALUATED_FLOAT = 2,
            BYTE = 3,
            INVALID_PARAM_TYPE = 4,
        };

        static constexpr SPSCQueue<Message>::SizeType MESSAGE_QUEUE_SIZE = 8192;

        static constexpr Number MIDI_WORD_SCALE = 1.0 / 16384.0;
        static constexpr Number MIDI_BYTE_SCALE = 1.0 / 127.0;

        static constexpr Integer INVALID_VOICE = -1;

        static constexpr Integer NOTE_ID_MASK = 0x7fffffff;

        static constexpr Integer BIQUAD_FILTER_SHARED_BUFFERS = 6;

        static std::vector<bool> supported_midi_controllers;

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

        static std::vector<bool> initialize_supported_midi_controllers() noexcept;

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
        void register_param(ParamId const param_id, ParamClass& param) noexcept;

        ParamType find_param_type(ParamId const param_id) const noexcept;

        bool may_be_controllable(
            ParamId const param_id,
            ParamType const type
        ) const noexcept;

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

        void handle_set_param_smoothly(
            ParamId const param_id,
            Number const ratio
        ) noexcept;

        void handle_assign_controller(
            ParamId const param_id,
            Byte const controller_id
        ) noexcept;

        void handle_refresh_param(ParamId const param_id) noexcept;

        void handle_clear() noexcept;

        bool assign_controller_to_byte_param(
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

        void clear_note_stack() noexcept;

        void reset_voice_statuses() noexcept;
        void update_voice_statuses() noexcept;

        void clear_voice_status(
            Midi::Channel const channel,
            Midi::Note const note
        ) noexcept;

        void set_voice_status_flag(
            Midi::Channel const channel,
            Midi::Note const note,
            Byte const flag
        ) noexcept;

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

        template<bool retrigger>
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

        void release_held_notes(Seconds const time_offset) noexcept;

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
        FloatParamS* sample_evaluated_float_params[ParamId::PARAM_ID_COUNT];
        FloatParamB* block_evaluated_float_params[ParamId::PARAM_ID_COUNT];
        ByteParam* byte_params[ParamId::PARAM_ID_COUNT];
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
        std::atomic<Integer> active_voices_count;
        Integer samples_since_gc;
        Integer samples_between_gc;
        Integer next_voice;
        Integer next_note_id;
        Midi::Note previous_note;
        bool is_learning;
        bool is_sustain_pedal_on;
        bool is_polyphonic_;
        bool is_holding_;
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
