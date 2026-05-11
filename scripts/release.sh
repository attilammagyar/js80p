#!/bin/bash

###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023, 2024, 2025, 2026  Attila M. Magyar
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
MACOS="macos"
PLUGIN_TYPES="fst vst3"
TEXT_FILES="CONTRIBUTING.txt LICENSE.txt NEWS.txt README.txt"
DIST_DIR_BASE="dist"
README_HTML="$DIST_DIR_BASE/README.html"

main()
{
    local version_tag
    local version_str
    local version_int
    local version_dot
    local dev_os
    local source_dir
    local source_archive
    local uncommitted
    local date
    local build_platform
    local target_platforms="$@"

    if [[ "$target_platforms" = "" ]]
    then
        error_usage
    fi

    if [[ "$target_platforms" =~ "$MACOS" ]] && [[ "$target_platforms" != "$MACOS" ]]
    then
        error_usage
    fi

    if [[ "$target_platforms" = "$MACOS" ]] && [[ "$SIGNAPP" = "" ]]
    then
        error_usage
    fi

    if [[ "$target_platforms" = "$MACOS" ]] && [[ "$SIGNINST" = "" ]]
    then
        error_usage
    fi

    if [[ "$target_platforms" = "$MACOS" ]] && [[ "$KEYCHAIN" = "" ]]
    then
        error_usage
    fi

    log "Verifying repository"

    if [[ ! -d "src" ]] || [[ ! -d "presets" ]]
    then
        error "This script must be run in the root directory of the repository."
    fi

    version_tag="$(git tag --points-at HEAD | grep -E "^v[0-9]+\\.[0-9]\\.[0-9]$" || true)"
    version_dot="${version_tag:1}"
    version_str="$version_dot"
    version_int="$(version_str_to_int "$version_str")"

    if [[ "$version_tag" = "" ]]
    then
        version_tag="v999.0.0"
        version_dot="${version_tag:1}"
        version_str="$version_dot-$(git log -1 --pretty=%h)"
        version_int="$(version_str_to_int "$version_dot")"
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

    cp -v -r \
        build.bat \
        build.ps1 \
        build.sh \
        CONTRIBUTING.txt \
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

    cp -v -r \
        doc/pm-fm-equivalence.pdf \
        doc/pm-fm-equivalence.tex \
        doc/distortions.html \
        doc/distortions.ipynb \
        "$DIST_DIR_BASE/$source_dir/doc/"

    find "$DIST_DIR_BASE/$source_dir/" -name ".*.swp" -delete

    log "Generating $README_HTML"

    build_readme_html >"$README_HTML"

    log "Creating source archive"

    cd "$DIST_DIR_BASE"
    zip --recurse-paths -9 "$source_archive" "$source_dir"
    cd ..

    if [[ "$target_platforms" = "$MACOS" ]]
    then
        build_platform="$MACOS"
        dev_os="macos"
    else
        build_platform="$(uname -m)"
        dev_os="linux"
    fi

    log "Generating js80p.vstxml ($build_platform)"

    call_make_for_build_platform \
        "$dev_os" "$build_platform" \
        "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
        vstxml

    log "Running unit tests ($build_platform)"

    call_make_for_build_platform \
        "$dev_os" "$build_platform" \
        "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
        check

    if [[ "$target_platforms" = "$MACOS" ]]
    then
        build_packages_on_macos "$version_str" "$version_dot" "$version_int" "$version_as_file_name"
    else
        build_packages_on_linux \
            "$build_platform" "$target_platforms" \
            "$version_str" "$version_dot" "$version_int" "$version_as_file_name"
    fi

    log "Done"
}

error_usage()
{
    cat >&2 <<USAGE
Usage:

  on Linux:

    bash scripts/release.sh $TARGET_PLATFORMS

  on macOS:

    SIGNAPP="Developer ID Application: Name (ID)" \\
        SIGNINST="Developer ID Installer: Name (ID)" \\
        KEYCHAIN="notarytool-kc" \\
        bash scripts/release.sh $MACOS
USAGE

    exit 1
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
    local dev_os="$1"
    local build_platform="$2"

    shift
    shift

    case "$build_platform" in
        "x86_64")       call_make "$dev_os" "x86_64-gpp" "avx" "$@" ;;
        "riscv64")      call_make "$dev_os" "riscv64-gpp" "none" "$@" ;;
        "loongarch64")  call_make "$dev_os" "loongarch64-gpp" "lsx" "$@" ;;
        "$MACOS")       call_make "$dev_os" "gpp" "native" "$@" ;;
        *) error "Unsupported build platform: $build_platform" ;;
    esac
}

call_make()
{
    local dev_os="$1"
    local target_platform="$2"
    local instruction_set="$3"
    local version_str="$4"
    local version_dot="$5"
    local version_int="$6"
    local version_as_file_name="$7"

    shift
    shift
    shift
    shift
    shift
    shift
    shift

    DEV_OS="$dev_os" \
        TARGET_PLATFORM="$target_platform" \
        INSTRUCTION_SET="$instruction_set" \
        VERSION_STR="$version_str" \
        VERSION_DOT="$version_dot" \
        VERSION_INT="$version_int" \
        VERSION_AS_FILE_NAME="$version_as_file_name" \
        make "$@"
}

build_packages_on_linux()
{
    local build_platform="$1"
    local target_platforms="$2"
    local version_str="$3"
    local version_dot="$4"
    local version_int="$5"
    local version_as_file_name="$6"
    local target_platform
    local instruction_set

    for target_platform in $target_platforms
    do
        log "Building for target: $target_platform"

        instruction_set="$(printf "%s\n" "$target_platform" | cut -d ":" -f 2)"
        target_platform="$(printf "%s\n" "$target_platform" | cut -d ":" -f 1)"

        call_make \
            "linux" "$target_platform" "$instruction_set" \
            "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
            clean

        call_make_for_build_platform \
            "linux" "$build_platform" \
            "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
            vst3moduleinfo

        call_make \
            "linux" "$target_platform" "$instruction_set" \
            "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
            all

        build_fst_zip \
            "linux" "$target_platform" "$instruction_set" \
            "$version_str" "$version_dot" "$version_int" "$version_as_file_name"

        build_vst3_single_zip \
            "linux" "$target_platform" "$instruction_set" \
            "$version_str" "$version_dot" "$version_int" "$version_as_file_name"
    done

    if [[ "$target_platforms" =~ :sse2 ]]
    then
        log "Bulding VST 3 bundle for SSE 2"
        build_vst3_bundle_zip "$version_as_file_name" "sse2"
    else
        log "Skipping VST 3 bundle for SSE 2"
    fi

    if [[ "$target_platforms" =~ :avx ]]
    then
        log "Bulding VST 3 bundle for AVX"
        build_vst3_bundle_zip "$version_as_file_name" "avx"
    else
        log "Skipping VST 3 bundle for AVX"
    fi

    if [[ "$target_platforms" =~ riscv64-gpp:none ]]
    then
        log "Bulding VST 3 bundle for RISC-V 64"
        build_vst3_bundle_zip "$version_as_file_name" "none"
    else
        log "Skipping VST 3 bundle for RISC-V 64"
    fi

    if [[ "$target_platforms" =~ loongarch64-gpp:lsx ]]
    then
        log "Bulding VST 3 bundle for loongarch64"
        build_vst3_bundle_zip "$version_as_file_name" "lsx"
    else
        log "Skipping VST 3 bundle for loongarch64"
    fi
}

build_fst_zip()
{
    local dev_os="$1"
    local target_platform="$2"
    local instruction_set="$3"
    local version_str="$4"
    local version_dot="$5"
    local version_int="$6"
    local version_as_file_name="$7"
    local dist_dir

    dist_dir=$(
        get_dist_dir \
            "$dev_os" "$target_platform" "$instruction_set" \
            "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
            "show_fst_dir"
    )

    if [[ "$target_platform" =~ "-mingw32" ]]
    then
        finalize_zip "$dist_dir" "convert" "$DIST_DIR_BASE/js80p.vstxml"
    else
        finalize_zip "$dist_dir" "" "$DIST_DIR_BASE/js80p.vstxml"
    fi
}

get_dist_dir()
{
    local dev_os="$1"
    local target_platform="$2"
    local instruction_set="$3"
    local version_str="$4"
    local version_dot="$5"
    local version_int="$6"
    local version_as_file_name="$7"
    local make_target="$8"

    basename "$(call_make "$dev_os" "$target_platform" "$instruction_set" "$version_str" "$version_dot" "$version_int" "$version_as_file_name" "$make_target")"
}

finalize_zip()
{
    local dist_dir="$1"
    local convert_newlines="$2"
    local extra_file="$3"
    local dist_archive
    local src_file
    local dst_file

    log "Copying presets, etc. to $DIST_DIR_BASE/$dist_dir"

    cp -v "$README_HTML" "$DIST_DIR_BASE/$dist_dir/"
    cp -v -r presets "$DIST_DIR_BASE/$dist_dir/"

    if [[ ! -z "$extra_file" ]]
    then
        cp -v "$extra_file" "$DIST_DIR_BASE/$dist_dir/"
    fi

    if [[ "$convert_newlines" = "convert" ]]
    then
        for src_file in $TEXT_FILES
        do
            dst_file="$DIST_DIR_BASE/$dist_dir/$src_file"
            convert_text_file "$src_file" "$dst_file"
        done
    else
        cp -v $TEXT_FILES "$DIST_DIR_BASE/$dist_dir/"
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

build_vst3_single_zip()
{
    local dev_os="$1"
    local target_platform="$2"
    local instruction_set="$3"
    local version_str="$4"
    local version_dot="$5"
    local version_int="$6"
    local version_as_file_name="$7"
    local dist_dir

    dist_dir=$(
        get_dist_dir \
            "$dev_os" "$target_platform" "$instruction_set" \
            "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
            "show_vst3_dir"
    )

    if [[ "$target_platform" =~ "-mingw32" ]]
    then
        finalize_zip "$dist_dir" "convert" ""
    else
        finalize_zip "$dist_dir" "" ""
    fi
}

build_vst3_bundle_zip()
{
    local version_as_file_name="$1"
    local instruction_set="$2"
    local dist_dir="js80p-$version_as_file_name-$instruction_set-vst3_bundle"
    local vst3_base_dir="$DIST_DIR_BASE/$dist_dir"
    local proc_id
    local res_dir="$vst3_base_dir/js80p.vst3/Contents/Resources"
    local doc_dir="$res_dir/Documentation"
    local plugin

    proc_id="$(get_vst3_snapshot_id)"

    mkdir -v -p "$res_dir/Snapshots"
    mkdir -v -p "$doc_dir"

    cp -v "js80p.png" "$res_dir/Snapshots/${proc_id}_snapshot.png"

    cp -v "$README_HTML" "$doc_dir/README.html"

    case "$instruction_set" in
        "sse2")
            copy_vst3 "$version_as_file_name" "linux-x86-sse2" "$vst3_base_dir" "i386-linux" "js80p.so" ""
            copy_vst3 "$version_as_file_name" "linux-x86-sse2" "$vst3_base_dir" "i686-linux" "js80p.so" ""
            copy_vst3 "$version_as_file_name" "windows-x86-sse2" "$vst3_base_dir" "x86-win" "js80p.vst3" ""
            copy_vst3 "$version_as_file_name" "linux-x86_64-sse2" "$vst3_base_dir" "x86_64-linux" "js80p.so" \
                "x86_64-gpp-x86_64-avx"
            copy_vst3 "$version_as_file_name" "windows-x86_64-sse2" "$vst3_base_dir" "x86_64-win" "js80p.vst3" ""
            ;;
        "avx")
            copy_vst3 "$version_as_file_name" "linux-x86_64-avx" "$vst3_base_dir" "x86_64-linux" "js80p.so" \
                "x86_64-gpp-x86_64-avx"
            copy_vst3 "$version_as_file_name" "windows-x86_64-avx" "$vst3_base_dir" "x86_64-win" "js80p.vst3" ""
            ;;
        "lsx")
            copy_vst3 "$version_as_file_name" "linux-loongarch64-lsx" "$vst3_base_dir" "loongarch64-linux" "js80p.so" \
                "loongarch64-gpp-loongarch64-lsx"
            ;;

        "none")
            copy_vst3 "$version_as_file_name" "linux-riscv64-none" "$vst3_base_dir" "riscv64-linux" "js80p.so" \
                "riscv64-gpp-riscv64-none"
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

    finalize_zip "$dist_dir" "convert" ""
}

get_vst3_snapshot_id()
{
    grep -A2 'Vst3Plugin::Processor::ID(' src/plugin/vst3/plugin.cpp \
        | grep -o '0x[0-9a-fx, ]*' \
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
    local module_info_tool_dir="$6"
    local module_dir="$base_dir/js80p.vst3"
    local contents_dir="$module_dir/Contents"
    local res_dir="$contents_dir/Resources"
    local out_dir="$contents_dir/$dst_dir"

    mkdir -v -p "$res_dir" "$out_dir"
    cp -v \
        "$DIST_DIR_BASE/js80p-$version_as_file_name-$src_dir-vst3_single/js80p.vst3" \
        "$out_dir/$dst_file"

    if [[ "$module_info_tool_dir" != "" ]]
    then
        ./build/"$module_info_tool_dir"/vst3_module_info_tool \
            "$module_dir" > "$res_dir/moduleinfo.json"
    fi
}

build_packages_on_macos()
{
    local version_str="$1"
    local version_dot="$2"
    local version_int="$3"
    local version_as_file_name="$4"
    local fst_arm64_dir
    local fst_x86_64_dir
    local fst_universal_bundle
    local vst3_arm64_dir
    local vst3_x86_64_dir
    local vst3_universal_bundle
    local vst3_res_dir
    local vst3_proc_id

    fst_universal_bundle="$DIST_DIR_BASE/js80p-$version_as_file_name-macos-universal-fst_bundle"
    vst3_universal_bundle="$DIST_DIR_BASE/js80p-$version_as_file_name-macos-universal-vst3_bundle"

    log "Cleaning up"

    rm -vrf "$fst_universal_bundle"
    rm -vrf "$vst3_universal_bundle"
    rm -vf "$fst_universal_bundle.dmg"
    rm -vf "$vst3_universal_bundle.dmg"

    SUFFIX=arm64 call_make \
        "macos" "gpp" "none" \
        "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
        clean

    SUFFIX=x86_64 call_make \
        "macos" "gpp" "none" \
        "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
        clean

    log "Compiling"

    SUFFIX=arm64 \
        CPP_DEV_PLATFORM="/usr/bin/g++ -target arm64-apple-macos11 -mcpu=apple-m1" \
        call_make \
            "macos" "gpp" "none" \
            "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
            all

    SUFFIX=arm64 \
        CPP_DEV_PLATFORM="/usr/bin/g++ -target arm64-apple-macos11 -mcpu=apple-m1" \
        call_make \
            "macos" "gpp" "none" \
            "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
            vst3moduleinfo

    SUFFIX=x86_64 \
        CPP_DEV_PLATFORM="/usr/bin/g++ -target x86_64-apple-macos10.12 -mavx" \
        call_make \
            "macos" "gpp" "none" \
            "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
            all

    fst_arm64_dir=$(
        SUFFIX=arm64 \
            get_dist_dir \
                "macos" "gpp" "none" \
                "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
                "show_fst_dir"
    )
    fst_x86_64_dir=$(
        SUFFIX=x86_64 \
            get_dist_dir \
                "macos" "gpp" "none" \
                "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
                "show_fst_dir"
    )

    vst3_arm64_dir=$(
        SUFFIX=arm64 \
            get_dist_dir \
                "macos" "gpp" "none" \
                "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
                "show_vst3_dir"
    )
    vst3_x86_64_dir=$(
        SUFFIX=x86_64 \
            get_dist_dir \
                "macos" "gpp" "none" \
                "$version_str" "$version_dot" "$version_int" "$version_as_file_name" \
                "show_vst3_dir"
    )

    log "Building FST bundle"

    cp -v "dist/js80p.vstxml" "$DIST_DIR_BASE/$fst_arm64_dir/js80p.vst/Contents/Resources"

    build_macos_universal_bundle \
        "$version_dot" \
        "$DIST_DIR_BASE/$fst_arm64_dir/js80p.vst" \
        "$DIST_DIR_BASE/$fst_arm64_dir/js80p.vst/Contents/MacOS/js80p" \
        "$DIST_DIR_BASE/$fst_x86_64_dir/js80p.vst/Contents/MacOS/js80p" \
        "$fst_universal_bundle/js80p.vst" \
        "$fst_universal_bundle" \
        "/Library/Audio/Plug-Ins/VST" \
        "io.github.attilammagyar.JS80P.vst"

    log "Building VST 3 bundle"

    vst3_res_dir="$DIST_DIR_BASE/$vst3_arm64_dir/js80p.vst3/Contents/Resources"

    ./build/gpp-arm64-none/vst3_module_info_tool \
        "$DIST_DIR_BASE/$vst3_arm64_dir/js80p.vst3" > "$vst3_res_dir/moduleinfo.json"

    vst3_proc_id="$(get_vst3_snapshot_id)"

    mkdir -v "$vst3_res_dir/Snapshots"
    cp -v "js80p.png" "$vst3_res_dir/Snapshots/${vst3_proc_id}_snapshot.png"

    build_macos_universal_bundle \
        "$version_dot" \
        "$DIST_DIR_BASE/$vst3_arm64_dir/js80p.vst3" \
        "$DIST_DIR_BASE/$vst3_arm64_dir/js80p.vst3/Contents/MacOS/js80p" \
        "$DIST_DIR_BASE/$vst3_x86_64_dir/js80p.vst3/Contents/MacOS/js80p" \
        "$vst3_universal_bundle/js80p.vst3" \
        "$vst3_universal_bundle" \
        "/Library/Audio/Plug-Ins/VST3" \
        "io.github.attilammagyar.JS80P.vst3"
}

build_macos_universal_bundle()
{
    local version_dot="$1"
    local template_dir="$2"
    local bin_arm64="$3"
    local bin_x86_64="$4"
    local bundle_dir="$5"
    local package_base="$6"
    local install_location="$7"
    local identifier="$8"
    local bin_univ="$bundle_dir/Contents/MacOS/js80p"

    rm -rvf "$bundle_dir"
    rm -rvf "$package_base"

    mkdir -v -p "$bundle_dir/Contents/MacOS"
    mkdir -v -p "$bundle_dir/Contents/Resources/Documentation"

    cp -v "$template_dir/Contents/Info.plist" "$bundle_dir/Contents/"
    cp -v -r "$template_dir/Contents/Resources"/* "$bundle_dir/Contents/Resources/"
    cp -v -r presets "$bundle_dir/Contents/Resources/"
    cp -v "$README_HTML" "$bundle_dir/Contents/Resources/Documentation/"
    cp -v $TEXT_FILES "$bundle_dir/Contents/Resources/Documentation/"

    echo "BNDL????" >"$bundle_dir/Contents/PkgInfo"

    lipo -create -output "$bin_univ" "$bin_x86_64" "$bin_arm64"
    codesign --options=runtime --deep --timestamp --sign "$SIGNAPP" "$bin_univ"
    ditto -c -k --keepParent "$bundle_dir" "$bundle_dir.zip"
    xcrun notarytool submit "$bundle_dir.zip" --keychain-profile "$KEYCHAIN" --wait
    xcrun stapler staple "$bundle_dir"

    pkgbuild \
        --identifier "$identifier" \
        --version "$version_dot" \
        --component "$bundle_dir" \
        --install-location "$install_location" \
        "$package_base-component.pkg"

    productbuild \
        --package "$package_base-component.pkg" \
        "$package_base.pkg"

    productsign \
        --timestamp \
        --sign "$SIGNINST" \
        "$package_base.pkg" \
        "$package_base-signed.pkg"

    xcrun notarytool submit "$package_base-signed.pkg" --keychain-profile "$KEYCHAIN" --wait
    xcrun stapler staple "$package_base-signed.pkg"

    rm -vf "$package_base.pkg"
    rm -vf "$package_base-component.pkg"
}

main "$@"
