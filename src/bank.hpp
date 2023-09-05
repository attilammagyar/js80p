/*
 * This file is part of JS80P, a synthesizer plugin.
 * Copyright (C) 2023  Attila M. Magyar
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

#ifndef JS80P__BANK_HPP
#define JS80P__BANK_HPP

#include <cstddef>
#include <string>

#include "js80p.hpp"
#include "serializer.hpp"


namespace JS80P
{

class Bank
{
    public:
        class Program
        {
            public:
                static constexpr Integer NAME_MAX_LENGTH = 24;
                static constexpr Integer SHORT_NAME_MAX_LENGTH = 8;

                Program();

                Program(
                    std::string const& name,
                    std::string const& default_name,
                    std::string const& serialized
                );

                Program(Program const& program) = default;
                Program(Program&& program) = default;

                Program& operator=(Program const& program) = default;
                Program& operator=(Program&& program) = default;

                std::string const& get_name() const;
                std::string const& get_short_name() const;
                void set_name(std::string const& new_name);

                bool is_blank() const;

                std::string const& serialize() const;

                void import(std::string const& serialized);

                void import(
                    Serializer::Lines::const_iterator& it,
                    Serializer::Lines::const_iterator const& end
                );

            private:
                std::string sanitize_name(std::string const& name) const;

                std::string truncate(
                    std::string const& text,
                    std::string::size_type const max_length
                ) const;

                bool is_allowed_char(char const c) const;

                void set_name_without_update(std::string const& new_name);

                void import_without_update(std::string const& serialized);

                void import_without_update(
                    Serializer::Lines::const_iterator& it,
                    Serializer::Lines::const_iterator const& end
                );

                void update();

                std::string name;
                std::string short_name;
                std::string default_name;
                std::string serialized;
                std::string::size_type params_start;
        };

        static constexpr size_t NUMBER_OF_PROGRAMS = 128;

        static size_t normalized_parameter_value_to_program_index(
            Number const parameter_value
        );
        static Number program_index_to_normalized_parameter_value(
            size_t const index
        );

        Bank();

        Program& operator[](size_t const index);
        Program const& operator[](size_t const index) const;

        size_t get_current_program_index() const;
        void set_current_program_index(size_t const new_index);

        void import(std::string const& serialized_bank);
        void import_names(std::string const& serialized_bank);
        std::string serialize() const;

    private:
        static size_t const NUMBER_OF_BUILT_IN_PROGRAMS;
        static Program const BUILT_IN_PROGRAMS[];
        static constexpr Number FLOAT_TO_PROGRAM_INDEX_SCALE = (
            (Number)(NUMBER_OF_PROGRAMS - 1)
        );
        static constexpr Number PROGRAM_INDEX_TO_FLOAT_SCALE = (
            1.0 / (Number)(NUMBER_OF_PROGRAMS - 1)
        );

        void generate_empty_programs(size_t const start_index);

        Program programs[NUMBER_OF_PROGRAMS];
        size_t current_program_index;
        size_t non_blank_programs;
};

}

#endif
