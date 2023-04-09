#!/bin/bash
# -------------------------------------------------------------------
# Usage
# -------------------------------------------------------------------
function usage()
{
cat <<- _EOF_
${bold}Usage:${normal}
  ${0##*/} \\
  [--help | -h] [--simulate | -s] [--install | -i] [--monocrome | -m]

  If no command option (-s, -i) is given, an interactive version is started.

${bold}Description:${normal}
  Installs required packages for the VST 3 SDK.

${bold}Options:${normal}
  -h, --help        Show this help
  -s, --simulate    Simulate; Do not actually change the system
  -i, --install     Install required packages
  -m, --monocrome   Do not use colored output 

_EOF_
}
# -------------------------------------------------------------------
# Init Colors if we can print colors
# -------------------------------------------------------------------
function initColors()
{
    if [ "$MONOCHROME" == "false" ]; then 
        if test -t 1; then
            ncolors=$(tput colors)

            if test -n "$ncolors" && test $ncolors -ge 8; then
                bold="$(tput bold)"
                underline="$(tput smul)"
                standout="$(tput smso)"
                normal="$(tput sgr0)"
                black="$(tput setaf 0)"
                red="$(tput setaf 1)"
                green="$(tput setaf 2)"
                yellow="$(tput setaf 3)"
                blue="$(tput setaf 4)"
                magenta="$(tput setaf 5)"
                cyan="$(tput setaf 6)"
                white="$(tput setaf 7)"
            fi
        fi
    fi
}
# -------------------------------------------------------------------
# Process the command-line arguments
# -------------------------------------------------------------------
function parceArguments()
{
    COMMAND=""
    MONOCHROME="false"
    args=( )
    # replace long arguments
    for arg; do
        case "$arg" in
            --help)      args+=( -h ) ;;
            --simulate)  args+=( -s ) ;;
            --install)   args+=( -i ) ;;
            --monocrome) args+=( -m ) ;;
            *)           args+=( "$arg" ) ;;
        esac
    done
    # set "${args[@]}" as new positional parameter
    set -- "${args[@]}"
    # Process args
    while getopts "hsim" OPTION; do
        : "$OPTION" "$OPTARG"
        case $OPTION in
        h)  initColors; usage; exit 0;;
        s)  COMMAND="s";;
        i)  COMMAND="i";;
        m)  MONOCHROME="true";;
        esac
    done
    initColors
}
# -------------------------------------------------------------------
# Print note 
# -------------------------------------------------------------------
function note()
{
    echo "${blue}${bold}-----------------------------------------------${normal}"
    for var in "$@"
    do
        echo "${blue}${bold}${var}${normal}"
    done
    echo "${blue}${bold}-----------------------------------------------${normal}"
}
# -------------------------------------------------------------------
# Get os ID string
# -------------------------------------------------------------------
function osID()
{
    declare -a fileList=("/etc/os-release" " /usr/lib/os-release")
    local distName=$(uname -s)
    for f in ${fileList[@]}; do
        if test -f "${f}"; then
            distName=$(sed -n -E '/^ID\s*=\s*/{s/^ID\s*=\s*([^\n]*)/\1/;p}' "${f}")
            break
        fi
    done
    echo $distName
}
# -------------------------------------------------------------------
# Get os VERSION_ID string
# -------------------------------------------------------------------
function osVersionID()
{
    declare -a fileList=("/etc/os-release" " /usr/lib/os-release")
    local versionID=$(uname -v)
    for f in ${fileList[@]}; do
        if test -f "${f}"; then
            versionID=$(sed -n -E '/^VERSION_ID\s*=\s*/{s/^VERSION_ID\s*=\s*([^\n]*)/\1/;p}' "${f}")
            break
        fi
    done
    echo $versionID
}
# -------------------------------------------------------------------
# Install required packages for the VST 3 SDK under Linux distributions
# -------------------------------------------------------------------
function install()
{
    local notes=( ) 
    local simulate="${1}"
    if [ "${simulate}" == "-s" ]; then
        notes+=( "Simulate ..." ) 
    fi    
    notes+=( "Install required packages for the VST 3 SDK" ) 
    notes+=( "Recognized operating system: $(osID)" ) 
    note "${notes[@]}"
    
    # -------------------------------------------------------------------
    # Install all needed libs for VSTSDK
    note "Libs for VSTSDK"
    sudo apt-get install cmake git "libstdc++6" -y ${simulate}

    # -------------------------------------------------------------------
    # Install all needed libs for VSTGUI
    note "Libs for VSTGUI"

    # On Raspbian/Debian, we replace "libxcb-util-dev" with "libxcb-util0-dev"
    libxcb_util_dev="libxcb-util-dev"
    if [ "$(osID)" == "raspbian" ]; then
        libxcb_util_dev="libxcb-util0-dev"
    fi
    sudo apt-get install libx11-xcb-dev \
    ${libxcb_util_dev} \
    libxcb-cursor-dev \
    libxcb-xkb-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libfontconfig1-dev \
    libcairo2-dev \
    libgtkmm-3.0-dev \
    libsqlite3-dev \
    libxcb-keysyms1-dev \
    -y ${simulate}

    # -------------------------------------------------------------------
    # Install needed lib for Jack Audio
    note "Libs for Jack Audio"
    sudo apt-get install libjack-jackd2-dev -y ${simulate}

    # Add current user to 'audio' group. Use 'deluser' to remove user again.
    note "Add current user to 'audio' group"
    THIS_USER=${USER:-${USERNAME:-${LOGNAME}}}
    
    if [ "${simulate}" == "-s" ]; then
        echo simulate adduser ${THIS_USER} audio
    else
        sudo adduser ${THIS_USER} audio
    fi
    # -------------------------------------------------------------------
}
# -------------------------------------------------------------------
# Print the menu
# -------------------------------------------------------------------
function printMenu()
{
    note "  ${bold}${0##*/}${normal}"
cat <<- _EOF_
${blue}${bold}Description:${normal}
  This script will install required packages${normal}
  for the VST 3 SDK${normal}

${blue}${bold}Recognized operating system:${normal}
  $(osID) - Version: $(osVersionID)

${blue}${bold}Commands:${normal}
  ${underline}${bold}S${normal}imulate - Do not actually change the system
  ${underline}${bold}I${normal}nstall  - Install required packages
  ${underline}${bold}H${normal}elp     - Print usage help
  ${underline}${bold}Q${normal}uit.    - Exit; no action

_EOF_
}
# -------------------------------------------------------------------
# Interactive menu if no command option is provided
# -------------------------------------------------------------------
function menu()
{
    MENU_REPLY=""
    local inputprompt="${bold}Hit Key to execute Command:${normal}"
    if [ "$(osID)" == "Darwin" ]; then 
        MENU_REPLY="q"
        inputprompt="${bold}${red}$(osID) is not supported by this script!${normal}"
    elif [ "${COMMAND}" != "" ]; then 
        MENU_REPLY=${COMMAND}
        return
    fi
    printMenu
    printf "%s" "${inputprompt}"
    if [ "$MENU_REPLY" == "" ]; then 
        read -rsn1 MENU_REPLY
    fi
    echo ""
}
# -------------------------------------------------------------------
# Script start
# -------------------------------------------------------------------
parceArguments "$@"
menu
case "${MENU_REPLY}" in
    s) install "-s";;
    i) install;;
    h) usage;;
    q) ;;
    *) echo "${red}${bold}Invalid command: ${normal}${MENU_REPLY}";;
esac

