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

$Env:ROOT_DIR = 'C:/mingw64'

if ($Linux) {
    $Env:DEV_OS = 'linux'
} else {
    $Env:DEV_OS = 'windows'
}

if ($B32) {
    #$Env:ROOT_DIR = 'C:/mingw32'
    $Env:TARGET_PLATFORM = 'i686-w64-mingw32'
} else {
    $Env:TARGET_PLATFORM = 'x86_64-w64-mingw32'
}

<#
cd "%0"\..
#>

if ($BTests) {
    mingw32-make.exe check
    if (!$?) {
        exit 1
    }
}
mingw32-make.exe all
if (!$?) {
    exit 1
}
if ($BDocs) {
    mingw32-make.exe docs
    if (!$?) {
        exit 1
    }
}
exit 0