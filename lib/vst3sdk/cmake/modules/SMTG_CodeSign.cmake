
if(XCODE)
    option(SMTG_DISABLE_CODE_SIGNING "Disable All Code Signing" OFF)
    set(SMTG_XCODE_OTHER_CODE_SIGNING_FLAGS "--timestamp" CACHE STRING "Other code signing flags [Xcode]")
    option(SMTG_XCODE_MANUAL_CODE_SIGN_STYLE "Manual Xcode sign style" OFF)
endif()

#------------------------------------------------------------------------
function(smtg_target_codesign target)
    if(XCODE AND (NOT SMTG_DISABLE_CODE_SIGNING))
        if(ARGC GREATER 2)
            set(team "${ARGV1}")
            set(identity "${ARGV2}")
            message(STATUS "[SMTG] Codesign ${target} with team '${team}' and identity '${identity}'")
            set(SMTG_CODESIGN_ATTRIBUTES 
                XCODE_ATTRIBUTE_DEVELOPMENT_TEAM    "${team}"
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY  "${identity}"
            )
        else()
            message(STATUS "[SMTG] Codesign ${target} for local machine only")
            set(SMTG_CODESIGN_ATTRIBUTES 
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
            )
        endif(ARGC GREATER 2)
        if(SMTG_XCODE_OTHER_CODE_SIGNING_FLAGS)
            set(SMTG_CODESIGN_ATTRIBUTES 
                ${SMTG_CODESIGN_ATTRIBUTES}
                XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "${SMTG_XCODE_OTHER_CODE_SIGNING_FLAGS}"
            )
        endif()
        message(DEBUG "[SMTG] Code Sign Attributes: ${SMTG_CODESIGN_ATTRIBUTES}")
        set_target_properties(${target}
            PROPERTIES
                ${SMTG_CODESIGN_ATTRIBUTES}
        )
        if(SMTG_XCODE_MANUAL_CODE_SIGN_STYLE)
            set_target_properties(${target}
                PROPERTIES
                    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual"
            )
        endif(SMTG_XCODE_MANUAL_CODE_SIGN_STYLE)
        if(SMTG_MAC AND (XCODE_VERSION VERSION_GREATER_EQUAL 12))
            # make sure that the executable is signed before cmake post build commands are run as the
            # Xcode code-sign step is run after the post build commands are run which would prevent
            # using the target output on system where everything needs to be code-signed.
            target_link_options(${target} PRIVATE LINKER:-adhoc_codesign)
        endif()
    endif(XCODE AND (NOT SMTG_DISABLE_CODE_SIGNING))
endfunction(smtg_target_codesign)

#------------------------------------------------------------------------
function(smtg_target_add_custom_codesign_post_build_step)
    if(XCODE AND (NOT SMTG_DISABLE_CODE_SIGNING))
        set(mono_args PATH TARGET IDENTITY)
        cmake_parse_arguments(PARSE_ARGV 0
            PARSED_ARGS    # Prefix of output variables e.g. PARSED_ARGS_RESOURCES
            ""             # List of names for boolean arguments
            "${mono_args}" # List of names for mono-valued arguments
            "FLAGS"        # List of names for multi-valued arguments resp. lists
        )
        if(NOT PARSED_ARGS_TARGET)
            message(FATAL_ERROR "Need TARGET argument")
        endif()
        if(NOT PARSED_ARGS_PATH)
            message(FATAL_ERROR "Need PATH argument")
        endif()
        if(PARSED_ARGS_IDENTITY)
            set(identity "${PARSED_ARGS_IDENTITY}")
        else()
            set(identity "-")
        endif()
        add_custom_command(TARGET ${PARSED_ARGS_TARGET} POST_BUILD 
            COMMAND /usr/bin/codesign -v -s "${identity}" --force ${PARSED_ARGS_FLAGS} "${PARSED_ARGS_PATH}"
        )
    endif()
endfunction(smtg_target_add_custom_codesign_post_build_step)

#------------------------------------------------------------------------
# Deprecated since 3.7.4 -----------------------------
function(smtg_codesign_target target)
    message(DEPRECATION "[SMTG] Use smtg_target_codesign instead of smtg_codesign_target")
    smtg_target_codesign (${target})
endfunction(smtg_codesign_target)
