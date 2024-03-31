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

#ifndef JS80P__JS80P_HPP
#define JS80P__JS80P_HPP

#include <cstdint>

#ifdef JS80P_ASSERTIONS
#include <cassert>
#endif


#ifdef JS80P_ASSERTIONS
#define JS80P_ASSERT(condition) assert(condition)
#else
#define JS80P_ASSERT(condition)
#endif



#define JS80P_TO_STRING(x) _JS80P_EXPAND(x)
#define _JS80P_EXPAND(x) #x


#if defined(__GNUC__) || defined(__clang__)
  #define JS80P_LIKELY(condition) __builtin_expect((condition), 1)
  #define JS80P_UNLIKELY(condition) __builtin_expect((condition), 0)
#else
  #define JS80P_LIKELY(condition) (condition)
  #define JS80P_UNLIKELY(condition) (condition)
#endif


namespace JS80P
{

typedef double Number;
typedef intptr_t Integer;

/*
Aliases for Number to make signatures more informative while avoiding type
conversions.
*/
typedef Number Sample;
typedef Number Seconds;
typedef Number Frequency;

typedef unsigned char Byte;


namespace Constants {
    constexpr char const* COMPANY_NAME = "Attila M. Magyar";
    constexpr char const* COMPANY_WEB = "https://github.com/attilammagyar/js80p";
    constexpr char const* COMPANY_EMAIL = "";

    constexpr char const* PLUGIN_NAME = "JS80P";
    constexpr char const* PLUGIN_VERSION_STR = JS80P_TO_STRING(JS80P_VERSION_STR);
    constexpr int PLUGIN_VERSION_INT = JS80P_VERSION_INT;

    constexpr Integer PARAM_NAME_MAX_LENGTH = 8;

    constexpr Byte ENVELOPES = 12;
    constexpr Byte ENVELOPE_INDEX_MASK = 0x0f;
    constexpr Byte ENVELOPE_INDEX_BITS = 4;
    constexpr Byte INVALID_ENVELOPE_INDEX = ENVELOPES;

    constexpr Byte LFOS = 8;
    constexpr Byte INVALID_LFO_INDEX = LFOS;

    constexpr Number AM_DEFAULT = 0.0;
    constexpr Number AM_MAX = 3.0;
    constexpr Number AM_MIN = 0.0;

    constexpr Number BIQUAD_FILTER_FREQUENCY_DEFAULT = 24000.0;
    constexpr Number BIQUAD_FILTER_FREQUENCY_MAX = 24000.0;
    constexpr Number BIQUAD_FILTER_FREQUENCY_MIN = 1.0;  /* NOTE: this must be greater than 0.0 */

    constexpr Number BIQUAD_FILTER_GAIN_DEFAULT = 0.0;
    constexpr Number BIQUAD_FILTER_GAIN_MAX = 24.0;
    constexpr Number BIQUAD_FILTER_GAIN_MIN = -48.0;
    constexpr Number BIQUAD_FILTER_GAIN_SCALE = 1.0 / 40.0;

    constexpr Number BIQUAD_FILTER_Q_DEFAULT = 1.0;
    constexpr Number BIQUAD_FILTER_Q_MAX = 30.0;
    constexpr Number BIQUAD_FILTER_Q_MIN = 0.0;
    constexpr Number BIQUAD_FILTER_Q_SCALE = 1.0 / 20.0;

    constexpr Number CHORUS_DELAY_TIME_DEFAULT = 0.015625;
    constexpr Number CHORUS_DELAY_TIME_MAX = 1.0;

    constexpr Integer CHORUS_FEEDBACK_SCALE = 4;

    constexpr Number DELAY_FEEDBACK_DEFAULT = 0.75;
    constexpr Number DELAY_FEEDBACK_MIN = 0.0;
    constexpr Number DELAY_FEEDBACK_MAX = 0.999;

    constexpr Number DELAY_GAIN_DEFAULT = 0.5;
    constexpr Number DELAY_GAIN_MIN = 0.0;
    constexpr Number DELAY_GAIN_MAX = 1.0;

    constexpr Number DELAY_TIME_DEFAULT = 0.5;
    constexpr Number DELAY_TIME_MIN = 0.0;
    constexpr Number DELAY_TIME_MAX = 3.0;

    constexpr Number DETUNE_DEFAULT = 0.0;
    constexpr Number DETUNE_MAX = 4800.0;
    constexpr Number DETUNE_MIN = -4800.0;
    constexpr Number DETUNE_SCALE = 1.0 / 100.0;

    constexpr Number FINE_DETUNE_DEFAULT = 0.0;
    constexpr Number FINE_DETUNE_MAX = 1200.0;
    constexpr Number FINE_DETUNE_MIN = -1200.0;

    constexpr Number FM_DEFAULT = 0.0;
    constexpr Number FM_MAX = 4800.0;
    constexpr Number FM_MIN = 0.0;

    constexpr Number FOLD_DEFAULT = 0.0;
    constexpr Number FOLD_TRANSITION = 0.5;
    constexpr Number FOLD_MAX = 5.0 + FOLD_TRANSITION;
    constexpr Number FOLD_MIN = 0.0;

    constexpr Number PM_DEFAULT = 0.0;
    constexpr Number PM_MAX = 5.0;
    constexpr Number PM_MIN = 0.0;
}

}

#endif
