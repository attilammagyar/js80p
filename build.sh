#!/bin/bash

set -e

(
    export DEV_OS=linux
    export TARGET_PLATFORM=x86_64-gpp
    # export TARGET_PLATFORM=i686-gpp

    make check
    make all
    make docs
)
