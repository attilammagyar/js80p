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

#ifndef JS80P__DSP__CHORUS_HPP
#define JS80P__DSP__CHORUS_HPP

#include <cstddef>

#include "js80p.hpp"

#include "dsp/biquad_filter.hpp"
#include "dsp/delay.hpp"
#include "dsp/effect.hpp"
#include "dsp/gain.hpp"
#include "dsp/lfo.hpp"
#include "dsp/mixer.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class Chorus : public Effect<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        static constexpr Byte CHORUS_1  = 0;
        static constexpr Byte CHORUS_2  = 1;
        static constexpr Byte CHORUS_3  = 2;
        static constexpr Byte CHORUS_4  = 3;
        static constexpr Byte CHORUS_5  = 4;
        static constexpr Byte CHORUS_6  = 5;
        static constexpr Byte CHORUS_7  = 6;
        static constexpr Byte CHORUS_8  = 7;
        static constexpr Byte CHORUS_9  = 8;
        static constexpr Byte CHORUS_10 = 9;
        static constexpr Byte CHORUS_11 = 10;
        static constexpr Byte CHORUS_12 = 11;
        static constexpr Byte CHORUS_13 = 12;
        static constexpr Byte CHORUS_14 = 13;
        static constexpr Byte CHORUS_15 = 14;

        typedef BiquadFilter<InputSignalProducerClass> HighPassedInput;
        typedef PannedDelay<HighPassedInput> CombFilter;
        typedef BiquadFilter< Mixer<CombFilter> > HighShelfFilter;

        class TypeParam : public ByteParam
        {
            public:
                explicit TypeParam(std::string const name) noexcept;
        };

        Chorus(std::string const name, InputSignalProducerClass& input);

        void start_lfos(Seconds const time_offset) noexcept;
        void stop_lfos(Seconds const time_offset) noexcept;

        void skip_round_for_lfos(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        TypeParam type;

        FloatParamS delay_time;
        FloatParamS frequency;
        FloatParamS depth;
        FloatParamS feedback;
        FloatParamS damping_frequency;
        FloatParamS damping_gain;
        FloatParamS width;
        FloatParamS high_pass_frequency;
        FloatParamS high_pass_q;

        ToggleParam tempo_sync;
        ToggleParam log_scale_filter_frequencies;
        ToggleParam log_scale_high_pass_q;
        ToggleParam log_scale_lfo_frequency;

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
        class Tuning
        {
            public:
                constexpr Tuning(
                        Number const weight,
                        Number const panning_scale,
                        Number const lfo_phase
                )
                    : weight(weight),
                    panning_scale(panning_scale),
                    lfo_phase(lfo_phase) {}

                Number const weight;
                Number const panning_scale;
                Number const lfo_phase;
        };

        static constexpr Number FEEDBACK_SCALE_INV = (
            1.0 / (Number)Constants::CHORUS_FEEDBACK_SCALE
        );

        /*
        The delay_time parameter controls the maximum of the centered LFOs
        which control the actual delay time of the delay lines, but for the
        chorus effect, we want to control the midpoint of the oscillation
        instead of the maximum.  Thus, the actual delay time range needs to be
        twice as large as the delay time range that we present to the user.
        */
        static constexpr Number DELAY_TIME_MAX = Constants::CHORUS_DELAY_TIME_MAX * 2.0;
        static constexpr Number DELAY_TIME_DEFAULT = Constants::CHORUS_DELAY_TIME_DEFAULT * 2.0;

        static constexpr size_t VOICES = 7;

        static constexpr Tuning TUNINGS[][VOICES] = {
            /* CHORUS_1 */
            {
                {1.0,  1.0, 0.0 / 3.0},
                {1.0, -1.0, 1.0 / 3.0},
                {1.0,  0.0, 2.0 / 3.0},
                {0.0,  1.0, 0.0 / 3.0},
                {0.0,  1.0, 0.0 / 3.0},
                {0.0,  1.0, 0.0 / 3.0},
                {0.0,  1.0, 0.0 / 3.0},
            },

            /* CHORUS_2 */
            {
                {1.0,  0.0, 0.0 / 3.0},
                {0.6, -1.0, 1.0 / 3.0},
                {0.6,  1.0, 2.0 / 3.0},
                {0.0,  1.0, 0.0 / 3.0},
                {0.0,  1.0, 0.0 / 3.0},
                {0.0,  1.0, 0.0 / 3.0},
                {0.0,  1.0, 0.0 / 3.0},
            },

            /* CHORUS_3 */
            {
                {0.6,  0.0, 0.0 / 3.0},
                {1.0, -1.0, 1.0 / 3.0},
                {1.0,  1.0, 2.0 / 3.0},
                {0.0,  1.0, 0.0 / 3.0},
                {0.0,  1.0, 0.0 / 3.0},
                {0.0,  1.0, 0.0 / 3.0},
                {0.0,  1.0, 0.0 / 3.0},
            },

            /* CHORUS_4 */
            {
                {1.0,  0.5, 0.0 / 4.0},
                {1.0, -0.5, 1.0 / 4.0},
                {1.0,  1.0, 2.0 / 4.0},
                {1.0, -1.0, 3.0 / 4.0},
                {0.0,  1.0, 0.0 / 4.0},
                {0.0,  1.0, 0.0 / 4.0},
                {0.0,  1.0, 0.0 / 4.0},
            },

            /* CHORUS_5 */
            {
                {1.0,  0.5, 0.0 / 4.0},
                {1.0, -0.5, 1.0 / 4.0},
                {0.5,  1.0, 2.0 / 4.0},
                {0.5, -1.0, 3.0 / 4.0},
                {0.0,  1.0, 0.0 / 4.0},
                {0.0,  1.0, 0.0 / 4.0},
                {0.0,  1.0, 0.0 / 4.0},
            },

            /* CHORUS_6 */
            {
                {0.5,  0.5, 0.0 / 4.0},
                {0.5, -0.5, 1.0 / 4.0},
                {1.0,  1.0, 2.0 / 4.0},
                {1.0, -1.0, 3.0 / 4.0},
                {0.0,  1.0, 0.0 / 4.0},
                {0.0,  1.0, 0.0 / 4.0},
                {0.0,  1.0, 0.0 / 4.0},
            },

            /* CHORUS_7 */
            {
                {1.0,  0.0, 0.0 / 5.0},
                {1.0,  0.5, 1.0 / 5.0},
                {1.0, -0.5, 2.0 / 5.0},
                {1.0,  1.0, 3.0 / 5.0},
                {1.0, -1.0, 4.0 / 5.0},
                {0.0,  1.0, 0.0 / 5.0},
                {0.0,  1.0, 0.0 / 5.0},
            },

            /* CHORUS_8 */
            {
                {1.0,  0.0, 0.0 / 5.0},
                {0.8,  0.5, 1.0 / 5.0},
                {0.8, -0.5, 2.0 / 5.0},
                {0.5,  1.0, 3.0 / 5.0},
                {0.5, -1.0, 4.0 / 5.0},
                {0.0,  1.0, 0.0 / 5.0},
                {0.0,  1.0, 0.0 / 5.0},
            },

            /* CHORUS_9 */
            {
                {0.5,  0.0, 0.0 / 5.0},
                {0.8,  0.5, 1.0 / 5.0},
                {0.8, -0.5, 2.0 / 5.0},
                {1.0,  1.0, 3.0 / 5.0},
                {1.0, -1.0, 4.0 / 5.0},
                {0.0,  1.0, 0.0 / 5.0},
                {0.0,  1.0, 0.0 / 5.0},
            },

            /* CHORUS_10 */
            {
                {1.0,  0.3, 0.0 / 6.0},
                {1.0, -0.3, 1.0 / 6.0},
                {1.0,  0.6, 2.0 / 6.0},
                {1.0, -0.6, 3.0 / 6.0},
                {1.0,  1.0, 4.0 / 6.0},
                {1.0, -1.0, 5.0 / 6.0},
                {0.0,  1.0, 0.0 / 6.0},
            },

            /* CHORUS_11 */
            {
                {1.0,  0.3, 0.0 / 6.0},
                {1.0, -0.3, 1.0 / 6.0},
                {0.7,  0.6, 2.0 / 6.0},
                {0.7, -0.6, 3.0 / 6.0},
                {0.5,  1.0, 4.0 / 6.0},
                {0.5, -1.0, 5.0 / 6.0},
                {0.0,  1.0, 0.0 / 6.0},
            },

            /* CHORUS_12 */
            {
                {0.5,  0.3, 0.0 / 6.0},
                {0.5, -0.3, 1.0 / 6.0},
                {0.7,  0.6, 2.0 / 6.0},
                {0.7, -0.6, 3.0 / 6.0},
                {1.0,  1.0, 4.0 / 6.0},
                {1.0, -1.0, 5.0 / 6.0},
                {0.0,  1.0, 0.0 / 6.0},
            },

            /* CHORUS_13 */
            {
                {1.0,  0.0, 0.0 / 7.0},
                {1.0, -0.3, 1.0 / 7.0},
                {1.0,  0.3, 2.0 / 7.0},
                {1.0, -0.6, 3.0 / 7.0},
                {1.0,  0.6, 4.0 / 7.0},
                {1.0, -1.0, 5.0 / 7.0},
                {1.0,  1.0, 6.0 / 7.0},
            },

            /* CHORUS_14 */
            {
                {1.0,  0.0, 0.0 / 7.0},
                {0.9, -0.3, 1.0 / 7.0},
                {0.9,  0.3, 2.0 / 7.0},
                {0.7, -0.6, 3.0 / 7.0},
                {0.7,  0.6, 4.0 / 7.0},
                {0.5, -1.0, 5.0 / 7.0},
                {0.5,  1.0, 6.0 / 7.0},
            },

            /* CHORUS_15 */
            {
                {0.5,  0.0, 0.0 / 7.0},
                {0.7, -0.3, 1.0 / 7.0},
                {0.7,  0.3, 2.0 / 7.0},
                {0.9, -0.6, 3.0 / 7.0},
                {0.9,  0.6, 4.0 / 7.0},
                {1.0, -1.0, 5.0 / 7.0},
                {1.0,  1.0, 6.0 / 7.0},
            },
        };

        void update_tunings(Byte const type) noexcept;

        FloatParamS biquad_filter_q;
        typename HighPassedInput::TypeParam high_pass_filter_type;
        FloatParamS high_pass_filter_gain;
        HighPassedInput high_pass_filter;
        LFO lfos[VOICES];
        FloatParamS delay_times[VOICES];
        CombFilter comb_filters[VOICES];
        Mixer<CombFilter> mixer;
        typename HighShelfFilter::TypeParam high_shelf_filter_type;
        HighShelfFilter high_shelf_filter;
        Gain<HighShelfFilter> feedback_gain;
        Sample const* const* chorused;
        Byte previous_type;
        bool should_start_lfos;
};

}

#endif
