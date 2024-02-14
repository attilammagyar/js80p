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
#include <string>
#include <utility>

#include "test.cpp"
#include "utils.hpp"

#include "js80p.hpp"

#include "bank.hpp"
#include "serializer.hpp"
#include "synth.hpp"


using namespace JS80P;


TEST(long_program_names_are_trimmed_and_truncated, {
    constexpr char const* long_name = "a long program name, way over the limit";
    constexpr char const* truncated = "a long program name,..t";
    constexpr char const* truncated_short = "a lo..t";
    constexpr char const* just_below_the_limit = "   just below length limit   ";
    constexpr char const* just_below_the_limit_trimmed = "just below length limit";
    constexpr char const* becomes_empty = "    [\\]   ";

    Bank::Program program(long_name, " [Default Name] ", "");
    Bank::Program empty_default("   ", " [] ", "");

    assert_eq(truncated, program.get_name());
    assert_eq(truncated_short, program.get_short_name());

    program.set_name(long_name);
    assert_eq(truncated, program.get_name());
    assert_eq(truncated_short, program.get_short_name());

    program.set_name(just_below_the_limit);
    assert_eq(just_below_the_limit_trimmed, program.get_name());

    program.set_name(becomes_empty);
    assert_eq("Default Name", program.get_name());
    assert_eq("Defa..e", program.get_short_name());

    assert_eq("", empty_default.get_name());
    assert_eq("", empty_default.get_short_name());
})


TEST(only_latin_printable_characters_are_allowed_in_program_names, {
    Bank::Program program(
        "_[\\]\nÁrvíztűrő-Tükörfúrógép,;:. (#1)", "Default Name", ""
    );

    assert_eq("_rvztr-Tkrfrgp,;:. (#1)", program.get_name());

    program.set_name("[long name with disallowed characters]");
    assert_eq("long name with disal..s", program.get_name());
})


TEST(program_copy_and_move, {
    Bank::Program orig("Some Program Name", "Default Name", "");
    Bank::Program ctor_copy(orig);
    Bank::Program op_copy("Other Program Name", "Other Default Name", "");
    Bank::Program op_move("Other Program Name", "Other Default Name", "");

    op_copy = orig;

    assert_eq("Some Program Name", ctor_copy.get_name());
    assert_eq("Some..e", ctor_copy.get_short_name());

    assert_eq("Some Program Name", op_copy.get_name());
    assert_eq("Some..e", op_copy.get_short_name());

    Bank::Program ctor_move(std::move(ctor_copy));
    op_move = std::move(op_copy);

    assert_eq("Some Program Name", ctor_move.get_name());
    assert_eq("Some..e", ctor_move.get_short_name());

    assert_eq("Some Program Name", op_move.get_name());
    assert_eq("Some..e", op_move.get_short_name());
})


TEST(program_can_be_imported, {
    Bank::Program program("Name", "Default Name", "");

    program.import(
        "[someblock]\n"
        "NAME = not the name we are looking for\n"
        "MIX = 1.0\n"
        "\n"
        "[js80p]\n"
        "NAMENOT = not the program name again\n"
        "NAME = this is the name that we are looking for\n"
        "NAMEctl = also not the program name\n"
        "MIX = 1.0\n"
        "\n"
        "[js80p]\n"
        "NAME = not the name we are looking for\n"
        "MIX = 2.0\n"
        "\n"
    );

    assert_eq("this is the name tha..r", program.get_name());
    assert_eq(
        (
            "[js80p]\r\n"
            "NAME = this is the name tha..r\r\n"
            "NAMENOT = not the program name again\r\n"
            "NAMEctl = also not the program name\r\n"
            "MIX = 1.0\r\n"
        ),
        program.serialize()
    );
})


TEST(an_imported_program_may_be_empty, {
    Bank::Program program("Name", "Default Name", "[js80p]\nMIX = 1.0");

    program.import(
        "[someblock]\n"
        "NAME = not the name we are looking for\n"
        "MIX = 2.0\n"
        "\n"
    );

    assert_eq("Default Name", program.get_name());
    assert_eq(
        (
            "[js80p]\r\n"
            "NAME = Default Name\r\n"
        ),
        program.serialize()
    );
    assert_true(program.is_blank());
})


TEST(when_a_serialized_program_does_not_have_a_name_then_original_name_is_kept, {
    Bank::Program program("Name", "Default Name", "[js80p]\nMIX = 1.0");

    program.import(
        "[js80p]\n"
        "MIX = 2.0\n"
        "\n"
    );

    assert_eq("Name", program.get_name());
    assert_eq(
        (
            "[js80p]\r\n"
            "NAME = Name\r\n"
            "MIX = 2.0\r\n"
        ),
        program.serialize()
    );
    assert_false(program.is_blank());
})


TEST(serialized_program_buffer_remains_valid, {
    Bank::Program program(
        "Name",
        "Default Name",
        (
            "[js80p]\n"
            "MIX = 1.0\n"
            "MVOL = 0.123\n"
            "CVOL = 0.345\n"
        )
    );
    char const* const buffer = program.serialize().c_str();

    assert_eq(
        (
            "[js80p]\r\n"
            "NAME = Name\r\n"
            "MIX = 1.0\r\n"
            "MVOL = 0.123\r\n"
            "CVOL = 0.345\r\n"
        ),
        buffer
    );
})


TEST(bank_is_initialized_with_built_in_programs, {
    Bank bank;

    /* the name of the program is "Blank", but the patch itself isn't blank :-) */
    assert_eq("Blank", bank[0].get_name().c_str());
    assert_false(bank[0].is_blank());

    assert_eq("Prog128", bank[127].get_name().c_str());
    assert_true(bank[127].is_blank());

    assert_eq("Prog128", bank[128].get_name().c_str());
    assert_eq("Prog128", bank[500].get_name().c_str());
})


TEST(current_program_number_cannot_be_more_than_number_of_programs, {
    Bank bank;

    assert_eq(0, bank.get_current_program_index());

    bank.set_current_program_index(42);
    assert_eq(42, bank.get_current_program_index());

    bank.set_current_program_index(128);
    assert_eq(127, bank.get_current_program_index());
})


TEST(can_update_a_program, {
    constexpr size_t program = 122;

    Bank bank;

    bank[program].import("[js80p]\nMIX = 2.0");

    assert_eq(
        (
            "[js80p]\r\n"
            "NAME = Prog123\r\n"
            "MIX = 2.0\r\n"
        ),
        bank[program].serialize()
    );
})


TEST(serialization, {
    std::string const serialized_bank = (
        "[someblock]\n"
        "MIX = 0.5\n"
        "NAME = not a JS80P patch\n"
        "\n"
        "[js80p]\n"
        "NAME = preset 1\n"
        "MIX = 1.0\n"
        "\n"
        "[x]\n"
        "MIX = 1.5\n"
        "NAME = still not a JS80P patch\n"
        "\n"
        "  [js80p]\n"
        "; default name\n"
        "NAME =\n"
        "MIX = 2.0\n"
        "[js80p]\n"
        "; a comment containing the [js80p] section header\n"
        "NAME = preset 3\n"
        "MIX = 3.0\n"
        "[js80p]\n"
        "[js80p]\n"
    );
    std::string const expeced_serialized = (
        "[js80p]\r\n"
        "NAME = preset 1\r\n"
        "MIX = 1.0\r\n"
        "\r\n"
        "[js80p]\r\n"
        "NAME = Prog002\r\n"
        "; default name\r\n"
        "MIX = 2.0\r\n"
        "\r\n"
        "[js80p]\r\n"
        "NAME = preset 3\r\n"
        "; a comment containing the [js80p] section header\r\n"
        "MIX = 3.0\r\n\r\n"
    );
    Bank bank;

    bank.set_current_program_index(42);
    bank[5].import(
        "[js80p]\n"
        "NAME = to be reset name\n"
        "to be reset patch\n"
    );

    bank.import(serialized_bank);

    assert_eq("preset 1", bank[0].get_name().c_str());
    assert_eq(
        (
            "[js80p]\r\n"
            "NAME = preset 1\r\n"
            "MIX = 1.0\r\n"
        ),
        bank[0].serialize()
    );

    assert_eq("Prog002", bank[1].get_name().c_str());
    assert_eq(
        (
            "[js80p]\r\n"
            "NAME = Prog002\r\n"
            "; default name\r\n"
            "MIX = 2.0\r\n"
        ),
        bank[1].serialize()
    );

    assert_eq("preset 3", bank[2].get_name().c_str());
    assert_eq(
        (
            "[js80p]\r\n"
            "NAME = preset 3\r\n"
            "; a comment containing the [js80p] section header\r\n"
            "MIX = 3.0\r\n"
        ),
        bank[2].serialize()
    );

    assert_true(bank[3].is_blank());
    assert_true(bank[4].is_blank());
    assert_neq("to be reset name", bank[5].get_name().c_str());

    assert_eq(42, (int)bank.get_current_program_index());

    for (size_t i = 3; i != Bank::NUMBER_OF_PROGRAMS; ++i) {
        bank[i].import("");
    }

    assert_eq(
        expeced_serialized,
        bank.serialize().substr(0, expeced_serialized.length()).c_str()
    );
})


TEST(can_convert_normalized_parameter_value_to_program_index, {
    assert_eq(0, (int)Bank::normalized_parameter_value_to_program_index(-0.5));

    assert_eq(0, (int)Bank::normalized_parameter_value_to_program_index(0.0));
    assert_eq(
        0.0, Bank::program_index_to_normalized_parameter_value(0), DOUBLE_DELTA
    );

    assert_eq(
        (int)(Bank::NUMBER_OF_PROGRAMS / 2),
        (int)Bank::normalized_parameter_value_to_program_index(0.5)
    );
    assert_eq(
        0.5,
        Bank::program_index_to_normalized_parameter_value(
            Bank::NUMBER_OF_PROGRAMS / 2
        ),
        0.005
    );

    assert_eq(
        (int)(Bank::NUMBER_OF_PROGRAMS - 1),
        (int)Bank::normalized_parameter_value_to_program_index(1.0)
    );
    assert_eq(
        1.0,
        Bank::program_index_to_normalized_parameter_value(
            Bank::NUMBER_OF_PROGRAMS - 1
        ),
        DOUBLE_DELTA
    );

    assert_eq(
        (int)(Bank::NUMBER_OF_PROGRAMS - 1),
        (int)Bank::normalized_parameter_value_to_program_index(2.0)
    );
    assert_eq(
        1.0,
        Bank::program_index_to_normalized_parameter_value(
            Bank::NUMBER_OF_PROGRAMS + 1
        ),
        DOUBLE_DELTA
    );
})


TEST(bank_can_import_program_names_without_patches, {
    std::string const serialized_bank = (
        "[someblock]\n"
        "MIX = 0.5\n"
        "NAME = not a JS80P patch\n"
        "\n"
        "[js80p]\n"
        "NAME = preset 1\n"
        "MIX = 1.0\n"
        "\n"
        "[x]\n"
        "MIX = 1.5\n"
        "NAME = still not a JS80P patch\n"
        "\n"
        "  [js80p]\n"
        "; default name\n"
        "NAME =\n"
        "MIX = 2.0\n"
        "[js80p]\n"
        "; a comment containing the [js80p] section header\n"
        "NAME = preset 3\n"
        "MIX = 3.0\n"
    );
    std::string const expeced_serialized = (
        "[js80p]\r\n"
        "NAME = preset 1\r\n"
        "\r\n"
        "[js80p]\r\n"
        "NAME = Prog002\r\n"
        "\r\n"
        "[js80p]\r\n"
        "NAME = preset 3\r\n"
        "\r\n"
        "[js80p]\r\n"
        "NAME = Prog004\r\n"
        "\r\n"
        "[js80p]\r\n"
        "NAME = Prog005\r\n"
        "\r\n"
        "[js80p]\r\n"
        "NAME = Prog006\r\n"
    );
    Bank bank;

    bank.import_names(serialized_bank);

    assert_eq("preset 1", bank[0].get_name());
    assert_eq("Prog002", bank[1].get_name());
    assert_eq("preset 3", bank[2].get_name());
    assert_eq("Prog004", bank[3].get_name());
    assert_eq("Prog005", bank[4].get_name());
    assert_eq("Prog006", bank[5].get_name());

    assert_eq(
        expeced_serialized,
        bank.serialize().substr(0, expeced_serialized.length()).c_str()
    );
})
