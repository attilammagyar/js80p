###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023, 2024  Attila M. Magyar
#
# JS80P is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# JS80P is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
###############################################################################

import collections
import glob
import os.path
import re
import sys


kVstMaxProgNameLen = 24

BLANK_PRESET_FILE_NAME = "blank.js80p"
BLANK_PRESET_NAME = "Blank"
INDENT_PROGRAM = " " * 4
INDENT_PROGRAM_BODY = " " * 8
INDENT_PROGRAM_BODY_CONT = " " * 12
PROGRAM_START = "Program("
WARNING = """\
/*
  #############################################
  #                                           #
  # THIS IS A GENERATED FILE, DO NOT EDIT IT! #
  # USE scritps/gen_programs.py TO UPDATE.    #
  #                                           #
  #############################################
*/"""


Preset = collections.namedtuple(
    "Preset", ("basename", "path", "program_name", "lines", "comments")
)


def main(argv):
    presets_dir = os.path.join(os.path.dirname(argv[0]), "..", "presets")
    src_dir = os.path.join(os.path.dirname(argv[0]), "..", "src")
    programs_cpp = os.path.join(src_dir, "programs.cpp")

    if not os.path.isdir(presets_dir):
        print(f"Cannot find presets directory: {presets_dir!r}", file=sys.stderr)
        return 1

    if not os.path.isdir(src_dir):
        print(f"Cannot find src directory: {src_dir!r}", file=sys.stderr);
        return 1

    with open(os.path.join(src_dir, "programs_tpl.cpp"), "r") as f:
        template = f.read()

    number_of_programs = 0

    with open(os.path.join(src_dir, "bank.hpp"), "r") as f:
        src = f.read()
        matches = re.search(
            r"^ *static +.* +NUMBER_OF_PROGRAMS *= *([0-9]+) *;",
            src,
            re.DOTALL | re.MULTILINE
        )

        if not matches:
            print(f"Cannot find NUMBER_OF_PROGRAMS in bank.hpp", file=sys.stderr)
            return 1

        number_of_programs = int(matches[1])

    programs = []
    presets = collect_presets(presets_dir, programs_cpp)
    readme_toc = []
    readme_presets = []

    for i, preset in enumerate(presets):
        (
            preset_file_name,
            preset_file_path,
            preset_name,
            preset_lines,
            preset_comments
        ) = preset

        if i == number_of_programs:
            break

        default_name = f"Prog{i+1:03}"

        if len(preset_name) > kVstMaxProgNameLen:
            raise ValueError(
                f"Name too long; preset_name={preset_name!r},"
                f" length={len(preset_name)},"
                f" max_length={kVstMaxProgNameLen}"
            )

        program = (
            [
                "",
                f"{INDENT_PROGRAM}{PROGRAM_START}",
                f"{INDENT_PROGRAM_BODY}\"{preset_name}\",",
                f"{INDENT_PROGRAM_BODY}\"{default_name}\",",
                f"{INDENT_PROGRAM_BODY}(",
            ]
            + preset_lines_to_cpp_lines(INDENT_PROGRAM_BODY_CONT, preset_lines)
            + [
                f"{INDENT_PROGRAM_BODY})",
                f"{INDENT_PROGRAM}),"
            ]
        )

        programs.append("\n".join(program))

        anchor = (
            "preset-"
            + preset_file_name.replace(".js80p", "").replace("_", "-")
        )
        readme_toc.append(f"    * [{preset_name}](#{anchor})")
        readme_presets.append(
            f"\n<a id=\"{anchor}\"></a>\n\n### {preset_name}\n"
        )
        readme_presets.append("\n".join(preset_comments))

    with open(programs_cpp, "w") as f:
        print(
            (
                template
                    .replace("/* WARNING */", WARNING)
                    .replace(
                        "/* NUMBER_OF_BUILT_IN_PROGRAMS */", str(len(programs))
                    )
                    .replace("/* PROGRAMS */", "\n".join(programs))
            ),
            file=f
        )

    print("\n".join(readme_toc))
    print("\n".join(readme_presets))

    return 0


def collect_presets(presets_dir, programs_cpp):
    all_presets = []
    existing_programs = set()
    existing_programs_order = []
    blank_preset_file_path = None
    presets_by_program_name = {}
    preset_files = glob.glob(os.path.join(presets_dir, "*.js80p"))

    for basename, path in sorted(
        ((os.path.basename(path), path) for path in preset_files),
        key=lambda basename_path_pair: basename_path_pair[0]
    ):
        if basename == BLANK_PRESET_FILE_NAME:
            blank_preset_file_path = path
            continue

        program_name = preset_file_name_to_program_name(basename)

        if program_name in presets_by_program_name:
            raise Exception(
                "Non-unique program names:"
                f" {presets_by_program_name[program_name].basename}"
                f" vs {basename}"
            )

        preset = make_preset(basename, path, program_name)
        presets_by_program_name[program_name] = preset
        all_presets.append(preset)

    if blank_preset_file_path is None:
        raise Exception(
            f"{BLANK_PRESET_FILE_NAME!r} preset not found in {presets_dir!r}"
        )

    try:
        with open(programs_cpp, "r") as f:
            next_line_is_program_name = False

            for line in f:
                if next_line_is_program_name:
                    program_name = line.strip("\r\n \",")
                    existing_programs.add(program_name)
                    existing_programs_order.append(program_name)
                    next_line_is_program_name = False
                elif line.strip().endswith(PROGRAM_START):
                    next_line_is_program_name = True
    except FileNotFoundError:
        pass

    return (
        [
            make_preset(
                BLANK_PRESET_FILE_NAME, blank_preset_file_path, BLANK_PRESET_NAME
            )
        ]
        + [
            presets_by_program_name[program_name]
            for program_name in existing_programs_order
                if program_name in presets_by_program_name
        ]
        + [
            preset
            for preset in all_presets
                if preset.program_name not in existing_programs
        ]
    )


def make_preset(basename, file_path, program_name):
    lines = []
    comments = []

    for line in read_lines(file_path):
        s = line.strip()

        if not s or s.startswith(";"):
            comments.append(line.lstrip("; ").strip())
        else:
            lines.append(line)

    return Preset(basename, file_path, program_name, lines, comments)


def read_lines(file_path):
    with open(file_path, "r") as f:
        return [line for line in f]


def preset_file_name_to_program_name(file_name):
    return (
        file_name
            .rsplit(".", 1)[0]
            .replace("_", " ")
            .replace("demo-", "demo ")
            .title()
            .replace("Fm ", "FM ")
            .replace(" At", " AT")
            .replace("Lo-Fi ", "Lo-fi ")
            .replace(" Mod", " mod")
    )


def preset_lines_to_cpp_lines(indentation, lines):
    return [f"{indentation}\"{line.strip()}\\n\"" for line in lines]


if __name__ == "__main__":
    sys.exit(main(sys.argv))
