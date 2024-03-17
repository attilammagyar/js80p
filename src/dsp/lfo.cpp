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

LFO::LFO(std::string const& name) noexcept
    : SignalProducer(1, 12, 0, &oscillator),
    waveform(name + "WAV", Oscillator_::SOFT_SQUARE),
    frequency(name + "FRQ", 0.01, 30.0, 1.0),
    phase(name + "PHS", 0.0, 1.0, 0.0),
    min(name + "MIN", 0.0, 1.0, 0.0),
    max(name + "MAX", 0.0, 1.0, 1.0),
    amount(name + "AMT", 0.0, 0.5, 0.5),
    distortion(name + "DST", 0.0, 1.0, 0.0),
    randomness(name + "RND", 0.0, 1.0, 0.0),
    tempo_sync(name + "SYN", ToggleParam::OFF),
    center(name + "CEN", ToggleParam::OFF),
    amount_envelope(name + "AEN", 0, Constants::ENVELOPES, Constants::ENVELOPES),
    oscillator(waveform, amount, frequency, phase, tempo_sync, center)
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
    register_child(tempo_sync);
    register_child(center);
    register_child(amount_envelope);
    register_child(oscillator);
}


LFO::LFO(
        std::string const& name,
        FloatParamS& frequency_leader,
        FloatParamS& max_leader,
        FloatParamS& amount_leader,
        ToggleParam& tempo_sync_,
        Number const phase_offset
) noexcept
    : SignalProducer(1, 12, 0, &oscillator),
    waveform(name + "WAV", Oscillator_::SOFT_SQUARE),
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
    oscillator(waveform, amount, frequency, phase, tempo_sync_, center)
{
    initialize_instance();
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
            round, first_sample_index, last_sample_index, oscillator_buffer[0], buffer[0]
        );
        apply_range(
            round, first_sample_index, last_sample_index, buffer[0], buffer[0]
        );
    } else {
        apply_distortions_centered(
            round, first_sample_index, last_sample_index, oscillator_buffer[0], buffer[0]
        );
        apply_range_centered(
            round, first_sample_index, last_sample_index, buffer[0], buffer[0]
        );
    }
}


void LFO::produce_with_envelope(
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
) noexcept {
    if (JS80P_UNLIKELY(!has_envelope())) {
        Sample const* const lfo_buffer = SignalProducer::produce<LFO>(
            *this, round, sample_count
        )[0];

        for (Integer i = first_sample_index; i != last_sample_index; ++i) {
            buffer[i] = lfo_buffer[i];
        }

        return;
    }

    min_buffer = FloatParamS::produce_if_not_constant(min, round, sample_count);
    max_buffer = FloatParamS::produce_if_not_constant(max, round, sample_count);
    distortion_buffer = FloatParamS::produce_if_not_constant(
        distortion, round, sample_count
    );
    randomness_buffer = FloatParamS::produce_if_not_constant(
        randomness, round, sample_count
    );

    oscillator.produce_for_lfo_with_envelope(
        wavetable_state,
        round,
        sample_count,
        first_sample_index,
        last_sample_index,
        buffer
    );

    bool envelope_is_constant = false;

    Envelope::render<Envelope::RenderingMode::MULTIPLY>(
        envelope_snapshot,
        envelope_time,
        envelope_stage,
        envelope_is_constant,
        envelope_value,
        this->sample_rate,
        this->sampling_period,
        first_sample_index,
        last_sample_index,
        buffer
    );

    if (center.get_value() == ToggleParam::OFF) {
        apply_distortions(
            round, first_sample_index, last_sample_index, buffer, buffer
        );
    } else {
        apply_distortions_centered(
            round, first_sample_index, last_sample_index, buffer, buffer
        );
    }

    if (center.get_value() == ToggleParam::OFF) {
        apply_range(
            round, first_sample_index, last_sample_index, buffer, buffer
        );
    } else {
        apply_range_centered(
            round, first_sample_index, last_sample_index, buffer, buffer
        );
    }
}


void LFO::apply_distortions(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample const* source_buffer,
        Sample* target_buffer
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
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample const* source_buffer,
        Sample* target_buffer
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
                Sample const range = max_buffer[i] - min_value;

                target_buffer[i] = min_value + range * source_buffer[i];

                JS80P_ASSERT(target_buffer[i] >= min_value);
                JS80P_ASSERT(target_buffer[i] <= max_buffer[i]);
            }
        }
    } else {
        if (max_buffer == NULL) {
            Sample const max_value = (Sample)max.get_value();

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const range = max_value - min_buffer[i];

                target_buffer[i] = min_buffer[i] + range * source_buffer[i];

                JS80P_ASSERT(target_buffer[i] >= min_buffer[i]);
                JS80P_ASSERT(target_buffer[i] <= max_value);
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const range = max_buffer[i] - min_buffer[i];

                target_buffer[i] = min_buffer[i] + range * source_buffer[i];

                JS80P_ASSERT(target_buffer[i] >= min_buffer[i]);
                JS80P_ASSERT(target_buffer[i] <= max_buffer[i]);
            }
        }
    }
}


void LFO::apply_distortions_centered(
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample const* source_buffer,
        Sample* target_buffer
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
        Integer const round,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample const* source_buffer,
        Sample* target_buffer
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

                JS80P_ASSERT(target_buffer[i] >= min_value);
                JS80P_ASSERT(target_buffer[i] <= max_value);
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const max_value = max_buffer[i];
                Sample const center = (min_value + max_value) * 0.5;
                Sample const range = max_value - min_value;

                target_buffer[i] = (
                    center + range * (source_buffer[i])
                );

                JS80P_ASSERT(target_buffer[i] >= min_value);
                JS80P_ASSERT(target_buffer[i] <= max_value);
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

                JS80P_ASSERT(target_buffer[i] >= min_value);
                JS80P_ASSERT(target_buffer[i] <= max_value);
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

                JS80P_ASSERT(target_buffer[i] >= min_value);
                JS80P_ASSERT(target_buffer[i] <= max_value);
            }
        }
    }
}

}

#endif
