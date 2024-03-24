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
        enum RenderingMode
        {
            OVERWRITE = 0,
            MULTIPLY = 1,
        };

        static constexpr Seconds TIME_INACCURACY_MAX = 0.3;
        static constexpr Seconds DYNAMIC_ENVELOPE_RAMP_TIME = 0.1;

        typedef Byte Shape;

        static constexpr Shape SHAPE_SMOOTH_SMOOTH = 0;
        static constexpr Shape SHAPE_SMOOTH_SMOOTH_STEEP = 1;
        static constexpr Shape SHAPE_SMOOTH_SMOOTH_STEEPER = 2;
        static constexpr Shape SHAPE_SMOOTH_SHARP = 3;
        static constexpr Shape SHAPE_SMOOTH_SHARP_STEEP = 4;
        static constexpr Shape SHAPE_SMOOTH_SHARP_STEEPER = 5;
        static constexpr Shape SHAPE_SHARP_SMOOTH = 6;
        static constexpr Shape SHAPE_SHARP_SMOOTH_STEEP = 7;
        static constexpr Shape SHAPE_SHARP_SMOOTH_STEEPER = 8;
        static constexpr Shape SHAPE_LINEAR = 9;

        typedef Param<Shape, ParamEvaluation::BLOCK> ShapeParam;

        static Number get_value_at_time(
                EnvelopeSnapshot const& snapshot,
                Seconds const time,
                EnvelopeStage const stage,
                Number const last_rendered_value,
                Seconds const sampling_period
        ) noexcept;

        template<RenderingMode rendering_mode = RenderingMode::OVERWRITE>
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

        void make_snapshot(
            EnvelopeRandoms const& randoms,
            EnvelopeSnapshot& snapshot
        ) const noexcept;

        void make_end_snapshot(
            EnvelopeRandoms const& randoms,
            EnvelopeSnapshot& snapshot
        ) const noexcept;

        ToggleParam dynamic;
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
            bool& becomes_constant
        ) noexcept;

        template<bool adjust_initial_value_during_dahds>
        static void set_up_interpolation(
            Number& initial_value,
            Number& delta,
            Number& initial_ratio,
            Number const last_rendered_value,
            Number const target_value,
            EnvelopeStage const stage,
            Seconds const duration,
            Seconds const time_until_target,
            Seconds const sampling_period,
            Number const duration_inv
        ) noexcept;

        template<RenderingMode rendering_mode = RenderingMode::OVERWRITE>
        static void render_constant(
            Seconds& time,
            Number const value,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample* buffer
        ) noexcept;

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

        Integer dynamic_change_index;
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
