/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2025  Attila M. Magyar
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

#ifndef JS80P__DSP__NOISE_GENERATOR_HPP
#define JS80P__DSP__NOISE_GENERATOR_HPP

#include <cstddef>
#include <string>

#include "js80p.hpp"

#include "dsp/filter.hpp"
#include "dsp/math.hpp"
#include "dsp/param.hpp"
#include "dsp/signal_producer.hpp"


namespace JS80P
{

/**
 * \brief Generate pseudo-random noise that is filtered to fall between the
 *        given frequency range.
 *
 * \warning When multiple instances of \c NoiseGenerator are used, make sure to
 *          pass the same \c Math::RNG instance to all of them in order to avoid
 *          different noise sources generating phase-shifted versions of the
 *          same pattern.
 */
template<class InputSignalProducerClass>
class NoiseGenerator : public Filter<InputSignalProducerClass>
{
    friend class SignalProducer;

    public:
        NoiseGenerator(
            InputSignalProducerClass& input,
            FloatParamB& level,
            Frequency const high_pass_frequency,
            Frequency const low_pass_frequency,
            Math::RNG& rng
        ) noexcept;

        virtual ~NoiseGenerator();

        virtual void set_sample_rate(
            Frequency const new_sample_rate
        ) noexcept override;

        virtual void reset() noexcept override;

        Frequency const high_pass_frequency;
        Frequency const low_pass_frequency;

        FloatParamB& level;

    protected:
        Sample const* const* initialize_rendering(
            Integer const round,
            Integer const sample_count
        ) noexcept;

        void render(
            Integer const round,
            Integer const first_sample_index,
            Integer const last_sample_index,
            Sample** buffer
        ) noexcept;

    private:
        void update_filter_coefficients() noexcept;
        void clear_filters_state() noexcept;

        Math::RNG& rng;
        Sample* r_n_m1;
        Sample* x_n_m1;
        Sample* y_n_m1;
        Sample a;
        Sample w1;
        Sample w2;
};

}

#endif
