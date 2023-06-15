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

#ifndef JS80P__SYNTH__LFO_HPP
#define JS80P__SYNTH__LFO_HPP

#include <string>

#include "js80p.hpp"

#include "synth/oscillator.hpp"
#include "synth/param.hpp"
#include "synth/signal_producer.hpp"


namespace JS80P
{

/**
 * \note The \c amount parameter goes from 0.0 to 0.5 because the oscillator's
 *       range goes from -1.0 to +1.0, which we want to transform to go from
 *       0.0 to 1.0, for which we need to halve its output. This halving is
 *       what's built into the \c amount parameter.
 */
class LFO : public SignalProducer
{
    friend class SignalProducer;

    public:
        typedef Oscillator<SignalProducer, true> Oscillator_;

        LFO(std::string const name) noexcept;

        LFO(
            std::string const name,
            FloatParam& frequency_leader,
            FloatParam& max_leader,
            FloatParam& amount_leader,
            ToggleParam& tempo_sync_,
            Number const phase_offset
        ) noexcept;

        void start(Seconds const time_offset) noexcept;
        void stop(Seconds const time_offset) noexcept;
        bool is_on() const noexcept;

        void skip_round(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        typename Oscillator_::WaveformParam waveform;
        FloatParam frequency;
        FloatParam phase;
        FloatParam min;
        FloatParam max;
        FloatParam amount;
        FloatParam distortion;
        FloatParam randomness;
        ToggleParam tempo_sync;
        ToggleParam center;

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

        void apply_range(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample const* const* source_buffer,
            Sample** target_buffer
        );

        void apply_range_centered(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample const* const* source_buffer,
            Sample** target_buffer
        );

        void apply_distortions(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample const* const* source_buffer,
            Sample** target_buffer
        );

        Oscillator_ oscillator;

        Sample const* min_buffer;
        Sample const* max_buffer;
        Sample const* distortion_buffer;
        Sample const* randomness_buffer;
        Sample const* const* oscillator_buffer;
};

}

#endif
