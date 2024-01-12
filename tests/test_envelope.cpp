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

#include <cmath>

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "dsp/envelope.cpp"
#include "dsp/lfo.cpp"
#include "dsp/macro.cpp"
#include "dsp/math.cpp"
#include "dsp/midi_controller.cpp"
#include "dsp/oscillator.cpp"
#include "dsp/param.cpp"
#include "dsp/queue.cpp"
#include "dsp/signal_producer.cpp"
#include "dsp/wavetable.cpp"


using namespace JS80P;


TEST(an_envelope_is_a_collection_of_params, {
    Envelope envelope("N1");

    assert_eq("N1DYN", envelope.dynamic.get_name());
    assert_eq("N1AMT", envelope.amount.get_name());
    assert_eq("N1INI", envelope.initial_value.get_name());
    assert_eq("N1DEL", envelope.delay_time.get_name());
    assert_eq("N1ATK", envelope.attack_time.get_name());
    assert_eq("N1PK", envelope.peak_value.get_name());
    assert_eq("N1HLD", envelope.hold_time.get_name());
    assert_eq("N1DEC", envelope.decay_time.get_name());
    assert_eq("N1SUS", envelope.sustain_value.get_name());
    assert_eq("N1REL", envelope.release_time.get_name());
    assert_eq("N1FIN", envelope.final_value.get_name());
    assert_eq("N1TIN", envelope.time_inaccuracy.get_name());
    assert_eq("N1VIN", envelope.value_inaccuracy.get_name());
})


TEST(can_tell_whether_envelope_is_dynamic, {
    Envelope envelope("E");

    envelope.dynamic.set_value(ToggleParam::OFF);
    assert_false(envelope.is_dynamic());

    envelope.dynamic.set_value(ToggleParam::ON);
    assert_true(envelope.is_dynamic());
})


TEST(when_a_param_of_an_envelope_changes_then_the_change_index_of_the_envelope_is_changed, {
    Envelope envelope("E");
    Integer old_change_index;

    old_change_index = envelope.get_change_index();
    envelope.amount.set_value(0.99);
    envelope.dynamic.set_value(ToggleParam::OFF);
    envelope.update();
    assert_neq((int)old_change_index, (int)envelope.get_change_index());

    old_change_index = envelope.get_change_index();
    envelope.dynamic.set_value(ToggleParam::ON);
    envelope.update();
    assert_neq((int)old_change_index, (int)envelope.get_change_index());
})


TEST(when_inaccuracy_is_non_zero_then_randomizes_times_and_levels, {
    constexpr Number amount = 0.1;
    constexpr Number initial_value = 0.3;
    constexpr Number peak_value = 0.5;
    constexpr Number sustain_value = 0.7;
    constexpr Number final_value = 0.8;

    constexpr Seconds delay_time = 1.0;
    constexpr Seconds attack_time = 2.0;
    constexpr Seconds hold_time = 3.0;
    constexpr Seconds decay_time = 4.0;
    constexpr Seconds release_time = 5.0;

    constexpr Number time_inaccuracy = 0.3;
    constexpr Number value_inaccuracy = 0.9;

    constexpr Number random = 0.6;
    constexpr EnvelopeRandoms randoms = {
        random, random, random, random, random, random, random, random, random,
    };

    constexpr Seconds time_offset = (
        Envelope::TIME_INACCURACY_MAX * time_inaccuracy * random
    );
    constexpr Number value_scale = (
        ((random - 0.5) * value_inaccuracy + 1.0) * amount
    );

    Envelope envelope("E");
    EnvelopeSnapshot snapshot;

    envelope.amount.set_value(amount);
    envelope.initial_value.set_value(initial_value);
    envelope.delay_time.set_value(delay_time);
    envelope.attack_time.set_value(attack_time);
    envelope.peak_value.set_value(peak_value);
    envelope.hold_time.set_value(hold_time);
    envelope.decay_time.set_value(decay_time);
    envelope.sustain_value.set_value(sustain_value);
    envelope.release_time.set_value(release_time);
    envelope.final_value.set_value(final_value);
    envelope.time_inaccuracy.set_value(time_inaccuracy);
    envelope.value_inaccuracy.set_value(value_inaccuracy);

    envelope.update();
    envelope.make_snapshot(randoms, snapshot);

    assert_eq(value_scale * initial_value, snapshot.initial_value, DOUBLE_DELTA);
    assert_eq(value_scale * peak_value, snapshot.peak_value, DOUBLE_DELTA);
    assert_eq(value_scale * sustain_value, snapshot.sustain_value, DOUBLE_DELTA);
    assert_eq(value_scale * final_value, snapshot.final_value, DOUBLE_DELTA);

    assert_eq(time_offset + delay_time, snapshot.delay_time, DOUBLE_DELTA);
    assert_eq(time_offset + attack_time, snapshot.attack_time, DOUBLE_DELTA);
    assert_eq(time_offset + hold_time, snapshot.hold_time, DOUBLE_DELTA);
    assert_eq(time_offset + decay_time, snapshot.decay_time, DOUBLE_DELTA);
    assert_eq(time_offset + release_time, snapshot.release_time, DOUBLE_DELTA);
})
