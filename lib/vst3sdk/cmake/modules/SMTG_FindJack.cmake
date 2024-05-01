
# - Try to find Jack Audio Libraries
# libjack; libjackserver;
# Once done this will define
#  LIBJACK_FOUND - System has libjack.so
#  LIBJACK_INCLUDE_DIRS - The jack.h include directories
#  LIBJACK_LIBRARIES - The libraries needed to use jack

# Install jack on Ubuntu: apt-get install libjack-jackd2-dev
# Add user to audio group: usermod -a -G audio <yourUserName>

# Install jack on Windows: winget install Jackaudio.JACK2
# Install jack on macOS: brew install jack

#------------------------------------------------------------------------

# Uncomment to see all locations/paths which are searched
# set(CMAKE_FIND_DEBUG_MODE TRUE)

if(NOT PROJECT_NAME)
	message(FATAL_ERROR "[SMTG] Only include SMTG_FindJack AFTER the root \"project(...)\" call is done!")
endif()

find_path(LIBJACK_INCLUDE_DIR
    NAMES
        jack/jack.h
    PATH_SUFFIXES
		JACK2/include
)

find_library(LIBJACK_libjack_LIBRARY
    NAMES
        libjack64 
        libjack
        jack
    PATH_SUFFIXES
		JACK2/lib
)

find_library(LIBJACK_libjackserver_LIBRARY
    NAMES
        libjackserver64
        libjackserver
        jackserver
    PATH_SUFFIXES
		JACK2/lib
)

# For debugging
# message(STATUS "LIBJACK_libjackserver_LIBRARY ${LIBJACK_libjackserver_LIBRARY}")
# message(STATUS "LIBJACK_libjack_LIBRARY ${LIBJACK_libjack_LIBRARY}")
# message(STATUS "LIBJACK_INCLUDE_DIR ${LIBJACK_INCLUDE_DIR}")

set(LIBJACK_LIBRARIES ${LIBJACK_libjack_LIBRARY} ${LIBJACK_libjackserver_LIBRARY})
set(LIBJACK_LIBRARY ${LIBJACK_LIBRARIES})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBJACK_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LIBJACK DEFAULT_MSG
                                  LIBJACK_LIBRARIES LIBJACK_INCLUDE_DIR)
