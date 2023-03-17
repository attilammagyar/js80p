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

#ifndef JS80P__SYNTH__BIQUAD_FILTER_HPP
#define JS80P__SYNTH__BIQUAD_FILTER_HPP

#include <cmath>

#include "js80p.hpp"

#include "synth/filter.hpp"
#include "synth/param.hpp"
#include "synth/signal_producer.hpp"


namespace JS80P
{

template<class InputSignalProducerClass>
class BiquadFilter;


typedef BiquadFilter<SignalProducer> SimpleBiquadFilter;


template<class InputSignalProducerClass>
class BiquadFilter : public Filter<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        typedef Byte Type;

        static constexpr Type LOW_PASS = 0;
        static constexpr Type HIGH_PASS = 1;
        static constexpr Type BAND_PASS = 2;
        static constexpr Type NOTCH = 3;
        static constexpr Type PEAKING = 4;
        static constexpr Type LOW_SHELF = 5;
        static constexpr Type HIGH_SHELF = 6;

        enum Unicity
        {
            UNIQUE = 0, ///< Filter instance should not share its calculated
                        ///< coefficients with other instances.

            CLONED = 1, ///< Filter instance should share its calculated
                        ///< coefficients with other instances that are using
                        ///< the same set of parameter leaders.
        };

        class TypeParam : public Param<Type>
        {
            public:
                TypeParam(std::string const name) noexcept;
        };

        BiquadFilter(
            std::string const name,
            InputSignalProducerClass& input,
            TypeParam& type,
            Unicity const unicity = Unicity::UNIQUE
        ) noexcept;
        BiquadFilter(
            InputSignalProducerClass& input,
            TypeParam& type,
            FloatParam& frequency_leader,
            FloatParam& q_leader,
            FloatParam& gain_leader,
            Unicity const unicity = Unicity::UNIQUE
        ) noexcept;
        virtual ~BiquadFilter();

        virtual void set_sample_rate(
            Frequency const new_sample_rate
        ) noexcept override;
        virtual void set_block_size(
            Integer const new_block_size
        ) noexcept override;

        void clear() noexcept;

        FloatParam frequency;
        FloatParam q;
        FloatParam gain;
        TypeParam& type;

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
        static constexpr Number DB_TO_LINEAR_GAIN_SCALE = 1.0 / 20.0;
        static constexpr Number FREQUENCY_SINE_SCALE = std::sqrt(2.0);
        static constexpr Number GAIN_SCALE_HALF = (
            Constants::BIQUAD_FILTER_GAIN_SCALE / 2.0
        );
        static constexpr Number THRESHOLD = 0.000001;

        static Integer shared_buffers_round;
        static Sample const* shared_b0_buffer;
        static Sample const* shared_b1_buffer;
        static Sample const* shared_b2_buffer;
        static Sample const* shared_a1_buffer;
        static Sample const* shared_a2_buffer;
        static bool shared_are_coefficients_constant;
        static bool shared_is_silent;
        static bool shared_is_no_op;

        void initialize_instance() noexcept;
        void register_children() noexcept;

        void reallocate_buffers() noexcept;
        void allocate_buffers() noexcept;
        void free_buffers() noexcept;

        Sample const* const* initialize_rendering_with_shared_coefficients(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        bool initialize_low_pass_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;
        void store_low_pass_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const q_value
        ) noexcept;

        bool initialize_high_pass_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;
        void store_high_pass_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const q_value
        ) noexcept;

        bool initialize_low_shelf_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;
        void store_low_shelf_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const gain_value
        ) noexcept;

        bool initialize_high_shelf_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;
        void store_high_shelf_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const gain_value
        ) noexcept;

        bool initialize_band_pass_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;
        void store_band_pass_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const q_value
        ) noexcept;

        bool initialize_notch_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;
        void store_notch_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const q_value
        ) noexcept;

        bool initialize_peaking_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;
        void store_peaking_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const q_value,
            Number const gain_value
        ) noexcept;

        void store_gain_coefficient_samples(
            Integer const index,
            Number const gain_value
        ) noexcept;

        void store_normalized_coefficient_samples(
            Integer const index,
            Sample const b0,
            Sample const b1,
            Sample const b2,
            Sample const a0,
            Sample const a1,
            Sample const a2
        ) noexcept;

        void store_no_op_coefficient_samples(Integer const index) noexcept;
        void store_silent_coefficient_samples(Integer const index) noexcept;

        bool const has_clones;

        /*
        Notation:
           https://www.w3.org/TR/webaudio/#filters-characteristics
           https://www.w3.org/TR/2021/NOTE-audio-eq-cookbook-20210608/
         */
        Sample* b0_buffer;
        Sample* b1_buffer;
        Sample* b2_buffer;
        Sample* a1_buffer;
        Sample* a2_buffer;

        Sample* x_n_m1;
        Sample* x_n_m2;
        Sample* y_n_m1;
        Sample* y_n_m2;

        Sample w0_scale;

        Number low_pass_no_op_frequency;

        bool is_silent;
        bool are_coefficients_constant;
        bool can_use_shared_coefficients;
};

}

#endif
