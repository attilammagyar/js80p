#!/bin/bash

# set -x
set -e
set -u
set -o pipefail

TARGETS="i686-w64-mingw32 x86_64-w64-mingw32"
PLUGIN_TYPES="fst vst3"

main()
{
    local version_tag
    local version
    local fversion
    local target
    local source_dir
    local source_archive
    local dist_dir
    local dist_archive
    local uncommitted

    log "Checking repository"

    if [[ ! -d src ]] || [[ ! -d presets ]]
    then
        error "This script must be run in the root directory of the repository."
    fi

    version_tag="$(git tag --points-at HEAD | grep -E "^v[0-9]+\\.[0-9]+$" || true)"

    if [[ "$version_tag" = "" ]]
    then
        version_tag="vdev"
        log "WARNING: No version tag (format: \"v123.456\") found on git HEAD, creating dev release."
    fi

    if [[ "$version_tag" != "vdev" ]]
    then
        uncommitted="$(git status --porcelain | wc -l)"

        if [[ $uncommitted -ne 0 ]]
        then
            error "Commit your changes first."
        fi
    fi

    version="${version_tag:1}"
    fversion="$(printf "%s\n" "$version" | sed "s/[^a-z0-9]/_/g")"
    source_dir="js80p-$fversion-src"
    source_archive="$source_dir.zip"

    log "Cleaning up"

    cleanup "dist"
    cleanup "dist/$source_dir"

    log "Copying source"

    mkdir -p "dist/$source_dir"
    cp --verbose --recursive \
        build.bat \
        Doxyfile \
        gui \
        js80p.png \
        lib \
        LICENSE.txt \
        make \
        Makefile \
        pm-fm-equivalence.md \
        presets \
        README.md \
        README.txt \
        scripts \
        src \
        tests \
        "dist/$source_dir/"

    find "dist/$source_dir/" -name ".*.swp" -delete

    log "Creating source archive"

    cd dist
    zip --recurse-paths -9 "$source_archive" "$source_dir"
    cd ..

    log "Running unit tests"

    make check

    for target in $TARGETS
    do
        log "Building for target: $target"

        TARGET_PLATFORM="$target" make clean
        TARGET_PLATFORM="$target" VERSION="$version" FVERSION="$fversion" make

        for plugin_type in $PLUGIN_TYPES
        do
            dist_dir="$(TARGET_PLATFORM="$target" VERSION="$version" FVERSION="$fversion" make show_dist_dir_prefix)-$plugin_type"
            dist_dir="$(basename "$dist_dir")"
            dist_archive="$dist_dir.zip"

            log "Copying presets, etc. to dist/$dist_dir"

            cp --verbose --recursive presets LICENSE.txt README.txt "dist/$dist_dir/"

            log "Creating release archive: dist/$dist_archive"

            cd dist
            zip --recurse-paths -9 "$dist_archive" "$dist_dir"
            cd ..
        done
    done

    log "Done"
}

error()
{
    local message_tpl="$1"

    shift

    printf "ERROR: $message_tpl\n" "$@" >&2

    exit 1
}

log()
{
    local message_tpl="$1"

    shift

    printf "### %s\t$message_tpl\n" "$(date)" "$@" >&2
}

cleanup()
{
    local name="$1"

    if [[ "$name" = "" ]] || [[ "${name:0:1}" = "/" ]]
    then
        error "Invalid argument for cleanup: %s" "$name"
    else
        rm -rf -- "$name"
    fi
}

main "$@"
