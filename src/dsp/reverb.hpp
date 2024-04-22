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

#ifndef JS80P__DSP__REVERB_HPP
#define JS80P__DSP__REVERB_HPP

#include <cstddef>

#include "js80p.hpp"

#include "dsp/biquad_filter.hpp"
#include "dsp/delay.hpp"
#include "dsp/mixer.hpp"
#include "dsp/param.hpp"
#include "dsp/side_chain_compressable_effect.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class Reverb : public SideChainCompressableEffect<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        typedef BiquadFilter<InputSignalProducerClass> HighPassedInput;
        typedef DistortedHighShelfPannedDelay<HighPassedInput> CombFilter;

        class TypeParam : public ByteParam
        {
            public:
                explicit TypeParam(std::string const& name) noexcept;
        };

        static constexpr Byte REVERB_1  = 0;
        static constexpr Byte REVERB_2  = 1;
        static constexpr Byte REVERB_3  = 2;
        static constexpr Byte REVERB_4  = 3;
        static constexpr Byte REVERB_5  = 4;
        static constexpr Byte REVERB_6  = 5;
        static constexpr Byte REVERB_7  = 6;
        static constexpr Byte REVERB_8  = 7;
        static constexpr Byte REVERB_9  = 8;
        static constexpr Byte REVERB_10 = 9;

        Reverb(
            std::string const& name,
            InputSignalProducerClass& input,
            BiquadFilterSharedBuffers& high_shelf_filter_shared_buffers
        );

        virtual void reset() noexcept override;

        TypeParam type;
        FloatParamS room_reflectivity;
        FloatParamS damping_frequency;
        FloatParamS damping_gain;
        FloatParamS width;
        FloatParamS high_pass_frequency;
        FloatParamS high_pass_q;
        FloatParamS distortion_level;
        ToggleParam log_scale_frequencies;
        ToggleParam log_scale_high_pass_q;

    protected:
        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

    private:
        class Tuning
        {
            public:
                constexpr Tuning(
                        Seconds const delay_time,
                        Number const weight,
                        Number const panning_scale
                ) : delay_time(delay_time),
                    weight(weight),
                    panning_scale(panning_scale) {}

                Seconds const delay_time;
                Number const weight;
                Number const panning_scale;
        };

        static constexpr size_t COMB_FILTERS = 10;

        static constexpr Seconds DELAY_TIME_MAX = 0.150;

        static constexpr Tuning TUNINGS[][COMB_FILTERS] = {
            /*
            REVERB_1

            Tunings from Freeverb:
            https://ccrma.stanford.edu/~jos/pasp/Freeverb.html
            */
            {
                {1557.0 / 44100.0,  1.000000,        1.000000},
                {1617.0 / 44100.0,  1.000000,       -1.000000},
                {1491.0 / 44100.0,  1.000000,        1.000000},
                {1422.0 / 44100.0,  1.000000,       -1.000000},
                {1277.0 / 44100.0,  1.000000,        1.000000},
                {1356.0 / 44100.0,  1.000000,       -1.000000},
                {1188.0 / 44100.0,  1.000000,        1.000000},
                {1116.0 / 44100.0,  1.000000,       -1.000000},
                {0.000000,          0.000000,        1.000000},
                {0.000000,          0.000000,        1.000000},
            },

            /* REVERB_2: Freeverb variant */
            {
                {1557.0 / 44100.0,  0.640000,        1.000000},
                {1617.0 / 44100.0,  0.600000,       -1.000000},
                {1491.0 / 44100.0,  0.680000,        0.800000},
                {1422.0 / 44100.0,  0.700000,       -0.800000},
                {1277.0 / 44100.0,  0.860000,        0.600000},
                {1356.0 / 44100.0,  0.800000,       -0.600000},
                {1188.0 / 44100.0,  0.980000,        0.400000},
                {1116.0 / 44100.0,  1.000000,       -0.400000},
                {0.000000,          0.000000,        1.000000},
                {0.000000,          0.000000,        1.000000},
            },

            /* REVERB_3: Another Freeverb variant */
            {
                {1557.0 / 44100.0,  0.920000,       -1.000000},
                {1617.0 / 44100.0,  1.000000,        1.000000},
                {1491.0 / 44100.0,  0.870000,       -0.860000},
                {1422.0 / 44100.0,  0.770000,        0.860000},
                {1277.0 / 44100.0,  0.700000,       -0.700000},
                {1356.0 / 44100.0,  0.820000,        0.700000},
                {1188.0 / 44100.0,  0.660000,       -0.500000},
                {1116.0 / 44100.0,  0.600000,        0.500000},
                {0.000000,          0.000000,        1.000000},
                {0.000000,          0.000000,        1.000000},
            },

            /* REVERB_4 */
            {
                {0.052746,          0.793491,       -0.207912},
                {0.052758,          0.793453,        0.258819},
                {0.055550,          0.766887,       -0.453990},
                {0.056172,          0.765007,        0.500000},
                {0.058085,          0.776934,       -0.156434},
                {0.058275,          0.776352,        0.052336},
                {0.058389,          0.776001,       -0.104528},
                {0.063536,          0.709636,        0.629320},
                {0.072847,          0.699951,       -0.965926},
                {0.072971,          0.699609,        0.987688},
            },

            /* REVERB_5 */
            {
                {0.038844,          0.640038,       -0.710799},
                {0.040232,          0.667222,        0.669131},
                {0.046468,          0.795061,       -0.401948},
                {0.046960,          0.805544,        0.453990},
                {0.048794,          0.910060,       -0.056434},
                {0.000000,          0.000000,        1.000000},
                {0.000000,          0.000000,        1.000000},
                {0.000000,          0.000000,        1.000000},
                {0.000000,          0.000000,        1.000000},
                {0.000000,          0.000000,        1.000000},
            },

            /* REVERB_6 */
            {
                {0.051757,          0.641907,        0.353475},
                {0.053229,          0.634489,       -0.299041},
                {0.056125,          0.620141,        0.990024},
                {0.060597,          0.578299,        0.786935},
                {0.067569,          0.566549,        0.893371},
                {0.071181,          0.531922,       -0.883766},
                {0.071525,          0.530480,       -0.958820},
                {0.073242,          0.541726,       -0.218143},
                {0.077545,          0.523624,       -0.182236},
                {0.000000,          0.000000,        1.000000},
            },

            /* REVERB_7 */
            {
                {0.061751,          0.707106,        0.838671},
                {0.063110,          0.713816,       -0.809017},
                {0.093176,          0.871877,       -0.965926},
                {0.096321,          0.889519,        0.994522},
                {0.098140,          0.899828,        0.777146},
                {0.101675,          0.920067,       -0.156434},
                {0.106969,          0.950909,       -0.791007},
                {0.109397,          0.965275,       -0.406737},
                {0.112125,          0.946942,        0.207912},
                {0.114776,          0.997592,        0.358368},
            },

            /* REVERB_8 */
            {
                {0.021123,          0.643971,        0.913545},
                {0.023468,          0.639602,        0.352336},
                {0.023582,          0.638173,       -0.304528},
                {0.024623,          0.637647,        0.891007},
                {0.025433,          0.627532,       -0.912545},
                {0.027962,          0.621009,       -0.891007},
                {0.028202,          0.547230,        0.029320},
                {0.000000,          0.000000,        1.000000},
                {0.000000,          0.000000,        1.000000},
                {0.000000,          0.000000,        1.000000},
            },

            /* REVERB_9 */
            {
                {0.088876,          0.893988,       -0.034899},
                {0.110670,          0.882121,       -0.909961},
                {0.112268,          0.881257,        0.917060},
                {0.114758,          0.879912,       -0.615661},
                {0.117675,          0.878340,        0.008727},
                {0.119764,          0.877216,        0.629320},
                {0.131932,          0.870695,        0.382683},
                {0.135389,          0.868851,       -0.366501},
                {0.142547,          0.816656,        0.898794},
                {0.142686,          0.816586,       -0.927184},
            },

            /* REVERB_10 */
            {
                {0.041589,          0.694236,       -0.444635},
                {0.045160,          0.767454,        0.793353},
                {0.054233,          0.812593,       -0.120137},
                {0.054599,          0.864228,        1.000000},
                {0.059436,          0.924283,       -0.767165},
                {0.073655,          1.000000,        0.849893},
                {0.074854,          1.000000,       -0.669131},
                {0.000000,          0.000000,        1.000000},
                {0.000000,          0.000000,        1.000000},
                {0.000000,          0.000000,        1.000000},
            },
        };

        void update_tunings(Byte const type) noexcept;

        Mixer<CombFilter> mixer;

        typename HighPassedInput::TypeParam high_pass_filter_type;
        FloatParamS high_pass_filter_gain;

        HighPassedInput high_pass_filter;
        CombFilter comb_filters[COMB_FILTERS];
        Byte previous_type;
};

}

#endif
