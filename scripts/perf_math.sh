#!/bin/bash

set -e

main()
{
    platform="$1"

    if [[ "$platform" = "" ]]
    then
        platform="x86_64-w64-mingw32"
    fi

    ./build/"$platform"/perf_math 2>&1 \
        | grep '(' \
        | tr -d ' ' \
        | while read
          do echo "###" >&2
              for i in 1 2 3
              do
                  echo >&2
                  time ./build/"$platform"/perf_math "$REPLY" 100000000
              done
          done 2>&1
}

main "$@"
