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

#ifndef JS80P__DSP__DELAY_HPP
#define JS80P__DSP__DELAY_HPP

#include <string>

#include "js80p.hpp"

#include "dsp/filter.hpp"
#include "dsp/biquad_filter.hpp"
#include "dsp/distortion.hpp"
#include "dsp/math.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class Delay : public Filter<InputSignalProducerClass>
{
    friend class SignalProducer;

    private:
        static constexpr Integer OVERSIZE_DELAY_BUFFER_FOR_TEMPO_SYNC = 2;

    public:
        static constexpr Number BPM_MIN = (
            Math::SECONDS_IN_ONE_MINUTE / (Number)OVERSIZE_DELAY_BUFFER_FOR_TEMPO_SYNC
        );

        explicit Delay(
            InputSignalProducerClass& input,
            ToggleParam const* tempo_sync = NULL
        ) noexcept;

        Delay(
            InputSignalProducerClass& input,
            FloatParamS& time_leader,
            ToggleParam const* tempo_sync = NULL
        ) noexcept;

        Delay(
            InputSignalProducerClass& input,
            FloatParamS& gain_leader,
            FloatParamS& time_leader,
            ToggleParam const* tempo_sync = NULL
        ) noexcept;

        Delay(
            InputSignalProducerClass& input,
            FloatParamS& gain_leader,
            Seconds const time,
            Seconds const time_max,
            ToggleParam const* tempo_sync = NULL
        ) noexcept;

        virtual ~Delay();

        virtual void set_block_size(Integer const new_block_size) noexcept override;
        virtual void set_sample_rate(Frequency const new_sample_rate) noexcept override;
        virtual void reset() noexcept override;

        /**
         * \warning The number of channels of the \c feedback \c SignalProducer
         *          must be the same as the \c input, and the feedback signal
         *          producer must follow the \c Delay object in the signal
         *          chain, and the \c Delay's signal must be rendered before
         *          the signal of the feedback object is rendered (so that the
         *          \c Delay can use the previously rendered block of the
         *          feedback object).
         */
        void set_feedback_signal_producer(
            SignalProducer* feedback_signal_producer
        ) noexcept;

        void use_shared_delay_buffer(
            Delay<InputSignalProducerClass> const& shared_buffer_owner
        ) noexcept;

        ToggleParam const* const tempo_sync;

        FloatParamS gain;
        FloatParamS time;

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
        enum DelayBufferWritingMode {
            CLEAR = 0,
            ADD = 1,
        };

        void initialize_instance() noexcept;

        void reallocate_delay_buffer_if_needed() noexcept;
        void free_delay_buffer() noexcept;
        void allocate_delay_buffer() noexcept;

        template<bool is_delay_buffer_shared>
        void clear_delay_buffer(Integer const sample_count) noexcept;

        template<bool is_delay_buffer_shared>
        void mix_feedback_into_delay_buffer(Integer const sample_count) noexcept;

        template<DelayBufferWritingMode mode>
        Integer write_delay_buffer(
            Sample const* const* source_buffer,
            Integer const delay_buffer_index,
            Integer const sample_count
        ) noexcept;

        template<bool is_delay_buffer_shared>
        void mix_input_into_delay_buffer(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        Integer advance_delay_buffer_index(
            Integer const position,
            Integer const increment
        ) const noexcept;

        bool is_delay_buffer_silent() const noexcept;

        template<bool need_gain, bool is_gain_constant>
        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer,
            Sample const gain
        ) noexcept;

        Integer const delay_buffer_oversize;
        bool const is_gain_constant_1;

        Delay<InputSignalProducerClass> const* shared_buffer_owner;

        SignalProducer* feedback_signal_producer;
        Sample** delay_buffer;
        Sample const* gain_buffer;
        Sample const* time_buffer;
        Sample time_scale;
        Number feedback_value;
        Integer write_index_input;
        Integer silent_input_samples;
        Integer write_index_feedback;
        Integer silent_feedback_samples;
        Integer read_index;
        Integer clear_index;
        Integer delay_buffer_size;
        Integer previous_round;
        Number delay_buffer_size_float;
        bool is_starting;
        bool need_gain;
        bool need_to_render_silence;
};


enum PannedDelayStereoMode {
    NORMAL = 0,
    FLIPPED = 1
};


template<class InputSignalProducerClass, class FilterInputClass = Delay<InputSignalProducerClass> >
class PannedDelay : public Filter<FilterInputClass>
{
    friend class SignalProducer;

    public:
        PannedDelay(
            InputSignalProducerClass& input,
            PannedDelayStereoMode const stereo_mode,
            ToggleParam const* tempo_sync = NULL
        );

        PannedDelay(
            InputSignalProducerClass& input,
            PannedDelayStereoMode const stereo_mode,
            FloatParamS& delay_time_leader,
            ToggleParam const* tempo_sync = NULL
        );

        PannedDelay(
            InputSignalProducerClass& input,
            PannedDelayStereoMode const stereo_mode,
            FloatParamS& panning_leader,
            FloatParamS& delay_time_leader,
            ToggleParam const* tempo_sync = NULL,
            Integer const number_of_children = 0
        );

        virtual ~PannedDelay();

        virtual void set_block_size(Integer const new_block_size) noexcept override;

        void set_panning_scale(Number const scale) noexcept;

    protected:
        PannedDelay(
            InputSignalProducerClass& delay_input,
            FilterInputClass& filter_input,
            PannedDelayStereoMode const stereo_mode,
            ToggleParam const* tempo_sync = NULL,
            Integer const number_of_children = 0
        );

        PannedDelay(
            InputSignalProducerClass& delay_input,
            FilterInputClass& filter_input,
            PannedDelayStereoMode const stereo_mode,
            FloatParamS& panning_leader,
            FloatParamS& delay_time_leader,
            ToggleParam const* tempo_sync = NULL,
            Integer const number_of_children = 0
        );

        PannedDelay(
            InputSignalProducerClass& delay_input,
            FilterInputClass& filter_input,
            PannedDelayStereoMode const stereo_mode,
            FloatParamS& panning_leader,
            FloatParamS& delay_gain_leader,
            FloatParamS& delay_time_leader,
            ToggleParam const* tempo_sync = NULL,
            Integer const number_of_children = 0
        );

        PannedDelay(
            InputSignalProducerClass& delay_input,
            FilterInputClass& filter_input,
            PannedDelayStereoMode const stereo_mode,
            FloatParamS& panning_leader,
            FloatParamS& delay_gain_leader,
            Seconds const delay_time,
            Seconds const delay_time_max,
            ToggleParam const* tempo_sync = NULL,
            Integer const number_of_children = 0
        );

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
        static constexpr Integer NUMBER_OF_CHILDREN = 2;

        void initialize_instance() noexcept;

        template<int channel_1, int channel_2>
        void render_with_constant_panning(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        template<int channel_1, int channel_2>
        void render_with_changing_panning(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        bool const is_flipped;

        Sample** stereo_gain_buffer;
        Sample** panning_buffer_scaled;
        Sample const* panning_buffer;
        Sample stereo_gain_value[2];
        Sample panning_value;
        Number panning_scale;

    public:
        FloatParamS panning;

        Delay<InputSignalProducerClass> delay;
};


template<class InputSignalProducerClass>
using DistortedDelay = Distortion::Distortion< Delay<InputSignalProducerClass> >;


template<class InputSignalProducerClass>
using DistortedHighShelfDelay = BiquadFilter< DistortedDelay<InputSignalProducerClass> >;


template<class InputSignalProducerClass>
using DistortedHighShelfPannedDelayBase = PannedDelay<
    InputSignalProducerClass, DistortedHighShelfDelay<InputSignalProducerClass>
>;


template<class InputSignalProducerClass>
class DistortedHighShelfPannedDelay : public DistortedHighShelfPannedDelayBase<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        DistortedHighShelfPannedDelay(
            InputSignalProducerClass& input,
            PannedDelayStereoMode const stereo_mode,
            FloatParamS& distortion_level_leader,
            Distortion::TypeParam const& distortion_type,
            ToggleParam const* tempo_sync = NULL
        );

        DistortedHighShelfPannedDelay(
            InputSignalProducerClass& input,
            PannedDelayStereoMode const stereo_mode,
            FloatParamS& panning_leader,
            FloatParamS& delay_gain_leader,
            FloatParamS& delay_time_leader,
            BiquadFilterSharedBuffers& high_shelf_filter_shared_buffers,
            FloatParamS& high_shelf_filter_frequency_leader,
            FloatParamS& high_shelf_filter_gain_leader,
            FloatParamS& distortion_level_leader,
            Distortion::TypeParam const& distortion_type,
            ToggleParam const* tempo_sync = NULL
        );

        DistortedHighShelfPannedDelay(
            InputSignalProducerClass& input,
            PannedDelayStereoMode const stereo_mode,
            FloatParamS& panning_leader,
            FloatParamS& delay_gain_leader,
            Seconds const delay_time,
            Seconds const delay_time_max,
            BiquadFilterSharedBuffers& high_shelf_filter_shared_buffers,
            FloatParamS& high_shelf_filter_frequency_leader,
            FloatParamS& high_shelf_filter_gain_leader,
            FloatParamS& distortion_level_leader,
            Distortion::TypeParam const& distortion_type,
            ToggleParam const* tempo_sync = NULL
        );

    private:
        static constexpr Integer NUMBER_OF_CHILDREN = 4;

        void initialize_instance() noexcept;

        typename DistortedHighShelfDelay<InputSignalProducerClass>::TypeParam high_shelf_filter_type;
        FloatParamS high_shelf_filter_q;
        DistortedDelay<InputSignalProducerClass> distortion;

    public:
        DistortedHighShelfDelay<InputSignalProducerClass> high_shelf_filter;
};

}

#endif
