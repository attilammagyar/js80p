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

#ifndef JS80P__DSP__ENVELOPE_CPP
#define JS80P__DSP__ENVELOPE_CPP

#include <algorithm>

#include "dsp/envelope.hpp"


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


Envelope::Envelope(std::string const name) noexcept
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
