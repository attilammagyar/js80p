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

    executable=./build/"$platform"/perf_math

    if [[ ! -x "$executable" ]]
    then
        echo "Unable to find $executable, run \"make perf\" first" >&2
        echo "or pass a platform name in the first argument." >&2
        return 1
    fi

    "$executable" 2>&1 \
        | grep '(' \
        | tr -d ' ' \
        | while read
          do
              echo ""

              for i in 1 2 3
              do
                  (
                    time ./build/"$platform"/perf_math "$REPLY" 500000000
                  ) 2>&1 \
                    | cut -f2 \
                    | sed 's/^0m// ; s/s$//' \
                    | (
                          printf "$REPLY\t"

                          while read
                          do
                              printf "%s\t" "$REPLY"
                          done

                          printf "\n"
                      )
              done
          done
}

main "$@"
