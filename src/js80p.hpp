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

#ifndef JS80P__JS80P_HPP
#define JS80P__JS80P_HPP

#include <stdint.h>


#if defined(__GNUC__) || defined(__clang__)
  #define LIKELY(condition) __builtin_expect((condition), 1)
  #define UNLIKELY(condition) __builtin_expect((condition), 0)
#else
  #define LIKELY(condition) (condition)
  #define UNLIKELY(condition) (condition)
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
    constexpr char const* PLUGIN_VERSION = "1.0.0";

    constexpr Integer PARAM_NAME_MAX_LENGTH = 8;

    constexpr Number AM_DEFAULT = 0.0;
    constexpr Number AM_MAX = 3.0;
    constexpr Number AM_MIN = 0.0;

    constexpr Number BIQUAD_FILTER_FREQUENCY_DEFAULT = 24000.0;
    constexpr Number BIQUAD_FILTER_FREQUENCY_MAX = 24000.0;
    constexpr Number BIQUAD_FILTER_FREQUENCY_MIN = 1.0;

    constexpr Number BIQUAD_FILTER_GAIN_DEFAULT = 0.0;
    constexpr Number BIQUAD_FILTER_GAIN_MAX = 24.0;
    constexpr Number BIQUAD_FILTER_GAIN_MIN = -48.0;
    constexpr Number BIQUAD_FILTER_GAIN_SCALE = 1.0 / 40.0;

    constexpr Number BIQUAD_FILTER_Q_DEFAULT = 1.0;
    constexpr Number BIQUAD_FILTER_Q_MAX = 30.0;
    constexpr Number BIQUAD_FILTER_Q_MIN = 0.0;
    constexpr Number BIQUAD_FILTER_Q_SCALE = 1.0 / 20.0;

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
