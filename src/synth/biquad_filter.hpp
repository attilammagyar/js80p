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
    public:
        typedef Byte Type;

        static constexpr Type LOW_PASS = 0;
        static constexpr Type HIGH_PASS = 1;
        static constexpr Type BAND_PASS = 2;
        static constexpr Type NOTCH = 3;
        static constexpr Type PEAKING = 4;
        static constexpr Type LOW_SHELF = 5;
        static constexpr Type HIGH_SHELF = 6;

        class TypeParam : public Param<Type>
        {
            public:
                TypeParam(std::string const name);
        };

        BiquadFilter(
            std::string const name,
            InputSignalProducerClass& input,
            TypeParam& type,
            Integer const shared_coefficients_group = -1
        );
        BiquadFilter(
            InputSignalProducerClass& input,
            TypeParam& type,
            FloatParam& frequency_leader,
            FloatParam& q_leader,
            FloatParam& gain_leader,
            Integer const shared_coefficients_group = -1
        );
        virtual ~BiquadFilter();

        virtual void set_sample_rate(Frequency const new_sample_rate);
        virtual void set_block_size(Integer const new_block_size);

        void clear();

        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        );

        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        );

        FloatParam frequency;
        FloatParam q;
        FloatParam gain;
        TypeParam& type;

    private:
        static constexpr Number THRESHOLD = 0.000001;
        static constexpr Integer GROUPS = 4;

        static Integer shared_buffers_rounds[GROUPS];
        static Sample const* shared_b0_buffers[GROUPS];
        static Sample const* shared_b1_buffers[GROUPS];
        static Sample const* shared_b2_buffers[GROUPS];
        static Sample const* shared_a1_buffers[GROUPS];
        static Sample const* shared_a2_buffers[GROUPS];
        static bool shared_are_coefficients_constant[GROUPS];
        static bool shared_is_silent[GROUPS];
        static bool shared_is_no_op[GROUPS];

        void initialize_instance();
        void register_children();

        void reallocate_buffers();
        void allocate_buffers();
        void free_buffers();

        Sample const* const* initialize_rendering_with_shared_coefficients(
            Integer const round,
            Integer const sample_count
        );

        bool initialize_low_pass_rendering(
            Integer const round,
            Integer const sample_count
        );
        void store_low_pass_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const q_value
        );

        bool initialize_high_pass_rendering(
            Integer const round,
            Integer const sample_count
        );
        void store_high_pass_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const q_value
        );

        bool initialize_low_shelf_rendering(
            Integer const round,
            Integer const sample_count
        );
        void store_low_shelf_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const gain_value
        );

        bool initialize_high_shelf_rendering(
            Integer const round,
            Integer const sample_count
        );
        void store_high_shelf_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const gain_value
        );

        bool initialize_band_pass_rendering(
            Integer const round,
            Integer const sample_count
        );
        void store_band_pass_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const q_value
        );

        bool initialize_notch_rendering(
            Integer const round,
            Integer const sample_count
        );
        void store_notch_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const q_value
        );

        bool initialize_peaking_rendering(
            Integer const round,
            Integer const sample_count
        );
        void store_peaking_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const q_value,
            Number const gain_value
        );

        void store_gain_coefficient_samples(
            Integer const index,
            Number const gain_value
        );

        void store_normalized_coefficient_samples(
            Integer const index,
            Sample const b0,
            Sample const b1,
            Sample const b2,
            Sample const a0,
            Sample const a1,
            Sample const a2
        );

        void store_no_op_coefficient_samples(Integer const index);
        void store_silent_coefficient_samples(Integer const index);

        Integer const shared_coefficients_group;

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
