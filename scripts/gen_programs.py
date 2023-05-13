import glob
import os.path
import re
import sys


kVstMaxProgNameLen = 24


def main(argv):
    BLANK_PRESET_NAME = "blank.js80p"
    INDENT_PROGRAM = " " * 4
    INDENT_PROGRAM_BODY = " " * 8
    INDENT_PROGRAM_BODY_CONT = " " * 12
    WARNING = """\
/*
  #############################################
  #                                           #
  # THIS IS A GENERATED FILE, DO NOT EDIT IT! #
  # USE scritps/gen_programs.py TO UPDATE.    #
  #                                           #
  #############################################
*/"""

    presets_dir = os.path.join(os.path.dirname(argv[0]), "..", "presets")
    src_dir = os.path.join(os.path.dirname(argv[0]), "..", "src")

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
    preset_file_paths = []
    blank_preset_file_path = None

    for basename, path in sorted(
        ((os.path.basename(path), path) for path in glob.glob(os.path.join(presets_dir, "*.js80p"))),
        key=lambda basename_path_pair: basename_path_pair[0]
    ):
        if basename == BLANK_PRESET_NAME:
            blank_preset_file_path = path
            continue

        preset_file_paths.append((basename, path))

    if blank_preset_file_path is None:
        print(f"{BLANK_PRESET_NAME!r} preset not found in {presets_dir!r}", file=sys.stderr)
        return 1

    preset_file_paths = [(BLANK_PRESET_NAME, blank_preset_file_path)] + preset_file_paths

    for i, (preset_file_name, preset_file_path) in enumerate(preset_file_paths):
        if i == number_of_programs:
            break

        name = (
            os.path.basename(preset_file_path)
                .rsplit(".", 1)[0]
                .replace("_", " ")
                .replace("demo-", "demo ")
                .title()
        )
        default_name = f"Blank Slot {i+1}"

        if len(name) > kVstMaxProgNameLen:
            raise ValueError(
                f"Name too long; name={name!r}, length={len(name)}, max_length={kVstMaxProgNameLen}"
            )

        with open(preset_file_path, "r") as preset_file:
            program = (
                [
                    "",
                    f"{INDENT_PROGRAM}Program(",
                    f"{INDENT_PROGRAM_BODY}\"{name}\",",
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

    with open(os.path.join(src_dir, "programs.cpp"), "w") as f:
        print(
            (
                template.replace("/* WARNING */", WARNING)
                    .replace("/* NUMBER_OF_BUILT_IN_PROGRAMS */", str(len(programs)))
                    .replace("/* PROGRAMS */", "\n".join(programs))
            ),
            file=f
        )

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

