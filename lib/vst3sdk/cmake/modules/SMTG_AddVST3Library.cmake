
#------------------------------------------------------------------------
# Includes
#------------------------------------------------------------------------
include(SMTG_AddSMTGLibrary)

set(SMTG_RUN_VST_VALIDATOR_DEFAULT ON)
set(SMTG_CREATE_MODULE_INFO_DEFAULT ON)

# Run the Validator after each new compilation of VST3 Plug-ins
option(SMTG_RUN_VST_VALIDATOR "Run VST validator on VST3 Plug-ins" ${SMTG_RUN_VST_VALIDATOR_DEFAULT})

# Create the moduleinfo.json file after building
option(SMTG_CREATE_MODULE_INFO "Create the moduleinfo.json file" ${SMTG_CREATE_MODULE_INFO_DEFAULT})

#------------------------------------------------------------------------
# Runs the validator on a VST3 target.
#
# The validator will be run on the target as a post build step.
#
# @param target The target which the validator will validate. 
function(smtg_target_run_vst_validator target)
    if(TARGET validator)
        message(STATUS "[SMTG] Setup running validator for ${target}")
        add_dependencies(${target} validator)
        if(SMTG_WIN)
            set(TARGET_PATH $<TARGET_FILE:${target}>)
            add_custom_command(
                TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo [SMTG] Validator started...
                COMMAND 
                    $<TARGET_FILE:validator> 
                    "${TARGET_PATH}" 
                    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
                COMMAND ${CMAKE_COMMAND} -E echo [SMTG] Validator finished.
            )
        else()
            get_target_property(PLUGIN_PACKAGE_PATH ${target} SMTG_PLUGIN_PACKAGE_PATH)
            add_custom_command(
                TARGET ${target} POST_BUILD
                COMMAND 
                    $<TARGET_FILE:validator> 
                    $<$<CONFIG:Debug>:${PLUGIN_PACKAGE_PATH}>
                    $<$<CONFIG:Release>:${PLUGIN_PACKAGE_PATH}> 
                    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
            )
        endif(SMTG_WIN)
    endif(TARGET validator)
endfunction(smtg_target_run_vst_validator)

#------------------------------------------------------------------------
# Runs the moduleinfotool on a VST3 target.
#
# The moduleinfotool will be run on the target as a post build step and
# generates a moduleinfo.json file inside the VST 3 bundle.
#
# @param target The target for which the moduleinfotool will create the file. 
# @param MODULEINFO_COMPATIBILITY <Path> The path to the compatibility json file [optional]
function(smtg_target_create_module_info_file target)   
    set(oneValueArgs MODULEINFO_COMPATIBILITY)
    cmake_parse_arguments(PARSE_ARGV 1 PARAMS "" "${oneValueArgs}" "")
   
    if(NOT SMTG_CREATE_MODULE_INFO)
        return()
    endif()
    if(SMTG_WIN AND NOT SMTG_CREATE_BUNDLE_FOR_WINDOWS)
        message(WARNING "[SMTG] No moduleinfo.json file for ${target} generated. Consider to enable SMTG_CREATE_BUNDLE_FOR_WINDOWS.")
        return()
    endif()

    message(STATUS "[SMTG] Setup running moduleinfotool for ${target}")

    add_dependencies(${target} moduleinfotool)

    get_target_property(PLUGIN_PACKAGE_PATH ${target} SMTG_PLUGIN_PACKAGE_PATH)
    set(SMTG_MODULEINFO_PATH_INSIDE_BUNDLE Contents/Resources/moduleinfo.json)

    if(DEFINED PARAMS_MODULEINFO_COMPATIBILITY)
        add_custom_command(
            TARGET ${target} POST_BUILD
            COMMAND 
                $<TARGET_FILE:moduleinfotool> -create -version ${PROJECT_VERSION} 
                -path "${PLUGIN_PACKAGE_PATH}"
                -compat "${CMAKE_CURRENT_SOURCE_DIR}/${PARAMS_MODULEINFO_COMPATIBILITY}"
                -output "${PLUGIN_PACKAGE_PATH}/${SMTG_MODULEINFO_PATH_INSIDE_BUNDLE}"
                WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        )
    else()
        add_custom_command(
            TARGET ${target} POST_BUILD
            COMMAND 
                $<TARGET_FILE:moduleinfotool> -create -version ${PROJECT_VERSION} 
                -path "${PLUGIN_PACKAGE_PATH}"
                -output "${PLUGIN_PACKAGE_PATH}/${SMTG_MODULEINFO_PATH_INSIDE_BUNDLE}"
                WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        )
    endif()
    if(SMTG_MAC)
        add_custom_command(
            TARGET ${target} POST_BUILD
            COMMAND 
                codesign -f -s "-" -v "${PLUGIN_PACKAGE_PATH}/${SMTG_MODULEINFO_PATH_INSIDE_BUNDLE}"
                WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        )
    endif()
endfunction()

#------------------------------------------------------------------------
# Set SMTG_PLUGIN_TARGET_PATH to default VST3 Path
function(smtg_get_vst3_path)
    # here you can define the VST3 Plug-ins folder (it will be created), SMTG_PLUGIN_TARGET_DEFAULT_PATH will be set
    smtg_get_default_vst3_path()

    set(SMTG_PLUGIN_TARGET_USER_PATH "" CACHE PATH "Here you can redefine the VST3 Plug-ins folder")
    if(NOT ${SMTG_PLUGIN_TARGET_USER_PATH} STREQUAL "")
        set(PLUGIN_TARGET_PATH ${SMTG_PLUGIN_TARGET_USER_PATH})
    else()
        set(PLUGIN_TARGET_PATH ${SMTG_PLUGIN_TARGET_DEFAULT_PATH})
    endif()
    
    if(NOT ${PLUGIN_TARGET_PATH} STREQUAL "" AND SMTG_CREATE_PLUGIN_LINK)
        if(NOT EXISTS ${PLUGIN_TARGET_PATH})
            message(STATUS "[SMTG] Create folder: " ${PLUGIN_TARGET_PATH})
            if(SMTG_WIN)
                smtg_create_directory_as_admin_win(${PLUGIN_TARGET_PATH})
            else()
                file(MAKE_DIRECTORY ${PLUGIN_TARGET_PATH})
            endif(SMTG_WIN)
        endif(NOT EXISTS ${PLUGIN_TARGET_PATH})
    endif(NOT ${PLUGIN_TARGET_PATH} STREQUAL "" AND SMTG_CREATE_PLUGIN_LINK)
    if(EXISTS ${PLUGIN_TARGET_PATH})
        message(STATUS "[SMTG] SMTG_PLUGIN_TARGET_PATH is set to: " ${PLUGIN_TARGET_PATH})
    else()
        message(STATUS "[SMTG] SMTG_PLUGIN_TARGET_PATH is not set!")
    endif(EXISTS ${PLUGIN_TARGET_PATH})
    set(SMTG_PLUGIN_TARGET_PATH ${PLUGIN_TARGET_PATH} PARENT_SCOPE)
endfunction(smtg_get_vst3_path)

#------------------------------------------------------------------------
# Adds a VST3 target.
#
# @param target The target to which a VST3 Plug-in will be added.
# @param pkg_name name of the created package
# @param MODULEINFO_COMPATIBILITY <path> The path to the compatibility json file [optional]
function(smtg_add_vst3plugin_with_pkgname target pkg_name)
    #message(STATUS "[SMTG] target is ${target}")
    #message(STATUS "[SMTG] pkg_name is ${pkg_name}")

    set(oneValueArgs MODULEINFO_COMPATIBILITY)
    set(sourcesArgs SOURCES_LIST)
    cmake_parse_arguments(PARSE_ARGV 2 PARAMS "" "${oneValueArgs}" "${sourcesArgs}")

    #message(STATUS "[SMTG] PARAMS_UNPARSED_ARGUMENTS is ${PARAMS_UNPARSED_ARGUMENTS}")
    #message(STATUS "[SMTG] PARAMS_MODULEINFO_COMPATIBILITY is ${PARAMS_MODULEINFO_COMPATIBILITY}")
    #message(STATUS "[SMTG] PARAMS_SOURCES_LIST is ${PARAMS_SOURCES_LIST}")

    if(NOT EXISTS ${SMTG_PACKAGE_ICON_PATH})
        set(SMTG_PACKAGE_ICON_PATH ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../templates/VST_Logo_Steinberg.ico)
    endif()
    #message(STATUS "[SMTG] SMTG_PACKAGE_ICON_PATH is ${SMTG_PACKAGE_ICON_PATH}")

    if(DEFINED PARAMS_MODULEINFO_COMPATIBILITY)
        set(SOURCES "${PARAMS_SOURCES_LIST}")
    else()
        set(SOURCES "${PARAMS_UNPARSED_ARGUMENTS}")
    endif()

    add_library(${target} MODULE "${SOURCES}")
  
    smtg_target_set_vst_win_architecture_name(${target})
    smtg_target_make_plugin_package(${target} ${pkg_name} vst3)

    ####################
    # Workaround for Xcode 15 not allowing to add adhoc_codesign linker setting
    ####################
    if(SMTG_MAC AND XCODE_VERSION VERSION_GREATER_EQUAL 15)
	    get_target_property(PLUGIN_PACKAGE_PATH ${target} SMTG_PLUGIN_PACKAGE_PATH)
        add_custom_command(
            TARGET ${target} POST_BUILD
            COMMENT "[SMTG] Adhoc codesign workaround for Xcode 15"
            COMMAND codesign --force --verbose --sign -
                "${PLUGIN_PACKAGE_PATH}"
        )
	endif()
    
    if(SMTG_ENABLE_TARGET_VARS_LOG)
        smtg_target_dump_plugin_package_variables(${target})
    endif(SMTG_ENABLE_TARGET_VARS_LOG)

    if(DEFINED PARAMS_MODULEINFO_COMPATIBILITY)
        smtg_target_create_module_info_file(${target} MODULEINFO_COMPATIBILITY "${PARAMS_MODULEINFO_COMPATIBILITY}")
    else()
        smtg_target_create_module_info_file(${target})
    endif()

    if(SMTG_RUN_VST_VALIDATOR)
        smtg_target_run_vst_validator(${target})
    endif(SMTG_RUN_VST_VALIDATOR)

    if(SMTG_CREATE_PLUGIN_LINK)
        smtg_get_vst3_path()
        smtg_target_create_link_to_plugin(${target} ${SMTG_PLUGIN_TARGET_PATH})
    endif(SMTG_CREATE_PLUGIN_LINK)

endfunction(smtg_add_vst3plugin_with_pkgname)

#------------------------------------------------------------------------
# Adds a VST3 target.
#
# @param target The target to which a VST3 Plug-in will be added. 
# @param PACKAGE_NAME <Name> The name of the package [optional] if not present
# @param MODULEINFO_COMPATIBILITY <path> The path to the compatibility json file [optional]
# @param SOURCES_LIST <sources> List of sources to add to target project [mandatory when PACKAGE_NAME is used]
# for example: 
# smtg_add_vst3plugin(${target} PACKAGE_NAME "A Gain" SOURCES_LIST ${again_sources})
# smtg_add_vst3plugin(${target} PACKAGE_NAME "A Gain" MODULEINFO_COMPATIBILITY compat.json SOURCES_LIST ${again_sources})
# or
# smtg_add_vst3plugin(${target} ${again_sources})
function(smtg_add_vst3plugin target)
    set(oneValueArgs PACKAGE_NAME MODULEINFO_COMPATIBILITY)
    set(sourcesArgs SOURCES_LIST)
    cmake_parse_arguments(PARSE_ARGV 1 PARAMS "${options}" "${oneValueArgs}" "${sourcesArgs}")

    #message(STATUS "[SMTG] PARAMS_UNPARSED_ARGUMENTS is ${PARAMS_UNPARSED_ARGUMENTS}")
    #message(STATUS "[SMTG] PARAMS_PACKAGE_NAME is ${PARAMS_PACKAGE_NAME}")
    #message(STATUS "[SMTG] PARAMS_SOURCES is ${PARAMS_SOURCES_NAME}")
    #message(STATUS "[SMTG] PARAMS_MODULEINFO_COMPATIBILITY is ${PARAMS_MODULEINFO_COMPATIBILITY}")

    set(SOURCES "${PARAMS_SOURCES_LIST}")
    if(SOURCES STREQUAL "")
        set(SOURCES ${PARAMS_UNPARSED_ARGUMENTS})
    endif()
   
    set(pkg_name "${PARAMS_PACKAGE_NAME}")
    if(pkg_name STREQUAL "")
        set(pkg_name ${target})
    endif()

    #message(STATUS "[SMTG] target is ${target}.")
    #message(STATUS "[SMTG] pkg_name is ${pkg_name}.")
    #message(STATUS "[SMTG] SOURCES is ${SOURCES}.")

    if(DEFINED PARAMS_MODULEINFO_COMPATIBILITY)
        set(SMTG_MODULEINFO_COMPATIBILITY "${PARAMS_MODULEINFO_COMPATIBILITY}")
        smtg_add_vst3plugin_with_pkgname(${target} ${pkg_name} MODULEINFO_COMPATIBILITY "${SMTG_MODULEINFO_COMPATIBILITY}" SOURCES_LIST ${SOURCES})
    else()
        smtg_add_vst3plugin_with_pkgname(${target} ${pkg_name} ${SOURCES})
    endif()
endfunction(smtg_add_vst3plugin)

#------------------------------------------------------------------------
# Adds a VST3 target for iOS
#
# @param sign_identity which code signing identity to use
# @param target The target to which a VST3 Plug-in will be added.
function(smtg_add_ios_vst3plugin target sign_identity pkg_name)
    if(SMTG_MAC AND XCODE AND SMTG_ENABLE_IOS_TARGETS)
        add_library(${target} MODULE ${ARGN})
        set_target_properties(${target}
            PROPERTIES 
                XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY  "${SMTG_CODE_SIGN_IDENTITY_IOS}"
                XCODE_ATTRIBUTE_ENABLE_BITCODE      "NO"
				SMTG_DISABLE_CREATE_RESOURCE_FOLDER	1
        )
		if(SMTG_IOS_DEVELOPMENT_TEAM)
			set_target_properties(${target}
				PROPERTIES 
					XCODE_ATTRIBUTE_DEVELOPMENT_TEAM    ${SMTG_IOS_DEVELOPMENT_TEAM}
			)
		endif(SMTG_IOS_DEVELOPMENT_TEAM)
        smtg_target_make_plugin_package(${target} ${pkg_name} vst3)

        if(SMTG_ENABLE_TARGET_VARS_LOG)
            smtg_target_dump_plugin_package_variables(${target})
        endif(SMTG_ENABLE_TARGET_VARS_LOG)

        smtg_target_set_platform_ios(${target})
        get_target_property(PLUGIN_PACKAGE_PATH ${target} SMTG_PLUGIN_PACKAGE_PATH)
        add_custom_command(
            TARGET ${target} POST_BUILD
            COMMENT "[SMTG] Codesign"            
            COMMAND codesign --force --verbose --sign "${sign_identity}"
                "${PLUGIN_PACKAGE_PATH}"
        )

    endif(SMTG_MAC AND XCODE AND SMTG_ENABLE_IOS_TARGETS)
endfunction(smtg_add_ios_vst3plugin)

#------------------------------------------------------------------------
# Deprecated since 3.7.4 -----------------------------
function(smtg_run_vst_validator target)
    message(DEPRECATION "[SMTG] Use smtg_target_run_vst_validator instead of smtg_run_vst_validator")
    smtg_target_run_vst_validator (${target})
endfunction(smtg_run_vst_validator)

# Deprecated since 3.7.4 -----------------------------
function(smtg_add_vst3_resource target input_file)
    message(DEPRECATION "[SMTG] Use smtg_target_add_plugin_resources instead of smtg_add_vst3_resource")
    smtg_target_add_plugin_resources (${target}
        RESOURCES
            ${input_file}
    )
endfunction(smtg_add_vst3_resource)

# Deprecated since 3.7.4 -----------------------------
function(smtg_add_vst3_snapshot target snapshot)
    message(DEPRECATION "[SMTG] Use smtg_target_add_plugin_snapshots instead of smtg_add_vst3_snapshot")
    smtg_target_add_plugin_snapshots (${target}
        RESOURCES
            ${snapshot}
    )
endfunction(smtg_add_vst3_snapshot)
