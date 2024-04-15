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

#ifndef JS80P__DSP__LFO_CPP
#define JS80P__DSP__LFO_CPP

#include "dsp/lfo.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

LFO::LFO(std::string const& name, bool const can_have_envelope) noexcept
    : SignalProducer(1, 13, 0, &oscillator),
    waveform(name + "WAV", Oscillator_::SOFT_SQUARE),
    freq_log_scale(name + "LOG", ToggleParam::OFF),
    frequency(
        name + "FRQ",
        Constants::LFO_FREQUENCY_MIN,
        Constants::LFO_FREQUENCY_MAX,
        Constants::LFO_FREQUENCY_DEFAULT,
        0.0,
        NULL,
        &freq_log_scale,
        Math::log_lfo_freq_table(),
        Math::LOG_LFO_FREQ_TABLE_MAX_INDEX,
        Math::LOG_LFO_FREQ_TABLE_INDEX_SCALE
    ),
    phase(name + "PHS", 0.0, 1.0, 0.0),
    min(name + "MIN", 0.0, 1.0, 0.0),
    max(name + "MAX", 0.0, 1.0, 1.0),
    amount(name + "AMT", 0.0, 0.5, 0.5),
    distortion(name + "DST", 0.0, 1.0, 0.0),
    randomness(name + "RND", 0.0, 1.0, 0.0),
    tempo_sync(name + "SYN", ToggleParam::OFF),
    center(name + "CEN", ToggleParam::OFF),
    amount_envelope(name + "AEN", 0, Constants::ENVELOPES, Constants::ENVELOPES),
    can_have_envelope(can_have_envelope),
    oscillator(waveform, amount, frequency, phase, tempo_sync, center),
    is_being_visited(false)
{
    initialize_instance();
}


void LFO::initialize_instance() noexcept
{
    register_child(waveform);
    register_child(frequency);
    register_child(phase);
    register_child(min);
    register_child(max);
    register_child(amount);
    register_child(distortion);
    register_child(randomness);
    register_child(freq_log_scale);
    register_child(tempo_sync);
    register_child(center);
    register_child(amount_envelope);
    register_child(oscillator);

    if (can_have_envelope) {
        env_buffer_1 = new Sample[block_size];
        env_buffer_2 = new Sample[block_size];
    } else {
        env_buffer_1 = NULL;
        env_buffer_2 = NULL;
    }
}


LFO::~LFO()
{
    if (can_have_envelope) {
        delete[] env_buffer_1;
        delete[] env_buffer_2;

        env_buffer_1 = NULL;
        env_buffer_2 = NULL;
    }
}


LFO::LFO(
        std::string const& name,
        FloatParamS& frequency_leader,
        FloatParamS& max_leader,
        FloatParamS& amount_leader,
        ToggleParam& tempo_sync_,
        Number const phase_offset
) noexcept
    : SignalProducer(1, 13, 0, &oscillator),
    waveform(name + "WAV", Oscillator_::SOFT_SQUARE),
    freq_log_scale(name + "LOG", ToggleParam::OFF),
    frequency(frequency_leader),
    phase(name + "PHS", 0.0, 1.0, phase_offset),
    min(name + "MIN", 0.0, 1.0, 0.0),
    max(max_leader),
    amount(amount_leader),
    distortion(name + "DST", 0.0, 1.0, 0.0),
    randomness(name + "RND", 0.0, 1.0, 0.0),
    tempo_sync(name + "SYN", ToggleParam::OFF),
    center(name + "CEN", ToggleParam::OFF),
    amount_envelope(name + "AEN", 0, Constants::ENVELOPES, Constants::ENVELOPES),
    can_have_envelope(false),
    oscillator(waveform, amount, frequency, phase, tempo_sync_, center),
    is_being_visited(false)
{
    initialize_instance();
}


void LFO::set_block_size(Integer const new_block_size) noexcept
{
    Integer const old_block_size = get_block_size();

    SignalProducer::set_block_size(new_block_size);

    if (can_have_envelope && old_block_size != new_block_size) {
        delete[] env_buffer_1;
        delete[] env_buffer_2;

        env_buffer_1 = new Sample[new_block_size];
        env_buffer_2 = new Sample[new_block_size];
    }
}


void LFO::start(Seconds const time_offset) noexcept
{
    oscillator.start(time_offset);
}


void LFO::stop(Seconds const time_offset) noexcept
{
    oscillator.stop(time_offset);

    frequency.cancel_events_at(time_offset);
    phase.cancel_events_at(time_offset);
    min.cancel_events_at(time_offset);
    max.cancel_events_at(time_offset);
    amount.cancel_events_at(time_offset);
    distortion.cancel_events_at(time_offset);
    randomness.cancel_events_at(time_offset);
}


bool LFO::is_on() const noexcept
{
    return oscillator.is_on();
}


bool LFO::has_envelope() const noexcept
{
    return amount_envelope.get_value() != Constants::INVALID_ENVELOPE_INDEX;
}


void LFO::collect_envelopes(LFOEnvelopeList& envelope_list) noexcept
{
    EnvelopeCollector collector(envelope_list);
    Byte depth = 0;

    envelope_list.clear();

    if (!should_visit_lfo_as_polyphonic<EnvelopeCollector>(*this, depth, collector)) {
        return;
    }

    traverse_lfo_graph<EnvelopeCollector>(*this, depth, collector);
}


template<class VisitorClass>
void LFO::traverse_lfo_graph(
        LFO& lfo,
        Byte& depth,
        VisitorClass& visitor
) noexcept {
    is_being_visited = true;

    Byte const lfo_depth = depth;

    visitor.visit_lfo_as_polyphonic(lfo, lfo_depth);

    visit_param_lfo<VisitorClass>(lfo.amount.get_lfo(), depth, visitor);
    visitor.visit_amount_param(lfo, lfo_depth, lfo.amount);

    visit_param_lfo<VisitorClass>(lfo.frequency.get_lfo(), depth, visitor);
    visitor.visit_frequency_param(lfo, lfo_depth, lfo.frequency);

    visit_param_lfo<VisitorClass>(lfo.phase.get_lfo(), depth, visitor);
    visitor.visit_phase_param(lfo, lfo_depth, lfo.phase);

    visitor.visit_oscillator(lfo, lfo_depth, lfo.oscillator);

    visit_param_lfo<VisitorClass>(lfo.distortion.get_lfo(), depth, visitor);
    visitor.visit_distortion_param(lfo, lfo_depth, lfo.distortion);

    visit_param_lfo<VisitorClass>(lfo.randomness.get_lfo(), depth, visitor);
    visitor.visit_randomness_param(lfo, lfo_depth, lfo.randomness);

    visit_param_lfo<VisitorClass>(lfo.min.get_lfo(), depth, visitor);
    visitor.visit_min_param(lfo, lfo_depth, lfo.min);

    visit_param_lfo<VisitorClass>(lfo.max.get_lfo(), depth, visitor);
    visitor.visit_max_param(lfo, lfo_depth, lfo.max);

    is_being_visited = false;
}


template<class VisitorClass>
bool LFO::should_visit_lfo_as_polyphonic(
        LFO const& lfo,
        Byte const depth,
        VisitorClass const& visitor
) noexcept {
    return (
        !lfo.is_being_visited
        && depth != Constants::PARAM_LFO_ENVELOPE_STATES
        && lfo.has_envelope()
        && visitor.should_visit_lfo_as_polyphonic(lfo, depth)
    );
}


template<class VisitorClass>
void LFO::visit_param_lfo(
        LFO* lfo,
        Byte& depth,
        VisitorClass& visitor
) noexcept {
    if (lfo == NULL) {
        return;
    }

    if (should_visit_lfo_as_polyphonic<VisitorClass>(*lfo, depth + 1, visitor)) {
        ++depth;

        traverse_lfo_graph<VisitorClass>(*lfo, depth, visitor);
    } else {
        visitor.visit_lfo_as_global(*lfo);
    }
}


bool LFO::Visitor::should_visit_lfo_as_polyphonic(
        LFO const& lfo,
        Byte const depth
) const noexcept {
    return true;
}


void LFO::Visitor::visit_lfo_as_polyphonic(LFO& lfo, Byte const depth) noexcept
{
}


void LFO::Visitor::visit_lfo_as_global(LFO& lfo) noexcept
{
}


void LFO::Visitor::visit_amount_param(
        LFO& lfo,
        Byte const depth,
        FloatParamS& amount
) noexcept {
}


void LFO::Visitor::visit_frequency_param(
        LFO& lfo,
        Byte const depth,
        FloatParamS& frequency
) noexcept {
}


void LFO::Visitor::visit_phase_param(
        LFO& lfo,
        Byte const depth,
        FloatParamS& phase
) noexcept {
}


void LFO::Visitor::visit_oscillator(
        LFO& lfo,
        Byte const depth,
        Oscillator_& oscillator
) noexcept {
}


void LFO::Visitor::visit_distortion_param(
        LFO& lfo,
        Byte const depth,
        FloatParamS& distortion
) noexcept {
}


void LFO::Visitor::visit_randomness_param(
        LFO& lfo,
        Byte const depth,
        FloatParamS& randomness
) noexcept {
}


void LFO::Visitor::visit_min_param(
        LFO& lfo,
        Byte const depth,
        FloatParamS& min
) noexcept {
}


void LFO::Visitor::visit_max_param(
        LFO& lfo,
        Byte const depth,
        FloatParamS& max
) noexcept {
}


LFO::EnvelopeCollector::EnvelopeCollector(LFOEnvelopeList& envelope_list) noexcept
    : envelope_list(&envelope_list)
{
}


void LFO::EnvelopeCollector::visit_lfo_as_polyphonic(
        LFO& lfo,
        Byte const depth
) noexcept {
    (*envelope_list)[depth] = lfo.amount_envelope.get_value();
}


LFO::LFOWithEnvelopeRenderer::LFOWithEnvelopeRenderer(
        LFOEnvelopeStates& lfo_envelope_states,
        Integer const round,
        Integer const sample_count,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample* buffer
) noexcept
    : lfo_envelope_states(lfo_envelope_states),
    buffer(buffer),
    param_buffer_1(NULL),
    param_buffer_2(NULL),
    param_buffer_3(NULL),
    round(round),
    sample_count(sample_count),
    first_sample_index(first_sample_index),
    last_sample_index(last_sample_index)
{
}


bool LFO::LFOWithEnvelopeRenderer::should_visit_lfo_as_polyphonic(
        LFO const& lfo,
        Byte const depth
) const noexcept {
    LFOEnvelopeState& lfo_envelope_state = lfo_envelope_states[depth];

    return lfo_envelope_state.stage != EnvelopeStage::ENV_STG_NONE;
}


void LFO::LFOWithEnvelopeRenderer::visit_lfo_as_polyphonic(
        LFO& lfo,
        Byte const depth
) noexcept {
}


void LFO::LFOWithEnvelopeRenderer::visit_lfo_as_global(LFO& lfo) noexcept
{
    Sample const* const lfo_buffer = (
        SignalProducer::produce<LFO>(lfo, round, sample_count)[0]
    );

    if (JS80P_UNLIKELY(lfo_buffer == NULL)) {
        /*
        LFO dependency cycle, and no previously cached rounds are available. We
        go with the LFO's minimum value.
        */

        Sample const value = lfo.min.get_value();

        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[i] = value;
        }
    } else {
        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[i] = lfo_buffer[i];
        }
    }
}


void LFO::LFOWithEnvelopeRenderer::visit_amount_param(
        LFO& lfo,
        Byte const depth,
        FloatParamS& amount
) noexcept {
    if (amount.get_lfo() == NULL) {
        param_buffer_1 = FloatParamS::produce_if_not_constant(
            amount, round, sample_count
        );
    } else {
        amount.ratios_to_values(
            buffer, lfo.env_buffer_1, first_sample_index, last_sample_index
        );
        param_buffer_1 = lfo.env_buffer_1;
    }
}


void LFO::LFOWithEnvelopeRenderer::visit_frequency_param(
        LFO& lfo,
        Byte const depth,
        FloatParamS& frequency
) noexcept {
    if (frequency.get_lfo() == NULL) {
        param_buffer_2 = FloatParamS::produce_if_not_constant(
            frequency, round, sample_count
        );
    } else {
        frequency.ratios_to_values(
            buffer, lfo.env_buffer_2, first_sample_index, last_sample_index
        );
        param_buffer_2 = lfo.env_buffer_2;
    }
}


void LFO::LFOWithEnvelopeRenderer::visit_phase_param(
        LFO& lfo,
        Byte const depth,
        FloatParamS& phase
) noexcept {
    if (phase.get_lfo() == NULL) {
        param_buffer_3 = FloatParamS::produce_if_not_constant(
            phase, round, sample_count
        );
    } else {
        phase.ratios_to_values(
            buffer, first_sample_index, last_sample_index
        );
        param_buffer_3 = buffer;
    }
}


void LFO::LFOWithEnvelopeRenderer::visit_oscillator(
        LFO& lfo,
        Byte const depth,
        Oscillator_& oscillator
) noexcept {
    LFOEnvelopeState& lfo_envelope_state = lfo_envelope_states[depth];

    if (!JS80P_UNLIKELY(lfo_envelope_state.is_wavetable_initialized)) {
        lfo_envelope_state.is_wavetable_initialized = true;

        Sample const frequency_value = (
            param_buffer_2 == NULL
                ? lfo.frequency.get_value()
                : param_buffer_2[first_sample_index]
        );

        Wavetable::reset_state(
            lfo_envelope_state.wavetable_state,
            lfo.sampling_period,
            lfo.nyquist_frequency,
            frequency_value,
            lfo_envelope_state.time
        );
    }

    oscillator.produce_for_lfo_with_envelope(
        lfo_envelope_state.wavetable_state,
        round,
        sample_count,
        first_sample_index,
        last_sample_index,
        lfo.env_buffer_1,
        param_buffer_1,
        param_buffer_2,
        param_buffer_3
    );

    bool envelope_is_constant = false;

    Envelope::render<Envelope::RenderingMode::MULTIPLY>(
        lfo_envelope_state.snapshot,
        lfo_envelope_state.time,
        lfo_envelope_state.stage,
        envelope_is_constant,
        lfo_envelope_state.value,
        lfo.sample_rate,
        lfo.sampling_period,
        first_sample_index,
        last_sample_index,
        lfo.env_buffer_1
    );

    param_buffer_1 = NULL;
    param_buffer_2 = NULL;
    param_buffer_3 = NULL;
}


void LFO::LFOWithEnvelopeRenderer::visit_distortion_param(
        LFO& lfo,
        Byte const depth,
        FloatParamS& distortion
) noexcept {
    if (distortion.get_lfo() == NULL) {
        param_buffer_1 = FloatParamS::produce_if_not_constant(
            distortion, round, sample_count
        );
    } else {
        distortion.ratios_to_values(
            buffer, lfo.env_buffer_2, first_sample_index, last_sample_index
        );
        param_buffer_1 = lfo.env_buffer_2;
    }
}


void LFO::LFOWithEnvelopeRenderer::visit_randomness_param(
        LFO& lfo,
        Byte const depth,
        FloatParamS& randomness
) noexcept {
    if (randomness.get_lfo() == NULL) {
        param_buffer_2 = FloatParamS::produce_if_not_constant(
            randomness, round, sample_count
        );
    } else {
        randomness.ratios_to_values(
            buffer, first_sample_index, last_sample_index
        );
        param_buffer_2 = buffer;
    }

    if (lfo.center.get_value() == ToggleParam::OFF) {
        lfo.apply_distortions(
            param_buffer_1,
            param_buffer_2,
            round,
            first_sample_index,
            last_sample_index,
            lfo.env_buffer_1,
            lfo.env_buffer_1
        );
    } else {
        lfo.apply_distortions_centered(
            param_buffer_1,
            param_buffer_2,
            round,
            first_sample_index,
            last_sample_index,
            lfo.env_buffer_1,
            lfo.env_buffer_1
        );
    }
}


void LFO::LFOWithEnvelopeRenderer::visit_min_param(
        LFO& lfo,
        Byte const depth,
        FloatParamS& min
) noexcept {
    if (min.get_lfo() == NULL) {
        param_buffer_1 = FloatParamS::produce_if_not_constant(
            min, round, sample_count
        );
    } else {
        min.ratios_to_values(
            buffer, lfo.env_buffer_2, first_sample_index, last_sample_index
        );
        param_buffer_1 = lfo.env_buffer_2;
    }
}


void LFO::LFOWithEnvelopeRenderer::visit_max_param(
        LFO& lfo,
        Byte const depth,
        FloatParamS& max
) noexcept {
    if (max.get_lfo() == NULL) {
        param_buffer_2 = FloatParamS::produce_if_not_constant(
            max, round, sample_count
        );
    } else {
        max.ratios_to_values(buffer, first_sample_index, last_sample_index);
        param_buffer_2 = buffer;
    }

    if (lfo.center.get_value() == ToggleParam::OFF) {
        lfo.apply_range(
            param_buffer_1,
            param_buffer_2,
            round,
            first_sample_index,
            last_sample_index,
            lfo.env_buffer_1,
            buffer
        );
    } else {
        lfo.apply_range_centered(
            param_buffer_1,
            param_buffer_2,
            round,
            first_sample_index,
            last_sample_index,
            lfo.env_buffer_1,
            buffer
        );
    }
}


void LFO::skip_round(Integer const round, Integer const sample_count) noexcept
{
    oscillator.skip_round(round, sample_count);

    frequency.skip_round(round, sample_count);
    phase.skip_round(round, sample_count);
    min.skip_round(round, sample_count);
    max.skip_round(round, sample_count);
    amount.skip_round(round, sample_count);
    distortion.skip_round(round, sample_count);
    randomness.skip_round(round, sample_count);
}


Sample const* const* LFO::initialize_rendering(
        Integer const round,
        Integer const sample_count
) noexcept {
    min_buffer = FloatParamS::produce_if_not_constant(min, round, sample_count);
    max_buffer = FloatParamS::produce_if_not_constant(max, round, sample_count);
    distortion_buffer = FloatParamS::produce_if_not_constant(
        distortion, round, sample_count
    );
    randomness_buffer = FloatParamS::produce_if_not_constant(
        randomness, round, sample_count
    );
    oscillator_buffer = SignalProducer::produce<Oscillator_>(
        oscillator, round, sample_count
    );

    return NULL;
}


void LFO::render(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample** buffer
) noexcept {
    if (center.get_value() == ToggleParam::OFF) {
        apply_distortions(
            distortion_buffer,
            randomness_buffer,
            round,
            first_sample_index,
            last_sample_index,
            oscillator_buffer[0],
            buffer[0]
        );
        apply_range(
            min_buffer,
            max_buffer,
            round,
            first_sample_index,
            last_sample_index,
            buffer[0],
            buffer[0]
        );
    } else {
        apply_distortions_centered(
            distortion_buffer,
            randomness_buffer,
            round,
            first_sample_index,
            last_sample_index,
            oscillator_buffer[0],
            buffer[0]
        );
        apply_range_centered(
            min_buffer,
            max_buffer,
            round,
            first_sample_index,
            last_sample_index,
            buffer[0],
            buffer[0]
        );
    }
}


void LFO::produce_with_envelope(
        LFOEnvelopeStates& lfo_envelope_states,
        Integer const round,
        Integer const sample_count,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample* buffer
) noexcept {
    LFOWithEnvelopeRenderer renderer(
        lfo_envelope_states,
        round,
        sample_count,
        first_sample_index,
        last_sample_index,
        buffer
    );
    Byte depth = 0;

    if (JS80P_UNLIKELY(!should_visit_lfo_as_polyphonic(*this, depth, renderer))) {
        renderer.visit_lfo_as_global(*this);

        return;
    }

    traverse_lfo_graph<LFOWithEnvelopeRenderer>(*this, depth, renderer);
}


void LFO::apply_distortions(
        Sample const* const distortion_buffer,
        Sample const* const randomness_buffer,
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample const* const source_buffer,
        Sample* const target_buffer
) {
    if (distortion_buffer == NULL) {
        Number const distortion = this->distortion.get_value();

        if (randomness_buffer == NULL) {
            Number const randomness = this->randomness.get_value();

            if (randomness < ALMOST_ZERO && distortion < ALMOST_ZERO) {
                if ((void*)target_buffer != (void*)source_buffer) {
                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        target_buffer[i] = source_buffer[i];
                    }
                }
            } else {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    target_buffer[i] = Math::randomize(
                        randomness,
                        Math::distort(distortion, source_buffer[i])
                    );
                }
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                target_buffer[i] = Math::randomize(
                    randomness_buffer[i],
                    Math::distort(distortion, source_buffer[i])
                );
            }
        }
    } else {
        if (randomness_buffer == NULL) {
            Number const randomness = this->randomness.get_value();

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                target_buffer[i] = Math::randomize(
                    randomness,
                    Math::distort(distortion_buffer[i], source_buffer[i])
                );
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                target_buffer[i] = Math::randomize(
                    randomness_buffer[i],
                    Math::distort(distortion_buffer[i], source_buffer[i])
                );
            }
        }
    }
}


void LFO::apply_range(
        Sample const* const min_buffer,
        Sample const* const max_buffer,
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample const* const source_buffer,
        Sample* const target_buffer
) {
    if (min_buffer == NULL) {
        Sample const min_value = (Sample)min.get_value();

        if (max_buffer == NULL) {
            Sample const max_value = (Sample)max.get_value();

            if (
                    min_value <= ALMOST_ZERO
                    && Math::is_close(max_value, max.get_max_value(), ALMOST_ZERO)
            ) {
                if ((void*)target_buffer != (void*)source_buffer) {
                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        target_buffer[i] = source_buffer[i];

                        JS80P_ASSERT(target_buffer[i] >= min_value);
                        JS80P_ASSERT(target_buffer[i] <= max_value);
                    }
                }
            } else {
                Sample const range = max_value - min_value;

                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    target_buffer[i] = min_value + range * source_buffer[i];

                    JS80P_ASSERT(target_buffer[i] >= min_value);
                    JS80P_ASSERT(target_buffer[i] <= max_value);
                }
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const max_value = max_buffer[i];
                Sample const range = max_value - min_value;

                target_buffer[i] = min_value + range * source_buffer[i];

                JS80P_ASSERT(target_buffer[i] >= min_value);
                JS80P_ASSERT(target_buffer[i] <= max_value);
            }
        }
    } else {
        if (max_buffer == NULL) {
            Sample const max_value = (Sample)max.get_value();

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const min_value = min_buffer[i];
                Sample const range = max_value - min_value;

                target_buffer[i] = min_buffer[i] + range * source_buffer[i];

                JS80P_ASSERT(target_buffer[i] >= min_value);
                JS80P_ASSERT(target_buffer[i] <= max_value);
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const max_value = max_buffer[i];
                Sample const min_value = min_buffer[i];
                Sample const range = max_value - min_value;

                target_buffer[i] = min_buffer[i] + range * source_buffer[i];

                JS80P_ASSERT(target_buffer[i] >= min_value);
                JS80P_ASSERT(target_buffer[i] <= max_value);
            }
        }
    }
}


void LFO::apply_distortions_centered(
        Sample const* const distortion_buffer,
        Sample const* const randomness_buffer,
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample const* const source_buffer,
        Sample* const target_buffer
) {
    if (distortion_buffer == NULL) {
        Number const distortion = this->distortion.get_value();

        if (randomness_buffer == NULL) {
            Number const randomness = this->randomness.get_value();

            if (randomness < ALMOST_ZERO && distortion < ALMOST_ZERO) {
                if ((void*)target_buffer != (void*)source_buffer) {
                    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                        target_buffer[i] = source_buffer[i];
                    }
                }
            } else {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    target_buffer[i] = Math::randomize_centered_lfo(
                        randomness,
                        Math::distort_centered_lfo(distortion, source_buffer[i])
                    );
                }
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                target_buffer[i] = Math::randomize_centered_lfo(
                    randomness_buffer[i],
                    Math::distort_centered_lfo(distortion, source_buffer[i])
                );
            }
        }
    } else {
        if (randomness_buffer == NULL) {
            Number const randomness = this->randomness.get_value();

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                target_buffer[i] = Math::randomize_centered_lfo(
                    randomness,
                    Math::distort_centered_lfo(distortion_buffer[i], source_buffer[i])
                );
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                target_buffer[i] = Math::randomize_centered_lfo(
                    randomness_buffer[i],
                    Math::distort_centered_lfo(distortion_buffer[i], source_buffer[i])
                );
            }
        }
    }
}


void LFO::apply_range_centered(
        Sample const* const min_buffer,
        Sample const* const max_buffer,
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample const* const source_buffer,
        Sample* const target_buffer
) {
    if (min_buffer == NULL) {
        Sample const min_value = (Sample)min.get_value();

        if (max_buffer == NULL) {
            Sample const max_value = (Sample)max.get_value();
            Sample const center = (min_value + max_value) * 0.5;
            Sample const range = max_value - min_value;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                target_buffer[i] = (
                    center + range * (source_buffer[i])
                );

                JS80P_ASSERT(target_buffer[i] >= min_value || Math::is_close(target_buffer[i], min_value));
                JS80P_ASSERT(target_buffer[i] <= max_value || Math::is_close(target_buffer[i], max_value));

                target_buffer[i] = std::min(max_value, std::max(min_value, target_buffer[i]));
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const max_value = max_buffer[i];
                Sample const center = (min_value + max_value) * 0.5;
                Sample const range = max_value - min_value;

                target_buffer[i] = (
                    center + range * (source_buffer[i])
                );

                JS80P_ASSERT(target_buffer[i] >= min_value || Math::is_close(target_buffer[i], min_value));
                JS80P_ASSERT(target_buffer[i] <= max_value || Math::is_close(target_buffer[i], max_value));

                target_buffer[i] = std::min(max_value, std::max(min_value, target_buffer[i]));
            }
        }
    } else {
        if (max_buffer == NULL) {
            Sample const max_value = (Sample)max.get_value();

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const min_value = min_buffer[i];
                Sample const center = (min_value + max_value) * 0.5;
                Sample const range = max_value - min_value;

                target_buffer[i] = (
                    center + range * (source_buffer[i])
                );

                JS80P_ASSERT(target_buffer[i] >= min_value || Math::is_close(target_buffer[i], min_value));
                JS80P_ASSERT(target_buffer[i] <= max_value || Math::is_close(target_buffer[i], max_value));

                target_buffer[i] = std::min(max_value, std::max(min_value, target_buffer[i]));
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const min_value = min_buffer[i];
                Sample const max_value = max_buffer[i];
                Sample const center = (min_value + max_value) * 0.5;
                Sample const range = max_value - min_value;

                target_buffer[i] = (
                    center + range * (source_buffer[i])
                );

                JS80P_ASSERT(target_buffer[i] >= min_value || Math::is_close(target_buffer[i], min_value));
                JS80P_ASSERT(target_buffer[i] <= max_value || Math::is_close(target_buffer[i], max_value));

                target_buffer[i] = std::min(max_value, std::max(min_value, target_buffer[i]));
            }
        }
    }
}

}

#endif
