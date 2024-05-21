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
#include <cmath>

#include "test.cpp"
#include "utils.cpp"

#include "js80p.hpp"
#include "midi.hpp"
#include "renderer.hpp"

#include "synth.cpp"


using namespace JS80P;


enum RenderMode {
    OVERWRITE = 0,
    ADD = 1,
};


void test_varaible_size_rounds(RenderMode const mode)
{
    constexpr Integer buffer_size = 4096;
    constexpr Frequency sample_rate = 11025.0;
    constexpr Number volume_per_channel = std::sin(Math::PI / 4.0);
    constexpr Integer round_sizes[] = {
        123, 150, 106, 1, 120,
        20, 20, 20, 10, 10,
        10, 90, 150, 160, 0,
        9, 0, 0, 15, 10,
        100, 99, 100, 99, 101,
        8, 1, 1, 6, 1, 101, 6,
        100, 101, 99, 20, 81,
        1000, 512, 24, 384, 128,
        -1,
    };

    Synth synth;

    Integer const channels = synth.get_channels();

    Renderer renderer(synth);
    Integer const latency = renderer.get_latency_samples();
    SumOfSines input(
        0.5, 110.0,
        0.0, 0.0,
        0.0, 0.0,
        channels,
        (Number)latency / sample_rate
    );
    SumOfSines intro_reference(
        volume_per_channel, 220.0,
        0.0, 0.0,
        0.0, 0.0,
        channels
    );
    SumOfSines reference(
        volume_per_channel, 220.0,
        0.5, 110.0,
        0.0, 0.0,
        channels,
        0.005079
    );
    Sample const* const* in_samples;
    Sample const* const* expected_samples;
    double* buffer[channels];
    double* batch[channels];
    Integer next_round_start = 0;

    input.set_block_size(buffer_size);
    input.set_sample_rate(sample_rate);

    intro_reference.set_block_size(buffer_size);
    intro_reference.set_sample_rate(sample_rate);

    reference.set_block_size(buffer_size);
    reference.set_sample_rate(sample_rate);

    synth.set_block_size(buffer_size);
    synth.set_sample_rate(sample_rate);

    synth.modulator_params.amplitude.set_value(1.0);
    synth.modulator_params.volume.set_value(1.0);
    synth.modulator_params.waveform.set_value(SimpleOscillator::SINE);
    synth.modulator_params.width.set_value(0.0);

    synth.carrier_params.volume.set_value(0.0);

    synth.input_volume.set_value(1.0);

    synth.note_on(0.0, 1, Midi::NOTE_A_3, 127);

    for (Integer c = 0; c != channels; ++c) {
        buffer[c] = new double[buffer_size];
        batch[c] = buffer[c];
        std::fill_n(buffer[c], buffer_size, 0.0);
    }

    in_samples = SignalProducer::produce<SumOfSines>(input, 999, latency);
    renderer.render<double>(latency, in_samples, batch);
    expected_samples = SignalProducer::produce<SumOfSines>(
        intro_reference, 999, latency
    );

    for (Integer c = 0; c != channels; ++c) {
        assert_eq(
            expected_samples[c],
            batch[c],
            latency,
            DOUBLE_DELTA,
            "channel=%d",
            (int)c
        );
    }

    for (Integer c = 0; c != channels; ++c) {
        std::fill_n(batch[c], buffer_size, 0.0);
    }

    for (Integer i = 0; round_sizes[i] >= 0; ++i) {
        Integer const sample_count = round_sizes[i];

        in_samples = SignalProducer::produce<SumOfSines>(input, i, sample_count);

        for (Integer c = 0; c != channels; ++c) {
            batch[c] = &buffer[c][next_round_start];
        }

        if (mode == RenderMode::ADD) {
            renderer.render<double, Renderer::Operation::ADD>(sample_count, in_samples, batch);
        } else {
            renderer.render<double>(sample_count, in_samples, batch);
        }

        next_round_start += sample_count;
    }

    expected_samples = SignalProducer::produce<SumOfSines>(reference, 1);

    for (Integer c = 0; c != channels; ++c) {
        assert_close(expected_samples[c], buffer[c], buffer_size, 0.03);
    }

    for (Integer c = 0; c != channels; ++c) {
        delete[] buffer[c];
    }
}


TEST(number_of_samples_to_render_may_vary_between_rounds, {
    test_varaible_size_rounds(OVERWRITE);
    test_varaible_size_rounds(ADD);
})
