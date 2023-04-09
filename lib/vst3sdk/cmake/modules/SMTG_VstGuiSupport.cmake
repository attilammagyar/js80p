
#------------------------------------------------------------------------

macro(smtg_add_vstgui_subdirectory vstgui_SOURCE_DIR)
    set(VSTGUI_DISABLE_UNITTESTS 1)
    set(VSTGUI_STANDALONE_EXAMPLES OFF)
    set(VSTGUI_STANDALONE ON)
    set(VSTGUI_TOOLS OFF)

    add_subdirectory(${vstgui_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/VSTGUI.build)
endmacro()

macro(smtg_target_setup_vstgui_deprecated_methods target)
   if(VSTGUI_ENABLE_DEPRECATED_METHODS)
        target_compile_definitions(${target} 
            PUBLIC
                VSTGUI_ENABLE_DEPRECATED_METHODS=1
        )
    else()
        target_compile_definitions(${target} 
            PUBLIC
                VSTGUI_ENABLE_DEPRECATED_METHODS=0
        )
    endif()
endmacro()

macro(smtg_target_prepare_bundle target)
    target_compile_definitions(${target}
        PUBLIC
            SMTG_MODULE_IS_BUNDLE=1
    )

    target_sources(${target}
        PRIVATE
            ${public_sdk_SOURCE_DIR}/source/vst/vstgui_win32_bundle_support.cpp
            ${public_sdk_SOURCE_DIR}/source/vst/vstgui_win32_bundle_support.h
    )
endmacro()

#[=======================================================================[.txt:
  Adds VSTGUI support.
  
  Usage:
    smtg_enable_vstgui_support(VSTGUI_SOURCE_DIR "<path-to-the-vstgui-source-folder>")

  Exports:
    smtg_enable_vstgui_support exports "<path-to-the-vstgui-source-folder>" 
    as SMTG_VSTGUI_SOURCE_DIR to its parent scope.

  Deprecation:
    The usage wihout parameters is depricated.
    SMTG_VSTGUI_ROOT is exported for compatibility reasons and is also depricated.
    Replace "${SMTG_VSTGUI_ROOT}/vstgui4" with "${SMTG_VSTGUI_SOURCE_DIR}".
    The export of "${SMTG_VSTGUI_ROOT}" may be removed in future SDK versions.

#]=======================================================================]
function(smtg_enable_vstgui_support)
    if(${ARGC} GREATER 0)
        set(options)
        set(oneValueArgs VSTGUI_SOURCE_DIR)
        set(multiValueArgs)
        cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
        if(ARG_UNPARSED_ARGUMENTS)
            message(FATAL_ERROR "[SMTG] The following parameters are unrecognized: ${ARG_UNPARSED_ARGUMENTS}")
        endif()
        get_filename_component(vstgui_root "${ARG_VSTGUI_SOURCE_DIR}" DIRECTORY)
        set(SMTG_VSTGUI_ROOT "${vstgui_root}" PARENT_SCOPE)
        set(vstgui_source_dir "${ARG_VSTGUI_SOURCE_DIR}")
    else()
        # See function documentation for more information. 
        message(DEPRECATION "[SMTG] Use smtg_enable_vstgui_support(VSTGUI_SOURCE_DIR \"<vstgui-source-folder>\") "
                            "instead of set(SMTG_VSTGUI_ROOT \"<parrent-of-vstgui-source-dir>\") and smtg_enable_vstgui_support()")
        set(vstgui_source_dir "${SMTG_VSTGUI_ROOT}/vstgui4")
    endif()
    # Export SMTG_VSTGUI_SOURCE_DIR to parent scope
    set(SMTG_VSTGUI_SOURCE_DIR "${vstgui_source_dir}" PARENT_SCOPE)

    message(STATUS "[SMTG] SMTG_VSTGUI_SOURCE_DIR is set to: ${vstgui_source_dir}")

    smtg_add_vstgui_subdirectory(${vstgui_source_dir})

    add_library(vstgui_support STATIC 
        ${vstgui_source_dir}/vstgui/plugin-bindings/vst3groupcontroller.cpp
        ${vstgui_source_dir}/vstgui/plugin-bindings/vst3groupcontroller.h
        ${vstgui_source_dir}/vstgui/plugin-bindings/vst3padcontroller.cpp
        ${vstgui_source_dir}/vstgui/plugin-bindings/vst3padcontroller.h
        ${vstgui_source_dir}/vstgui/plugin-bindings/vst3editor.cpp
        ${vstgui_source_dir}/vstgui/plugin-bindings/vst3editor.h

        ${public_sdk_SOURCE_DIR}/source/vst/vstguieditor.cpp
    )

    target_link_libraries(vstgui_support 
        PUBLIC
            base
            vstgui_uidescription
    )

    target_include_directories(vstgui_support
        PUBLIC
            ${vstgui_source_dir}
    )
    
    target_compile_definitions(vstgui_support
        PUBLIC 
            $<$<CONFIG:Debug>:VSTGUI_LIVE_EDITING>
    )

    smtg_target_setup_vstgui_deprecated_methods(vstgui_support)

    smtg_target_setup_universal_binary(vstgui_support)
    smtg_target_setup_universal_binary(vstgui)
    smtg_target_setup_universal_binary(vstgui_uidescription)

    if(SMTG_WIN AND SMTG_CREATE_BUNDLE_FOR_WINDOWS)
        smtg_target_prepare_bundle(vstgui_support)
    endif()
endfunction()
