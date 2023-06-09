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

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "synth/envelope.cpp"
#include "synth/flexible_controller.cpp"
#include "synth/lfo.cpp"
#include "synth/math.cpp"
#include "synth/midi_controller.cpp"
#include "synth/oscillator.cpp"
#include "synth/param.cpp"
#include "synth/queue.cpp"
#include "synth/signal_producer.cpp"
#include "synth/wavetable.cpp"


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
})


TEST(can_calculate_dahd_length, {
    Envelope envelope("E");

    envelope.amount.set_value(0.1);
    envelope.initial_value.set_value(0.2);
    envelope.delay_time.set_value(1.0);
    envelope.attack_time.set_value(2.0);
    envelope.peak_value.set_value(0.3);
    envelope.hold_time.set_value(3.0);
    envelope.decay_time.set_value(4.0);
    envelope.sustain_value.set_value(0.5);
    envelope.release_time.set_value(5.0);
    envelope.final_value.set_value(0.6);

    envelope.update();

    assert_eq(10.0, envelope.get_dahd_length(), DOUBLE_DELTA);
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
