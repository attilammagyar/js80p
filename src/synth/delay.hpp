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

#ifndef JS80P__SYNTH__DELAY_HPP
#define JS80P__SYNTH__DELAY_HPP

#include <string>

#include "js80p.hpp"

#include "synth/filter.hpp"
#include "synth/biquad_filter.hpp"
#include "synth/param.hpp"
#include "synth/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class Delay : public Filter<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        Delay(InputSignalProducerClass& input) noexcept;
        Delay(
            InputSignalProducerClass& input,
            FloatParam& gain_leader,
            FloatParam& time_leader
        ) noexcept;
        Delay(
            InputSignalProducerClass& input,
            FloatParam& gain_leader,
            Seconds const time
        ) noexcept;
        virtual ~Delay();

        virtual void set_block_size(Integer const new_block_size) noexcept override;
        virtual void set_sample_rate(Frequency const new_sample_rate) noexcept override;
        virtual void reset() noexcept override;

        /**
         * \warning The number of channels of the \c feedback \c SignalProducer
         *          must be the same as the \c input.
         */
        void set_feedback_signal_producer(
            SignalProducer const* feedback_signal_producer
        ) noexcept;

        FloatParam gain;
        FloatParam time;

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
        void initialize_instance() noexcept;

        void reallocate_delay_buffer() noexcept;
        void free_delay_buffer() noexcept;
        void allocate_delay_buffer() noexcept;

        void initialize_feedback(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        void merge_inputs_into_delay_buffer(Integer const sample_count) noexcept;
        void copy_input_into_delay_buffer(Integer const sample_count) noexcept;
        void mix_feedback_into_delay_buffer(Integer const sample_count) noexcept;

        void apply_gain(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

        SignalProducer const* feedback_signal_producer;
        Sample const* const* feedback_signal_producer_buffer;
        Sample** delay_buffer;
        Sample const* gain_buffer;
        Sample const* time_buffer;
        Number feedback_value;
        Integer feedback_sample_count;
        Integer write_index;
        Integer delay_buffer_size;
        Number delay_buffer_size_float;
        Number delay_buffer_size_inv;
        Number read_index;
};

}

#endif
