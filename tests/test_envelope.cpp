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
