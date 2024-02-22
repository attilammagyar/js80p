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

#ifndef JS80P__TESTS__UTILS_CPP
#define JS80P__TESTS__UTILS_CPP

#include "utils.hpp"

#include "js80p.hpp"

#include "dsp/math.cpp"
#include "dsp/queue.cpp"
#include "dsp/signal_producer.cpp"


namespace JS80P
{

class Buffer
{
    public:
        explicit Buffer(Integer const size, Integer const channels = 1)
            : size(size),
            channels(channels),
            samples(NULL),
            append_index(0)
        {
            if (channels > 0) {
                samples = new Sample*[channels];
            } else {
                samples = NULL;
            }

            for (Integer channel = 0; channel != channels; ++channel) {
                samples[channel] = new Sample[size];

                for (Integer i = 0; i != size; ++i) {
                    samples[channel][i] = 0.0;
                }
            }
        }

        ~Buffer()
        {
            if (samples == NULL) {
                return;
            }

            for (Integer channel = 0; channel != channels; ++channel) {
                delete[] samples[channel];
            }

            delete[] samples;
        }

        Buffer(Buffer const& buffer) = delete;
        Buffer(Buffer&& buffer) = delete;

        Buffer& operator=(Buffer const& buffer) = delete;
        Buffer& operator=(Buffer&& buffer) = delete;

        void append(Sample const* const* samples, Integer const sample_count)
        {
            for (Integer channel = 0; channel != channels; ++channel) {
                for (Integer i = 0; i != sample_count; ++i) {
                    this->samples[channel][append_index + i] = samples[channel][i];
                }
            }

            append_index += sample_count;
        }

        void reset()
        {
            append_index = 0;
        }

        Integer const size;
        Integer const channels;

        Sample** samples;
        Integer append_index;
};


class Constant : public SignalProducer
{
    friend class SignalProducer;

    public:
        explicit Constant(Sample const value, Integer const channels = 1) noexcept
            : SignalProducer(channels, 0),
            value(value)
        {
        }

    protected:
        void render(
                Integer const round,
                Integer const first_sample_index,
                Integer const last_sample_index,
                Sample** buffer
        ) noexcept {
            Integer const channels = get_channels();

            for (Integer c = 0; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = value;
                }
            }
        }

    private:
        Sample const value;
};


class FixedSignalProducer : public SignalProducer
{
    friend class SignalProducer;

    public:
        static constexpr Integer CHANNELS = 2;

        explicit FixedSignalProducer(Sample const* const* fixed_samples) noexcept
            : SignalProducer(CHANNELS, 0),
            fixed_samples(fixed_samples)
        {
        }

        Integer get_cached_round() const noexcept
        {
            return this->cached_round;
        }

    protected:
        Sample const* const* initialize_rendering(
                Integer const round,
                Integer const sample_count
        ) noexcept {
            return fixed_samples;
        }

    private:
        Sample const* const* const fixed_samples;
};


class SumOfSines : public SignalProducer
{
    friend class SignalProducer;

    public:
        SumOfSines(
                Number const amplitude_1 = 0.0,
                Frequency const frequency_1 = 0.0,
                Number const amplitude_2 = 0.0,
                Frequency const frequency_2 = 0.0,
                Number const amplitude_3 = 0.0,
                Frequency const frequency_3 = 0.0,
                Integer const channels = 1,
                Seconds const phase_offset = 0.0,
                Sample const sample_offset = 0.0
        ) noexcept
            : SignalProducer(channels, 0),
            amplitude_1((Sample)amplitude_1),
            amplitude_2((Sample)amplitude_2),
            amplitude_3((Sample)amplitude_3),
            frequency_1_times_pi_double(
                (Sample)frequency_1 * (Sample)Math::PI_DOUBLE
            ),
            frequency_2_times_pi_double(
                (Sample)frequency_2 * (Sample)Math::PI_DOUBLE
            ),
            frequency_3_times_pi_double(
                (Sample)frequency_3 * (Sample)Math::PI_DOUBLE
            ),
            phase_offset((Sample)phase_offset),
            sample_offset(sample_offset),
            rendered_samples(0)
        {
        }

        void reset() noexcept override
        {
            current_time = 0.0;
            rendered_samples = 0;
            cached_round = -1;
            events.drop(0);
        }

    protected:
        void render(
                Integer const round,
                Integer const first_sample_index,
                Integer const last_sample_index,
                Sample** buffer
        ) noexcept {
            Integer const channels = get_channels();

            if (channels == 0) {
                return;
            }

            Seconds time = (
                (Seconds)rendered_samples * sampling_period + phase_offset
            );

            for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                buffer[0][i] = (Sample)(
                    amplitude_1 * Math::sin(
                        frequency_1_times_pi_double * time
                    )
                    + amplitude_2 * Math::sin(
                        frequency_2_times_pi_double * time
                    )
                    + amplitude_3 * Math::sin(
                        frequency_3_times_pi_double * time
                    )
                ) + sample_offset;

                time += (Sample)sampling_period;
            }

            for (Integer c = 1; c != channels; ++c) {
                for (Integer i = first_sample_index; i != last_sample_index; ++i) {
                    buffer[c][i] = buffer[0][i];
                }
            }

            rendered_samples += last_sample_index - first_sample_index;
        }

    private:
        Sample const amplitude_1;
        Sample const amplitude_2;
        Sample const amplitude_3;
        Sample const frequency_1_times_pi_double;
        Sample const frequency_2_times_pi_double;
        Sample const frequency_3_times_pi_double;
        Sample const phase_offset;
        Sample const sample_offset;

        Integer rendered_samples;
};


template<class SignalProducerClass>
void render_rounds(
        SignalProducerClass& signal_producer,
        Buffer& buffer,
        Integer const rounds,
        Integer const chunk_size = 0,
        Integer const first_round = 1
) {
    Integer const size = (
        0 < chunk_size ? chunk_size : signal_producer.get_block_size()
    );

    buffer.reset();

    for (Integer i = 0; i != rounds; ++i) {
        Sample const* const* block = (
            SignalProducer::produce<SignalProducerClass>(
                signal_producer, i + first_round, size
            )
        );

        buffer.append(block, size);
    }
}


/**
 * \brief Assert that two identical signal chains render the same output,
 *        regardless of rendering chunk and block size.
 *
 * \warning The two signal chains must be identical, but they must not share any
 *          \c SignalProducer instances with each other, even if it would seem
 *          rational to do so (e.g. to use the same \c FloatParamS leader
 *          instance for both chains). The reason is that the two signal chains
 *          are rendered independently from each other, so their rendering
 *          rounds are not synchronized to each other.
 */
template<class SignalProducerClass>
void assert_rendering_is_independent_from_chunk_size(
        SignalProducerClass& signal_producer_1,
        SignalProducerClass& signal_producer_2,
        Number const tolerance = 0.001,
        char const* message = NULL
) {
    constexpr Integer block_size = 500;
    constexpr Integer rounds_1 = 360;
    constexpr Integer buffer_size = block_size * rounds_1;
    constexpr Integer short_round_size = 150;
    constexpr Integer rounds_2 = buffer_size / short_round_size;
    Integer const channels = signal_producer_1.get_channels();

    Buffer buffer_1(buffer_size, channels);
    Buffer buffer_2(buffer_size, channels);

    signal_producer_1.set_block_size(block_size);
    signal_producer_2.set_block_size(block_size * 10);

    render_rounds<SignalProducerClass>(signal_producer_1, buffer_1, rounds_1, block_size);
    render_rounds<SignalProducerClass>(signal_producer_2, buffer_2, rounds_2, short_round_size);

    for (Integer c = 0; c != channels; ++c) {
        assert_eq(buffer_1.samples[c], buffer_2.samples[c], buffer_size, tolerance, message);
    }
}


void assert_statistics(
        Number const expected_validity,
        Number const expected_min,
        Number const expected_median,
        Number const expected_max,
        Number const expected_mean,
        Number const expected_standard_deviation,
        Math::Statistics const& statistics,
        Number const tolerance = DOUBLE_DELTA,
        char const* message = ""
) {
    if (!expected_validity) {
        assert_false(statistics.is_valid);

        return;
    }

    assert_true(statistics.is_valid, message);
    assert_eq(expected_min, statistics.min, tolerance, message);
    assert_eq(expected_median, statistics.median, tolerance, message);
    assert_eq(expected_max, statistics.max, tolerance, message);
    assert_eq(expected_mean, statistics.mean, tolerance, message);
    assert_eq(
        expected_standard_deviation,
        statistics.standard_deviation,
        tolerance,
        message
    );
}

}

#endif
