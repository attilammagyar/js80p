/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2024  Attila M. Magyar
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

#include "js80p.hpp"

#include "dsp/lfo_envelope_list.cpp"


using namespace JS80P;


TEST(lfo_envelope_list_stores_a_few_envelope_indices, {
    LFOEnvelopeList lfo_envelope_list_1;
    LFOEnvelopeList lfo_envelope_list_2;

    lfo_envelope_list_1[0] = 5;
    lfo_envelope_list_1[1] = 11;
    lfo_envelope_list_1[2] = 0;

    assert_eq(5, lfo_envelope_list_1[0]);
    assert_eq(11, lfo_envelope_list_1[1]);
    assert_eq(0, lfo_envelope_list_1[2]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_1[3]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_1[4]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_1[5]);

    lfo_envelope_list_2 = lfo_envelope_list_1;

    lfo_envelope_list_1[1] = 6;

    assert_eq(5, lfo_envelope_list_1[0]);
    assert_eq(6, lfo_envelope_list_1[1]);
    assert_eq(0, lfo_envelope_list_1[2]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_1[3]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_1[4]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_1[5]);

    lfo_envelope_list_1.clear();

    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_1[0]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_1[1]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_1[2]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_1[3]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_1[4]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_1[5]);

    assert_eq(5, lfo_envelope_list_2[0]);
    assert_eq(11, lfo_envelope_list_2[1]);
    assert_eq(0, lfo_envelope_list_2[2]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_2[3]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_1[4]);
    assert_eq(Constants::INVALID_ENVELOPE_INDEX, lfo_envelope_list_1[5]);
})
