###############################################################################
# This file is part of JS80P, a synthesizer plugin.
# Copyright (C) 2023  Patrik Ehringer
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

#Requires -Version 5.1
Param (
    [Switch] $B32
    ,
    [Switch] $Linux
    ,
    [Switch] $BTests
    ,
    [Switch] $BDocs
)
Set-StrictMode -Version:Latest

if ($Linux) {
    $Env:DEV_OS = 'linux'
} else {
    $Env:DEV_OS = 'windows'
}

if ($B32) {
    $Env:ROOT_DIR = 'C:/mingw/32'
    $Env:TARGET_PLATFORM = 'i686-w64-mingw32'
} else {
    $Env:ROOT_DIR = 'C:/mingw/64'
    $Env:TARGET_PLATFORM = 'x86_64-w64-mingw32'
}

<#
cd "%0"\..
#>

if ($BTests) {
    & $Env:ROOT_DIR/bin/mingw32-make.exe check
    if (!$?) {
        exit 1
    }
}

& $Env:ROOT_DIR/bin/mingw32-make.exe all
if (!$?) {
    exit 1
}
if ($BDocs) {
    & $Env:ROOT_DIR/bin/mingw32-make.exe docs
    if (!$?) {
        exit 1
    }
}
exit 0
