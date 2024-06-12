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

#ifndef JS80P__DSP__ENVELOPE_HPP
#define JS80P__DSP__ENVELOPE_HPP

#include <cstddef>
#include <string>

#include "js80p.hpp"

#include "dsp/math.hpp"
#include "dsp/param.hpp"


namespace JS80P
{

/**
 * \brief A collection of parameters specifying an envelope.
 *
 * \code
 *                      /--o---o---o--\  Peak
 *                     o|  :   :   : | o Value
 *                    /:|  :   :   : | :\
 *                   / :|  :   :   : | : \
 *                  /  :|  :   :   : | :  \
 *                 o   :|  :   :   : | :   o   Sustain
 *                /:   :|  :   :   : | :   :\  Value
 *               / :   :|  :   :   : | :   : \-o---o- ~ ~ ~ -o-\
 *              /  :   :|  :   :   : | :   : | :   :         :| \    Final
 *  Initial    o   :   :|  :   :   : | :   : | :   :         :|  o   Value
 *  Value     /:   :   :|  :   :   : | :   : | :   :         :|  :\--o---o-
 * o---o---o-/-+---+---+---+---+---+---+---+---+---+- ~ ~ ~ -+---+---+---+-->
 * 0   1   2|  3   4   5|  6   7   8 | 9  10 |11  12          |   |         time
 *          |           |            |       |                |   |
 *          |           |            |       |                |   |
 *          |           |            |       |                |   |
 * Delay    | Attack    | Hold       | Decay | Sustain        | Release
 * \endcode
 *
 * Envelope-capable parameters hold their envelope rendering state:
 *
 * - \c Seconds \c time \n
 *   How much time will have elapsed since the last envelope event (start,
 *   release start, sustain level change, etc.), at the next sample to be
 *   rendered.
 *
 * - \c Number \c last_rendered_value \n
 *   Last rendered value. (At the start, the envelope's initial value.)
 *
 * - \c EnvelopeStage \c stage \n
 *   Which stage (delay, attack, hold, etc.) was active when the last sample
 *   was rendered.
 *
 * (\c Envelope::render() will update these as necessary.)
 */
class Envelope
{
    public:
        enum RenderingMode {
            OVERWRITE = 0,
            MULTIPLY = 1,
        };

        static constexpr Seconds TIME_INACCURACY_MAX = 0.3;
        static constexpr Seconds DYNAMIC_ENVELOPE_RAMP_TIME = 0.1;

        static constexpr EnvelopeShape SHAPE_SMOOTH_SMOOTH = Math::EnvelopeShape::ENV_SHAPE_SMOOTH_SMOOTH;
        static constexpr EnvelopeShape SHAPE_SMOOTH_SMOOTH_STEEP = Math::EnvelopeShape::ENV_SHAPE_SMOOTH_SMOOTH_STEEP;
        static constexpr EnvelopeShape SHAPE_SMOOTH_SMOOTH_STEEPER = Math::EnvelopeShape::ENV_SHAPE_SMOOTH_SMOOTH_STEEPER;
        static constexpr EnvelopeShape SHAPE_SMOOTH_SHARP = Math::EnvelopeShape::ENV_SHAPE_SMOOTH_SHARP;
        static constexpr EnvelopeShape SHAPE_SMOOTH_SHARP_STEEP = Math::EnvelopeShape::ENV_SHAPE_SMOOTH_SHARP_STEEP;
        static constexpr EnvelopeShape SHAPE_SMOOTH_SHARP_STEEPER = Math::EnvelopeShape::ENV_SHAPE_SMOOTH_SHARP_STEEPER;
        static constexpr EnvelopeShape SHAPE_SHARP_SMOOTH = Math::EnvelopeShape::ENV_SHAPE_SHARP_SMOOTH;
        static constexpr EnvelopeShape SHAPE_SHARP_SMOOTH_STEEP = Math::EnvelopeShape::ENV_SHAPE_SHARP_SMOOTH_STEEP;
        static constexpr EnvelopeShape SHAPE_SHARP_SMOOTH_STEEPER = Math::EnvelopeShape::ENV_SHAPE_SHARP_SMOOTH_STEEPER;
        static constexpr EnvelopeShape SHAPE_SHARP_SHARP = Math::EnvelopeShape::ENV_SHAPE_SHARP_SHARP;
        static constexpr EnvelopeShape SHAPE_SHARP_SHARP_STEEP = Math::EnvelopeShape::ENV_SHAPE_SHARP_SHARP_STEEP;
        static constexpr EnvelopeShape SHAPE_SHARP_SHARP_STEEPER = Math::EnvelopeShape::ENV_SHAPE_SHARP_SHARP_STEEPER;
        static constexpr EnvelopeShape SHAPE_LINEAR = 12;

        static constexpr Byte UPDATE_MODE_DYNAMIC_LAST = 0;
        static constexpr Byte UPDATE_MODE_DYNAMIC_OLDEST = 1;
        static constexpr Byte UPDATE_MODE_DYNAMIC_LOWEST = 2;
        static constexpr Byte UPDATE_MODE_DYNAMIC_HIGHEST = 3;
        static constexpr Byte UPDATE_MODE_STATIC = 4;
        static constexpr Byte UPDATE_MODE_END = 5;
        static constexpr Byte UPDATE_MODE_DYNAMIC = 6;

        class ShapeParam : public ByteParam
        {
            public:
                explicit ShapeParam(std::string const& name) noexcept;
        };

        static Number get_value_at_time(
                EnvelopeSnapshot const& snapshot,
                Seconds const time,
                EnvelopeStage const stage,
                Number const last_rendered_value,
                Seconds const sampling_period
        ) noexcept;

        template<RenderingMode rendering_mode>
        static void render(
            EnvelopeSnapshot const& snapshot,
            Seconds& time,
            EnvelopeStage& stage,
            bool& becomes_constant,
            Number& last_rendered_value,
            Frequency const sample_rate,
            Seconds const sampling_period,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample* buffer
        ) noexcept;

        explicit Envelope(std::string const& name) noexcept;

        void update() noexcept;
        Integer get_change_index() const noexcept;

        bool is_dynamic() const noexcept;
        bool is_static() const noexcept;
        bool is_tempo_synced() const noexcept;

        bool needs_update(Byte const voice_status) const noexcept;

        void make_snapshot(
            EnvelopeRandoms const& randoms,
            Byte const envelope_index,
            EnvelopeSnapshot& snapshot
        ) const noexcept;

        void make_end_snapshot(
            EnvelopeRandoms const& randoms,
            Byte const envelope_index,
            EnvelopeSnapshot& snapshot
        ) const noexcept;

        ByteParam update_mode;
        ToggleParam tempo_sync;
        ShapeParam attack_shape;
        ShapeParam decay_shape;
        ShapeParam release_shape;
        FloatParamB amount;
        FloatParamB initial_value;
        FloatParamB delay_time;
        FloatParamB attack_time;
        FloatParamB peak_value;
        FloatParamB hold_time;
        FloatParamB decay_time;
        FloatParamB sustain_value;
        FloatParamB release_time;
        FloatParamB final_value;
        FloatParamB time_inaccuracy;
        FloatParamB value_inaccuracy;

    private:
        static constexpr Number ALMOST_ZERO = 0.0000001;

        static void set_up_next_target(
            EnvelopeSnapshot const& snapshot,
            Number const last_rendered_value,
            Seconds& time,
            EnvelopeStage& stage,
            Number& initial_value,
            Number& target_value,
            Seconds& time_until_target,
            Seconds& duration,
            EnvelopeShape& shape,
            bool& becomes_constant,
            Seconds const sampling_period
        ) noexcept;

        static void set_up_next_dahds_target(
            EnvelopeSnapshot const& snapshot,
            Number const last_rendered_value,
            Seconds& time,
            EnvelopeStage& stage,
            Number& initial_value,
            Number& target_value,
            Seconds& time_until_target,
            Seconds& duration,
            EnvelopeShape& shape,
            bool& becomes_constant,
            Seconds const sampling_period
        ) noexcept;

        static void set_up_next_sustain_target(
            EnvelopeSnapshot const& snapshot,
            Number const last_rendered_value,
            Seconds const time,
            Number& initial_value,
            Number const target_value,
            Seconds& time_until_target,
            Seconds& duration,
            EnvelopeShape& shape,
            bool& becomes_constant
        ) noexcept;

        static void set_up_next_release_target(
            EnvelopeSnapshot const& snapshot,
            Seconds& time,
            EnvelopeStage& stage,
            Number& initial_value,
            Number& target_value,
            Seconds& time_until_target,
            Seconds& duration,
            EnvelopeShape& shape,
            bool& becomes_constant
        ) noexcept;

        template<bool adjust_initial_value_during_dahds, bool need_shaping_for_initial_value_adjustment>
        static void set_up_interpolation(
            Number& initial_value,
            Number& delta,
            Number& initial_ratio,
            Number const last_rendered_value,
            Number const target_value,
            Seconds const duration,
            Seconds const time_until_target,
            Seconds const sampling_period,
            Number const duration_inv,
            EnvelopeStage const stage,
            EnvelopeShape const shape
        ) noexcept;

        template<bool need_shaping>
        static Number find_adjusted_initial_value(
            Seconds const elapsed_time,
            Seconds const sampling_period,
            Number const duration_inv,
            Number const last_rendered_value,
            Number const target_value,
            EnvelopeShape const shape
        ) noexcept;

        template<RenderingMode rendering_mode>
        static void render_constant(
            Seconds& time,
            Number const value,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample* buffer
        ) noexcept;

        template<RenderingMode rendering_mode, bool need_shaping>
        static void render(
            Seconds& time,
            EnvelopeStage const stage,
            Number& last_rendered_value,
            Number& initial_value,
            Number const target_value,
            Seconds const duration,
            Seconds const time_until_target,
            Frequency const sample_rate,
            Seconds const sampling_period,
            Integer const first_sample_index,
            Integer const last_sample_index,
            EnvelopeShape const shape,
            Sample* buffer,
            Integer& i
        ) noexcept;

        void update_bpm(Number const new_bpm) noexcept;

        template<class ParamType>
        bool update_change_index(ParamType const& param, Integer& change_index) noexcept;

        Number randomize_value(
            FloatParamB const& param,
            Number const random
        ) const noexcept;

        Seconds randomize_time(
            FloatParamB const& param,
            Number const random
        ) const noexcept;

        Number bpm;
        Number tempo_sync_time_scale;

        Integer update_mode_change_index;
        Integer tempo_sync_change_index;
        Integer attack_shape_change_index;
        Integer decay_shape_change_index;
        Integer release_shape_change_index;
        Integer amount_change_index;
        Integer initial_value_change_index;
        Integer delay_time_change_index;
        Integer attack_time_change_index;
        Integer peak_value_change_index;
        Integer hold_time_change_index;
        Integer decay_time_change_index;
        Integer sustain_value_change_index;
        Integer release_time_change_index;
        Integer final_value_change_index;
        Integer time_inaccuracy_change_index;
        Integer value_inaccuracy_change_index;
        Integer change_index;
};

}

#endif
