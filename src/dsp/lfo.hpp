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

#ifndef JS80P__DSP__LFO_HPP
#define JS80P__DSP__LFO_HPP

#include <string>

#include "js80p.hpp"

#include "dsp/envelope.hpp"
#include "dsp/oscillator.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"
#include "dsp/wavetable.hpp"


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

        typedef Param<Byte, ParamEvaluation::BLOCK> AmountEnvelopeParam;

        explicit LFO(std::string const& name) noexcept;

        /* No, this is not a macro. */
        // cppcheck-suppress unknownMacro
        LFO(
            std::string const& name,
            FloatParamS& frequency_leader,
            FloatParamS& max_leader,
            FloatParamS& amount_leader,
            ToggleParam& tempo_sync_,
            Number const phase_offset
        ) noexcept;

        void start(Seconds const time_offset) noexcept;
        void stop(Seconds const time_offset) noexcept;
        bool is_on() const noexcept;

        bool has_envelope() const noexcept;

        void produce_with_envelope(
            Seconds& envelope_time,
            Sample& envelope_value,
            EnvelopeStage& envelope_stage,
            EnvelopeSnapshot const& envelope_snapshot,
            WavetableState& wavetable_state,
            Integer const round,
            Integer const sample_count,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample* buffer
        ) noexcept;

        void skip_round(Integer const round, Integer const sample_count) noexcept;

        typename Oscillator_::WaveformParam waveform;
        FloatParamS frequency;
        FloatParamS phase;
        FloatParamS min;
        FloatParamS max;
        FloatParamS amount;
        FloatParamS distortion;
        FloatParamS randomness;
        ToggleParam tempo_sync;
        ToggleParam center;
        AmountEnvelopeParam amount_envelope;

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
        static constexpr Number ALMOST_ZERO = 0.000001;

        void initialize_instance() noexcept;

        void apply_distortions(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample const* source_buffer,
            Sample* target_buffer
        );

        void apply_range(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample const* source_buffer,
            Sample* target_buffer
        );

        void apply_distortions_centered(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample const* source_buffer,
            Sample* target_buffer
        );

        void apply_range_centered(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample const* source_buffer,
            Sample* target_buffer
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
