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

#ifndef JS80P__DSP__SIDE_CHAIN_COMPRESSABLE_EFFECT_HPP
#define JS80P__DSP__SIDE_CHAIN_COMPRESSABLE_EFFECT_HPP

#include <string>

#include "js80p.hpp"

#include "dsp/effect.hpp"
#include "dsp/param.hpp"
#include "dsp/peak_tracker.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

enum CompressionCurve {
    COMPRESSION_CURVE_LINEAR = 0,
    COMPRESSION_CURVE_SMOOTH = 1,
};


enum CompressionMode {
    COMPRESSION_MODE_COMPRESSOR = 0,
    COMPRESSION_MODE_EXPANDER = 1,
    COMPRESSION_MODE_UPWARD_COMPRESSOR = 2,
    COMPRESSION_MODES = 3,
};


template<
    class InputSignalProducerClass,
    CompressionCurve curve = CompressionCurve::COMPRESSION_CURVE_LINEAR,
    bool allow_upward_mode = false
>
class SideChainCompressableEffect : public Effect<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        SideChainCompressableEffect(
            std::string const& name,
            InputSignalProducerClass& input,
            Integer const number_of_children = 0,
            SignalProducer* const wet_buffer_owner = NULL,
            Number const makeup_gain = 1.0
        );

        FloatParamB side_chain_compression_threshold;
        FloatParamB side_chain_compression_attack_time;
        FloatParamB side_chain_compression_release_time;
        FloatParamB side_chain_compression_ratio;
        ByteParam side_chain_compression_mode;

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
        enum Action {
            BYPASS_OR_RELEASE = 0,
            COMPRESS = 1,
        };

        static constexpr Number NO_OP_RATIO = 1.0;
        static constexpr Number BYPASS_GAIN = 1.0;

        static constexpr Number UPWARD_GAIN_INCREASE_MAX_DB = 30.0;

        void clear_state() noexcept;
        void fast_bypass() noexcept;
        void copy_input(Integer const sample_count) noexcept;

        void compress(
            Sample const peak,
            Number const target_peak_db,
            Number const zero_peak_target,
            FloatParamB const& time_param
        ) noexcept;

        void release(FloatParamB const& time_param) noexcept;

        void schedule_gain_ramp(
            Number const target_gain,
            FloatParamB const& time_param
        ) noexcept;

        Number const makeup_gain;
        bool const no_makeup;

        FloatParamS gain;
        PeakTracker peak_tracker;
        Sample const* gain_buffer;
        Number target_gain;
        Action previous_action;
        Byte previous_mode;
        bool is_bypassing;
};

}

#endif

