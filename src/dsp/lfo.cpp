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

#ifndef JS80P__DSP__LFO_CPP
#define JS80P__DSP__LFO_CPP

#include "dsp/lfo.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

LFO::LFO(std::string const name) noexcept
    : SignalProducer(1, 10),
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
    register_child(oscillator);
}


LFO::LFO(
        std::string const name,
        FloatParam& frequency_leader,
        FloatParam& max_leader,
        FloatParam& amount_leader,
        ToggleParam& tempo_sync_,
        Number const phase_offset
) noexcept
    : SignalProducer(1, 10),
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

    frequency.cancel_events(time_offset);
    phase.cancel_events(time_offset);
    min.cancel_events(time_offset);
    max.cancel_events(time_offset);
    amount.cancel_events(time_offset);
    distortion.cancel_events(time_offset);
    randomness.cancel_events(time_offset);
}


bool LFO::is_on() const noexcept
{
    return oscillator.is_on();
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
    min_buffer = FloatParam::produce_if_not_constant(min, round, sample_count);
    max_buffer = FloatParam::produce_if_not_constant(max, round, sample_count);
    distortion_buffer = FloatParam::produce_if_not_constant(
        distortion, round, sample_count
    );
    randomness_buffer = FloatParam::produce_if_not_constant(
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

            if (randomness < 0.000001 && distortion < 0.000001) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    target_buffer[i] = source_buffer[i];
                }

                return;
            }

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                target_buffer[i] = Math::randomize(
                    randomness,
                    Math::distort(distortion, source_buffer[i])
                );
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
            Sample const range = max_value - min_value;

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                target_buffer[i] = (
                    min_value + range * (source_buffer[i])
                );
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const range = max_buffer[i] - min_value;

                target_buffer[i] = (
                    min_value + range * (source_buffer[i])
                );
            }
        }
    } else {
        if (max_buffer == NULL) {
            Sample const max_value = (Sample)max.get_value();

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const range = max_value - min_buffer[i];

                target_buffer[i] = (
                    min_buffer[i] + range * (source_buffer[i])
                );
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const range = max_buffer[i] - min_buffer[i];

                target_buffer[i] = (
                    min_buffer[i] + range * (source_buffer[i])
                );
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

            if (randomness < 0.000001 && distortion < 0.000001) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    target_buffer[i] = source_buffer[i];
                }

                return;
            }

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                target_buffer[i] = Math::randomize_centered_lfo(
                    randomness,
                    Math::distort_centered_lfo(distortion, source_buffer[i])
                );
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
            }
        } else {
            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                Sample const max_value = max_buffer[i];
                Sample const center = (min_value + max_value) * 0.5;
                Sample const range = max_value - min_value;

                target_buffer[i] = (
                    center + range * (source_buffer[i])
                );
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
            }
        }
    }
}

}

#endif
