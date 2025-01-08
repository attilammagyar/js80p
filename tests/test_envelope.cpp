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

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"

#include "dsp/envelope.cpp"
#include "dsp/lfo.cpp"
#include "dsp/lfo_envelope_list.cpp"
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

    assert_eq("N1UPD", envelope.update_mode.get_name());
    assert_eq("N1AMT", envelope.scale.get_name());
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


TEST(can_tell_whether_envelope_is_dynamic_or_static, {
    Envelope envelope("E");

    envelope.update_mode.set_value(Envelope::UPDATE_MODE_STATIC);
    assert_false(envelope.is_dynamic());
    assert_true(envelope.is_static());

    envelope.update_mode.set_value(Envelope::UPDATE_MODE_END);
    assert_false(envelope.is_dynamic());
    assert_false(envelope.is_static());

    envelope.update_mode.set_value(Envelope::UPDATE_MODE_DYNAMIC);
    assert_true(envelope.is_dynamic());
    assert_false(envelope.is_static());
})


void assert_needs_update(
        bool const expected,
        Byte const update_mode,
        Byte const voice_status
) {
    Envelope envelope("E");

    envelope.update_mode.set_value(update_mode);

    assert_eq(
        expected,
        envelope.needs_update(voice_status),
        "update_mode=%hhd, voice_status=%hhd",
        update_mode,
        voice_status
    );
}


TEST(can_tell_if_snapshot_update_is_needed_for_a_given_voice_status, {
    assert_needs_update(true, Envelope::UPDATE_MODE_DYNAMIC, Constants::VOICE_STATUS_NORMAL);
    assert_needs_update(false, Envelope::UPDATE_MODE_STATIC, Constants::VOICE_STATUS_NORMAL);
    assert_needs_update(false, Envelope::UPDATE_MODE_END, Constants::VOICE_STATUS_NORMAL);

    assert_needs_update(true, Envelope::UPDATE_MODE_DYNAMIC_LAST, Constants::VOICE_STATUS_LAST);
    assert_needs_update(false, Envelope::UPDATE_MODE_DYNAMIC_LAST, Constants::VOICE_STATUS_OLDEST);
    assert_needs_update(false, Envelope::UPDATE_MODE_DYNAMIC_LAST, Constants::VOICE_STATUS_LOWEST);
    assert_needs_update(false, Envelope::UPDATE_MODE_DYNAMIC_LAST, Constants::VOICE_STATUS_HIGHEST);

    assert_needs_update(false, Envelope::UPDATE_MODE_DYNAMIC_OLDEST, Constants::VOICE_STATUS_LAST);
    assert_needs_update(true, Envelope::UPDATE_MODE_DYNAMIC_OLDEST, Constants::VOICE_STATUS_OLDEST);
    assert_needs_update(false, Envelope::UPDATE_MODE_DYNAMIC_OLDEST, Constants::VOICE_STATUS_LOWEST);
    assert_needs_update(false, Envelope::UPDATE_MODE_DYNAMIC_OLDEST, Constants::VOICE_STATUS_HIGHEST);

    assert_needs_update(false, Envelope::UPDATE_MODE_DYNAMIC_LOWEST, Constants::VOICE_STATUS_LAST);
    assert_needs_update(false, Envelope::UPDATE_MODE_DYNAMIC_LOWEST, Constants::VOICE_STATUS_OLDEST);
    assert_needs_update(true, Envelope::UPDATE_MODE_DYNAMIC_LOWEST, Constants::VOICE_STATUS_LOWEST);
    assert_needs_update(false, Envelope::UPDATE_MODE_DYNAMIC_LOWEST, Constants::VOICE_STATUS_HIGHEST);

    assert_needs_update(false, Envelope::UPDATE_MODE_DYNAMIC_HIGHEST, Constants::VOICE_STATUS_LAST);
    assert_needs_update(false, Envelope::UPDATE_MODE_DYNAMIC_HIGHEST, Constants::VOICE_STATUS_OLDEST);
    assert_needs_update(false, Envelope::UPDATE_MODE_DYNAMIC_HIGHEST, Constants::VOICE_STATUS_LOWEST);
    assert_needs_update(true, Envelope::UPDATE_MODE_DYNAMIC_HIGHEST, Constants::VOICE_STATUS_HIGHEST);

    assert_needs_update(
        true,
        Envelope::UPDATE_MODE_DYNAMIC_LAST,
        Constants::VOICE_STATUS_LAST | Constants::VOICE_STATUS_HIGHEST
    );
    assert_needs_update(
        true,
        Envelope::UPDATE_MODE_DYNAMIC_OLDEST,
        Constants::VOICE_STATUS_OLDEST | Constants::VOICE_STATUS_LOWEST
    );
    assert_needs_update(
        true,
        Envelope::UPDATE_MODE_DYNAMIC_LOWEST,
        Constants::VOICE_STATUS_LOWEST | Constants::VOICE_STATUS_OLDEST
    );
    assert_needs_update(
        true,
        Envelope::UPDATE_MODE_DYNAMIC_HIGHEST,
        Constants::VOICE_STATUS_HIGHEST | Constants::VOICE_STATUS_LAST
    );

    assert_needs_update(
        false,
        Envelope::UPDATE_MODE_DYNAMIC_LOWEST,
        Constants::VOICE_STATUS_LAST | Constants::VOICE_STATUS_HIGHEST
    );
    assert_needs_update(
        false,
        Envelope::UPDATE_MODE_DYNAMIC_HIGHEST,
        Constants::VOICE_STATUS_OLDEST | Constants::VOICE_STATUS_LOWEST
    );
    assert_needs_update(
        false,
        Envelope::UPDATE_MODE_DYNAMIC_LAST,
        Constants::VOICE_STATUS_LOWEST | Constants::VOICE_STATUS_OLDEST
    );
    assert_needs_update(
        false,
        Envelope::UPDATE_MODE_DYNAMIC_OLDEST,
        Constants::VOICE_STATUS_HIGHEST | Constants::VOICE_STATUS_LAST
    );
});


TEST(when_a_param_of_an_envelope_changes_then_the_change_index_of_the_envelope_is_changed, {
    Envelope envelope("E");
    Integer old_change_index;

    old_change_index = envelope.get_change_index();
    envelope.scale.set_value(0.99);
    envelope.update_mode.set_value(Envelope::UPDATE_MODE_STATIC);
    envelope.update();
    assert_neq((int)old_change_index, (int)envelope.get_change_index());

    old_change_index = envelope.get_change_index();
    envelope.update_mode.set_value(Envelope::UPDATE_MODE_DYNAMIC);
    envelope.update();
    assert_neq((int)old_change_index, (int)envelope.get_change_index());
})


TEST(when_the_tempo_is_changed_then_tempo_synced_envelope_change_index_is_changed, {
    Envelope tempo_synced("T");
    Envelope not_tempo_synced("N");
    Integer old_change_index_tempo_synced;
    Integer old_change_index_not_tempo_synced;

    tempo_synced.tempo_sync.set_value(ToggleParam::ON);
    not_tempo_synced.tempo_sync.set_value(ToggleParam::OFF);

    assert_true(tempo_synced.is_tempo_synced());
    assert_false(not_tempo_synced.is_tempo_synced());

    tempo_synced.update();
    not_tempo_synced.update();

    old_change_index_tempo_synced = tempo_synced.get_change_index();
    old_change_index_not_tempo_synced = not_tempo_synced.get_change_index();

    tempo_synced.tempo_sync.set_bpm(123.0);
    not_tempo_synced.tempo_sync.set_bpm(123.0);

    tempo_synced.update();
    not_tempo_synced.update();

    assert_neq((int)old_change_index_tempo_synced, (int)tempo_synced.get_change_index());
    assert_eq((int)old_change_index_not_tempo_synced, (int)not_tempo_synced.get_change_index());
})


void test_tempo_synced_snapshot_creation(
        Envelope& envelope,
        Number const time_inaccuracy,
        Number const time_scale
) {
    constexpr EnvelopeRandoms randoms = {
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    };
    EnvelopeSnapshot snapshot;

    envelope.delay_time.set_value(1.0);
    envelope.attack_time.set_value(2.0);
    envelope.hold_time.set_value(3.0);
    envelope.decay_time.set_value(4.0);
    envelope.release_time.set_value(5.0);
    envelope.time_inaccuracy.set_value(time_inaccuracy);

    envelope.update();
    envelope.make_snapshot(randoms, 1, snapshot);

    assert_eq(snapshot.delay_time, 1.0 * time_scale, DOUBLE_DELTA);
    assert_eq(snapshot.attack_time, 2.0 * time_scale, DOUBLE_DELTA);
    assert_eq(snapshot.hold_time, 3.0 * time_scale, DOUBLE_DELTA);
    assert_eq(snapshot.decay_time, 4.0 * time_scale, DOUBLE_DELTA);
    assert_eq(snapshot.release_time, 5.0 * time_scale, DOUBLE_DELTA);
    assert_eq(1, snapshot.envelope_index);

    envelope.release_time.set_value(6.0);
    envelope.update();

    envelope.make_end_snapshot(randoms, 2, snapshot);

    assert_eq(snapshot.release_time, 6.0 * time_scale, DOUBLE_DELTA);
    assert_eq(2, snapshot.envelope_index);
}


TEST(when_envelope_is_tempo_synced_then_snapshot_times_are_measured_in_beats_instead_of_seconds, {
    Envelope tempo_synced("T");
    Envelope not_tempo_synced("N");

    tempo_synced.tempo_sync.set_value(ToggleParam::ON);
    tempo_synced.tempo_sync.set_bpm(120.0);
    not_tempo_synced.tempo_sync.set_value(ToggleParam::OFF);
    not_tempo_synced.tempo_sync.set_bpm(120.0);

    tempo_synced.update();
    not_tempo_synced.update();

    test_tempo_synced_snapshot_creation(not_tempo_synced, 0.0, 1.0);
    test_tempo_synced_snapshot_creation(tempo_synced, 0.0, 0.5);

    test_tempo_synced_snapshot_creation(not_tempo_synced, 1.0, 1.0);
    test_tempo_synced_snapshot_creation(tempo_synced, 1.0, 0.5);
})


TEST(too_small_bpm_is_ignored_when_considering_tempo_sync, {
    Envelope envelope("E");
    Integer old_change_index;

    envelope.tempo_sync.set_value(ToggleParam::ON);
    envelope.update();

    old_change_index = envelope.get_change_index();

    envelope.tempo_sync.set_bpm(0.0);
    envelope.update();

    assert_eq((int)old_change_index, (int)envelope.get_change_index());
})


TEST(when_inaccuracy_is_non_zero_then_randomizes_times_and_levels, {
    constexpr Number scale = 0.1;
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
        ((random - 0.5) * value_inaccuracy + 1.0) * scale
    );

    Envelope envelope("E");
    EnvelopeSnapshot snapshot;

    envelope.scale.set_value(scale);
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
    envelope.make_snapshot(randoms, 0, snapshot);

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


#define JS80P_ENVELOPE_RENDER_DEBUG_FMT                 \
    "test_name=\"%s\"\n    "                            \
        "rendering_mode=%d,\n    "                      \
        "initial_buffer_value=%f,    \n"                \
        "batch_size=%d,\n    "                          \
        "elapsed_time_at_start=%f, "                    \
        "value_at_start=%f,\n    "                      \
        "initial_stage=%d,\n    "                       \
        "v=[%f, %f, %f, %f], d=[%f, %f, %f, %f, %f]"

#define JS80P_ENVELOPE_RENDER_DEBUG_PARAMS              \
    test_name,                                          \
    (int)rendering_mode,                                \
    initial_buffer_value,                               \
    (int)batch_size,                                    \
    elapsed_time_at_start,                              \
    value_at_start,                                     \
    (int)initial_stage,                                 \
    envelope_values[0],                                 \
    envelope_values[1],                                 \
    envelope_values[2],                                 \
    envelope_values[3],                                 \
    envelope_durations[0],                              \
    envelope_durations[1],                              \
    envelope_durations[2],                              \
    envelope_durations[3],                              \
    envelope_durations[4]


template<int expected_samples_count, Envelope::RenderingMode rendering_mode>
void test_envelope_rendering_with_batch_size(
        char const* const test_name,
        Integer const batch_size,
        std::array<Number, 4> const& envelope_values,
        std::array<Seconds, 5> const& envelope_durations,
        Seconds const elapsed_time_at_start,
        EnvelopeStage const initial_stage,
        Number const value_at_start,
        Frequency const sample_rate,
        std::array<Sample, expected_samples_count> const& expected_samples,
        bool const expected_constantness,
        Sample const initial_buffer_value
) {
    Seconds const sampling_period = 1.0 / sample_rate;

    EnvelopeSnapshot snapshot;

    snapshot.initial_value = envelope_values[0];
    snapshot.peak_value = envelope_values[1];
    snapshot.sustain_value = envelope_values[2];
    snapshot.final_value = envelope_values[3];

    snapshot.delay_time = envelope_durations[0];
    snapshot.attack_time = envelope_durations[1];
    snapshot.hold_time = envelope_durations[2];
    snapshot.decay_time = envelope_durations[3];
    snapshot.release_time = envelope_durations[4];

    Sample buffer[expected_samples_count];
    Seconds time = elapsed_time_at_start;
    Number value = value_at_start;
    EnvelopeStage stage = initial_stage;
    bool becomes_constant = false;

    std::fill_n(buffer, expected_samples_count, initial_buffer_value);

    Integer index = 0;

    while (index != expected_samples_count) {
        Integer const increment = std::min(
            batch_size, expected_samples_count - index
        );
        Number const calculated_first_value = (
            Envelope::get_value_at_time(
                snapshot,
                time,
                stage,
                value,
                sampling_period
            )
            * (
                rendering_mode == Envelope::RenderingMode::OVERWRITE
                    ? 1.0
                    : initial_buffer_value
            )
        );

        Envelope::render<rendering_mode>(
            snapshot,
            time,
            stage,
            becomes_constant,
            value,
            sample_rate,
            sampling_period,
            index,
            index + increment,
            buffer
        );

        assert_eq(
            calculated_first_value,
            buffer[index],
            DOUBLE_DELTA,
            JS80P_ENVELOPE_RENDER_DEBUG_FMT ",\n    index=%d",
            JS80P_ENVELOPE_RENDER_DEBUG_PARAMS,
            (int)index
        );

        index += increment;
    }

    assert_eq(
        expected_samples.data(),
        buffer,
        expected_samples_count,
        DOUBLE_DELTA,
        JS80P_ENVELOPE_RENDER_DEBUG_FMT,
        JS80P_ENVELOPE_RENDER_DEBUG_PARAMS
    );
    assert_eq(
        expected_constantness,
        becomes_constant,
        JS80P_ENVELOPE_RENDER_DEBUG_FMT,
        JS80P_ENVELOPE_RENDER_DEBUG_PARAMS
    );
}


template<int expected_samples_count>
void test_envelope_rendering(
        char const* const test_name,
        std::array<Number, 4> const& envelope_values,
        std::array<Seconds, 5> const& envelope_durations,
        Seconds const elapsed_time_at_start,
        EnvelopeStage const initial_stage,
        Number const value_at_start,
        Frequency const sample_rate,
        std::array<Sample, expected_samples_count> const& expected_samples,
        bool const expected_constantness
) {
    Integer const max_batch_size = std::min(12, expected_samples_count);

    std::array<Sample, expected_samples_count> expected_zeros;
    std::array<Sample, expected_samples_count> expected_samples_half;

    std::fill(expected_zeros.begin(), expected_zeros.end(), 0.0);

    for (int i = 0; i != expected_samples_count; ++i) {
        expected_samples_half[i] = 0.5 * expected_samples[i];
    }

    for (Integer batch_size = 1; batch_size != max_batch_size; ++batch_size) {
        test_envelope_rendering_with_batch_size<expected_samples_count, Envelope::RenderingMode::OVERWRITE>(
            test_name,
            batch_size,
            envelope_values,
            envelope_durations,
            elapsed_time_at_start,
            initial_stage,
            value_at_start,
            sample_rate,
            expected_samples,
            expected_constantness,
            0.0
        );
        test_envelope_rendering_with_batch_size<expected_samples_count, Envelope::RenderingMode::MULTIPLY>(
            test_name,
            batch_size,
            envelope_values,
            envelope_durations,
            elapsed_time_at_start,
            initial_stage,
            value_at_start,
            sample_rate,
            expected_zeros,
            expected_constantness,
            0.0
        );
        test_envelope_rendering_with_batch_size<expected_samples_count, Envelope::RenderingMode::MULTIPLY>(
            test_name,
            batch_size,
            envelope_values,
            envelope_durations,
            elapsed_time_at_start,
            initial_stage,
            value_at_start,
            sample_rate,
            expected_samples_half,
            expected_constantness,
            0.5
        );
        test_envelope_rendering_with_batch_size<expected_samples_count, Envelope::RenderingMode::MULTIPLY>(
            test_name,
            batch_size,
            envelope_values,
            envelope_durations,
            elapsed_time_at_start,
            initial_stage,
            value_at_start,
            sample_rate,
            expected_samples,
            expected_constantness,
            1.0
        );
    }
}


TEST(envelope_rendering, {
    test_envelope_rendering<12>(
        "DAH, starting at 0",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 2.0, 3.0, 2.0, 1.0},
        0.0,
        EnvelopeStage::ENV_STG_DAHD,
        0.0,
        2.0,
        {
            0.000, 0.000, 0.000, 0.250, 0.500, 0.750,
            1.000, 1.000, 1.000, 1.000, 1.000, 1.000,
        },
        false
    );
    test_envelope_rendering<18>(
        "DAHDS, starting at 0",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 2.0, 3.0, 2.0, 1.0},
        0.0,
        EnvelopeStage::ENV_STG_DAHD,
        0.0,
        2.0,
        {
            0.000, 0.000, 0.000, 0.250, 0.500, 0.750,
            1.000, 1.000, 1.000, 1.000, 1.000, 1.000,
            1.000, 0.875, 0.750, 0.625, 0.500, 0.500,
        },
        true
    );
    test_envelope_rendering<18>(
        "DAHDS with offset",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 2.0, 3.0, 2.0, 1.0},
        0.25,
        EnvelopeStage::ENV_STG_DAHD,
        0.0,
        2.0,
        {
            0.0000, 0.0000, 0.1250, 0.3750, 0.6250, 0.8750,
            1.0000, 1.0000, 1.0000, 1.0000, 1.0000, 1.0000,
            0.9375, 0.8125, 0.6875, 0.5625, 0.5000, 0.5000,
        },
        true
    );
    test_envelope_rendering<18>(
        "DAHDS, from the middle of attack",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 2.0, 3.0, 2.0, 1.0},
        2.0,
        EnvelopeStage::ENV_STG_DAHD,
        0.25,
        2.0,
        {
            0.500, 0.750, 1.000, 1.000, 1.000, 1.000,
            1.000, 1.000, 1.000, 0.875, 0.750, 0.625,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
        },
        true
    );
    test_envelope_rendering<18>(
        "DAHDS, from the end of decay",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 2.0, 3.0, 2.0, 1.0},
        9.0,
        EnvelopeStage::ENV_STG_DAHD,
        0.5,
        2.0,
        {
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
        },
        true
    );
    test_envelope_rendering<18>(
        "DAHDS, from sustaining",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 2.0, 3.0, 2.0, 1.0},
        0.0,
        EnvelopeStage::ENV_STG_SUSTAIN,
        0.5,
        2.0,
        {
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
        },
        true
    );
    test_envelope_rendering<12>(
        "R, from the beginning of release",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 2.0, 3.0, 2.0, 2.0},
        0.0,
        EnvelopeStage::ENV_STG_RELEASE,
        0.5,
        2.0,
        {
            0.500, 0.375, 0.250, 0.125, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
        },
        true
    );
    test_envelope_rendering<12>(
        "R, from the beginning of release with offset",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 2.0, 3.0, 2.0, 2.0},
        0.25,
        EnvelopeStage::ENV_STG_RELEASE,
        0.5,
        2.0,
        {
            0.4375, 0.3125, 0.1875, 0.0625, 0.0000, 0.0000,
            0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000,
        },
        true
    );

    test_envelope_rendering<18>(
        "extreme short envelope, starting at 0",
        {0.0, 1.0, 0.5, 0.0},
        {0.0, 0.0, 0.0, 0.001, 0.0},
        0.0,
        EnvelopeStage::ENV_STG_DAHD,
        0.0,
        2.0,
        {

            1.000, 0.500, 0.500, 0.500, 0.500, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
        },
        true
    );
    test_envelope_rendering<18>(
        "extreme short envelope with delay, starting at 0",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 0.0, 0.0, 0.001, 0.0},
        0.0,
        EnvelopeStage::ENV_STG_DAHD,
        0.0,
        2.0,
        {

            0.000, 0.000, 1.000, 0.500, 0.500, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
        },
        true
    );
    test_envelope_rendering<18>(
        "extreme short envelope with delay, from the middle of delay",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 0.0, 0.0, 0.001, 0.0},
        0.5,
        EnvelopeStage::ENV_STG_DAHD,
        0.0,
        2.0,
        {

            0.000, 1.000, 0.500, 0.500, 0.500, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
        },
        true
    );
    test_envelope_rendering<18>(
        "extreme short envelope with delay, from the end of delay",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 0.0, 0.0, 0.001, 0.0},
        1.0,
        EnvelopeStage::ENV_STG_DAHD,
        0.0,
        2.0,
        {

            1.000, 0.500, 0.500, 0.500, 0.500, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
        },
        true
    );
    test_envelope_rendering<18>(
        "extreme short envelope, from sustain",
        {0.0, 1.0, 0.5, 0.0},
        {0.0, 0.0, 0.0, 0.001, 0.0},
        0.0,
        EnvelopeStage::ENV_STG_SUSTAIN,
        0.5,
        2.0,
        {

            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
        },
        true
    );
    test_envelope_rendering<12>(
        "extreme short envelope, from release",
        {0.0, 1.0, 0.5, 0.0},
        {0.0, 0.0, 0.0, 0.001, 0.0},
        0.0,
        EnvelopeStage::ENV_STG_RELEASE,
        0.5,
        2.0,
        {

            0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
        },
        true
    );

    test_envelope_rendering<12>(
        "released during hold",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 2.0, 3.0, 3.0, 2.0},
        0.0,
        EnvelopeStage::ENV_STG_RELEASE,
        1.0,
        2.0,
        {
            1.000, 0.750, 0.500, 0.250, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
        },
        true
    );

    test_envelope_rendering<12>(
        "snapshot changed during delay",
        {0.0, 0.5, 0.0, 0.0},
        {1.5, 2.0, 1.0, 1.0, 2.0},
        0.5,
        EnvelopeStage::ENV_STG_DAHD,
        0.300,
        2.0,
        {
            0.200, 0.100, 0.000, 0.125, 0.250, 0.375,
            0.500, 0.500, 0.500, 0.250, 0.000, 0.000,
        },
        true
    );
    test_envelope_rendering<12>(
        "snapshot changed during attack",
        {0.0, 0.5, 0.1, 0.0},
        {1.0, 2.0, 1.0, 2.0, 2.0},
        2.0,
        EnvelopeStage::ENV_STG_DAHD,
        0.200,
        2.0,
        {
            0.300, 0.400, 0.500, 0.500, 0.500, 0.400,
            0.300, 0.200, 0.100, 0.100, 0.100, 0.100,
        },
        true
    );
    test_envelope_rendering<12>(
        "snapshot changed during hold",
        {0.0, 0.5, 0.1, 0.0},
        {2.0, 3.0, 2.5, 2.0, 2.0},
        6.0,
        EnvelopeStage::ENV_STG_DAHD,
        1.0,
        2.0,
        {
            0.875, 0.750, 0.625, 0.500, 0.400, 0.300,
            0.200, 0.100, 0.100, 0.100, 0.100, 0.100,
        },
        true
    );
    test_envelope_rendering<12>(
        "snapshot changed during decay",
        {0.0, 0.5, 0.1, 0.0},
        {1.0, 1.0, 1.0, 3.0, 2.0},
        5.0,
        EnvelopeStage::ENV_STG_DAHD,
        1.0,
        2.0,
        {
            0.700, 0.400, 0.100, 0.100, 0.100, 0.100,
            0.100, 0.100, 0.100, 0.100, 0.100, 0.100,
        },
        true
    );
    test_envelope_rendering<12>(
        "snapshot changed during sustain",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 1.0, 1.0, 1.0, 1.0},
        0.0,
        EnvelopeStage::ENV_STG_SUSTAIN,
        1.0,
        50.0,
        {
            1.000, 0.900, 0.800, 0.700, 0.600, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
        },
        true
    );
    test_envelope_rendering<12>(
        "snapshot changed at the beginning of sustain",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 1.0, 1.0, 1.0, 1.0},
        6.0,
        EnvelopeStage::ENV_STG_DAHD,
        1.0,
        50.0,
        {
            1.000, 0.900, 0.800, 0.700, 0.600, 0.500,
            0.500, 0.500, 0.500, 0.500, 0.500, 0.500,
        },
        true
    );
    test_envelope_rendering<12>(
        "snapshot changed during release",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 1.0, 1.0, 1.0, 5.0},
        3.0,
        EnvelopeStage::ENV_STG_RELEASE,
        1.0,
        2.0,
        {
            0.800, 0.600, 0.400, 0.200, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
        },
        true
    );
    test_envelope_rendering<12>(
        "snapshot changed after release",
        {0.0, 1.0, 0.5, 0.0},
        {1.0, 1.0, 1.0, 1.0, 1.0},
        0.0,
        EnvelopeStage::ENV_STG_RELEASED,
        1.0,
        50.0,
        {
            1.000, 0.800, 0.600, 0.400, 0.200, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
        },
        true
    );
})


constexpr int SHAPE_TEST_SAMPLE_COUNT = 9;
constexpr int SHAPE_TEST_REL_LENGTH = SHAPE_TEST_SAMPLE_COUNT + 1;


std::string shape_test_array_to_string(Sample const samples[SHAPE_TEST_SAMPLE_COUNT])
{
    constexpr int max_length = 256;
    char buffer[max_length];
    int pos = 0;

    std::fill_n(buffer, max_length, '\x00');

    for (int i = 0; i != SHAPE_TEST_SAMPLE_COUNT && pos < max_length; ++i) {
        int const written = snprintf(
            &buffer[pos], (size_t)max_length, "%.5f ", samples[i]
        );

        if (written <= 0) {
            break;
        }

        pos += written;
    }

    return std::string(buffer);
}


void test_envelope_shape(
        EnvelopeShape const shape,
        Sample const reference_samples[SHAPE_TEST_SAMPLE_COUNT],
        char const expected_relations[SHAPE_TEST_REL_LENGTH]
) {
    constexpr Frequency sample_rate = (Frequency)(SHAPE_TEST_SAMPLE_COUNT - 2);
    constexpr Seconds sampling_period = 1.0 / sample_rate;
    constexpr EnvelopeRandoms randoms = {
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    };

    Envelope envelope("E");
    EnvelopeSnapshot snapshot;
    Sample buffer_1[SHAPE_TEST_SAMPLE_COUNT];
    Sample buffer_2[SHAPE_TEST_SAMPLE_COUNT];
    char actual_relations[SHAPE_TEST_REL_LENGTH];
    Number last_rendered_value = 0.0;
    Seconds time = 0.0;
    EnvelopeStage stage = EnvelopeStage::ENV_STG_DAHD;
    bool becomes_constant;

    std::fill_n(actual_relations, SHAPE_TEST_REL_LENGTH, '\x00');

    envelope.attack_shape.set_value(shape);
    envelope.initial_value.set_value(0.0);
    envelope.peak_value.set_value(1.0);
    envelope.delay_time.set_value(0.0);
    envelope.attack_time.set_value((sample_rate - 1.0) / sample_rate);
    envelope.hold_time.set_value(1.0);

    envelope.make_snapshot(randoms, 0, snapshot);

    Envelope::render<Envelope::RenderingMode::OVERWRITE>(
        snapshot,
        time,
        stage,
        becomes_constant,
        last_rendered_value,
        sample_rate,
        sampling_period,
        0,
        SHAPE_TEST_SAMPLE_COUNT,
        buffer_1
    );

    for (int i = 0; i != SHAPE_TEST_SAMPLE_COUNT; ++i) {
        if (Math::is_close(buffer_1[i], reference_samples[i])) {
            actual_relations[i] = '=';
        } else if (buffer_1[i] < reference_samples[i]) {
            actual_relations[i] = '<';
        } else {
            actual_relations[i] = '>';
        }
    }

    assert_eq(
        expected_relations,
        actual_relations,
        "shape=%d\n    reference=%s\n     rendered=%s",
        (int)shape,
        shape_test_array_to_string(reference_samples).c_str(),
        shape_test_array_to_string(buffer_1).c_str()
    );

    last_rendered_value = 0.0;
    time = 0.0;
    stage = EnvelopeStage::ENV_STG_DAHD;

    for (int i = 0; i != SHAPE_TEST_SAMPLE_COUNT; ++i) {
        last_rendered_value = Envelope::get_value_at_time(
            snapshot,
            time,
            stage,
            last_rendered_value,
            sampling_period
        );

        buffer_2[i] = last_rendered_value;
        time += sampling_period;
    }

    assert_eq(
        buffer_1, buffer_2, SHAPE_TEST_SAMPLE_COUNT, DOUBLE_DELTA, "shape=%d", (int)shape
    );
}


TEST(envelope_shapes, {
    constexpr Sample reference_samples[SHAPE_TEST_REL_LENGTH] = {
        0.0, 1.0 / 6.0, 2.0 / 6.0, 3.0 / 6.0, 4.0 / 6.0, 5.0 / 6.0, 1.0, 1.0, 1.0,
    };

    test_envelope_shape(Envelope::SHAPE_LINEAR, reference_samples, "=========");
    test_envelope_shape(Envelope::SHAPE_SMOOTH_SMOOTH, reference_samples, "=<<=>>===");
    test_envelope_shape(Envelope::SHAPE_SMOOTH_SMOOTH_STEEP, reference_samples, "=<<=>>===");
    test_envelope_shape(Envelope::SHAPE_SMOOTH_SMOOTH_STEEPER, reference_samples, "=<<=>>===");
    test_envelope_shape(Envelope::SHAPE_SMOOTH_SHARP, reference_samples, "=<<<<<===");
    test_envelope_shape(Envelope::SHAPE_SMOOTH_SHARP_STEEP, reference_samples, "=<<<<<===");
    test_envelope_shape(Envelope::SHAPE_SMOOTH_SHARP_STEEPER, reference_samples, "=<<<<<===");
    test_envelope_shape(Envelope::SHAPE_SHARP_SMOOTH, reference_samples, "=>>>>>===");
    test_envelope_shape(Envelope::SHAPE_SHARP_SMOOTH_STEEP, reference_samples, "=>>>>>===");
    test_envelope_shape(Envelope::SHAPE_SHARP_SMOOTH_STEEPER, reference_samples, "=>>>>>===");
    test_envelope_shape(Envelope::SHAPE_SHARP_SHARP, reference_samples, "=>>=<<===");
    test_envelope_shape(Envelope::SHAPE_SHARP_SHARP_STEEP, reference_samples, "=>>=<<===");
    test_envelope_shape(Envelope::SHAPE_SHARP_SHARP_STEEPER, reference_samples, "=>>=<<===");
})
