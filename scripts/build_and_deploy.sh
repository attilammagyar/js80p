#!/bin/bash

###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023, 2024  Attila M. Magyar
# Copyright (C) 2023  @aimixsaka (https://github.com/aimixsaka/)
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

set -e

BASE_DIR=~

FST_DIRS_LINUX_32="$BASE_DIR/vst"
FST_DIRS_LINUX_64="$BASE_DIR/vst"

FST_DIRS_WINE_32="$BASE_DIR/.wine/drive_c/vst"
FST_DIRS_WINE_64="$BASE_DIR/.wine/drive_c/vst"

VST3_DIRS_LINUX_32="$BASE_DIR/vst $BASE_DIR/.vst3"
VST3_DIRS_LINUX_64="$BASE_DIR/vst $BASE_DIR/.vst3"

VST3_DIRS_WINE_32="$BASE_DIR/.wine/drive_c/vst $BASE_DIR/.wine/drive_c/Program*Files/Common*Files/VST3"
VST3_DIRS_WINE_64="$BASE_DIR/.wine/drive_c/vst $BASE_DIR/.wine/drive_c/Program*Files/Common*Files/VST3"


main()
{
    local plugin_type="$1"
    local target_os="$2"
    local arch="$3"
    local instruction_set="$4"
    local target_platform=""
    local built_plugin=""
    local suffix=""

    if [[ "$plugin_type$target_os$arch" = "" ]]
    then
        echo "Usage: $0 fst|vst3 linux|windows x86|x86_64|riscv64 [avx|sse2|none]" >&2
        return 1
    fi

    if [[ "$plugin_type" = "" ]]; then plugin_type="fst"; fi
    if [[ "$target_os" = "" ]]; then target_os="linux"; fi
    if [[ "$arch" = "" ]]; then arch="64bit"; fi
    if [[ "$instruction_set" = "" ]]; then instruction_set="avx"; fi

    if [[ "$plugin_type" = "vst3" ]]; then suffix="_single_file" ; fi

    case "$arch" in
        "x86") target_platform="i686" ;;
        "x86_64"|"riscv64") target_platform="$arch" ;;
        *)
            echo "Unknown architecture: \"$arch\" - should be either \"x86\" or \"x86_64\" or \"riscv64\"" >&2
            return 1
            ;;
    esac

    case "$target_os" in
        "linux") target_platform="$target_platform-gpp" ;;
        "windows") target_platform="$target_platform-w64-mingw32" ;;
        *)
            echo "Unknown target OS: \"$target_os\" - should be either \"linux\" or \"windows\"" >&2
            return 1
            ;;
    esac

    built_plugin="dist/js80p-dev-$target_os-$arch-$instruction_set-$plugin_type$suffix"

    case "$target_os-$plugin_type" in
        "linux-fst") built_plugin="$built_plugin/js80p.so" ;;
        "windows-fst") built_plugin="$built_plugin/js80p.dll" ;;
        "linux-vst3"|"windows-vst3") built_plugin="$built_plugin/js80p.vst3" ;;
        *)
            echo "Unknown plugin type: \"$plugin_type\" - should be either \"fst\" or \"vst3\"" >&2
            return 1
            ;;
    esac

    echo "Building; plugin_type=\"$plugin_type\", target_os=\"$target_os\", arch=\"$arch\", instruction_set=\"$instruction_set\"" >&2

    TARGET_PLATFORM="$target_platform" INSTRUCTION_SET="$instruction_set" make "$plugin_type"

    case "$target_os-$plugin_type-$arch" in
        "linux-fst-x86_64")     replace_in_dir "$built_plugin" $FST_DIRS_LINUX_64 ;;
        "linux-vst3-x86_64")    replace_in_dir "$built_plugin" $VST3_DIRS_LINUX_64 ;;
        "linux-fst-riscv64")    replace_in_dir "$built_plugin" $VST3_DIRS_LINUX_64 ;;
        "linux-vst3-riscv64")   replace_in_dir "$built_plugin" $VST3_DIRS_LINUX_64 ;;
        "windows-fst-x86_64")   replace_in_dir "$built_plugin" $FST_DIRS_WINE_64 ;;
        "windows-vst3-x86_64")  replace_in_dir "$built_plugin" $VST3_DIRS_WINE_64 ;;
        "linux-fst-x86")        replace_in_dir "$built_plugin" $FST_DIRS_LINUX_32 ;;
        "linux-vst3-x86")       replace_in_dir "$built_plugin" $VST3_DIRS_LINUX_32 ;;
        "windows-fst-x86")      replace_in_dir "$built_plugin" $FST_DIRS_WINE_32 ;;
        "windows-vst3-x86")     replace_in_dir "$built_plugin" $VST3_DIRS_WINE_32 ;;
    esac

    echo "SUCCESS" >&2

    return 0
}


replace_in_dir()
{
    local built_plugin="$1"
    local dir

    shift

    while [[ "$1" != "" ]]
    do
        dir="$1"
        echo ""
        echo "Replacing JS80P; dir=\"$dir\""
        rm -rfv "$dir"/js80p.*
        cp -v "$built_plugin" "$dir/"
        shift
    done
}


main "$@"
