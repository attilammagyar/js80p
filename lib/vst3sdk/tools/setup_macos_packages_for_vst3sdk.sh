#!/bin/bash

# -------------------------------------------------------------------
# List of all required components
# This list as to be reviewd for every VST3 SDK release
# -------------------------------------------------------------------
init_component_list(){
	#	command	min-version	required	description			version-command				display-version-command install command
	COMPONENT_LIST_COLUMNS=7
	# shellcheck disable=SC2016
	COMPONENT_LIST=(
		"macos"	"10.13"		"required"	"Operating System"	"sw_vers -productVersion"	"macos_human_string"	""
		"xcode"	"10"		"required"	"Xcode version"		"%s_version"				""						""
		"brew"	"1.3.0"		"optional"	"homebrew version"  "command_version %s -v 2"	""						'install_homebrew'
		"cmake"	"3.19"		"required"	"cmake version"		"command_version %s"		""						'install_cmake'
		"git"	"2.0"		"optional"	"git version"		"%s_version"				""						'brew install git'
	)
}
# -------------------------------------------------------------------
# Print usage
# -------------------------------------------------------------------
usage(){
	cat <<- _EOF_
	${bold}Usage:${normal}
	  ${0##*/}
	  [--help | -h] [--install | -i] [--monocrome | -m]

	  If no command option (-s, -i) is given, an interactive version is started.

	${bold}Description:${normal}
	  Installs required packages for the VST 3 SDK.

	${bold}Options:${normal}
	  -h, --help              Show this help
	  -i, --install           Skip the first menu and start the installation
	                          of required packages
	  -o, --include_optional  Install also optional packages
	  -m, --monocrome         Do not use colored output 
	_EOF_
}
# -------------------------------------------------------------------
# Process the command-line arguments
# -------------------------------------------------------------------
parceArguments() {
	COMMAND=""
	MONOCHROME="false"
	INCLUDE_OPTIONAL=1
	while [[ $# -gt 0 ]]; do
		case "$1" in
			-h|--help)              initColors; usage; exit 0;;
			-o|--include_optional)  INCLUDE_OPTIONAL=0;;
			-i|--install)           COMMAND="i";;
			-m|--monocrome)         MONOCHROME="true";;
			*)                      usage >&2; exit 1;;        
		esac
		shift
	done
	initColors
}
# -------------------------------------------------------------------
# Init Colors if we can print colors
# -------------------------------------------------------------------
initColors(){
	tick_symbol="+"
	red_exclamation_mark="!"
	warning_symbol="-"
	information_source="i"
	if [ "$MONOCHROME" == "false" ]; then 
		if test -t 1; then
			ncolors=$(tput colors)
			if test -n "$ncolors" && test "$ncolors" -ge 8; then
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
				tick_symbol=$(printf '\xE2\x9C\x85')
				red_exclamation_mark=$(printf '\xE2\x9D\x97\xEF\xB8\x8F')
				warning_symbol=$(printf '\xE2\x9A\xA0\xEF\xB8\x8F ')
				information_source=$(printf '\xE2\x84\xB9\xEF\xB8\x8F ')
				
			fi
		fi
	fi
	# to make shellcheck SC2034 happy
	echo "$bold$underline$standout$normal$black$red$green$yellow$blue$magenta$cyan$white" > /dev/null
	echo "$tick_symbol$red_exclamation_mark$warning_symbol" > /dev/null
}
# -------------------------------------------------------------------
# Print in bold
# -------------------------------------------------------------------
b(){
	for var in "$@"
	do
		printf "%s%s%s" "${bold}" "${var}" "${normal}"
	done	
}
# -------------------------------------------------------------------
# Print $1 as hotkey
# -------------------------------------------------------------------
u(){
	for var in "$@"
	do
		printf "%s%s%s" "${underline}" "${var}" "${normal}"
	done	
}
# -------------------------------------------------------------------
# Print underlined
# -------------------------------------------------------------------
h(){
	if [ "$MONOCHROME" == "false" ]; then
		printf "%s%s%s" "${underline}${bold}" "${1}" "${normal}"
	else
		printf "%s%s%s" "(" "${1}" ")"
	fi
}
# -------------------------------------------------------------------
# Print note 
# -------------------------------------------------------------------
note(){
	echo "${blue}${bold}-----------------------------------------------${normal}"
	for var in "$@"
	do
		echo "${blue}${bold}${var}${normal}"
	done
	echo "${blue}${bold}-----------------------------------------------${normal}"
}
# -------------------------------------------------------------------
# Print info 
# -------------------------------------------------------------------
info() {
	local max_width=0
	local width=0
	local prefix="${blue}${bold}-->${normal}"
	local title="$1"; shift
	for arg in "$@"; do
		IFS=':' read -ra parts <<< "$arg"
		[ "${#parts[@]}" -gt 1 ] && width=${#parts[0]}
		[ "$width" -gt "$max_width" ] && max_width=$width
	done
	((max_width+=1))
	printf "%s %s\n" "${prefix}" "${bold}${title}${normal}"
	for arg in "$@"; do
		IFS=':' read -ra parts <<< "$arg"
		if [ "${#parts[@]}" -gt 1 ]; then
			printf "%s %${max_width}s" "${prefix}" "${parts[0]}:"
			printf "%s\n" "$(IFS=":"; echo "${parts[*]:1}")"
		else
			printf "%s %${max_width}s %s\n" "${prefix}" "" "$arg"
		fi
	done
}
# -------------------------------------------------------------------
download_url_to(){
	local url="$1"
	local dest="$2"
	local min_size="$3"
	local result=0
	local file_size=0
	curl --silent --show-error --location --retry 3 "${url}" --output "${dest}" 
	result=$?
	if [ $result -eq 0 ] ; then
		file_size="$(du -k "${dest}" | cut -f1)" 
	fi
	if [ $result -ne 0 ] || [ "$file_size" -lt "$min_size" ] ; then
		return 1
	fi
	return 0
}
# -------------------------------------------------------------------
command_version(){
	local command="$1"
	local version_option="--version"
	local version_column=3
	version_option="${2:-$version_option}"
	version_column="${3:-$version_column}"

	if ! command -v "${command}" &>/dev/null; then
		printf "not installed" ; return 0
	fi
	"${command}" "${version_option}" 2>&1 | head -n 1 | cut -d' ' -f"${version_column}"
}
# -------------------------------------------------------------------
# Sanitize version strings
# -------------------------------------------------------------------
sanitize_version() {
	local version="$1"
	version="$(echo "$version" | cut -d'p' -f1)"
	if [[ -z "$version" || ! "$version" =~ ^[0-9\.]+$ ]]; then
		echo "0.0.0"
	else
		echo "$version"
	fi
}
# -------------------------------------------------------------------
# Compare version strings
# -------------------------------------------------------------------
compare_versions() {
	local version1="$1"
	local version2="$2"
	if [[ "$version1" == "$version2" ]]; then
		echo "equal"
	else
		local arr1=()
		local arr2=()
		IFS="." read -ra arr1 <<< "$version1"
		IFS="." read -ra arr2 <<< "$version2"
		for ((i = 0; i < ${#arr1[@]}; i++)); do
			if [[ "${arr1[i]}" -lt "${arr2[i]}" ]]; then
				echo "less"
				return
			elif [[ "${arr1[i]}" -gt "${arr2[i]}" ]]; then
				echo "greater"
				return
			fi
		done
		echo "equal"
	fi
}
# -------------------------------------------------------------------
# Function to compare version strings
# -------------------------------------------------------------------
test_version() {
	local version1="$1"
	local operator="$2"
	local version2="$3"

	version1=$(sanitize_version "$version1")
	version2=$(sanitize_version "$version2")

	case $operator in
		-lt) [[ $(compare_versions "$version1" "$version2") == "less" ]] ;;
		-le) [[ $(compare_versions "$version1" "$version2") == "less" || $(compare_versions "$version1" "$version2") == "equal" ]] ;;
		-gt) [[ $(compare_versions "$version1" "$version2") == "greater" ]] ;;
		-ge) [[ $(compare_versions "$version1" "$version2") == "greater" || $(compare_versions "$version1" "$version2") == "equal" ]] ;;
		-eq) [[ $(compare_versions "$version1" "$version2") == "equal" ]] ;;
		*) echo "Invalid operator: $operator"; exit 1 ;;
	esac
}
# -------------------------------------------------------------------
get_component_count() {
	echo $((${#COMPONENT_LIST[@]} / COMPONENT_LIST_COLUMNS))
}
# -------------------------------------------------------------------
get_nth_component() {
	local n="$1"
	echo "${COMPONENT_LIST[(($n*$COMPONENT_LIST_COLUMNS))]}"
}
# -------------------------------------------------------------------
get_column_for_component() {
	local command="$1"
	local column="$2"
	local index
	for ((index=0; index<${#COMPONENT_LIST[@]}; index+=COMPONENT_LIST_COLUMNS)); do
		if [ "${COMPONENT_LIST[$index]}" = "$command" ]; then
			echo "${COMPONENT_LIST[$index+$column]}"
			return
		fi
	done
}
# -------------------------------------------------------------------
get_component_min_version() {
	get_column_for_component "$1" 1
}
# -------------------------------------------------------------------
get_component_required() {
	get_column_for_component "$1" 2
}
# -------------------------------------------------------------------
get_component_description() {
	get_column_for_component "$1" 3
}
# -------------------------------------------------------------------
get_component_command() {
	local component="$1"
	local column="$2"

	local command
	command="$(get_column_for_component "${component}" "${column}")"
	if [[ $command == *"%s"* ]]; then
		# shellcheck disable=SC2059
		command="$(printf "$command" "${component}")"
	fi
	${command}
}
# -------------------------------------------------------------------
get_component_version() {
	get_component_command "$1" 4
}
# -------------------------------------------------------------------
get_component_install_command() {
	get_column_for_component "$1" 6
}
# -------------------------------------------------------------------
get_component_display_version() {
	local component="$1"
	local command
	command="$(get_column_for_component "${component}" 5)"
	if [ -z "${command}" ] ; then
		get_component_version "${component}"
		return
	fi
	get_component_command "$1" 5
}
# -------------------------------------------------------------------
is_component_required() {
	local command="$1"
	if [ "$(get_component_required "${command}")" == "required" ] ; then
		return 0
	fi
	return 1
}
# -------------------------------------------------------------------
is_component_compatible() {
	local component="$1"
	local installed_versioin
	local min_version
	installed_versioin="$(get_component_version "${component}")"
	min_version="$(get_component_min_version "${component}")"
	test_version "${installed_versioin}" -ge "$min_version"
}
# -------------------------------------------------------------------
is_system_sufficient() {
	for ((i=0; i< $(get_component_count); i++)); do
		local command
		command="$(get_nth_component "$i")"
		if ! is_component_compatible "${command}" && is_component_required "${command}" ; then
			return 1
		fi
	done
	return 0
}
# -------------------------------------------------------------------
include_optional_components(){
	if [ "$INCLUDE_OPTIONAL" -ne 0 ] ; then
		return 1
	fi
	return 0
}
# -------------------------------------------------------------------
optional_components_compatible(){
	for ((i=0; i< $(get_component_count); i++)); do
		local command
		command="$(get_nth_component "$i")"
		if ! is_component_required "${command}" ; then
			if ! is_component_compatible "${command}" ; then
				return 1
			fi
		fi
	done
	return 0
}
# -------------------------------------------------------------------
are_optional_components_available_for_installation() {
	if ! include_optional_components ; then
		return 1
	fi
	if optional_components_compatible ; then
		return 1
	fi
	return 0
}
# -------------------------------------------------------------------
print_optional_info() {
	if ! include_optional_components  && ! optional_components_compatible; then
	print_status_line \
		"${information_source}" \
		"Note" \
		"use -o option to install" \
		"optional packages"

	fi
}
# -------------------------------------------------------------------
is_system_compatible() {
	if is_component_compatible "macos" ; then
		return 0
	fi
	return 1
}
# -------------------------------------------------------------------
print_version_state_for_component(){
	local component="$1"
	local state_string
	local min_version
	local spc=2
	local needed="needed"
	
	if is_component_compatible "${component}" ; then
		state_string="${tick_symbol}"
	else
		min_version="$(get_component_min_version "${component}")"
		if [ "$(get_component_required "${component}")" == "required" ] ; then
			state_string="${red_exclamation_mark}"
		else
			state_string="${warning_symbol}"
			needed="recommended"
		fi
		state_string="$(printf "%-${spc}s >= %s %s" "${state_string}" "${min_version}" "${needed}")"
	fi
	printf "%s" "${state_string}" 
}
# -------------------------------------------------------------------
macos_human_string() {
	printf "%s %s (%s)" "$(sw_vers -productName)" "$(sw_vers -productVersion)" "$(macos_marketing_string "$(get_component_version macos)")"
}
# -------------------------------------------------------------------
git_version(){
	local version_str=""
	if ! xcode-select -p &>/dev/null; then
		if ! command -v "${command}" &>/dev/null; then
			printf "not installed" ; return 0
		fi
		echo "command line tools required"
		return 0
	fi
	version_str="$(command_version git)"
	if [ -z "$version_str" ] && ! is_xcode_license_agreed ; then
		printf "%s" "license not jet agreed" 
	fi
	printf "%s" "$version_str"
}
# -------------------------------------------------------------------
xcode_version(){
	# Check whether xcode-select is installed
	if ! command -v xcode-select &>/dev/null; then
		printf "not installed" ; return 0
	fi
	# Check whether the Xcode Command Line Tools are selected
	if ! xcode-select -p &>/dev/null; then
		printf "not installed" ; return 0
	fi
	# Get Xcode vesrion
	if xcodebuild -version &>/dev/null ; then 
		xcodebuild -version | grep "Xcode" | cut -d " " -f 2
	else
		printf "not installed" ; return 0
	fi
}
# -------------------------------------------------------------------
macos_marketing_string(){
	local version="$1"
	local marketing_name
	case "${version}" in
		10.10*) marketing_name="Yosemite" ;;
		10.11*) marketing_name="El Capitan" ;;
		10.12*) marketing_name="Sierra" ;;
		10.13*) marketing_name="High Sierra" ;;
		10.14*) marketing_name="Mojave" ;;
		10.15*) marketing_name="Catalina" ;;
		10.0*) marketing_name="Cheetah" ;;
		10.1*) marketing_name="Puma" ;;
		10.2*) marketing_name="Jaguar" ;;
		10.3*) marketing_name="Panther" ;;
		10.4*) marketing_name="Tiger" ;;
		10.5*) marketing_name="Leopard" ;;
		10.6*) marketing_name="Snow Leopard" ;;
		10.7*) marketing_name="Lion" ;;
		10.8*) marketing_name="Mountain Lion" ;;
		10.9*) marketing_name="Mavericks" ;;
		11.*) marketing_name="Big Sur" ;;
		12.*) marketing_name="Monterey" ;;
		13.*) marketing_name="Ventura" ;;
		14.*) marketing_name="Sonoma" ;;
		*) marketing_name="Unknown" ;;
	esac
	printf "%s" "${marketing_name}"
}
# -------------------------------------------------------------------
macos_xcode_download_url(){
	local version="$1"
	local download_url
	case "${version}" in
		10.0*) download_url=null ;;
		10.10*) download_url="https://download.developer.apple.com/Developer_Tools/Xcode_7.2.1/Xcode_7.2.1.dmg" ;;
		10.11*) download_url="https://download.developer.apple.com/Developer_Tools/Xcode_8.2.1/Xcode_8.2.1.xip" ;;
		10.12*) download_url="https://download.developer.apple.com/Developer_Tools/Xcode_9.2/Xcode_9.2.xip" ;;
		10.13*) download_url="https://download.developer.apple.com/Developer_Tools/Xcode_10.1/Xcode_10.1.xip" ;;
		10.14*) download_url="https://download.developer.apple.com/Developer_Tools/Xcode_11.3.1/Xcode_11.3.1.xip" ;;
		10.15*) download_url="https://download.developer.apple.com/Developer_Tools/Xcode_12.4/Xcode_12.4.xip" ;;
		10.3*) download_url="https://download.developer.apple.com/Developer_Tools/xcode_v1.5/xcode_tools_1.5_cd.dmg.bin" ;;
		10.4*) download_url="https://download.developer.apple.com/Developer_Tools/xcode_2.5_developer_tools/xcode25_8m2558_developerdvd.dmg" ;;
		10.5*) download_url="https://download.developer.apple.com/Developer_Tools/xcode_3.1.4_developer_tools/xcode314_2809_developerdvd.dmg" ;;
		10.6*) download_url="https://download.developer.apple.com/Developer_Tools/xcode_4.2_for_snow_leopard/xcode_4.2_for_snow_leopard.dmg" ;;
		10.7*) download_url="https://download.developer.apple.com/Developer_Tools/xcode_4.6.3/xcode4630916281a.dmg" ;;
		10.8*) download_url="https://download.developer.apple.com/Developer_Tools/xcode_5.1.1/xcode_5.1.1.dmg" ;;
		10.9*) download_url="https://download.developer.apple.com/Developer_Tools/Xcode_6.2/Xcode_6.2.dmg" ;;
		11.*) download_url="https://download.developer.apple.com/Developer_Tools/Xcode_13.2.1/Xcode_13.2.1.xip" ;;
		12.*) download_url="https://download.developer.apple.com/Developer_Tools/Xcode_14.2/Xcode_14.2.xip" ;;
		13.*) download_url="https://download.developer.apple.com/Developer_Tools/Xcode_14.3.1/Xcode_14.3.1.xip" ;;
		14.*) download_url=null ;;
	esac
	printf "%s" "${download_url}"
}
# -------------------------------------------------------------------
xcode_version_for_macOS_version(){
	macos_xcode_download_url "$(get_component_version macos)" | cut -d'/' -f5
}
# -------------------------------------------------------------------
is_xcode_running(){
	local ps_text=""
	local grep_text=""	
	ps_text="$(ps -A)"
	grep_text="$(echo "$ps_text" | grep "Contents/MacOS/Xcode")"
	[ -n "$grep_text" ]
}
# -------------------------------------------------------------------
is_xcode_license_agreed(){
	xcodebuild -checkFirstLaunchStatus
}
# -------------------------------------------------------------------
ensure_xcode_license_agreed(){
	local wait_count=0
	local seconds_to_cancel=10
	if is_component_compatible xcode && ! is_xcode_license_agreed ; then
		local wait_count=0
		while is_component_compatible xcode && ! is_xcode_license_agreed ; do
			clear
			echo  "${yellow}${bold}Xcode is installed, but you haven't run the Xcode application yet.${normal}"
			echo  "${bold}  Please open Xcode to complete the installation.${normal}"
			echo  "${bold}  This script will wait until it detects that${normal}"
			echo  "${bold}  the Xcode installation is complete.${normal}"
			if ! is_xcode_running ; then
				echo  "  ${black}${bold}We will open Xcode for you in $((seconds_to_cancel-wait_count)) seconds ...${normal}"
				echo  "  ${black}${bold}Press [CTR][C] to exit. ${normal}"
			else
				echo  "  ${black}${bold}You must agree to the license agreements in order to use Xcode.${normal}"
				echo  "  ${black}${bold}Press [CTR][C] to exit. ${normal}"
				wait_count=0
			fi
			sleep 1
			wait_count=$((wait_count+1))
			if [ $wait_count -eq $seconds_to_cancel ] && ! is_xcode_running ; then
				xed > /dev/null 2>&1&
			fi
		done
	fi
}
# -------------------------------------------------------------------
open_xcode_appstore(){
	open https://apps.apple.com/de/app/xcode/id497799835
}
# -------------------------------------------------------------------
open_xcode_download(){
	open "https://developer.apple.com/download/all/"
	open "$(macos_xcode_download_url "$(get_component_version macos)")"
}
# -------------------------------------------------------------------
open_apple_developer_login_page(){
	open "https://developer.apple.com/download/all/"
}
# -------------------------------------------------------------------
print_status_line(){
	local status="$1" ; shift
	local col_1_text="$1" ; shift
	local col_2_text="$1" ; shift
 	local col_3_text="${status}"
	local status_color="${black}${normal}"

	case "${status}" in
		OK)        status_color="${green}${bold}";   col_3_text="${tick_symbol}" ;;
		WARNING)   status_color="${yellow}${bold}";  col_3_text="${warning_symbol}" ;;
		ERROR)     status_color="${red}${bold}";     col_3_text="${red_exclamation_mark}" ;;
		SEPERATOR) printf "%s\n" "  -------------------------------------------------" ; return 0 ;;
		*)         ;;
	esac

	printf "%s%s%16s:%s %s%-28s%s %s\n" \
		"  " "${bold}" "${col_1_text}" "${normal}" "${status_color}" "$col_2_text" "${normal}" "${col_3_text}"
	
	while [[ $# -gt 0 ]]; do
		printf "%s%s%16s %s %s%-28s%s %s\n" \
			"  " "${bold}" "" "${normal}" "${status_color}" "$1" "${normal}" ""
		shift
	done


}
# -------------------------------------------------------------------
print_component_status(){
	local component="$1"

	print_status_line \
		"$(print_version_state_for_component "${component}")" \
		"$(get_component_description "${component}")" \
		"$(get_component_display_version "${component}")" 
}
# -------------------------------------------------------------------
print_state_of_all_components(){
	for ((i=0; i< $(get_component_count); i++)); do
		print_component_status "$(get_nth_component "$i")"
	done
	print_status_line SEPERATOR
	if is_system_sufficient ; then
		print_status_line OK "This system" "is compatible" 
	else
		if test_version "$(get_component_version macos)" -ge "10.13" ; then
			print_status_line WARNING "This system" "needs updates" 
		else
			print_status_line ERROR "This system" "is not compatible" 
		fi
	fi
}
# -------------------------------------------------------------------
get_latest_cmake_release(){
	local url="https://cmake.org/download"
	local line=""
	local latest_release=""
	line=$(curl -s "$url" | grep -i "Latest Release")
	latest_release=$(echo "$line" | sed -n 's/.*(\(.*\)).*/\1/p')
	printf "%s" "$latest_release"
}
# -------------------------------------------------------------------
install_cmake(){
	local cmake_version=""
	local cmake_download_dir=""
	cmake_version="$(get_latest_cmake_release)"
	cmake_download_dir="$(mktemp -d)"
	
	local cmake_dowload_base_url="https://github.com/Kitware/CMake/releases/download/v"
	local cmake_dmg="cmake-${cmake_version}-macos-universal.dmg"
	local cmake_dmg_mount_point="/Volumes/cmake-${cmake_version}-macos-universal"
	local cmake_url="${cmake_dowload_base_url}${cmake_version}/${cmake_dmg}"
	local cmake_download="${cmake_download_dir}/${cmake_dmg}"

	info "Installing CMake:" \
		 "Version: ${cmake_version}" \
		 "Downloading: ${cmake_url}" \
		 "to: $cmake_download"

	if ! download_url_to "${cmake_url}" "${cmake_download}" "10000" ; then
		info "${red}${bold}Error:${normal}" \
			 "Downloading: $cmake_url" \
			 "CMake could not be installed!" \
			 "Note: Manual installation instructions can be found at https://cmake.org"

		printf "%s" "${bold}Press Any Key to Continue${normal}"
		wait_for_key > /dev/null
		return 1
	fi

	info "Mounting: $cmake_download"  "at: $cmake_dmg_mount_point"
	yes | hdiutil attach -quiet -mountpoint "${cmake_dmg_mount_point}" "${cmake_download}"

	info "Copy: ${cmake_dmg_mount_point}/CMake.app" "to: /Applications/"
	cp -R "${cmake_dmg_mount_point}/CMake.app" "/Applications/"

	info "Unmounting: $cmake_dmg_mount_point"
	hdiutil detach "${cmake_dmg_mount_point}"

	info "Installing CMake Command Line Support, which requires administrative privileges."
	info "${yellow}${bold}${warning_symbol} You will be prompted to enter your password for authentication.${normal}"

	sudo "/Applications/CMake.app/Contents/bin/cmake-gui" --install=/usr/local/bin

	info "Clean up ..."
	rm -r "$cmake_download_dir"
	return 0
}
# -------------------------------------------------------------------
install_homebrew(){
	info "Homebrew: ${normal}Package Manager for macOS" \
		 "See: https://brew.sh/" \
		 "Note: Installing Homebrew requires administrative privileges." \
		 "${yellow}${bold}${warning_symbol} You will be prompted to enter your password for authentication.${normal}"

	 /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
}
# -------------------------------------------------------------------
install_component(){
	local component="$1"
	clear
	echo "${blue}${bold}Installing component: ${normal}${component}"
	echo
 	eval "$(get_component_install_command "${component}")"
}
# -------------------------------------------------------------------
install_components(){
	for ((i=0; i< $(get_component_count); i++)); do
		if ! is_component_compatible "$(get_nth_component "$i")" ; then
			if include_optional_components || is_component_required "$(get_nth_component "$i")" ; then
				install_component "$(get_nth_component "$i")"
			fi
		fi
	done
}
# -------------------------------------------------------------------
wait_for_key(){
	local kReturn=""
	local key=""
	while true ; do 
		read -rsn 1 key
		case $key in
			"$(printf '\x1b')") # esc 
				read -rsn 1 -t1 key
				if [ "$key" = "[" ] ; then
					read -rsn 1 -t1 key
					case "$key" in
						"B") echo "cursorDown"; break ;;
						"A") echo "cursorUp"; break ;;
						"D") echo "cursorLeft"; break ;;
						"C") echo "cursorRight"; break ;;
						*) ;;
					esac
				else
					echo "escape"; break;
				fi
				;;
			"$kReturn")
				echo "return"; break ;;
			"$(printf '\x4f')" | "$(printf '\x7f')") # backspace
				echo "backSpace"; break ;;
			*)
				echo "$key" | tr '[:upper:]' '[:lower:]' ; break ;;
		esac
	done
}
# -------------------------------------------------------------------
menu(){
	local menu_print_command="${1}_menu_print"
	local menu_process_key_command="${1}_menu_process_key"
	local selected_command="$2"
	clear
	local inputprompt="${bold}Hit Key to execute Command:${normal}"
	if [ "$(uname -s)" != "Darwin" ]; then 
		inputprompt="${bold}${red}$(osID) is not supported by this script!${normal}"
		printf "%s" "${inputprompt}"
		return 1
	elif [ "${selected_command}" != "" ]; then 
		"${menu_process_key_command}" "${selected_command}"
		return $?
	fi
	while true ; do
		local result="2"
		clear
		if ! "${menu_print_command}" ; then
			break;
		fi
		printf "%s" "${inputprompt}"
		while [ "$result" -gt 1 ] ; do
			"${menu_process_key_command}" "$(wait_for_key)"
			result=$?
		done
		if [ "$result" -eq 1 ] ; then
			break;
		fi
	done
	return 1
}
# -------------------------------------------------------------------
install_xcode_menu_print(){
	if ! is_component_compatible "macos" ; then
		echo "${red}${bold}Fatal Error: This macOS is not supported:${normal}"
		return 1
	fi
	local component="xcode"
	echo "${blue}${bold}Xcode Installation needed:${normal}"
	if ! is_component_compatible "${component}" ; then
		print_component_status "${component}"
	fi	
	cat <<- _EOF_

	${blue}${bold}Install options:${normal}
	  ${bold}A) Install Xcode from App Store:${normal}	  
	     ${bold}Note:${normal} Installing Xcode from the App Store will only work if your
	           macOS is the latest macOS
	     $(h O)pen App Store
	
	  ${bold}B) Download $(xcode_version_for_macOS_version)${normal}
	     $(h 1). $(h L)ogin to your Apple Developer account
	     $(h 2). $(h D)ownload $(xcode_version_for_macOS_version)

	${underline}${bold}B${normal}ack to main menu
	
	_EOF_
	return 0
}
# -------------------------------------------------------------------
install_xcode_menu_process_key(){
	local key="$1"
	case "${key}" in
		o|a) open_xcode_appstore ;;
		l|1) open_apple_developer_login_page ;;
		d|2) open_xcode_download ;;
		b|"escape") return 1 ;;
		*) ;;
	esac
	return 0
}
# -------------------------------------------------------------------
install_menu_print(){
	echo "${blue}${bold}Available installations:${normal}"
	for ((i=0; i< $(get_component_count); i++)); do
		local component
		component="$(get_nth_component "$i")"
		if ! is_component_compatible "${component}" ; then
			print_component_status "${component}"
		fi	
	done
	cat <<- _EOF_
	${blue}${bold}Commands:${normal}
	  $(h S)tart installations
	  $(h B)ack to main menu
	_EOF_
	return 0
}
# -------------------------------------------------------------------
install_menu_process_key(){
	local key="$1"
	case "${key}" in
		s) install_components; return 1 ;;  
		b|"escape") return 1 ;;
		*) return 2 ;;
	esac
}
# -------------------------------------------------------------------
install(){
	if ! is_component_compatible "macos" ; then
		echo "${red}${bold}Fatal Error: This macOS is not supported:${normal}"
		return 1
	fi
	if ! is_component_compatible "xcode" ; then
		menu "install_xcode" "" ;
	else
		ensure_xcode_license_agreed
		menu "install" ""
	fi
}
# -------------------------------------------------------------------
should_exit_main_menu(){
	if ! is_system_compatible; then
		return 0
	fi
	if is_system_sufficient && ! are_optional_components_available_for_installation ; then
		return 0
	fi
	return 1 
}
# -------------------------------------------------------------------
main_menu_print(){
	note "  $(b "${0##*/}")"
	cat <<- _EOF_
	${blue}$(b "Description:")
	  This command-line tool checks whether your system meets the 
	  system requirements of the VST3 SDK. 
	  It assists in identifying any missing applications or tools
	  that need to be installed.
	${blue}$(b "Identified components:")
	_EOF_
	print_state_of_all_components
	if should_exit_main_menu; then
		return 0
	fi
	cat <<- _EOF_
	${blue}$(b "Commands:")
	  $(h I)nstall  - Install packages
	  $(h H)elp     - Print usage help
	  $(h Q)uit.    - Exit; no action
	_EOF_
	return 0
}
# -------------------------------------------------------------------
main_menu_process_key(){
	local key="$1"
	case "${key}" in
		i) 
			install; COMMAND=""
			if is_system_sufficient && ! are_optional_components_available_for_installation ; then 
				main_menu_print
				return 1
			fi 
			;;
		h) clear; usage; wait_for_key ;;
		q|"escape") return 1 ;;
		*) return 2 ;;
	esac
	return 0
}
# -------------------------------------------------------------------
run(){
	parceArguments "$@"
	note "  $(b "${0##*/}")"
	info "Determining system status" \
		 "This might take several minutes ..."
	init_component_list
	if should_exit_main_menu; then
		clear
		main_menu_print
		print_optional_info
		echo ""
		return 0
	fi	
	menu "main" "${COMMAND}"
	echo ""
}
# -------------------------------------------------------------------
run "$@"
