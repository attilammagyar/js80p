
include(CMakeParseArguments)

#[=======================================================================[.md:

    Configures the macOS bundle
    The usual usage is:
    ```cmake
    smtg_target_set_bundle(yourplugin
        BUNDLE_IDENTIFIER "com.yourcompany.vst3.yourplugin"
        COMPANY_NAME "yourplugin"
    )
    ```
#]=======================================================================]
function(smtg_target_set_bundle target)
    if(NOT SMTG_MAC)
        return()
    endif(NOT SMTG_MAC)

    set(options 
        PREPROCESS
    )
    set(oneValueArgs 
        EXTENSION 
        INFOPLIST
        INFOPLIST_IN
        BUNDLE_IDENTIFIER
        COMPANY_NAME
    )
    set(multiValueArgs 
        RESOURCES 
        PREPROCESSOR_FLAGS 
    )
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "[SMTG] The following parameters are unrecognized: ${ARG_UNPARSED_ARGUMENTS}")
    endif(ARG_UNPARSED_ARGUMENTS)

    # Adding the bundle resources to the target sources creates a warning, see https://cmake.org/Bug/view.php?id=15272
    target_sources(${target} 
        PRIVATE
            ${ARG_RESOURCES}
    )

    get_target_property(type ${target} TYPE)
    if(type STREQUAL MODULE_LIBRARY)
        set_target_properties(${target}
            PROPERTIES
                BUNDLE TRUE
        )
    elseif(type STREQUAL EXECUTABLE)
        set_target_properties(${target}
            PROPERTIES
                MACOSX_BUNDLE TRUE
        )
    endif()

    if(ARG_EXTENSION)
        if(XCODE)
            set_target_properties(${target}
                PROPERTIES
                    XCODE_ATTRIBUTE_WRAPPER_EXTENSION ${ARG_EXTENSION}
            )
        else()
            set_target_properties(${target}
                PROPERTIES
                    BUNDLE_EXTENSION ${ARG_EXTENSION}
            )
        endif(XCODE)
    endif(ARG_EXTENSION)

    if(ARG_RESOURCES)
        set_source_files_properties(${ARG_RESOURCES}
            PROPERTIES
                MACOSX_PACKAGE_LOCATION Resources
        )
    endif(ARG_RESOURCES)

    if(ARG_INFOPLIST AND XCODE)
        set_target_properties(${target}
            PROPERTIES
                XCODE_ATTRIBUTE_INFOPLIST_FILE "${ARG_INFOPLIST}"
        )
        if(ARG_PREPROCESS)
            set_target_properties(${target}
                PROPERTIES
                    XCODE_ATTRIBUTE_INFOPLIST_PREPROCESS "YES"
            )
        endif(ARG_PREPROCESS)
        if(ARG_PREPROCESSOR_FLAGS)
            set_target_properties(${target} 
                PROPERTIES
                    XCODE_ATTRIBUTE_INFOPLIST_OTHER_PREPROCESSOR_FLAGS "${ARG_PREPROCESSOR_FLAGS}"
            )
        endif(ARG_PREPROCESSOR_FLAGS)
    elseif(XCODE)
        # Build Copyright string:
        string(TIMESTAMP BUNDLE_INFO_STRING_YEAR "%Y")
        set(BUNDLE_COPYRIGHT "© ${BUNDLE_INFO_STRING_YEAR} ${ARG_COMPANY_NAME}.")

        # Check for custom info.plist template:
        if(ARG_INFOPLIST_IN)
            # dest Info.plist.in template file in cmake format
            # see:  https://duckduckgo.com/?q=MACOSX_BUNDLE_INFO_PLIST
            set(SMTG_BUNDLE_INFO_PLIST "${PROJECT_BINARY_DIR}/smtg_mac/Info.plist.in")
            # Configure SMTG_BUNDLE_INFO_PLIST to cmake MACOSX_BUNDLE_INFO_PLIST template
            # Use @VAR@ for variables not defined in MACOSX_BUNDLE_INFO_PLIST
            configure_file("${ARG_INFOPLIST_IN}" "${SMTG_BUNDLE_INFO_PLIST}" @ONLY)

            set_target_properties(${smtg_target}
                PROPERTIES
                    MACOSX_BUNDLE_INFO_PLIST ${SMTG_BUNDLE_INFO_PLIST}
            )        
        endif()

        set_target_properties(${target}
            PROPERTIES
                MACOSX_BUNDLE_GUI_IDENTIFIER ${ARG_BUNDLE_IDENTIFIER}
                MACOSX_BUNDLE_COPYRIGHT ${BUNDLE_COPYRIGHT}
                MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
                MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
                MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
                MACOSX_BUNDLE_INFO_STRING ${PROJECT_VERSION}
        )
    endif()

    if(ARG_BUNDLE_IDENTIFIER)
        if(XCODE)
        	set_target_properties(${target}
                PROPERTIES
                    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${ARG_BUNDLE_IDENTIFIER}"
            )
        endif(XCODE)
	endif(ARG_BUNDLE_IDENTIFIER)
endfunction()

#[=======================================================================[.md:
    Creates the file projectversion.h and adds the directory as an
    include directory.
#]=======================================================================]
function(smtg_target_configure_version_file target)

    set(SMTG_PROJECT_VERSION "")

    if(NOT "${PROJECT_VERSION_MAJOR}" STREQUAL "")
        string(APPEND SMTG_PROJECT_VERSION "${PROJECT_VERSION_MAJOR}")
    endif()
    if(NOT "${PROJECT_VERSION_MINOR}" STREQUAL "")
        string(APPEND SMTG_PROJECT_VERSION ".${PROJECT_VERSION_MINOR}")
    endif()
    if(NOT "${PROJECT_VERSION_PATCH}" STREQUAL "")
        string(APPEND SMTG_PROJECT_VERSION ".${PROJECT_VERSION_PATCH}")
    endif()

    configure_file(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../templates/projectversion.h.in projectversion.h)

    target_include_directories(${target} PUBLIC
        "${PROJECT_BINARY_DIR}"
    )
endfunction()

#[=======================================================================[.md:
    Deprecated since 3.7.4
#]=======================================================================]
function(smtg_set_bundle target)
    message(DEPRECATION "[SMTG] Use smtg_target_set_bundle instead of smtg_set_bundle")
    smtg_target_set_bundle (${target})
endfunction()
