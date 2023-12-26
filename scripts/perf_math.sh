#!/bin/bash

###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023  Attila M. Magyar
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
