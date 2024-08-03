/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023, 2024  Attila M. Magyar
 * Copyright (C) 2023  Patrik Ehringer
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

#ifndef JS80P__BANK_CPP
#define JS80P__BANK_CPP

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "bank.hpp"

#include "programs.cpp"


namespace JS80P
{

Bank::Program::Program(
        std::string const& name,
        std::string const& default_name,
        std::string const& serialized
) : name(""), short_name(""), default_name(""), serialized(""), params_start(0)
{
    this->default_name = truncate(sanitize_name(default_name), NAME_MAX_LENGTH);
    import_without_update(serialized);
    set_name(name);
}


void Bank::Program::set_name(std::string const& new_name)
{
    set_name_without_update(new_name);
    update();
}


void Bank::Program::set_name_without_update(std::string const& new_name)
{
    std::string const sanitized_name = sanitize_name(new_name);

    name = truncate(sanitized_name, NAME_MAX_LENGTH);
    short_name = truncate(sanitized_name, SHORT_NAME_MAX_LENGTH);
}


std::string Bank::Program::sanitize_name(std::string const& name) const
{
    Integer const buffer_size = name.length() + 1;

    char filtered[buffer_size];
    Integer next = 0;

    std::fill_n(filtered, buffer_size, '\x00');

    for (std::string::const_iterator it = name.begin(); it != name.end(); ++it) {
        if (is_allowed_char(*it) && (next > 0 || *it != ' ')) {
            filtered[next++] = *it;
        }
    }

    --next;

    while (next >= 0 && filtered[next] == ' ') {
        filtered[next--] = '\x00';
    }

    std::string sanitized(filtered, next + 1);

    if (sanitized.length() == 0) {
        sanitized = default_name;
    }

    return sanitized;
}


std::string Bank::Program::truncate(
        std::string const& text,
        std::string::size_type const max_length
) const {
    std::string::size_type const length = text.length();

    if (length < max_length) {
        return text;
    }

    if (max_length < 1) {
        return "";
    }

    std::string truncated(text.c_str(), max_length);

    if (max_length >= 6) {
        char const last_char = text[length - 1];

        truncated.erase(max_length - 4);
        truncated += "..";
        truncated += last_char;
    } else {
        truncated.erase(max_length);
    }

    return truncated;
}


bool Bank::Program::is_allowed_char(char const c) const
{
    /* printable ASCII, except for '[', '\\', and ']' */
    return c <= '~' && c >= ' ' && c != '[' && c != '\\' && c != ']';
}


void Bank::Program::update()
{
    std::string prefix("[js80p]\r\nNAME = ");
    prefix += name;
    prefix += "\r\n";

    std::string::size_type const new_params_start = prefix.length();

    serialized = prefix + serialized.substr(params_start);
    params_start = new_params_start;
}


Bank::Program::Program()
    : name(""),
    default_name(""),
    serialized(""),
    params_start(0)
{
    update();
}


std::string const& Bank::Program::get_name() const
{
    return name;
}


std::string const& Bank::Program::get_short_name() const
{
    return short_name;
}


std::string const& Bank::Program::serialize() const
{
    return serialized;
}


bool Bank::Program::is_blank() const
{
    return params_start == serialized.length();
}


void Bank::Program::import(std::string const& serialized)
{
    import_without_update(serialized);
    update();
}


void Bank::Program::import_without_update(std::string const& serialized)
{
    Serializer::Lines* lines = Serializer::parse_lines(serialized);
    Serializer::Lines::const_iterator it = lines->begin();

    import_without_update(it, lines->end());

    delete lines;
}


void Bank::Program::import(
        Serializer::Lines::const_iterator& it,
        Serializer::Lines::const_iterator const& end
) {
    import_without_update(it, end);
    update();
}


void Bank::Program::import_without_update(
        Serializer::Lines::const_iterator& it,
        Serializer::Lines::const_iterator const& end
) {
    std::string program_name("");
    std::string serialized_params("");
    Serializer::SectionName section_name;
    Serializer::ParamName param_name;
    Serializer::Suffix suffix;
    bool is_js80p_section = false;
    bool found_program_name = false;

    for (; it != end; ++it) {
        std::string const& line = *it;
        std::string::const_iterator line_it = line.begin();
        std::string::const_iterator line_end = line.end();

        if (Serializer::parse_section_name(line, section_name)) {
            if (is_js80p_section) {
                break;
            }

            serialized_params = "";
            program_name = "";
            param_name[0] = '\x00';
            is_js80p_section = Serializer::is_js80p_section_start(section_name);
        } else if (
                is_js80p_section
                && Serializer::parse_line_until_value(
                    line_it, line_end, param_name, suffix
                )
                && strncmp(param_name, "NAME", 8) == 0
                && strncmp(suffix, "", 4) == 0
        ) {
            Serializer::skipping_remaining_whitespace_or_comment_reaches_the_end(
                line_it, line_end
            );
            program_name = &(*line_it);
            found_program_name = true;
        } else if (is_js80p_section) {
            serialized_params += line;
            serialized_params += "\r\n";
        }
    }

    if (is_js80p_section) {
        if (found_program_name) {
            set_name_without_update(program_name);
        }

        params_start = 0;
        serialized = serialized_params;
    } else {
        set_name_without_update("");
        params_start = 0;
        serialized = "";
    }
}


size_t Bank::normalized_parameter_value_to_program_index(
        Number const parameter_value
) {
    return std::min(
        NUMBER_OF_PROGRAMS - 1,
        (size_t)std::max(
            0.0, std::round(parameter_value * FLOAT_TO_PROGRAM_INDEX_SCALE)
        )
    );
}


Number Bank::program_index_to_normalized_parameter_value(size_t const index)
{
    return std::min(
        1.0,
        std::max(0.0, (Number)index * PROGRAM_INDEX_TO_FLOAT_SCALE)
    );
}


Bank::Bank() : current_program_index(0)
{
    size_t i = 0;

    for (; i != NUMBER_OF_BUILT_IN_PROGRAMS; ++i) {
        programs[i] = BUILT_IN_PROGRAMS[i];
    }

    generate_empty_programs(i);
}


void Bank::generate_empty_programs(size_t const start_index)
{
    char default_name[Program::NAME_MAX_LENGTH];

    for (size_t i = start_index; i != NUMBER_OF_PROGRAMS; ++i) {
        snprintf(
            default_name,
            Program::NAME_MAX_LENGTH,
            "Prog%03lu", (long unsigned int)(i + 1)
        );
        default_name[Program::NAME_MAX_LENGTH - 1] = '\x00';
        programs[i] = Program("", default_name, "");
    }
}


Bank::Program& Bank::operator[](size_t const index)
{
    if (index >= NUMBER_OF_PROGRAMS) {
        return programs[NUMBER_OF_PROGRAMS - 1];
    }

    return programs[index];
}


Bank::Program const& Bank::operator[](size_t const index) const
{
    if (index >= NUMBER_OF_PROGRAMS) {
        return programs[NUMBER_OF_PROGRAMS - 1];
    }

    return programs[index];
}


size_t Bank::get_current_program_index() const
{
    return current_program_index;
}


void Bank::set_current_program_index(size_t const new_index)
{
    if (new_index >= NUMBER_OF_PROGRAMS) {
        current_program_index = NUMBER_OF_PROGRAMS - 1;
    } else {
        current_program_index = new_index;
    }
}


void Bank::import(std::string const& serialized_bank)
{
    Serializer::Lines* lines = Serializer::parse_lines(serialized_bank);
    Serializer::Lines::const_iterator it = lines->begin();
    Serializer::Lines::const_iterator end = lines->end();
    size_t next_program_index = 0;

    while (it != end && next_program_index < NUMBER_OF_PROGRAMS) {
        programs[next_program_index++].import(it, end);
    }

    generate_empty_programs(next_program_index);

    delete lines;
}


void Bank::import_names(std::string const& serialized_bank)
{
    Serializer::Lines* lines = Serializer::parse_lines(serialized_bank);
    Serializer::Lines::const_iterator it = lines->begin();
    Serializer::Lines::const_iterator end = lines->end();
    size_t next_program_index = 0;
    Program dummy_program;

    while (it != end && next_program_index < NUMBER_OF_PROGRAMS) {
        dummy_program.import(it, end);

        programs[next_program_index].import("");
        programs[next_program_index].set_name(dummy_program.get_name());

        ++next_program_index;
    }

    generate_empty_programs(next_program_index);

    delete lines;
}


std::string Bank::serialize() const
{
    size_t non_blank_programs = 0;

    for (size_t i = 0; i != NUMBER_OF_PROGRAMS; ++i) {
        if (!programs[i].is_blank()) {
            ++non_blank_programs;
        }
    }

    std::string result;

    result.reserve(non_blank_programs * 32768);

    for (size_t i = 0; i != NUMBER_OF_PROGRAMS; ++i) {
        result += programs[i].serialize();
        result += "\r\n";
    }

    return result;
}

}

#endif
