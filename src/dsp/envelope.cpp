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
#include "dsp/signal_producer.hpp"


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
    change_index(-1),
    attack_shape(Envelope::SHAPE_LINEAR),
    decay_shape(Envelope::SHAPE_LINEAR),
    release_shape(Envelope::SHAPE_LINEAR),
    envelope_index(Constants::INVALID_ENVELOPE_INDEX)
{
}


Envelope::ShapeParam::ShapeParam(std::string const& name) noexcept
    : ByteParam(name, SHAPE_SMOOTH_SMOOTH, SHAPE_LINEAR, SHAPE_LINEAR)
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
    EnvelopeShape shape = SHAPE_LINEAR;
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
        shape,
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

    if (shape == SHAPE_LINEAR) {
        set_up_interpolation<true, false>(
            initial_value,
            delta,
            ratio,
            last_rendered_value,
            target_value,
            duration,
            time_until_target,
            sampling_period,
            1.0 / duration,
            stage_,
            shape
        );
    } else {
        set_up_interpolation<true, true>(
            initial_value,
            delta,
            ratio,
            last_rendered_value,
            target_value,
            duration,
            time_until_target,
            sampling_period,
            1.0 / duration,
            stage_,
            shape
        );

        ratio = Math::apply_envelope_shape((Math::EnvelopeShape)shape, ratio);
    }

    return initial_value + ratio * delta;
}


template<bool adjust_initial_value_during_dahds, bool need_shaping_for_initial_value_adjustment>
void Envelope::set_up_interpolation(
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
) noexcept {
    JS80P_ASSERT(duration > 0.0);
    JS80P_ASSERT(time_until_target >= 0.0);

    Seconds const elapsed_time = duration - time_until_target;

    if (
            stage != EnvelopeStage::ENV_STG_DAHD
            || (adjust_initial_value_during_dahds && elapsed_time >= sampling_period)
    ) {
        Number const adjusted_initial_value = find_adjusted_initial_value<need_shaping_for_initial_value_adjustment>(
            elapsed_time, sampling_period, duration_inv, last_rendered_value, target_value, shape
        );

        if (JS80P_UNLIKELY(!Math::is_close(adjusted_initial_value, initial_value))) {
            initial_value = adjusted_initial_value;
        }
    }

    initial_ratio = elapsed_time * duration_inv;
    delta = target_value - initial_value;
}


template<bool need_shaping>
Number Envelope::find_adjusted_initial_value(
        Seconds const elapsed_time,
        Seconds const sampling_period,
        Number const duration_inv,
        Number const last_rendered_value,
        Number const target_value,
        EnvelopeShape const shape
) noexcept {
    /*
    If the envelope snapshot was changed since the last value had been rendered
    (e.g. dynamic envelope), then the initial value that comes from the current
    state of the snapshot does not agree with the current envelope time and the
    last_rendered_value, so we calculate an adjusted initial value which would
    yield last_rendered_value at the current envelope time with the current
    state of the snapshot.

    Let f(x) be the shaping function, f(0.0) = 0, f(1.0) = 1.0:

    last_rendered_v = adjusted_iv + f(ratio) * (target_v - adjusted_iv)
    adjusted_iv = last_rendered_v - f(ratio) * (target_v - adjusted_iv)
    adjusted_iv = last_rendered_v - f(ratio) * target_v + f(ratio) * adjusted_iv
    adjusted_iv - f(ratio) * adjusted_iv = last_rendered_v - f(ratio) * target_v
    adjusted_iv * (1.0 - f(ratio)) = last_rendered_v - f(ratio) * target_v
    adjusted_iv = (last_rendered_v - f(ratio) * target_v) / (1.0 - f(ratio))

    Note: the closer the ratio is to 1.0, the less the potential error in
    adjusted_iv matters, because the rest of the rendering calculation will
    eliminate it anyways - as long as we don't divide by 0, we're fine.
    */

    Number const last_rendered_value_ratio = (
        std::max(0.0, elapsed_time - sampling_period) * duration_inv
    );

    if constexpr (need_shaping) {
        Number const shaped_ratio = Math::apply_envelope_shape(
            (Math::EnvelopeShape)shape, last_rendered_value_ratio
        );

        return (
            (last_rendered_value - shaped_ratio * target_value)
            / std::max(ALMOST_ZERO, 1.0 - shaped_ratio)
        );
    } else {
        return (
            (last_rendered_value - last_rendered_value_ratio * target_value)
            / std::max(ALMOST_ZERO, 1.0 - last_rendered_value_ratio)
        );
    }
}


template<Envelope::RenderingMode rendering_mode>
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
        render_constant<rendering_mode>(
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
        EnvelopeShape shape;

        set_up_next_target(
            snapshot,
            last_rendered_value,
            time,
            stage,
            initial_value,
            target_value,
            time_until_target,
            duration,
            shape,
            becomes_constant,
            sampling_period
        );

        if (becomes_constant) {
            last_rendered_value = target_value;
            render_constant<rendering_mode>(
                time, target_value, i, last_sample_index, buffer
            );

            return;
        }

        if (JS80P_UNLIKELY(duration < ALMOST_ZERO || time_until_target < ALMOST_ZERO)) {
            time += ALMOST_ZERO;
            last_rendered_value = target_value;

            continue;
        }

        if (shape == SHAPE_LINEAR) {
            Envelope::render<rendering_mode, false>(
                time,
                stage,
                last_rendered_value,
                initial_value,
                target_value,
                duration,
                time_until_target,
                sample_rate,
                sampling_period,
                first_sample_index,
                last_sample_index,
                shape,
                buffer,
                i
            );
        } else {
            Envelope::render<rendering_mode, true>(
                time,
                stage,
                last_rendered_value,
                initial_value,
                target_value,
                duration,
                time_until_target,
                sample_rate,
                sampling_period,
                first_sample_index,
                last_sample_index,
                shape,
                buffer,
                i
            );
        }
    }
}


template<Envelope::RenderingMode rendering_mode, bool need_shaping>
void Envelope::render(
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
        Integer& next_sample_index
) noexcept {
    Number const duration_inv = 1.0 / duration;
    Number const scale = sampling_period * duration_inv;
    Integer const end_index = std::min(
        last_sample_index,
        next_sample_index + std::max(
            (Integer)1, (Integer)(time_until_target * sample_rate)
        )
    );

    Number rendered_value = last_rendered_value;
    Number done_samples = 0.0;
    Number initial_ratio;
    Number delta;

    if (next_sample_index == first_sample_index) {
        set_up_interpolation<true, need_shaping>(
            initial_value,
            delta,
            initial_ratio,
            last_rendered_value,
            target_value,
            duration,
            time_until_target,
            sampling_period,
            duration_inv,
            stage,
            shape
        );
    } else {
        set_up_interpolation<false, need_shaping>(
            initial_value,
            delta,
            initial_ratio,
            last_rendered_value,
            target_value,
            duration,
            time_until_target,
            sampling_period,
            duration_inv,
            stage,
            shape
        );
    }

    for (; next_sample_index != end_index; ++next_sample_index, done_samples += 1.0) {
        if constexpr (need_shaping) {
            Number const ratio = Math::apply_envelope_shape(
                (Math::EnvelopeShape)shape, initial_ratio + done_samples * scale
            );

            rendered_value = initial_value + ratio * delta;
        } else {
            Number const ratio = initial_ratio + done_samples * scale;

            rendered_value = initial_value + ratio * delta;
        }

        if constexpr (rendering_mode == RenderingMode::OVERWRITE) {
            buffer[next_sample_index] = rendered_value;
        } else {
            buffer[next_sample_index] *= rendered_value;
        }
    }

    last_rendered_value = rendered_value;
    time += done_samples * sampling_period;
}


template<Envelope::RenderingMode rendering_mode>
void Envelope::render_constant(
        Seconds& time,
        Number const value,
        Integer const first_sample_index,
        Integer const last_sample_index,
        Sample* buffer
) noexcept {
    time = 0.0;

    for (Integer i = first_sample_index; i != last_sample_index; ++i) {
        if constexpr (rendering_mode == RenderingMode::OVERWRITE) {
            buffer[i] = value;
        } else {
            buffer[i] *= value;
        }
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
        EnvelopeShape& shape,
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
                shape,
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
                shape,
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
                shape,
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
                shape,
                becomes_constant
            );

            break;

        default:
            time_until_target = 0.0;
            duration = 0.0;
            initial_value = last_rendered_value;
            target_value = last_rendered_value;
            shape = SHAPE_LINEAR;
            becomes_constant = true;

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
        EnvelopeShape& shape,
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
        shape = SHAPE_LINEAR;
        becomes_constant = false;

        return;
    }

    time_until_target += snapshot.attack_time;

    if (time_until_target > 0.0) {
        duration = snapshot.attack_time;
        initial_value = snapshot.initial_value;
        target_value = snapshot.peak_value;
        shape = snapshot.attack_shape;
        becomes_constant = false;

        return;
    }

    time_until_target += snapshot.hold_time;

    if (time_until_target > 0.0) {
        duration = snapshot.hold_time;
        initial_value = snapshot.peak_value;
        target_value = snapshot.peak_value;
        shape = SHAPE_LINEAR;
        becomes_constant = false;

        return;
    }

    time_until_target += snapshot.decay_time;
    target_value = snapshot.sustain_value;

    if (time_until_target > 0.0) {
        duration = snapshot.decay_time;
        initial_value = snapshot.peak_value;
        shape = snapshot.decay_shape;
        becomes_constant = false;

        return;
    }

    initial_value = snapshot.sustain_value;
    time = 0.0;
    stage = EnvelopeStage::ENV_STG_SUSTAIN;
    shape = SHAPE_LINEAR;
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
        EnvelopeShape& shape,
        bool& becomes_constant
) noexcept {
    initial_value = snapshot.sustain_value;
    shape = SHAPE_LINEAR;
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
        EnvelopeShape& shape,
        bool& becomes_constant
) noexcept {
    /* current-v ==release-t==> release-v */

    initial_value = snapshot.sustain_value;
    target_value = snapshot.final_value;
    duration = snapshot.release_time;
    shape = snapshot.release_shape;

    time_until_target = duration - time;
    becomes_constant = time_until_target < ALMOST_ZERO;

    if (JS80P_UNLIKELY(becomes_constant)) {
        stage = EnvelopeStage::ENV_STG_RELEASED;
        time_until_target = 0.0;
        duration = 0.0;
        time = 0.0;
        shape = SHAPE_LINEAR;
    }
}


Envelope::Envelope(std::string const& name) noexcept
    : update_mode(
        /*
        Envelopes used to have only 2 update modes: never update (static), or
        update continuously (dynamic), and an on-off toggle was used for
        turning on dynamic updates. The parameter's name is kept in order to be
        able to load old presets and host application saved states.
        */
        name + "UPD", UPDATE_MODE_DYNAMIC_LAST, UPDATE_MODE_DYNAMIC, UPDATE_MODE_STATIC
    ),
    tempo_sync(name + "SYN", ToggleParam::OFF),
    attack_shape(name + "ASH"),
    decay_shape(name + "DSH"),
    release_shape(name + "RSH"),
    scale(name + "AMT",             0.0,    1.0,  1.0),
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
    update_mode_change_index(-1),
    tempo_sync_change_index(-1),
    attack_shape_change_index(-1),
    decay_shape_change_index(-1),
    release_shape_change_index(-1),
    scale_change_index(-1),
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
    update_bpm(tempo_sync.get_bpm());
    update();
}


void Envelope::update_bpm(Number const new_bpm) noexcept
{
    JS80P_ASSERT(new_bpm >= SignalProducer::MIN_BPM);

    bpm = new_bpm;
    tempo_sync_time_scale = Math::SECONDS_IN_ONE_MINUTE / new_bpm;
}


void Envelope::update() noexcept
{
    bool is_dirty;

    is_dirty = update_change_index(delay_time, delay_time_change_index);
    is_dirty = update_change_index(attack_time, attack_time_change_index) || is_dirty;
    is_dirty = update_change_index(hold_time, hold_time_change_index) || is_dirty;
    is_dirty = update_change_index(decay_time, decay_time_change_index) || is_dirty;

    is_dirty = update_change_index<ByteParam>(update_mode, update_mode_change_index) || is_dirty;
    is_dirty = update_change_index<ToggleParam>(tempo_sync, tempo_sync_change_index) || is_dirty;

    is_dirty = update_change_index<ShapeParam>(attack_shape, attack_shape_change_index) || is_dirty;
    is_dirty = update_change_index<ShapeParam>(decay_shape, decay_shape_change_index) || is_dirty;
    is_dirty = update_change_index<ShapeParam>(release_shape, release_shape_change_index) || is_dirty;

    is_dirty = update_change_index(scale, scale_change_index) || is_dirty;
    is_dirty = update_change_index(initial_value, initial_value_change_index) || is_dirty;
    is_dirty = update_change_index(peak_value, peak_value_change_index) || is_dirty;
    is_dirty = update_change_index(sustain_value, sustain_value_change_index) || is_dirty;
    is_dirty = update_change_index(release_time, release_time_change_index) || is_dirty;
    is_dirty = update_change_index(final_value, final_value_change_index) || is_dirty;

    is_dirty = update_change_index(time_inaccuracy, time_inaccuracy_change_index) || is_dirty;
    is_dirty = update_change_index(value_inaccuracy, value_inaccuracy_change_index) || is_dirty;

    if (is_tempo_synced()) {
        Number const new_bpm = tempo_sync.get_bpm();

        if (JS80P_UNLIKELY(!Math::is_close(bpm, new_bpm))) {
            update_bpm(new_bpm);
            is_dirty = true;
        }
    }

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
    return update_mode.get_value() == UPDATE_MODE_DYNAMIC;
}


bool Envelope::is_static() const noexcept
{
    return update_mode.get_value() == UPDATE_MODE_STATIC;
}


bool Envelope::is_tempo_synced() const noexcept
{
    return tempo_sync.get_value() == ToggleParam::ON;
}


bool Envelope::needs_update(Byte const voice_status) const noexcept
{
    constexpr Byte masks[] = {
        [Envelope::UPDATE_MODE_DYNAMIC_LAST] = Constants::VOICE_STATUS_LAST,
        [Envelope::UPDATE_MODE_DYNAMIC_OLDEST] = Constants::VOICE_STATUS_OLDEST,
        [Envelope::UPDATE_MODE_DYNAMIC_LOWEST] = Constants::VOICE_STATUS_LOWEST,
        [Envelope::UPDATE_MODE_DYNAMIC_HIGHEST] = Constants::VOICE_STATUS_HIGHEST,
        [Envelope::UPDATE_MODE_STATIC] = 0,
        [Envelope::UPDATE_MODE_END] = 0,
        [Envelope::UPDATE_MODE_DYNAMIC] = 0,
    };

    return is_dynamic() || (voice_status & masks[update_mode.get_value()]) != 0;
}


template<class ParamType = FloatParamB>
bool Envelope::update_change_index(ParamType const& param, Integer& change_index) noexcept
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
        Byte const envelope_index,
        EnvelopeSnapshot& snapshot
) const noexcept {
    snapshot.change_index = get_change_index();

    if (value_inaccuracy.get_value() > 0.000001) {
        snapshot.initial_value = randomize_value(initial_value, randoms[0]);
        snapshot.peak_value = randomize_value(peak_value, randoms[1]);
        snapshot.sustain_value = randomize_value(sustain_value, randoms[2]);
        snapshot.final_value = randomize_value(final_value, randoms[3]);
    } else {
        Number const scale = this->scale.get_value();

        snapshot.initial_value = initial_value.get_value() * scale;
        snapshot.peak_value = peak_value.get_value() * scale;
        snapshot.sustain_value = sustain_value.get_value() * scale;
        snapshot.final_value = final_value.get_value() * scale;
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

    if (tempo_sync.get_value() == ToggleParam::ON) {
        snapshot.delay_time *= tempo_sync_time_scale;
        snapshot.attack_time *= tempo_sync_time_scale;
        snapshot.hold_time *= tempo_sync_time_scale;
        snapshot.decay_time *= tempo_sync_time_scale;
        snapshot.release_time *= tempo_sync_time_scale;
    }

    snapshot.attack_shape = attack_shape.get_value();
    snapshot.decay_shape = decay_shape.get_value();
    snapshot.release_shape = release_shape.get_value();

    snapshot.envelope_index = envelope_index;
}


void Envelope::make_end_snapshot(
        EnvelopeRandoms const& randoms,
        Byte const envelope_index,
        EnvelopeSnapshot& snapshot
) const noexcept {
    snapshot.change_index = get_change_index();

    if (value_inaccuracy.get_value() > 0.000001) {
        snapshot.final_value = randomize_value(final_value, randoms[3]);
    } else {
        snapshot.final_value = final_value.get_value() * scale.get_value();
    }

    if (time_inaccuracy.get_value() > 0.000001) {
        snapshot.release_time = randomize_time(release_time, randoms[8]);
    } else {
        snapshot.release_time = release_time.get_value();
    }

    if (tempo_sync.get_value() == ToggleParam::ON) {
        snapshot.release_time *= tempo_sync_time_scale;
    }

    snapshot.release_shape = release_shape.get_value();

    snapshot.envelope_index = envelope_index;
}


Number Envelope::randomize_value(
        FloatParamB const& param,
        Number const random
) const noexcept {
    Number const scale = ((random - 0.5) * value_inaccuracy.get_value() + 1.0);

    return std::min(1.0, scale * this->scale.get_value() * param.get_value());
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
