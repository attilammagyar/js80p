#!/bin/bash

###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023, 2024  Attila M. Magyar
# Copyright (C) 2023  @aimixsaka (https://github.com/aimixsaka/)
# Copyright (C) 2024  @YHStar (https://github.com/YHStar)
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

# set -x
set -e
set -u
set -o pipefail

TARGET_PLATFORMS="x86_64-w64-mingw32:avx x86_64-w64-mingw32:sse2 i686-w64-mingw32:sse2 x86_64-gpp:avx x86_64-gpp:sse2 i686-gpp:sse2 riscv64-gpp:none loongarch64-gpp:lsx"
PLUGIN_TYPES="fst vst3"
TEXT_FILES="LICENSE.txt README.txt NEWS.txt"
DIST_DIR_BASE="dist"
README_HTML="$DIST_DIR_BASE/README.html"

main()
{
    local version_tag
    local version_str
    local version_int
    local target_platform
    local instruction_set
    local source_dir
    local source_archive
    local uncommitted
    local date
    local build_platform
    local target_platforms="${@:-$TARGET_PLATFORMS}"

    log "Verifying repository"

    if [[ ! -d "src" ]] || [[ ! -d "presets" ]]
    then
        error "This script must be run in the root directory of the repository."
    fi

    version_tag="$(git tag --points-at HEAD | grep -E "^v[0-9]+\\.[0-9]\\.[0-9]$" || true)"
    version_str="${version_tag:1}"
    version_int="$(version_str_to_int "$version_str")"

    if [[ "$version_tag" = "" ]]
    then
        version_tag="v999.0.0"
        version_str="${version_tag:1}-$(git log -1 --pretty=%h)"
        version_int="$(version_str_to_int "999.0.0")"
        log "WARNING: No version tag (format: \"v123.4.5\") found on git HEAD, creating a development release."
    fi

    if [[ "$version_tag" != "v999.0.0" ]]
    then
        uncommitted="$(git status --porcelain | wc -l)"

        if [[ $uncommitted -ne 0 ]]
        then
            error "Commit your changes first. (Will not release $version_tag from a dirty repo.)"
        fi

        date="$(date '+%Y-%m-%d')"

        grep "^$(echo "$version_tag" | sed 's/\./\\./g') ($date)" NEWS.txt >/dev/null \
            || error "Cannot find '$version_tag ($date)' in NEWS.txt"
    fi

    version_as_file_name="$(version_str_to_file_name "$version_str")"

    source_dir="js80p-$version_as_file_name-src"
    source_archive="$source_dir.zip"

    log "Cleaning up"

    cleanup "$DIST_DIR_BASE"
    cleanup "$README_HTML"

    log "Copying source"

    mkdir -p "$DIST_DIR_BASE/$source_dir"
    mkdir -p "$DIST_DIR_BASE/$source_dir/doc"

    cp --verbose --recursive \
        build.bat \
        build.ps1 \
        build.sh \
        Doxyfile \
        gui \
        js80p.png \
        lib \
        LICENSE.txt \
        make \
        Makefile \
        pm-fm-equivalence.md \
        presets \
        README.md \
        README.txt \
        scripts \
        src \
        tests \
        NEWS.txt \
        "$DIST_DIR_BASE/$source_dir/"

    cp --verbose --recursive \
        doc/pm-fm-equivalence.pdf \
        doc/pm-fm-equivalence.tex \
        "$DIST_DIR_BASE/$source_dir/doc/"

    find "$DIST_DIR_BASE/$source_dir/" -name ".*.swp" -delete

    log "Generating $README_HTML"

    build_readme_html >"$README_HTML"

    log "Creating source archive"

    cd "$DIST_DIR_BASE"
    zip --recurse-paths -9 "$source_archive" "$source_dir"
    cd ..

    build_platform="$(uname -m)"

    log "Generating js80p.vstxml ($build_platform)"

    call_make_for_build_platform \
        "$build_platform" \
        "$version_str" "$version_int" "$version_as_file_name" \
        vstxml

    log "Running unit tests ($build_platform)"

    call_make_for_build_platform \
        "$build_platform" \
        "$version_str" "$version_int" "$version_as_file_name" \
        check

    for target_platform in $target_platforms
    do
        log "Building for target: $target_platform"

        instruction_set="$(printf "%s\n" "$target_platform" | cut -d ":" -f 2)"
        target_platform="$(printf "%s\n" "$target_platform" | cut -d ":" -f 1)"

        call_make \
            "$target_platform" "$instruction_set" \
            "$version_str" "$version_int" "$version_as_file_name" \
            clean

        call_make \
            "$target_platform" "$instruction_set" \
            "$version_str" "$version_int" "$version_as_file_name" \
            all

        package_fst \
            "$target_platform" "$instruction_set" \
            "$version_str" "$version_int" "$version_as_file_name"

        package_vst3_single_file \
            "$target_platform" "$instruction_set" \
            "$version_str" "$version_int" "$version_as_file_name"
    done

    if [[ "$target_platforms" =~ :sse2 ]]
    then
        log "Bulding VST 3 bundle for SSE 2"
        package_vst3_bundle "$version_as_file_name" "sse2"
    else
        log "Skipping VST 3 bundle for SSE 2"
    fi

    if [[ "$target_platforms" =~ :avx ]]
    then
        log "Bulding VST 3 bundle for AVX"
        package_vst3_bundle "$version_as_file_name" "avx"
    else
        log "Skipping VST 3 bundle for AVX"
    fi

    if [[ "$target_platforms" =~ riscv64-gpp:none ]]
    then
        log "Bulding VST 3 bundle for RISC-V 64"
        package_vst3_bundle "$version_as_file_name" "none"
    else
        log "Skipping VST 3 bundle for RISC-V 64"
    fi

    if [[ "$target_platforms" =~ loong64-gpp:lsx ]]
    then
        log "Bulding VST 3 bundle for loongarch64"
        package_vst3_bundle "$version_as_file_name" "lsx"
    else
        log "Skipping VST 3 bundle for loongarch64"
    fi

    log "Done"
}

error()
{
    local message_tpl="$1"

    shift

    printf "ERROR: $message_tpl\n" "$@" >&2

    exit 1
}

log()
{
    local message_tpl="$1"

    shift

    printf "### %s\t$message_tpl\n" "$(date)" "$@" >&2
}

cleanup()
{
    local name="$1"

    if [[ "$name" = "" ]] || [[ "${name:0:1}" = "/" ]]
    then
        error "Invalid argument for cleanup: %s" "$name"
    else
        log "Removing $name"
        rm -rf -- "$name"
    fi
}

build_readme_html()
{
    local placeholder="^{{HTML}}$"
    local template="scripts/readme_html.tpl"

    grep -B 10000000 "$placeholder" "$template" | grep -v "$placeholder"
    cat README.md | grep -v "<img.*src.*raw.githubusercontent.com" | markdown
    grep -A 10000000 "$placeholder" "$template" | grep -v "$placeholder"
}

version_str_to_int()
{
    local version_str="$1"

    printf "%s0\n" "$version_str" | sed "s/[^0-9]//g"
}

version_str_to_file_name()
{
    local version_str="$1"

    printf "%s\n" "$version_str" | sed "s/[^0-9a-f-]/_/g"
}

call_make_for_build_platform()
{
    local build_platform="$1"

    shift

    case "$build_platform" in
        "x86_64")       call_make "x86_64-w64-mingw32" "avx" "$@" ;;
        "riscv64")      call_make "riscv64-gpp" "none" "$@" ;;
        "loongarch64")  call_make "loong64-gpp" "lsx" "$@" ;;
        *) error "Unsupported build platform: $uname" ;;
    esac
}

call_make()
{
    local target_platform="$1"
    local instruction_set="$2"
    local version_str="$3"
    local version_int="$4"
    local version_as_file_name="$5"

    shift
    shift
    shift
    shift
    shift

    TARGET_PLATFORM="$target_platform" \
        INSTRUCTION_SET="$instruction_set" \
        VERSION_STR="$version_str" \
        VERSION_INT="$version_int" \
        VERSION_AS_FILE_NAME="$version_as_file_name" \
        make "$@"
}

package_fst()
{
    local target_platform="$1"
    local instruction_set="$2"
    local version_str="$3"
    local version_int="$4"
    local version_as_file_name="$5"
    local dist_dir

    dist_dir=$(
        get_dist_dir \
            "$target_platform" "$instruction_set" \
            "$version_str" "$version_int" "$version_as_file_name" \
            "show_fst_dir"
    )

    if [[ "$target_platform" =~ "-mingw32" ]]
    then
        finalize_package "$dist_dir" "convert" "$DIST_DIR_BASE/js80p.vstxml"
    else
        finalize_package "$dist_dir" "" "$DIST_DIR_BASE/js80p.vstxml"
    fi
}

get_dist_dir()
{
    local target_platform="$1"
    local instruction_set="$2"
    local version_str="$3"
    local version_int="$4"
    local version_as_file_name="$5"
    local make_target="$6"

    basename "$(call_make "$target_platform" "$instruction_set" "$version_str" "$version_int" "$version_as_file_name" "$make_target")"
}

finalize_package()
{
    local dist_dir="$1"
    local convert_newlines="$2"
    local extra_file="$3"
    local dist_archive
    local src_file
    local dst_file

    log "Copying presets, etc. to $DIST_DIR_BASE/$dist_dir"

    cp --verbose "$README_HTML" "$DIST_DIR_BASE/$dist_dir/"
    cp --verbose --recursive presets "$DIST_DIR_BASE/$dist_dir/"

    if [[ ! -z "$extra_file" ]]
    then
        cp --verbose "$extra_file" "$DIST_DIR_BASE/$dist_dir/"
    fi

    if [[ "$convert_newlines" = "convert" ]]
    then
        for src_file in $TEXT_FILES
        do
            dst_file="$DIST_DIR_BASE/$dist_dir/$src_file"
            convert_text_file "$src_file" "$dst_file"
        done
    else
        cp --verbose $TEXT_FILES "$DIST_DIR_BASE/$dist_dir/"
    fi

    dist_archive="$dist_dir.zip"

    log "Creating release archive: $DIST_DIR_BASE/$dist_archive"

    cd dist
    zip --recurse-paths -9 "$dist_archive" "$dist_dir"
    cd ..
}

convert_text_file()
{
    local src_file="$1"
    local dst_file="$2"

    printf "Converting %s to %s\n" "$src_file" "$dst_file"
    cat "$src_file" | sed 's/$/\r/g' >"$dst_file"
}

package_vst3_single_file()
{
    local target_platform="$1"
    local instruction_set="$2"
    local version_str="$3"
    local version_int="$4"
    local version_as_file_name="$5"
    local dist_dir

    dist_dir=$(
        get_dist_dir \
            "$target_platform" "$instruction_set" \
            "$version_str" "$version_int" "$version_as_file_name" \
            "show_vst3_dir"
    )

    if [[ "$target_platform" =~ "-mingw32" ]]
    then
        finalize_package "$dist_dir" "convert" ""
    else
        finalize_package "$dist_dir" "" ""
    fi
}

package_vst3_bundle()
{
    local version_as_file_name="$1"
    local instruction_set="$2"
    local dist_dir="js80p-$version_as_file_name-$instruction_set-vst3_bundle"
    local vst3_base_dir="$DIST_DIR_BASE/$dist_dir"
    local proc_id
    local doc_dir="$vst3_base_dir/js80p.vst3/Resources/Documentation"

    proc_id="$(get_vst3_snapshot_id)"

    mkdir --verbose --parents "$vst3_base_dir/js80p.vst3/Contents"
    mkdir --verbose --parents "$vst3_base_dir/js80p.vst3/Resources/Snapshots"
    mkdir --verbose --parents "$doc_dir"

    cp --verbose "js80p.png" "$vst3_base_dir/js80p.vst3/Resources/Snapshots/${proc_id}_snapshot.png"

    cp --verbose "$README_HTML" "$doc_dir/README.html"

    case "$instruction_set" in
        "sse2")
            copy_vst3 "$version_as_file_name" "linux-x86-sse2" "$vst3_base_dir" "i386-linux" "js80p.so"
            copy_vst3 "$version_as_file_name" "linux-x86-sse2" "$vst3_base_dir" "i686-linux" "js80p.so"
            copy_vst3 "$version_as_file_name" "windows-x86-sse2" "$vst3_base_dir" "x86-win" "js80p.vst3"
            copy_vst3 "$version_as_file_name" "linux-x86_64-sse2" "$vst3_base_dir" "x86_64-linux" "js80p.so"
            copy_vst3 "$version_as_file_name" "windows-x86_64-sse2" "$vst3_base_dir" "x86_64-win" "js80p.vst3"
            ;;
        "avx")
            copy_vst3 "$version_as_file_name" "linux-x86_64-avx" "$vst3_base_dir" "x86_64-linux" "js80p.so"
            copy_vst3 "$version_as_file_name" "windows-x86_64-avx" "$vst3_base_dir" "x86_64-win" "js80p.vst3"
            ;;
        "lsx")
            copy_vst3 "$version_as_file_name" "linux-loong64-lsx" "$vst3_base_dir" "loong64-linux" "js80p.so"
            ;;

        "none")
            copy_vst3 "$version_as_file_name" "linux-riscv64-none" "$vst3_base_dir" "riscv64-linux" "js80p.so"
            ;;
        *)
            error "Unknown instruction_set: $instruction_set."
            ;;
    esac


    for src_file in $TEXT_FILES
    do
        dst_file="$doc_dir/$src_file"
        convert_text_file "$src_file" "$dst_file"
    done

    finalize_package "$dist_dir" "convert" ""
}

get_vst3_snapshot_id()
{
    grep "Vst3Plugin::Processor::ID(.*);" src/plugin/vst3/plugin.cpp \
        | sed -r "s/^.*\\((.*)\\).*/\\1/g ; s/^0x//g ; s/, 0x//g" \
        | tr [[:lower:]] [[:upper:]]
}

copy_vst3()
{
    local version_as_file_name="$1"
    local src_dir="$2"
    local base_dir="$3"
    local dst_dir="$4"
    local dst_file="$5"
    local dir="$base_dir/js80p.vst3/Contents/$dst_dir"

    mkdir --verbose --parents "$dir"
    cp --verbose \
        "$DIST_DIR_BASE/js80p-$version_as_file_name-$src_dir-vst3_single_file/js80p.vst3" \
        "$dir/$dst_file"
}

main "$@"
