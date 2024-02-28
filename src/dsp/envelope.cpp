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

#ifndef JS80P__DSP__ENVELOPE_CPP
#define JS80P__DSP__ENVELOPE_CPP

#include <cmath>
#include <algorithm>

#include "dsp/envelope.hpp"

#include "dsp/math.hpp"


namespace JS80P
{

EnvelopeSnapshot::EnvelopeSnapshot() noexcept
    : initial_value(0.0),
    peak_value(1.0),
    sustain_value(0.7),
    final_value(0.0),
    delay_time(0.0),
    attack_time(0.02),
    hold_time(0.3),
    decay_time(0.6),
    release_time(0.1),
    change_index(-1)
{
}


Number Envelope::get_value_at_time(
        EnvelopeSnapshot const& snapshot,
        Seconds const time,
        EnvelopeStage const stage,
        Number const last_rendered_value,
        Seconds const sampling_period
) noexcept {
    if (JS80P_UNLIKELY(stage == EnvelopeStage::ENV_STG_NONE)) {
        return last_rendered_value;
    }

    Seconds time_ = time;
    EnvelopeStage stage_ = stage;
    Number initial_value;
    Number target_value;
    Seconds time_until_target;
    Seconds duration;
    bool becomes_constant;

    set_up_next_target(
        snapshot,
        last_rendered_value,
        time_,
        stage_,
        initial_value,
        target_value,
        time_until_target,
        duration,
        becomes_constant,
        sampling_period
    );

    if (
            becomes_constant
            || JS80P_UNLIKELY(duration < ALMOST_ZERO || time_until_target < ALMOST_ZERO)
    ) {
        return target_value;
    }

    Number delta;
    Number ratio;

    set_up_interpolation<true>(
        initial_value,
        delta,
        ratio,
        last_rendered_value,
        target_value,
        stage_,
        duration,
        time_until_target,
        sampling_period,
        1.0 / duration
    );

    return initial_value + ratio * delta;
}


template<bool adjust_initial_value_during_dahds>
void Envelope::set_up_interpolation(
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
) noexcept {
    JS80P_ASSERT(duration > 0.0);
    JS80P_ASSERT(time_until_target >= 0.0);

    Seconds const elapsed_time = duration - time_until_target;

    if (
            stage != EnvelopeStage::ENV_STG_DAHD
            || (adjust_initial_value_during_dahds && elapsed_time >= sampling_period)
    ) {
        Number const old_ratio = (
            std::max(0.0, elapsed_time - sampling_period) * duration_inv
        );
        Number const old_initial_value = (
            (last_rendered_value - old_ratio * target_value)
            / std::max(ALMOST_ZERO, 1.0 - old_ratio)
        );

        if (JS80P_UNLIKELY(!Math::is_close(old_initial_value, initial_value))) {
            initial_value = old_initial_value;
        }
    }

    initial_ratio = elapsed_time * duration_inv;
    delta = target_value - initial_value;
}


void Envelope::render(
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
) noexcept {
    if (JS80P_UNLIKELY(stage == EnvelopeStage::ENV_STG_NONE)) {
        becomes_constant = true;
        render_constant(
            time,
            last_rendered_value,
            first_sample_index,
            last_sample_index,
            buffer
        );

        return;
    }

    Integer i = first_sample_index;

    while (i != last_sample_index) {
        Number initial_value;
        Number target_value;
        Seconds time_until_target;
        Seconds duration;

        set_up_next_target(
            snapshot,
            last_rendered_value,
            time,
            stage,
            initial_value,
            target_value,
            time_until_target,
            duration,
            becomes_constant,
            sampling_period
        );

        if (becomes_constant) {
            last_rendered_value = target_value;
            render_constant(time, target_value, i, last_sample_index, buffer);

            return;
        }

        if (JS80P_UNLIKELY(duration < ALMOST_ZERO || time_until_target < ALMOST_ZERO)) {
            time += ALMOST_ZERO;
            last_rendered_value = target_value;

            continue;
        }

        Number const duration_inv = 1.0 / duration;
        Number const scale = sampling_period * duration_inv;
        Integer const end_index = std::min(
            last_sample_index,
            i + std::max((Integer)1, (Integer)(time_until_target * sample_rate))
        );

        Number done_samples = 0.0;
        Number initial_ratio;
        Number delta;

        if (i == first_sample_index) {
            set_up_interpolation<true>(
                initial_value,
                delta,
                initial_ratio,
                last_rendered_value,
                target_value,
                stage,
                duration,
                time_until_target,
                sampling_period,
                duration_inv
            );
        } else {
            set_up_interpolation<false>(
                initial_value,
                delta,
                initial_ratio,
                last_rendered_value,
                target_value,
                stage,
                duration,
                time_until_target,
                sampling_period,
                duration_inv
            );
        }

        for (; i != end_index; ++i, done_samples += 1.0) {
            Number const ratio = initial_ratio + done_samples * scale;

            buffer[i] = initial_value + ratio * delta;
        }

        time += done_samples * sampling_period;
        last_rendered_value = buffer[i - 1];
    }
}


void Envelope::render_constant(
        Seconds& time,
        Number const value,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample* buffer
) noexcept {
    time = 0.0;

    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
        buffer[i] = value;
    }
}


void Envelope::set_up_next_target(
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
) noexcept {
    switch (stage) {
        case EnvelopeStage::ENV_STG_DAHD:
            set_up_next_dahds_target(
                snapshot,
                last_rendered_value,
                time,
                stage,
                initial_value,
                target_value,
                time_until_target,
                duration,
                becomes_constant,
                sampling_period
            );

            break;

        case EnvelopeStage::ENV_STG_SUSTAIN:
            target_value = snapshot.sustain_value;

            set_up_next_sustain_target(
                snapshot,
                last_rendered_value,
                time,
                initial_value,
                target_value,
                time_until_target,
                duration,
                becomes_constant
            );

            break;

        case EnvelopeStage::ENV_STG_RELEASE:
            set_up_next_release_target(
                snapshot,
                time,
                stage,
                initial_value,
                target_value,
                time_until_target,
                duration,
                becomes_constant
            );

            break;

        case EnvelopeStage::ENV_STG_RELEASED:
            target_value = snapshot.final_value;

            set_up_next_sustain_target(
                snapshot,
                last_rendered_value,
                time,
                initial_value,
                target_value,
                time_until_target,
                duration,
                becomes_constant
            );

            break;

        default:
            becomes_constant = true;
            time_until_target = 0.0;
            duration = 0.0;
            initial_value = last_rendered_value;
            target_value = last_rendered_value;

            break;
    }
}


void Envelope::set_up_next_dahds_target(
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
) noexcept {
    /*
    init-v =del-t=> init-v =atk-t=> peak-v =hold-t=> peak-v =dec-t=> sust-v
    */

    time_until_target = snapshot.delay_time - time;

    if (time_until_target > 0.0) {
        duration = snapshot.delay_time;
        initial_value = snapshot.initial_value;
        target_value = snapshot.initial_value;
        becomes_constant = false;

        return;
    }

    time_until_target += snapshot.attack_time;

    if (time_until_target > 0.0) {
        duration = snapshot.attack_time;
        initial_value = snapshot.initial_value;
        target_value = snapshot.peak_value;
        becomes_constant = false;

        return;
    }

    time_until_target += snapshot.hold_time;

    if (time_until_target > 0.0) {
        duration = snapshot.hold_time;
        initial_value = snapshot.peak_value;
        target_value = snapshot.peak_value;
        becomes_constant = false;

        return;
    }

    time_until_target += snapshot.decay_time;
    target_value = snapshot.sustain_value;

    if (time_until_target > 0.0) {
        duration = snapshot.decay_time;
        initial_value = snapshot.peak_value;
        becomes_constant = false;

        return;
    }

    initial_value = snapshot.sustain_value;
    stage = EnvelopeStage::ENV_STG_SUSTAIN;
    time = 0.0;
    becomes_constant = std::fabs(time_until_target) < sampling_period;

    if (JS80P_LIKELY(becomes_constant)) {
        time_until_target = 0.0;
        duration = 0.0;
    } else {
        duration = DYNAMIC_ENVELOPE_RAMP_TIME;
        time_until_target = DYNAMIC_ENVELOPE_RAMP_TIME;
    }
}


void Envelope::set_up_next_sustain_target(
        EnvelopeSnapshot const& snapshot,
        Number const last_rendered_value,
        Seconds const time,
        Number& initial_value,
        Number const target_value,
        Seconds& time_until_target,
        Seconds& duration,
        bool& becomes_constant
) noexcept {
    initial_value = snapshot.sustain_value;
    becomes_constant = Math::is_close(last_rendered_value, target_value);

    if (JS80P_LIKELY(becomes_constant)) {
        duration = 0.0;
        time_until_target = 0.0;
    } else {
        duration = DYNAMIC_ENVELOPE_RAMP_TIME;
        time_until_target = std::max(0.0, duration - time);
    }
}


void Envelope::set_up_next_release_target(
        EnvelopeSnapshot const& snapshot,
        Seconds& time,
        EnvelopeStage& stage,
        Number& initial_value,
        Number& target_value,
        Seconds& time_until_target,
        Seconds& duration,
        bool& becomes_constant
) noexcept {
    /* current-v ==release-t==> release-v */

    initial_value = snapshot.sustain_value;
    target_value = snapshot.final_value;
    duration = snapshot.release_time;
    time_until_target = duration - time;
    becomes_constant = time_until_target < ALMOST_ZERO;

    if (JS80P_UNLIKELY(becomes_constant)) {
        stage = EnvelopeStage::ENV_STG_RELEASED;
        time_until_target = 0.0;
        duration = 0.0;
        time = 0.0;
    }
}


Envelope::Envelope(std::string const& name) noexcept
    : dynamic(name + "DYN", ToggleParam::OFF),
    amount(name + "AMT",            0.0,    1.0,  1.0),
    initial_value(name + "INI",     0.0,    1.0,  0.0),
    delay_time(name + "DEL",        0.0,    6.0,  0.0),
    attack_time(name + "ATK",       0.0,    6.0,  0.02),
    peak_value(name + "PK",         0.0,    1.0,  1.0),
    hold_time(name + "HLD",         0.0,   12.0,  0.3),
    decay_time(name + "DEC",        0.001, 15.0,  0.6),
    sustain_value(name + "SUS",     0.0,    1.0,  0.7),
    release_time(name + "REL",      0.0,    6.0,  0.1),
    final_value(name + "FIN",       0.0,    1.0,  0.0),
    time_inaccuracy(name + "TIN",   0.0,    1.0,  0.0),
    value_inaccuracy(name + "VIN",  0.0,    1.0,  0.0),
    dynamic_change_index(-1),
    amount_change_index(-1),
    initial_value_change_index(-1),
    delay_time_change_index(-1),
    attack_time_change_index(-1),
    peak_value_change_index(-1),
    hold_time_change_index(-1),
    decay_time_change_index(-1),
    sustain_value_change_index(-1),
    release_time_change_index(-1),
    final_value_change_index(-1),
    time_inaccuracy_change_index(-1),
    value_inaccuracy_change_index(-1),
    change_index(-1)
{
    update();
}


void Envelope::update() noexcept
{
    bool is_dirty;

    is_dirty = update_change_index(delay_time, delay_time_change_index);
    is_dirty |= update_change_index(attack_time, attack_time_change_index);
    is_dirty |= update_change_index(hold_time, hold_time_change_index);
    is_dirty |= update_change_index(decay_time, decay_time_change_index);

    is_dirty |= update_change_index<ToggleParam>(dynamic, dynamic_change_index);

    is_dirty |= update_change_index(amount, amount_change_index);
    is_dirty |= update_change_index(initial_value, initial_value_change_index);
    is_dirty |= update_change_index(peak_value, peak_value_change_index);
    is_dirty |= update_change_index(sustain_value, sustain_value_change_index);
    is_dirty |= update_change_index(release_time, release_time_change_index);
    is_dirty |= update_change_index(final_value, final_value_change_index);

    is_dirty |= update_change_index(time_inaccuracy, time_inaccuracy_change_index);
    is_dirty |= update_change_index(value_inaccuracy, value_inaccuracy_change_index);

    if (is_dirty) {
        ++change_index;
        change_index &= 0x7fffffff;
    }
}


Integer Envelope::get_change_index() const noexcept
{
    return change_index;
}


bool Envelope::is_dynamic() const noexcept
{
    return dynamic.get_value() == ToggleParam::ON;
}


template<class ParamType = FloatParamB>
bool Envelope::update_change_index(ParamType const& param, Integer& change_index)
{
    Integer const new_change_index = param.get_change_index();

    if (new_change_index != change_index) {
        change_index = new_change_index;

        return true;
    }

    return false;
}


void Envelope::make_snapshot(
        EnvelopeRandoms const& randoms,
        EnvelopeSnapshot& snapshot
) const noexcept {
    snapshot.change_index = get_change_index();

    if (value_inaccuracy.get_value() > 0.000001) {
        snapshot.initial_value = randomize_value(initial_value, randoms[0]);
        snapshot.peak_value = randomize_value(peak_value, randoms[1]);
        snapshot.sustain_value = randomize_value(sustain_value, randoms[2]);
        snapshot.final_value = randomize_value(final_value, randoms[3]);
    } else {
        Number const amount = this->amount.get_value();

        snapshot.initial_value = initial_value.get_value() * amount;
        snapshot.peak_value = peak_value.get_value() * amount;
        snapshot.sustain_value = sustain_value.get_value() * amount;
        snapshot.final_value = final_value.get_value() * amount;
    }

    if (time_inaccuracy.get_value() > 0.000001) {
        snapshot.delay_time = randomize_time(delay_time, randoms[4]);
        snapshot.attack_time = randomize_time(attack_time, randoms[5]);
        snapshot.hold_time = randomize_time(hold_time, randoms[6]);
        snapshot.decay_time = randomize_time(decay_time, randoms[7]);
        snapshot.release_time = randomize_time(release_time, randoms[8]);
    } else {
        snapshot.delay_time = delay_time.get_value();
        snapshot.attack_time = attack_time.get_value();
        snapshot.hold_time = hold_time.get_value();
        snapshot.decay_time = decay_time.get_value();
        snapshot.release_time = release_time.get_value();
    }
}


void Envelope::make_end_snapshot(
        EnvelopeRandoms const& randoms,
        EnvelopeSnapshot& snapshot
) const noexcept {
    snapshot.change_index = get_change_index();

    if (value_inaccuracy.get_value() > 0.000001) {
        snapshot.final_value = randomize_value(final_value, randoms[3]);
    } else {
        snapshot.final_value = final_value.get_value() * amount.get_value();
    }

    if (time_inaccuracy.get_value() > 0.000001) {
        snapshot.release_time = randomize_time(release_time, randoms[8]);
    } else {
        snapshot.release_time = release_time.get_value();
    }
}


Number Envelope::randomize_value(
        FloatParamB const& param,
        Number const random
) const noexcept {
    Number const scale = ((random - 0.5) * value_inaccuracy.get_value() + 1.0);

    return std::min(1.0, scale * amount.get_value() * param.get_value());
}


Seconds Envelope::randomize_time(
        FloatParamB const& param,
        Number const random
) const noexcept {
    Seconds const inaccuracy = (
        random * time_inaccuracy.get_value() * TIME_INACCURACY_MAX
    );

    return std::min(param.get_max_value(), inaccuracy + param.get_value());
}

}

#endif
