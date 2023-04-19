#!/bin/bash

# set -x
set -e
set -u
set -o pipefail

PLUGIN_TYPES="fst vst3"
ARCHITECTURES="32 64"
WINE_DIR=~/.wine/drive_c
TEST_DIR="$WINE_DIR"/test

main()
{
    local self="$1"
    local src_dir
    local plugin_type
    local architecture

    src_dir="$(dirname "$self")"

    for plugin_type in $PLUGIN_TYPES
    do
        for architecture in $ARCHITECTURES
        do
            run_test "$src_dir" "$plugin_type" "$architecture"
        done
    done

    return 0
}

run_test()
{
    local src_dir="$1"
    local plugin_type="$2"
    local architecture="$3"
    local reaper_exe="$WINE_DIR/reaper$architecture/reaper.exe"
    local rendered_wav="$TEST_DIR/test.wav"
    local msg_info="plugin_type='$plugin_type', architecture='$architecture', rendered_wav='$rendered_wav'"
    local stat
    local min
    local max

    echo "Running test; $msg_info" >&2

    rm -f "$TEST_DIR"/{test.{rpp,wav},js80p.{dll,vst3}}
    cp "$src_dir/smoke-test-"$plugin_type".rpp" "$TEST_DIR/test.rpp"
    cp \
        "$src_dir/../.."/dist/js80p-*_?_?-windows-"$architecture"bit-"$plugin_type"/js80p.* \
        "$TEST_DIR/"

    if [[ ! -f "$reaper_exe" ]]
    then
        echo "ERROR: expected $reaper_exe to be" >&2
        echo "       a $architecture bit portable installation of Reaper that is configured" >&2
        echo "       to look for VST plugins in the 'C:\\test' directory." >&2
        return 1
    fi

    wine "$reaper_exe" -renderproject "C:\\test\\test.rpp"
    stat=$(
        sox -m \
            -v 1.0 "$rendered_wav" \
            -v -1.0 "$src_dir/smoke-test.flac" \
            -n stat 2>&1
    )
    min=$(
        printf "%s\n" "$stat" \
            | grep "Minimum *amplitude" | cut -d":" -f2 | tr -d " "
    )
    max=$(
        printf "%s\n" "$stat" \
            | grep "Maximum *amplitude" | cut -d":" -f2 | tr -d " "
    )
    [[ "$max" =~ ^-*0\.000 ]] || fail "$msg_info" "$stat"
    [[ "$min" =~ ^-*0\.000 ]] || fail "$msg_info" "$stat"

    echo "PASS; $msg_info" >&2
}

fail()
{
    local msg_info="$1"
    local stat="$2"

    printf "FAIL: unexpected rendered sound; plugin_type='%s', architecture='%s', rendered_wav='%s'\n\nSoX stat for difference:\n\n%s\n" \
        "$plugin_type" "$architecture" "$rendered_wav" "$stat" >&2

    exit 1
}

main "$0" "$@"
