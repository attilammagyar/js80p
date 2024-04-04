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

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include "js80p.hpp"
#include "midi.hpp"
#include "synth.hpp"

#include "gui/gui.hpp"

#include "plugin/fst/plugin.hpp"


/*
XML schema as parsed by JUCE in
`modules/juce_audio_processors/format_types/juce_VSTPluginFormat.cpp`.
*/


using namespace JS80P;


void usage(char const* name)
{
    fprintf(stderr, "Usage: %s out_dir/js80p.vstxml\n", name);
}


void write_line(std::ofstream& out_file, char const* line)
{
    out_file << line << "\r\n";
}


void write_param(
        std::ofstream& out_file,
        size_t const id,
        char const* name,
        char const* short_name
) {
    constexpr size_t buffer_size = 256;
    constexpr char const* format = (
        "  <Param id=\"%lu\" name=\"%s\" shortName=\"%s\" />"
    );

    char buffer[buffer_size];

    snprintf(
        buffer, buffer_size, format, (long unsigned int)id, name, short_name
    );
    write_line(out_file, buffer);
}


void generate_xml(std::ofstream& out_file)
{
    Synth synth;
    FstPlugin::Parameters parameters;

    FstPlugin::populate_parameters(synth, parameters);

    FstPlugin::Parameter const& program = parameters[0];

    write_line(out_file, "<VSTParametersStructure>");
    write_param(out_file, 0, program.get_name(), program.get_name());

    for (size_t i = 1; i != FstPlugin::NUMBER_OF_PARAMETERS; ++i) {
        FstPlugin::Parameter const& parameter = parameters[i];
        Midi::Controller const controller_id = parameter.get_controller_id();
        GUI::Controller const& controller = *GUI::get_controller(
            (Synth::ControllerId)controller_id
        );

        if (i == FstPlugin::PATCH_CHANGED_PARAMETER_INDEX) {
            write_param(
                out_file,
                i,
                FstPlugin::PATCH_CHANGED_PARAMETER_LONG_NAME,
                FstPlugin::PATCH_CHANGED_PARAMETER_SHORT_NAME
            );
        } else {
            write_param(
                out_file, i, controller.long_name, controller.short_name
            );
        }
    }

    write_line(out_file, "</VSTParametersStructure>");
}


int main(int const argc, char const* argv[])
{
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    std::string const out_file_name(argv[1]);

    if (out_file_name.length() < 1) {
        fprintf(stderr, "ERROR: output file name must not be empty\n");
        return 2;
    }

    std::ofstream out_file(out_file_name, std::ios::out | std::ios::binary);

    if (!out_file.is_open()) {
        char const* error_msg = strerror(errno);

        fprintf(
            stderr,
            "ERROR: unable to open output file \"%s\": errno=%d, error=\"%s\"\n",
            out_file_name.c_str(),
            errno,
            error_msg == NULL ? "<NULL>" : error_msg
        );

        return 3;
    }

    generate_xml(out_file);

    return 0;
}
