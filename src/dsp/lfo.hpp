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
#include "dsp/lfo_envelope_list.hpp"
#include "dsp/oscillator.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"
#include "dsp/wavetable.hpp"


namespace JS80P
{

/**
 * \note The \c amplitude parameter goes from 0.0 to 0.5 because the
 *       oscillator's range goes from -1.0 to +1.0, which we want to transform
 *       to go from 0.0 to 1.0, for which we need to halve its output. This
 *       halving is what's built into the \c amplitude parameter.
 */
class LFO : public SignalProducer
{
    friend class SignalProducer;

    public:
        typedef Oscillator<SignalProducer, true> Oscillator_;

        typedef ByteParam AmplitudeEnvelopeParam;

        explicit LFO(
            std::string const& name,
            bool const can_have_envelope = false
        ) noexcept;

        /* No, this is not a macro. */
        // cppcheck-suppress unknownMacro
        LFO(
            std::string const& name,
            FloatParamS& frequency_leader,
            FloatParamS& max_leader,
            FloatParamS& amplitude_leader,
            ToggleParam& tempo_sync_,
            Number const phase_offset
        ) noexcept;

        virtual ~LFO();

        virtual void set_block_size(Integer const new_block_size) noexcept override;

        void start(Seconds const time_offset) noexcept;
        void stop(Seconds const time_offset) noexcept;
        bool is_on() const noexcept;

        bool has_envelope() const noexcept;

        void collect_envelopes(LFOEnvelopeList& envelope_list) noexcept;

        void produce_with_envelope(
            LFOEnvelopeStates& lfo_envelope_states,
            Integer const round,
            Integer const sample_count,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample* buffer
        ) noexcept;

        void skip_round(Integer const round, Integer const sample_count) noexcept;

        typename Oscillator_::WaveformParam waveform;
        ToggleParam freq_log_scale;
        FloatParamS frequency;
        FloatParamS phase;
        FloatParamS min;
        FloatParamS max;
        FloatParamS amplitude;
        FloatParamS distortion;
        FloatParamS randomness;
        ToggleParam tempo_sync;
        ToggleParam center;
        AmplitudeEnvelopeParam amplitude_envelope;

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
        class Visitor
        {
            public:
                bool should_visit_lfo_as_polyphonic(
                    LFO const& lfo,
                    Byte const depth
                ) const noexcept;

                void visit_lfo_as_polyphonic(LFO& lfo, Byte const depth) noexcept;

                void visit_lfo_as_global(LFO& lfo) noexcept;

                void visit_amplitude_param(
                    LFO& lfo,
                    Byte const depth,
                    FloatParamS& amplitude
                ) noexcept;

                void visit_frequency_param(
                    LFO& lfo,
                    Byte const depth,
                    FloatParamS& frequency
                ) noexcept;

                void visit_phase_param(
                    LFO& lfo,
                    Byte const depth,
                    FloatParamS& phase
                ) noexcept;

                void visit_oscillator(
                    LFO& lfo,
                    Byte const depth,
                    Oscillator_& oscillator
                ) noexcept;

                void visit_distortion_param(
                    LFO& lfo,
                    Byte const depth,
                    FloatParamS& distortion
                ) noexcept;

                void visit_randomness_param(
                    LFO& lfo,
                    Byte const depth,
                    FloatParamS& randomness
                ) noexcept;

                void visit_min_param(
                    LFO& lfo,
                    Byte const depth,
                    FloatParamS& min
                ) noexcept;

                void visit_max_param(
                    LFO& lfo,
                    Byte const depth,
                    FloatParamS& max
                ) noexcept;
        };

        class EnvelopeCollector : public Visitor
        {
            public:
                EnvelopeCollector(LFOEnvelopeList& envelope_list) noexcept;

                void visit_lfo_as_polyphonic(LFO& lfo, Byte const depth) noexcept;

            private:
                LFOEnvelopeList* envelope_list;
        };

        class LFOWithEnvelopeRenderer : public Visitor
        {
            public:
                LFOWithEnvelopeRenderer(
                    LFOEnvelopeStates& lfo_envelope_states,
                    Integer const round,
                    Integer const sample_count,
                    Integer const first_sample_index,
                    Integer const last_sample_index,
                    Sample* buffer
                ) noexcept;

                bool should_visit_lfo_as_polyphonic(
                    LFO const& lfo,
                    Byte const depth
                ) const noexcept;

                void visit_lfo_as_polyphonic(LFO& lfo, Byte const depth) noexcept;

                void visit_lfo_as_global(LFO& lfo) noexcept;

                void visit_amplitude_param(
                    LFO& lfo,
                    Byte const depth,
                    FloatParamS& amplitude
                ) noexcept;

                void visit_frequency_param(
                    LFO& lfo,
                    Byte const depth,
                    FloatParamS& frequency
                ) noexcept;

                void visit_phase_param(
                    LFO& lfo,
                    Byte const depth,
                    FloatParamS& phase
                ) noexcept;

                void visit_oscillator(
                    LFO& lfo,
                    Byte const depth,
                    Oscillator_& oscillator
                ) noexcept;

                void visit_distortion_param(
                    LFO& lfo,
                    Byte const depth,
                    FloatParamS& distortion
                ) noexcept;

                void visit_randomness_param(
                    LFO& lfo,
                    Byte const depth,
                    FloatParamS& randomness
                ) noexcept;

                void visit_min_param(
                    LFO& lfo,
                    Byte const depth,
                    FloatParamS& min
                ) noexcept;

                void visit_max_param(
                    LFO& lfo,
                    Byte const depth,
                    FloatParamS& max
                ) noexcept;

            private:
                LFOEnvelopeStates& lfo_envelope_states;
                Sample* const buffer;
                Sample const* param_buffer_1;
                Sample const* param_buffer_2;
                Sample const* param_buffer_3;
                Integer const round;
                Integer const sample_count;
                Integer const first_sample_index;
                Integer const last_sample_index;
        };

        static constexpr Number ALMOST_ZERO = 0.000001;

        void initialize_instance() noexcept;

        void apply_distortions(
            Sample const* const distortion_buffer,
            Sample const* const randomness_buffer,
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample const* const source_buffer,
            Sample* const target_buffer
        );

        void apply_range(
            Sample const* const min_buffer,
            Sample const* const max_buffer,
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample const* const source_buffer,
            Sample* const target_buffer
        );

        void apply_distortions_centered(
            Sample const* const distortion_buffer,
            Sample const* const randomness_buffer,
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample const* const source_buffer,
            Sample* const target_buffer
        );

        void apply_range_centered(
            Sample const* const min_buffer,
            Sample const* const max_buffer,
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample const* const source_buffer,
            Sample* const target_buffer
        );

        template<class VisitorClass>
        void traverse_lfo_graph(
            LFO& lfo,
            Byte& depth,
            VisitorClass& visitor
        ) noexcept;

        template<class VisitorClass>
        bool should_visit_lfo_as_polyphonic(
            LFO const& lfo,
            Byte const depth,
            VisitorClass const& visitor
        ) noexcept;

        template<class VisitorClass>
        void visit_param_lfo(
            LFO* lfo,
            Byte& depth,
            VisitorClass& visitor
        ) noexcept;

        bool const can_have_envelope;

        Oscillator_ oscillator;

        Sample* env_buffer_1;
        Sample* env_buffer_2;

        Sample const* min_buffer;
        Sample const* max_buffer;
        Sample const* distortion_buffer;
        Sample const* randomness_buffer;
        Sample const* const* oscillator_buffer;

        bool is_being_visited;
};

}

#endif
