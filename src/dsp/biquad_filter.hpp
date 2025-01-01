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

#ifndef JS80P__DSP__BIQUAD_FILTER_HPP
#define JS80P__DSP__BIQUAD_FILTER_HPP

#include <cmath>

#include "js80p.hpp"

#include "dsp/filter.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

enum BiquadFilterFixedType {
    BFFT_CUSTOMIZABLE = 0,
    BFFT_HIGH_PASS = 1,
    BFFT_HIGH_SHELF = 2,
    BFFT_LOW_PASS = 3,
};


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type = BiquadFilterFixedType::BFFT_CUSTOMIZABLE>
class BiquadFilter;


typedef BiquadFilter<SignalProducer> SimpleBiquadFilter;


class BiquadFilterSharedBuffers
{
    public:
        BiquadFilterSharedBuffers();

        Integer round;
        Sample* b0_buffer;
        Sample* b1_buffer;
        Sample* b2_buffer;
        Sample* a1_buffer;
        Sample* a2_buffer;
        bool are_coefficients_constant;
        bool is_silent;
        bool is_no_op;
};


class BiquadFilterTypeParam : public ByteParam
{
    public:
        explicit BiquadFilterTypeParam(std::string const& name) noexcept;
};


template<class InputSignalProducerClass, BiquadFilterFixedType fixed_type>
class BiquadFilter : public Filter<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        static constexpr Byte LOW_PASS = 0;
        static constexpr Byte HIGH_PASS = 1;
        static constexpr Byte BAND_PASS = 2;
        static constexpr Byte NOTCH = 3;
        static constexpr Byte PEAKING = 4;
        static constexpr Byte LOW_SHELF = 5;
        static constexpr Byte HIGH_SHELF = 6;

        BiquadFilter(
            std::string const& name,
            InputSignalProducerClass& input,
            BiquadFilterTypeParam& type,
            BiquadFilterSharedBuffers* shared_buffers = NULL,
            Number const inaccuracy_seed = 0.0,
            FloatParamB const* freq_inaccuracy_param = NULL,
            FloatParamB const* q_inaccuracy_param = NULL,
            SignalProducer* const buffer_owner = NULL
        ) noexcept;

        BiquadFilter(
            std::string const& name,
            InputSignalProducerClass& input,
            SignalProducer* const buffer_owner = NULL
        ) noexcept;

        BiquadFilter(
            std::string const& name,
            InputSignalProducerClass& input,
            BiquadFilterTypeParam& type,
            ToggleParam const& freq_log_scale_toggle,
            ToggleParam const& q_log_scale_toggle,
            SignalProducer* const buffer_owner = NULL
        ) noexcept;

        BiquadFilter(
            InputSignalProducerClass& input,
            BiquadFilterTypeParam& type,
            FloatParamS& frequency_leader,
            FloatParamS& q_leader,
            FloatParamS& gain_leader,
            BiquadFilterSharedBuffers* shared_buffers = NULL,
            Number const inaccuracy_seed = 0.0,
            FloatParamB const* freq_inaccuracy_param = NULL,
            FloatParamB const* q_inaccuracy_param = NULL,
            SignalProducer* buffer_owner = NULL
        ) noexcept;

        BiquadFilter(
            InputSignalProducerClass& input,
            FloatParamS& frequency_leader,
            FloatParamS& q_leader,
            FloatParamS& gain_leader,
            BiquadFilterSharedBuffers* shared_buffers = NULL,
            Number const inaccuracy_seed = 0.0,
            FloatParamB const* freq_inaccuracy_param = NULL,
            FloatParamB const* q_inaccuracy_param = NULL,
            SignalProducer* buffer_owner = NULL
        ) noexcept;

        BiquadFilter(
            InputSignalProducerClass& input,
            BiquadFilterTypeParam& type,
            FloatParamS& frequency_leader,
            FloatParamS& q_leader,
            FloatParamS& gain_leader,
            Byte const& voice_status,
            BiquadFilterSharedBuffers* shared_buffers = NULL,
            Number const inaccuracy_seed = 0.0,
            FloatParamB const* freq_inaccuracy_param = NULL,
            FloatParamB const* q_inaccuracy_param = NULL,
            SignalProducer* buffer_owner = NULL
        ) noexcept;

        virtual ~BiquadFilter();

        virtual void set_sample_rate(Frequency const new_sample_rate) noexcept override;

        virtual void set_block_size(Integer const new_block_size) noexcept override;

        virtual void reset() noexcept override;

        void update_inaccuracy(
            Number const random_1,
            Number const random_2
        ) noexcept;

        FloatParamS frequency;
        FloatParamS q;
        FloatParamS gain;
        BiquadFilterTypeParam& type;

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
        static constexpr Number FREQUENCY_SINE_SCALE = std::sqrt(2.0);
        static constexpr Number GAIN_SCALE_HALF = (
            Constants::BIQUAD_FILTER_GAIN_SCALE / 2.0
        );
        static constexpr Number THRESHOLD = 0.000001;

        void initialize_instance() noexcept;
        void update_helper_variables() noexcept;
        void register_children() noexcept;

        void reallocate_buffers() noexcept;
        void allocate_buffers() noexcept;
        void free_buffers() noexcept;

        Sample const* const* initialize_rendering_no_op(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        void update_state_for_no_op_round(Integer const sample_count) noexcept;
        void update_state_for_silent_round(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        Sample const* const* initialize_rendering_with_shared_coefficients(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        template<bool is_freq_inaccurate>
        Number apply_freq_inaccuracy(Number const frequency_value) const noexcept;

        template<bool is_q_inaccurate>
        Number apply_q_inaccuracy(Number const q_value) const noexcept;

        template<bool is_freq_inaccurate, bool is_q_inaccurate>
        bool initialize_low_pass_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        template<bool is_freq_inaccurate, bool is_q_inaccurate>
        void store_low_pass_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const q_value
        ) noexcept;

        template<bool is_freq_inaccurate, bool is_q_inaccurate>
        bool initialize_high_pass_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        template<bool is_freq_inaccurate, bool is_q_inaccurate>
        void store_high_pass_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const q_value
        ) noexcept;

        template<bool is_freq_inaccurate>
        bool initialize_low_shelf_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        template<bool is_freq_inaccurate>
        void store_low_shelf_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const gain_value
        ) noexcept;

        template<bool is_freq_inaccurate>
        bool initialize_high_shelf_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        template<bool is_freq_inaccurate>
        void store_high_shelf_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const gain_value
        ) noexcept;

        template<bool is_freq_inaccurate, bool is_q_inaccurate>
        bool initialize_band_pass_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        template<bool is_freq_inaccurate, bool is_q_inaccurate>
        void store_band_pass_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const q_value
        ) noexcept;

        template<bool is_freq_inaccurate, bool is_q_inaccurate>
        bool initialize_notch_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        template<bool is_freq_inaccurate, bool is_q_inaccurate>
        void store_notch_coefficient_samples(
            Integer const index,
            Number const frequency_value,
            Number const q_value
        ) noexcept;

        template<bool is_freq_inaccurate, bool is_q_inaccurate>
        bool initialize_peaking_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        template<bool is_freq_inaccurate, bool is_q_inaccurate>
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

        Number const inaccuracy_seed;
        FloatParamB const* const freq_inaccuracy_param;
        FloatParamB const* const q_inaccuracy_param;

        BiquadFilterSharedBuffers* const shared_buffers;

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
        Number freq_inaccuracy;
        Number q_inaccuracy;
        Number freq_inaccuracy_param_value;
        Number q_inaccuracy_param_value;

        bool is_silent_;
        bool are_coefficients_constant;
        bool can_use_shared_coefficients;
};

}

#endif
