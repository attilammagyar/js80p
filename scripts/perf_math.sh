#!/bin/bash

set -e

main()
{
    local platform="$1"
    local executable

    if [[ "$platform" = "" ]]
    then
        platform="x86_64-w64-mingw32"
    fi

    executable=./build/"$platform"/perf_math

    if [[ ! -x "$executable" ]]
    then
        echo "Unable to find $executable, run \"make check\" first" >&2
        return 1
    fi

    "$executable" 2>&1 \
        | grep '(' \
        | tr -d ' ' \
        | while read
          do echo "###" >&2
              for i in 1 2 3
              do
                  echo >&2
                  time ./build/"$platform"/perf_math "$REPLY" 500000000
              done
          done 2>&1
}

main "$@"
