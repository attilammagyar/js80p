/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2025  Attila M. Magyar
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

#ifndef JS80P__DSP__TAPE_HPP
#define JS80P__DSP__TAPE_HPP

#include <cstddef>
#include <string>

#include "js80p.hpp"

#include "dsp/biquad_filter.hpp"
#include "dsp/delay.hpp"
#include "dsp/distortion.hpp"
#include "dsp/filter.hpp"
#include "dsp/lfo.hpp"
#include "dsp/macro.hpp"
#include "dsp/math.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

class TapeParams
{
    public:
        enum State {
            TAPE_STATE_INIT = 0,
            TAPE_STATE_NORMAL = 1,
            TAPE_STATE_STOPPING = 2,
            TAPE_STATE_STOPPED = 3,
            TAPE_STATE_STARTABLE = 4,
            TAPE_STATE_STARTING = 5,
            TAPE_STATE_STARTED = 6,
            TAPE_STATE_FF_STARTABLE = 7,
            TAPE_STATE_FF_STARTING = 8,
            TAPE_STATE_FF_STARTED = 9,
        };

        static constexpr Number DELAY_TIME_MAX = 30.0;
        static constexpr Number DELAY_TIME_LFO_RANGE = DELAY_TIME_MAX / 310000.0;
        static constexpr size_t SIGNAL_PRODUCERS = 8 + 14 * Macro::PARAMS;

        explicit TapeParams(
            std::string const& name,
            ToggleParam& bypass_toggle
        ) noexcept;

        void start_lfos(Seconds const time_offset) noexcept;
        void stop_lfos(Seconds const time_offset) noexcept;

        void skip_round_for_lfos(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        SignalProducer* get_signal_producer(size_t const n) const noexcept;

        FloatParamB stop_start;
        FloatParamB& wnf_amp;
        FloatParamB& wnf_speed;
        FloatParamS distortion_level;
        FloatParamB& color;
        FloatParamB hiss_level;
        Distortion::TypeParam distortion_type;
        ToggleParam& bypass_toggle;

        FloatParamS volume;

        LFO delay_time_lfo;
        LFO wow_lfo;
        LFO flutter_lfo;
        Macro wnf_amp_macro;
        Macro wnf_amp_sharp_smooth_macro;
        Macro wnf_amp_smooth_sharp_macro;
        Macro wnf_speed_macro;
        Macro wnf_speed_delay_time_lfo_macro;
        Macro wnf_speed_wow_lfo_macro;
        Macro wnf_speed_flutter_lfo_macro;
        Macro color_macro;
        Macro high_shelf_filter_frequency_macro;
        Macro high_shelf_filter_gain_macro;
        Macro offset_below_midpoint;
        Macro offset_above_midpoint;
        Macro distance_from_midpoint;
        Macro low_pass_filter_frequency_macro;

    private:
        void store_signal_producers_from_macro(Macro& macro, size_t& i) noexcept;

        SignalProducer* signal_producers[SIGNAL_PRODUCERS];
};


template<class InputSignalProducerClass, Byte required_bypass_toggle_value = ToggleParam::ON>
class Tape : public Filter<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        template<class HissInputSignalProducerClass>
        class HissGenerator : public Filter<HissInputSignalProducerClass>
        {
            friend class SignalProducer;

            public:
                HissGenerator(
                    HissInputSignalProducerClass& input,
                    FloatParamB& level
                ) noexcept;

                virtual ~HissGenerator();

                virtual void set_sample_rate(
                    Frequency const new_sample_rate
                ) noexcept override;

                virtual void reset() noexcept override;

                FloatParamB& level;

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
                void update_filter_coefficients() noexcept;

                Math::RNG rng;
                Sample* r_n_m1;
                Sample* x_n_m1;
                Sample* y_n_m1;
                Sample a;
                Sample w1;
                Sample w2;
        };

        typedef Distortion::Distortion<InputSignalProducerClass> Distortion_;

        typedef HissGenerator<Distortion_> HissGenerator_;

        typedef BiquadFilter<
            HissGenerator_,
            BiquadFilterFixedType::BFFT_HIGH_SHELF
        > HighShelfFilter;

        typedef BiquadFilter<
            HighShelfFilter,
            BiquadFilterFixedType::BFFT_LOW_PASS
        > LowPassFilter;

        typedef Delay<LowPassFilter> Delay_;

        Tape(
            std::string const& name,
            TapeParams& params,
            InputSignalProducerClass& input
        ) noexcept;

        virtual void reset() noexcept override;

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

    public:
        static constexpr Number DELAY_TIME_MAX_INV = (
            1.0 / TapeParams::DELAY_TIME_MAX
        );

        static constexpr Seconds STOP_TIME_MIN = 0.15;
        static constexpr Seconds START_TIME_MIN = 0.05;
        static constexpr Seconds START_STOP_DELAY = 0.1;

        static Sample distort_volume(Sample const volume_level) noexcept;

        Sample const* const* initialize_init_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        Sample const* const* initialize_normal_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        Sample const* const* initialize_stopping_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        Sample const* const* initialize_stopped_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        Sample const* const* initialize_startable_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        Sample const* const* initialize_starting_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        Sample const* const* initialize_started_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        Sample const* const* initialize_ff_startable_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        Sample const* const* initialize_ff_starting_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        Sample const* const* initialize_ff_started_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        bool is_bypassable() const noexcept;

        void schedule_stop(Seconds const duration) noexcept;
        void schedule_start() noexcept;
        void schedule_fast_forward_start(Seconds const duration) noexcept;

        TapeParams& params;
        Distortion_ distortion;
        HissGenerator_ hiss_generator;
        HighShelfFilter high_shelf_filter;
        LowPassFilter low_pass_filter;
        Delay_ delay;
        Sample const* volume_buffer;
        Sample const* const* delay_output;
        Seconds transition_duration;
        TapeParams::State state;
        Byte previous_bypass_toggle_value;
};

}

#endif

