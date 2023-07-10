#!/bin/bash

DOWNLOAD_URL="https://github.com/attilammagyar/js80p/releases/download"

main()
{
    local arch
    local os
    local plugin_type
    local size
    local uri
    local version
    local zip

    find dist -name "js80p-*-vst3_bundle.zip" \
      | while read
        do
            zip="$(basename "$REPLY")"
            version="$(zip_to_version "$zip")"
            uri="$DOWNLOAD_URL/v$version/$zip"
            size="$(get_size "$REPLY")"
            cat <<HTML
          <li>
            VST 3 Bundle: <a href="$uri">Download ($size)</a>
          </li>
HTML
        done

    find dist -name "js80p-*.zip" \
      | grep -v "js80p-.*-src\\.zip" \
      | grep -v "js80p-.*-vst3_bundle\\.zip" \
      | sort \
      | while read
        do
            zip="$(basename "$REPLY")"
            version="$(zip_to_version "$zip")"
            uri="$DOWNLOAD_URL/v$version/$zip"
            os="$(zip_to_os_name "$zip")"
            arch="$(zip_to_arch_name "$zip")"
            plugin_type="$(zip_to_plugin_type "$zip")"
            size="$(get_size "$REPLY")"
            cat <<HTML
          <li>
            $os, $arch-bit, $plugin_type: <a href="$uri">Download ($size)</a>
          </li>
HTML
        done

    find dist -name "js80p-*-src.zip" \
      | while read
        do
            zip="$(basename "$REPLY")"
            version="$(zip_to_version "$zip")"
            uri="$DOWNLOAD_URL/v$version/$zip"
            size="$(get_size "$REPLY")"
            cat <<HTML
          <li>
            Source: <a href="$uri">Download ($size)</a>
          </li>
HTML
        done
}

zip_to_version()
{
    local zip="$1"

    printf "v%s\n" "$zip" \
        | cut -d"-" -f2 \
        | sed "s/_/./g"
}

zip_to_os_name()
{
    local zip="$1"

    printf "%s\n" "$zip" \
        | cut -d"-" -f3 \
        | uppercase_first
}

uppercase_first()
{
    local first
    local rest

    read

    first="$(printf "%s\n" "${REPLY:0:1}" | tr [[:lower:]] [[:upper:]])"
    rest="${REPLY:1}"

    printf "%s%s\n" "$first" "$rest"
}

zip_to_arch_name()
{
    local zip="$1"

    printf "%s\n" "$zip" \
        | cut -d"-" -f4 \
        | cut -c1-2
}

zip_to_plugin_type()
{
    local zip="$1"

    printf "%s\n" "$zip" \
        | cut -d"-" -f5 \
        | cut -d"." -f1 \
        | sed "s/fst/FST (VST 2.4)/ ; s/vst3_single_file/VST 3 Single File/"
}

get_size()
{
    local file_name="$1"

    du -hs "$file_name" \
        | cut -f1
}

main "$@"
