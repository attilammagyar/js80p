#!/bin/bash

# set -x
set -e
set -u
set -o pipefail


WINE_DIR=~/.wine/drive_c
WINE_TEST_DIR="$WINE_DIR"/test

LINUX_TEST_DIR=/tmp/js80p-smoke-test

# Some Reaper versions (e.g. 7.14) seem to send MIDI CC and parameter change
# events in varying order and with slight timing differences in some cases.
# There's a point in the test project where this can cause a short, slight
# difference in the start of a note in the monophonic synth track every once
# in a while, so we need to retry once or twice before concluding that it's
# the plugin's fault that the reference rendering could not be reproduced.
TRIES=5


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

    return 0
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
        echo "ERROR: expected $reaper_exe to be" >&2
        echo "       a(n) $architecture portable installation of Reaper that is configured" >&2
        echo "       to look for VST plugins in the 'C:\\test' directory," >&2
        echo "       and to render with a buffer size of 1024 samples." >&2
        return 1
    fi

    for ((try=0;try<$TRIES;++try))
    do
        sleep 1

        wine "$reaper_exe" -renderproject "C:\\test\\test.rpp" 2>&1 | tee /tmp/"wine-$architecture-$plugin_type-$try.txt"

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
        echo "ERROR: expected $reaper_exe to be" >&2
        echo "       a(n) $architecture portable installation of Reaper that is configured" >&2
        echo "       to look for VST plugins in the '$LINUX_TEST_DIR' directory," >&2
        echo "       and to render with a buffer size of 1024 samples." >&2
        return 1
    fi

    for ((try=0;try<$TRIES;++try))
    do
        sleep 1

        "$reaper_exe" -renderproject "$LINUX_TEST_DIR/test.rpp" 2>&1 | tee /tmp/"linux-$architecture-$plugin_type-$try.txt"

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
    local expected_flac="$2/smoke-test.flac"
    local rendered_wav="$3"
    local msg_info="$4"
    local fail=1
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

    if [[ "$max" =~ ^-*0\.000 ]] && [[ "$min" =~ ^-*0\.000 ]]
    then
        echo "PASS (try $try); $msg_info" >&2

        return 0
    fi

    cat >&2 <<ERROR
FAIL (try $try): unexpected rendered sound; rendered_wav='$rendered_wav', $msg_info

SoX stat for difference:

$stat

(NOTE: set Reaper's rendering buffer size to 1024 samples.)
ERROR

    return 1
}


main "$0" "$@"
