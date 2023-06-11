import collections
import glob
import os.path
import re
import sys


kVstMaxProgNameLen = 24

BLANK_PRESET_NAME = "blank.js80p"
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


Preset = collections.namedtuple("Preset", ("basename", "path", "program_name"))


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
        match = re.search(r"^ *static +.* +NUMBER_OF_PROGRAMS *= *([0-9]+) *;", src, re.DOTALL | re.MULTILINE)

        if not match:
            print(f"Cannot find NUMBER_OF_PROGRAMS in bank.hpp", file=sys.stderr)
            return 1

        number_of_programs = int(match[1])

    programs = []
    presets = collect_presets(presets_dir, programs_cpp)

    for i, (preset_file_name, preset_file_path, preset_name) in enumerate(presets):
        if i == number_of_programs:
            break

        default_name = f"Prog{i+1:03}"

        if len(preset_name) > kVstMaxProgNameLen:
            raise ValueError(
                f"Name too long; preset_name={preset_name!r}, length={len(preset_name)}, max_length={kVstMaxProgNameLen}"
            )

        with open(preset_file_path, "r") as preset_file:
            program = (
                [
                    "",
                    f"{INDENT_PROGRAM}{PROGRAM_START}",
                    f"{INDENT_PROGRAM_BODY}\"{preset_name}\",",
                    f"{INDENT_PROGRAM_BODY}\"{default_name}\",",
                    f"{INDENT_PROGRAM_BODY}(",
                ]
                + [f"{INDENT_PROGRAM_BODY_CONT}\"{line.strip()}\\n\"" for line in preset_file]
                + [
                    f"{INDENT_PROGRAM_BODY})",
                    f"{INDENT_PROGRAM}),"
                ]
            )

        programs.append("\n".join(program))

    with open(programs_cpp, "w") as f:
        print(
            (
                template.replace("/* WARNING */", WARNING)
                    .replace("/* NUMBER_OF_BUILT_IN_PROGRAMS */", str(len(programs)))
                    .replace("/* PROGRAMS */", "\n".join(programs))
            ),
            file=f
        )

    return 0


def collect_presets(presets_dir, programs_cpp):
    all_presets = []
    existing_programs = set()
    existing_programs_order = []
    blank_preset_file_path = None
    presets_by_program_name = {}

    for basename, path in sorted(
        ((os.path.basename(path), path) for path in glob.glob(os.path.join(presets_dir, "*.js80p"))),
        key=lambda basename_path_pair: basename_path_pair[0]
    ):
        if basename == BLANK_PRESET_NAME:
            blank_preset_file_path = path
            continue

        program_name = preset_file_name_to_program_name(basename)

        if program_name in presets_by_program_name:
            raise Exception(f"Non-unique program names: {presets_by_program_name[program_name].basename} vs {basename}")

        preset = Preset(basename, path, program_name)
        presets_by_program_name[program_name] = preset
        all_presets.append(preset)

    if blank_preset_file_path is None:
        raise Exception(f"{BLANK_PRESET_NAME!r} preset not found in {presets_dir!r}", file=sys.stderr)

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
        [Preset(BLANK_PRESET_NAME, blank_preset_file_path, "Blank")]
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


def preset_file_name_to_program_name(file_name):
    return (
        file_name
            .rsplit(".", 1)[0]
            .replace("_", " ")
            .replace("demo-", "demo ")
            .title()
            .replace("Fm ", "FM ")
    )


if __name__ == "__main__":
    sys.exit(main(sys.argv))

