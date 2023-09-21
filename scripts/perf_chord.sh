#!/bin/bash

set -e

main()
{
    local platform="$1"
    local executable

    if [[ "$platform" = "" ]]
    then
        platform="x86_64-gpp-avx"
    fi

    executable=./build/"$platform"/chord

    if [[ ! -x "$executable" ]]
    then
        echo "Unable to find $executable, run \"make perf\" first" >&2
        echo "or pass a platform name in the first argument." >&2
        return 1
    fi

    run_tests "$executable" 23 80
    run_tests "$executable" 26 10
}

run_tests()
{
    local executable="$1"
    local program="$2"
    local velocity="$3"

    for ((i=0;i!=10;++i))
    do
        run_test "$executable" "$i" "$program" "$velocity"
    done
}

run_test()
{
    local executable="$1"
    local iter="$2"
    local program="$3"
    local velocity="$4"

    (
        time "$executable" \
            "$program" "$velocity" "/tmp/chords-$program-$velocity.wav"
    ) 2>&1 \
      | cut -f2 \
      | sed 's/^0m// ; s/s$//' \
      | (
            printf "test-$iter-$program-$velocity\t"

            while read
            do
                printf "%s\t" "$REPLY"
            done

            printf "\n"
        )
}

main "$@"
