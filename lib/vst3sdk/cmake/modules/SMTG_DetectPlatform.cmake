
# Detect the platform which cmake builds for.
#
# Needed for check_language(CXX)
include(CheckLanguage)
# The detected platform var is stored within the internal cache of cmake in order to make it
# available globally.
set(SMTG_PLATFORM_DETECTION_COMMENT "The platform which was detected by SMTG")

#------------------------------------------------------------------------
# Detect the Platform
#
macro(smtg_detect_platform)
    if(APPLE)
        set(SMTG_MAC TRUE CACHE INTERNAL ${SMTG_PLATFORM_DETECTION_COMMENT})
    elseif(UNIX OR ANDROID_PLATFORM)
        set(SMTG_LINUX TRUE CACHE INTERNAL ${SMTG_PLATFORM_DETECTION_COMMENT})
    elseif(WIN32)
        set(SMTG_WIN TRUE CACHE INTERNAL ${SMTG_PLATFORM_DETECTION_COMMENT})
    endif(APPLE)
endmacro(smtg_detect_platform)

#------------------------------------------------------------------------
# Check if c++ compiler is available
# Handling cmake issue: https://gitlab.kitware.com/cmake/cmake/-/issues/20890
function(smtg_check_language_cxx)
    message(STATUS "[SMTG] Check C++ compiler")
    check_language(CXX)
    if(NOT CMAKE_CXX_COMPILER)
        if(SMTG_MAC)
            execute_process(COMMAND xcode-select -p OUTPUT_VARIABLE _xcode ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
            cmake_path(SET _xcode NORMALIZE "${_xcode}/../..")
            string(FIND "${_xcode}"  " " _pos)
            if(_pos GREATER "-1")
                message(FATAL_ERROR "[SMTG] Spaces in Xcode path are not supported by cmake: \"${_xcode}\"" )
            endif()
        endif()
    endif()
endfunction()

#------------------------------------------------------------------------
# Detect the default Xcode Version
#
macro(smtg_detect_xcode_version)
    if(SMTG_MAC)
        # Check if CXX compiler is available
        smtg_check_language_cxx()
        if(NOT DEFINED ENV{XCODE_VERSION})
            # find out Xcode version if not set by cmake
            execute_process(COMMAND xcodebuild -version OUTPUT_VARIABLE XCODE_VERSION ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
            string(REGEX MATCH "Xcode [0-9\\.]+" XCODE_VERSION "${XCODE_VERSION}")
            string(REGEX REPLACE "Xcode ([0-9\\.]+)" "\\1" XCODE_VERSION "${XCODE_VERSION}")
        endif()
        if(XCODE_VERSION VERSION_LESS "9")
            message(FATAL_ERROR "[SMTG] XCode 9 or newer is required")
        endif()
        message(STATUS "[SMTG] Building with Xcode version: ${XCODE_VERSION}")
        if(NOT DEFINED ENV{SDKROOT})
            execute_process(COMMAND xcrun --sdk macosx --show-sdk-path OUTPUT_VARIABLE CMAKE_OSX_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE)
        endif()
    endif(SMTG_MAC)
endmacro(smtg_detect_xcode_version)
