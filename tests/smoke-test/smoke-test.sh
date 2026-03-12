#!/bin/bash

###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023, 2024, 2025, 2026  Attila M. Magyar
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


WINE_DIR=~/.wine/drive_c
WINE_TEST_DIR="$WINE_DIR"/test

LINUX_TEST_DIR=/tmp/js80p-smoke-test

MACOS_TEST_DIR=/tmp/js80p-smoke-test

# Some REAPER versions (e.g. 7.14) seem to send MIDI CC and parameter change
# events in varying order and with slight timing differences in some cases.
# There's a point in the test project where this can cause a short, slight
# difference in the start of a note in the monophonic synth track every once
# in a while, so we need to retry once or twice before concluding that it's
# the plugin's fault that the reference rendering could not be reproduced.
TRIES=5


main()
{
    local self="$1"
    local platform="$2"
    local src_dir

    src_dir="$(dirname "$self")"

    case "$platform" in
        "linux") run_tests_linux "$src_dir" ;;
        "macos") run_tests_macos "$src_dir" ;;
        *) echo "Usage: $self linux|macos" >&2 ; return 1 ;;
    esac

    return 0
}


run_tests_linux()
{
    local src_dir="$1"
    local plugin_type
    local suffix
    local architecture
    local info

    for plugin_type in "fst" "vst3_single"
    do
        if [[ $plugin_type =~ _ ]]
        then
            suffix="_$(printf "%s" "$plugin_type" | cut -d"_" -f2-)"
            plugin_type="$(printf "%s" "$plugin_type" | cut -d"_" -f1)"
        else
            suffix=""
        fi

        for architecture in "x86:sse2" "x86_64:sse2" "x86_64:avx"
        do
            info="plugin_type='$plugin_type', architecture='$architecture'"

            run_test_wine \
                "$src_dir" \
                "-windows" \
                "$plugin_type" \
                "$suffix" \
                "$architecture" \
                "platform='wine', $info"

            run_test_linux \
                "$src_dir" \
                "-linux" \
                "$plugin_type" \
                "$suffix" \
                "$architecture" \
                "platform='linux', $info"
        done
    done

    info="plugin_type='vst3', architecture='x86_64:avx'"
    run_test_wine "$src_dir" "" "vst3" "_bundle" "x86_64:avx" "platform='wine', $info"
    run_test_linux "$src_dir" "" "vst3" "_bundle" "x86_64:avx" "platform='linux', $info"

    info="plugin_type='vst3', architecture='x86_64:sse2'"
    run_test_wine "$src_dir" "" "vst3" "_bundle" "x86_64:sse2" "platform='wine', $info"
    run_test_linux "$src_dir" "" "vst3" "_bundle" "x86_64:sse2" "platform='linux', $info"

    info="plugin_type='vst3', architecture='x86:sse2'"
    run_test_wine "$src_dir" "" "vst3" "_bundle" "x86:sse2" "platform='wine', $info"
    # run_test_linux "$src_dir" "" "vst3" "_bundle" "x86:sse2" "platform='linux', $info"

    # As of VST 3.7.8, loading the 32 bit plugin into a 32 bit host application
    # from a VST 3 bundle on a 64 bit Linux system is broken:
    # https://github.com/steinbergmedia/vst3sdk/issues/115
}


run_test_wine()
{
    local src_dir="$1"
    local infix="$2"
    local plugin_type="$3"
    local suffix="$4"
    local architecture="$5"
    local msg_info="$6"
    local instruction_set
    local reaper_exe
    local rendered_wav
    local try
    local ret

    instruction_set="$(printf "%s\n" "$architecture" | cut -d ":" -f 2)"
    architecture="$(printf "%s\n" "$architecture" | cut -d ":" -f 1)"
    reaper_exe="$WINE_DIR/reaper-$architecture/reaper.exe"
    rendered_wav="$WINE_TEST_DIR/test.wav"

    echo "Running test; $msg_info" >&2

    if [[ "$infix" != "" ]]
    then
        infix="$infix-${architecture}"
    fi

    rm -vrf "$WINE_TEST_DIR"/{test.{rpp,wav},js80p.{dll,vst3}}
    cp -v "$src_dir/smoke-test-"$plugin_type".rpp" "$WINE_TEST_DIR/test.rpp"
    cp -vr \
        "$src_dir/../.."/dist/js80p-*$infix-$instruction_set-"$plugin_type$suffix"/js80p.* \
        "$WINE_TEST_DIR/"

    if [[ ! -f "$reaper_exe" ]]
    then
        cat >&2 <<ERR
ERROR: expected $reaper_exe to be
       a(n) $architecture portable installation of REAPER that is configured
       to look for VST plugins in the 'C:\\test' directory,
       and to render with a buffer size of 1024 samples.
ERR
        return 1
    fi

    for ((try=0;try<$TRIES;++try))
    do
        sleep 1

        wine "$reaper_exe" -renderproject "C:\\test\\test.rpp" 2>&1 \
            | tee /tmp/"wine-$architecture-$plugin_type-$try.txt"

        set +e
        verify_rendered_file "$try" "$src_dir" "$rendered_wav" "$msg_info"
        ret=$?
        set -e

        if [[ $ret -eq 0 ]]
        then
            return 0
        fi

        sleep 2
    done

    return 1
}


run_test_linux()
{
    local src_dir="$1"
    local infix="$2"
    local plugin_type="$3"
    local suffix="$4"
    local architecture="$5"
    local msg_info="$6"
    local instruction_set
    local reaper_exe
    local rendered_wav
    local try
    local ret

    instruction_set="$(printf "%s\n" "$architecture" | cut -d ":" -f 2)"
    architecture="$(printf "%s\n" "$architecture" | cut -d ":" -f 1)"
    reaper_exe=~/programs/"reaper-$architecture/reaper"
    rendered_wav="$LINUX_TEST_DIR/test.wav"

    echo "Running test; $msg_info" >&2

    if [[ "$infix" != "" ]]
    then
        infix="$infix-${architecture}"
    fi

    rm -vrf "$LINUX_TEST_DIR"/{test.{rpp,wav},js80p.{so,vst3}}
    mkdir -v -p "$LINUX_TEST_DIR"
    cp -v "$src_dir/smoke-test-"$plugin_type".rpp" "$LINUX_TEST_DIR/test.rpp"
    sed -i \
        "s|RENDER_FILE \"[^\"]*\"|RENDER_FILE \"$LINUX_TEST_DIR/test.wav\"|" \
        "$LINUX_TEST_DIR/test.rpp"
    cp -vr \
        "$src_dir/../.."/dist/js80p-*$infix-$instruction_set-"$plugin_type$suffix"/js80p.* \
        "$LINUX_TEST_DIR/"

    if [[ ! -x "$reaper_exe" ]]
    then
        cat >&2 <<ERR
ERROR: expected $reaper_exe to be
       a(n) $architecture portable installation of REAPER that is configured
       to look for VST plugins in the '$LINUX_TEST_DIR' directory,
       and to render with a buffer size of 1024 samples.
ERR
        return 1
    fi

    for ((try=0;try<$TRIES;++try))
    do
        sleep 1

        "$reaper_exe" -renderproject "$LINUX_TEST_DIR/test.rpp" 2>&1 \
            | tee /tmp/"linux-$architecture-$plugin_type-$try.txt"

        set +e
        verify_rendered_file "$try" "$src_dir" "$rendered_wav" "$msg_info"
        ret=$?
        set -e

        if [[ $ret -eq 0 ]]
        then
            return 0
        fi

        sleep 2
    done

    return 1
}


verify_rendered_file()
{
    local try=$(($1+1))
    local expected_wav="$2/smoke-test.wav"
    local rendered_wav="$3"
    local msg_info="$4"
    local fail=1
    local diff

    diff=$(python3 "$src_dir/wavdiff.py" "$expected_wav" "$rendered_wav" 2>&1)

    if [[ "$diff" =~ ^0\.000 ]]
    then
        echo "PASS (try $try); $msg_info" >&2

        return 0
    fi

    cat >&2 <<ERROR
FAIL (try $try): unexpected rendered sound; rendered_wav='$rendered_wav', $msg_info

NOTES:
1. Use a REAPER version which is known to pass MIDI events and parameter
   automation in a fixed, deterministic order (e.g. before 7.14), and set its
   rendering buffer size to 1024 samples.
2. Make sure that none of the presets in the test use inaccuracies (oscillators,
   filters, envelope generators), since the RNG seeds are derived from platform
   dependent data.
3. Make sure that the RPP file for the FST plugin contains MIDI events in the
   exact order in which they are passed to the synth by the VST 3 plugin.
ERROR

    return 1
}


run_tests_macos()
{
    local src_dir="$1"
    local installed
    local architecture
    local plugin_type
    local rpp_suffix
    local dst_dir

    installed="$(find {,~}/Library/Audio/Plug-Ins/VST{,3} -iname "*js80p*" -depth 1)"

    if [[ "$installed" != "" ]]
    then
        cat >&2 <<ERR
Running the tests requires a clean system, but JS80P is already installed at
the following locations:

$installed

Remove these before running the test.
ERR
        return 1
    fi

    for plugin_type in "fst:fst:VST" "vst3_single:vst3:VST3"
    do
        dst_dir="$(printf "%s" "$plugin_type" | cut -d: -f3-)"
        rpp_suffix="$(printf "%s" "$plugin_type" | cut -d: -f2)"
        plugin_type="$(printf "%s" "$plugin_type" | cut -d: -f1)"

        for architecture in "x86_64" "arm64"
        do
            run_test_macos \
                "$src_dir" \
                "$plugin_type" \
                "$rpp_suffix" \
                "$architecture" \
                "$dst_dir"
        done
    done
}


run_test_macos()
{
    local src_dir="$1"
    local plugin_type="$2"
    local rpp_suffix="$3"
    local architecture="$4"
    local dst_dir=~/Library/Audio/Plug-Ins/"$5"
    local msg_info="plugin_type=$plugin_type, architecture=$architecture"
    local rpp_name

    reaper_exe="/Applications/REAPER.app/Contents/MacOS/REAPER"
    rendered_wav="$MACOS_TEST_DIR/test.wav"

    echo "Running test; $msg_info" >&2

    rm -vrf "$MACOS_TEST_DIR"/test.{rpp,wav}
    mkdir -v -p "$MACOS_TEST_DIR"
    cp -v "$src_dir/smoke-test-$rpp_suffix.rpp" "$MACOS_TEST_DIR/test.rpp"
    sed -i "" \
        "s|RENDER_FILE \"[^\"]*\"|RENDER_FILE \"$MACOS_TEST_DIR/test.wav\"|" \
        "$MACOS_TEST_DIR/test.rpp"
    cp -vr \
        "$src_dir/../../dist/"js80p-*macos-"$architecture-none-$plugin_type"/js80p.vst* \
        "$dst_dir/"

    if [[ ! -x "$reaper_exe" ]]
    then
        cat >&2 <<ERR
ERROR: expected $reaper_exe to be
       an installation of REAPER that is configured to look for VST plugins in
       the '$dst_dir' directory,
       and to render with a buffer size of 1024 samples.
ERR
        return 1
    fi

    for ((try=0;try<$TRIES;++try))
    do
        sleep 1

        arch "-$architecture" \
            "$reaper_exe" -renderproject "$MACOS_TEST_DIR/test.rpp" 2>&1 \
                | tee /tmp/"macos-$architecture-$plugin_type-$try.txt"

        set +e
        verify_rendered_file "$try" "$src_dir" "$rendered_wav" "$msg_info"
        ret=$?
        set -e

        if [[ $ret -eq 0 ]]
        then
            rm -vrf "$dst_dir/js80p.vst" "$dst_dir/js80p.vst3"
            return 0
        fi

        sleep 2
    done

    return 1
}


main "$0" "$@" ""
