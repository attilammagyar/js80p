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
