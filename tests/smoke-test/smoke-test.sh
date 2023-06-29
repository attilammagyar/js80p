#!/bin/bash

# set -x
set -e
set -u
set -o pipefail


WINE_DIR=~/.wine/drive_c
WINE_TEST_DIR="$WINE_DIR"/test

LINUX_TEST_DIR=/tmp/js80p-smoke-test


main()
{
    local self="$1"
    local src_dir
    local plugin_type
    local suffix
    local architecture
    local info

    src_dir="$(dirname "$self")"

    for plugin_type in "fst" "vst3_single_file"
    do
        if [[ $plugin_type =~ _ ]]
        then
            suffix="_$(printf "%s" "$plugin_type" | cut -d"_" -f2-)"
            plugin_type="$(printf "%s" "$plugin_type" | cut -d"_" -f1)"
        else
            suffix=""
        fi

        for architecture in "32" "64"
        do
            info="plugin_type='$plugin_type', architecture='$architecture'"
            run_test_wine "$src_dir" "$plugin_type" "$suffix" "$architecture" "platform='wine', $info"
            run_test_linux "$src_dir" "$plugin_type" "$suffix" "$architecture" "platform='linux', $info"
        done
    done

    return 0
}


run_test_wine()
{
    local src_dir="$1"
    local plugin_type="$2"
    local suffix="$3"
    local architecture="$4"
    local msg_info="$5"
    local reaper_exe="$WINE_DIR/reaper$architecture/reaper.exe"
    local rendered_wav="$WINE_TEST_DIR/test.wav"

    echo "Running test; $msg_info" >&2

    rm -vrf "$WINE_TEST_DIR"/{test.{rpp,wav},js80p.{dll,vst3}}
    cp -v "$src_dir/smoke-test-"$plugin_type".rpp" "$WINE_TEST_DIR/test.rpp"
    cp -vr \
        "$src_dir/../.."/dist/js80p-*-windows-"$architecture"bit-"$plugin_type$suffix"/js80p.* \
        "$WINE_TEST_DIR/"

    if [[ ! -f "$reaper_exe" ]]
    then
        echo "ERROR: expected $reaper_exe to be" >&2
        echo "       a $architecture bit portable installation of Reaper that is configured" >&2
        echo "       to look for VST plugins in the 'C:\\test' directory," >&2
        echo "       and to render with a buffer size of 1024 samples." >&2
        return 1
    fi

    wine "$reaper_exe" -renderproject "C:\\test\\test.rpp" 2>&1 | tee /tmp/"wine-$architecture-$plugin_type.txt"
    verify_rendered_file "$src_dir" "$rendered_wav"
}


run_test_linux()
{
    local src_dir="$1"
    local plugin_type="$2"
    local suffix="$3"
    local architecture="$4"
    local msg_info="$5"
    local reaper_exe=~/"reaper$architecture/reaper"
    local rendered_wav="$LINUX_TEST_DIR/test.wav"

    echo "Running test; $msg_info" >&2

    rm -vrf "$LINUX_TEST_DIR"/{test.{rpp,wav},js80p.{so,vst3}}
    mkdir -v -p "$LINUX_TEST_DIR"
    cp -v "$src_dir/smoke-test-"$plugin_type".rpp" "$LINUX_TEST_DIR/test.rpp"
    sed -i \
        "s|RENDER_FILE \"[^\"]*\"|RENDER_FILE \"$LINUX_TEST_DIR/test.wav\"|" \
        "$LINUX_TEST_DIR/test.rpp"
    cp -vr \
        "$src_dir/../.."/dist/js80p-*-linux-"$architecture"bit-"$plugin_type$suffix"/js80p.* \
        "$LINUX_TEST_DIR/"

    if [[ ! -x "$reaper_exe" ]]
    then
        echo "ERROR: expected $reaper_exe to be" >&2
        echo "       a $architecture bit portable installation of Reaper that is configured" >&2
        echo "       to look for VST plugins in the '$LINUX_TEST_DIR' directory," >&2
        echo "       and to render with a buffer size of 1024 samples." >&2
        return 1
    fi

    "$reaper_exe" -renderproject "$LINUX_TEST_DIR/test.rpp" 2>&1 | tee /tmp/"linux-$architecture-$plugin_type.txt"
    verify_rendered_file "$src_dir" "$rendered_wav"
}


verify_rendered_file()
{
    local expected_flac="$1/smoke-test.flac"
    local rendered_wav="$2"
    local stat
    local min
    local max

    stat=$(
        sox -m \
            -v 1.0 "$rendered_wav" \
            -v -1.0 "$expected_flac" \
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
    [[ "$max" =~ ^-*0\.000 ]] || fail "rendered_wav='$rendered_wav', $msg_info" "$stat"
    [[ "$min" =~ ^-*0\.000 ]] || fail "rendered_wav='$rendered_wav', $msg_info" "$stat"

    echo "PASS; $msg_info" >&2
}


fail()
{
    local msg_info="$1"
    local stat="$2"

    cat <<ERROR
FAIL: unexpected rendered sound; $msg_info

SoX stat for difference:

$stat

(NOTE: set Reaper's rendering buffer size to 1024 samples.)
ERROR

    exit 1
}


main "$0" "$@"
