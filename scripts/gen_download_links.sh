#!/bin/bash

main()
{
    local arch
    local os
    local plugin_type
    local size
    local uri
    local version
    local zip

    find dist -name "js80p-*-*bit-*.zip" \
      | sort \
      | while read
        do
            zip="$(basename "$REPLY")"
            version="v$(printf "%s\n" "$zip" | cut -d"-" -f2 | sed "s/_/./g")"
            uri="https://github.com/attilammagyar/js80p/releases/download/$version/$zip"
            os="$(printf "%s\n" "$zip" | cut -d"-" -f3 | uppercase_first)"
            arch="$(printf "%s\n" "$zip" | cut -d"-" -f4 | cut -c1-2)"
            plugin_type="$(printf "%s\n" "$zip" | cut -d"-" -f5 | cut -d"." -f1 | sed "s/fst/FST (VST 2.4)/ ; s/vst3/VST 3/")"
            size="$(du -hs "$REPLY" | cut -f1)"
            cat <<HTML
          <li>
            $os, $arch-bit, $plugin_type: <a class="button" href="$uri">Download ($size)</a>
          </li>
HTML
        done

    find dist -name "js80p-*-src.zip" \
      | while read
        do
            zip="$(basename "$REPLY")"
            version="v$(printf "%s\n" "$zip" | cut -d"-" -f2 | sed "s/_/./g")"
            uri="https://github.com/attilammagyar/js80p/releases/download/$version/$zip"
            size="$(du -hs "$REPLY" | cut -f1)"
            cat <<HTML
          <li>
            Source: <a class="button" href="$uri">Download ($size)</a>
          </li>
HTML
        done
}

uppercase_first()
{
    read

    printf "%s%s\n" \
        "$(printf "%s\n" "${REPLY:0:1}" | tr [[:lower:]] [[:upper:]])" \
        "${REPLY:1}"
}

main "$@"
