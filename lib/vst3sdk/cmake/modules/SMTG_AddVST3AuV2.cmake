
include(SMTG_Bundle)
include(SMTG_UniversalBinary)
include(SMTG_CodeSign)

if(XCODE AND SMTG_ENABLE_AUV2_BUILDS)

    set(SMTG_AUV2_FOLDER FOLDER "AudioUnit V2")

    #------------------------------------------------------------------------
    # Add an AudioUnit Version 2 target for macOS
    # @param target                 target name
    # @param BUNDLE_NAME            name of the bundle
    # @param BUNDLE_IDENTIFIER      bundle identifier
    # @param INFO_PLIST_TEMPLATE    the info.plist file containing the needed AudioUnit keys
    # @param VST3_PLUGIN_TARGET     the vst3 plugin target
    #------------------------------------------------------------------------
    function(smtg_target_add_auv2 target)

        set(oneValueArgs
            BUNDLE_NAME
            BUNDLE_IDENTIFIER
            INFO_PLIST_TEMPLATE
            VST3_PLUGIN_TARGET
        )

        cmake_parse_arguments(ARG "" "${oneValueArgs}" "" ${ARGN})

        if(ARG_UNPARSED_ARGUMENTS)
            message(FATAL_ERROR "[SMTG] The following parameters are unrecognized: ${ARG_UNPARSED_ARGUMENTS}")
        endif(ARG_UNPARSED_ARGUMENTS)

        if(ARG_KEYWORDS_MISSING_VALUES)
            message(FATAL_ERROR "[SMTG] The following parameter values are missing: ${ARG_KEYWORDS_MISSING_VALUES}")
        endif(ARG_KEYWORDS_MISSING_VALUES)

        foreach(ARG_NAME ${oneValueArgs})
            set(tmp ARG_${ARG_NAME})
            if(NOT ${tmp})
                message(FATAL_ERROR "[SMTG] Missing ${ARG_NAME} argument and value")
            endif()
        endforeach()

        string(TIMESTAMP CocoaIdStamp "%Y%j%H%M%S")
        string(MAKE_C_IDENTIFIER "SMTG_AUCocoaUIBase_${ARG_BUNDLE_NAME}${CocoaIdStamp}" SMTG_AUCocoaUIBase_CLASS_NAME)

        add_library(${target} MODULE 
            ${public_sdk_SOURCE_DIR}/source/vst/auwrapper/aucocoaview.mm
            ${public_sdk_SOURCE_DIR}/source/vst/auwrapper/aucocoaview.h
            ${public_sdk_SOURCE_DIR}/source/vst/auwrapper/auwrapper.mm
            ${public_sdk_SOURCE_DIR}/source/vst/auwrapper/auwrapper.h
            ${public_sdk_SOURCE_DIR}/source/vst/auwrapper/NSDataIBStream.mm
            ${public_sdk_SOURCE_DIR}/source/vst/auwrapper/NSDataIBStream.h
        )

        smtg_target_setup_universal_binary(${target})
        smtg_target_codesign(${target} ${SMTG_IOS_DEVELOPMENT_TEAM} "${SMTG_CODE_SIGN_IDENTITY_MAC}")
      
        target_compile_features(${target}
            PUBLIC
                cxx_std_17
        )

        target_compile_definitions(${target}
            PRIVATE
                SMTG_AUCocoaUIBase_CLASS_NAME=${SMTG_AUCocoaUIBase_CLASS_NAME}
                CA_USE_AUDIO_PLUGIN_ONLY=0 # TODO: If true will let instruments fail validation
        )

        target_link_libraries(${target} 
            PRIVATE 
                sdk_hosting 
                "-framework AudioUnit" 
                "-framework CoreMIDI"
                "-framework AudioToolbox"
                "-framework CoreFoundation"
                "-framework Carbon" 
                "-framework Cocoa" 
                "-framework CoreAudio"
        )
        if(NOT ${SMTG_COREAUDIO_SDK_PATH} STREQUAL "")
            target_sources(${target}
                PRIVATE
                    "${public_sdk_SOURCE_DIR}/source/vst/auwrapper/ausdk.mm"
            )
            target_include_directories(${target}
                PRIVATE 
                    "${SMTG_COREAUDIO_SDK_PATH}/**"
            )
        elseif(NOT ${SMTG_AUDIOUNIT_SDK_PATH} STREQUAL "")
            target_compile_definitions(${target}
                PRIVATE
                    SMTG_AUWRAPPER_USES_AUSDK
            )
## Adding the xcodeproj will crash Xcode when closing and reopening the cmake generated project
#           target_sources(${target} PRIVATE "${SMTG_AUDIOUNIT_SDK_PATH}/AudioUnitSDK.xcodeproj")
            target_include_directories(${target}
                PRIVATE 
                    "${SMTG_AUDIOUNIT_SDK_PATH}/include/**"
            )
            add_custom_command(TARGET ${target}
                PRE_BUILD
                COMMAND "xcodebuild" -project "${SMTG_AUDIOUNIT_SDK_PATH}/AudioUnitSDK.xcodeproj" -target AudioUnitSDK build -configuration Release SYMROOT=${CMAKE_BINARY_DIR}/AudioUnitSDK/
            )
            target_link_libraries(${target} 
                PRIVATE 
                    "${CMAKE_BINARY_DIR}/AudioUnitSDK/Release/libAudioUnitSDK.a"
            )
        endif()

        smtg_target_set_bundle(${target}
            INFOPLIST ${ARG_INFO_PLIST_TEMPLATE}
            EXTENSION component
        )

        set(VST3_OUTPUT_DIR ${CMAKE_BINARY_DIR}/VST3)
        set(outputdir ${VST3_OUTPUT_DIR}/$<CONFIG>)
        set_target_properties(${target}
            PROPERTIES
                XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE "YES"
                XCODE_ATTRIBUTE_PRODUCT_NAME "${ARG_BUNDLE_NAME}"
                XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${ARG_BUNDLE_IDENTIFIER}
                LIBRARY_OUTPUT_DIRECTORY ${VST3_OUTPUT_DIR}
                ${SMTG_AUV2_FOLDER}
        )

        add_dependencies(${target} ${ARG_VST3_PLUGIN_TARGET})

        add_custom_command(TARGET ${target} POST_BUILD 
            COMMAND /bin/mkdir "-p" ${outputdir}/${ARG_BUNDLE_NAME}.component/Contents/Resources
            COMMAND /bin/rm "-f" "${outputdir}/${ARG_BUNDLE_NAME}.component/Contents/Resources/plugin.vst3"
            COMMAND /bin/ln "-svfF" "${outputdir}/$<TARGET_FILE_NAME:${ARG_VST3_PLUGIN_TARGET}>.vst3" "${outputdir}/${ARG_BUNDLE_NAME}.component/Contents/Resources/plugin.vst3"
            COMMAND /bin/rm "-rf" "~/Library/Audio/Plug-Ins/Components/${ARG_BUNDLE_NAME}.component"
            COMMAND /bin/cp "-rpf" "${outputdir}/${ARG_BUNDLE_NAME}.component" "~/Library/Audio/Plug-Ins/Components/"
            BYPRODUCTS "~/Library/Audio/Plug-Ins/Components/${ARG_BUNDLE_NAME}.component"
        )
        smtg_target_add_custom_codesign_post_build_step(
            TARGET
                "${target}"
            IDENTITY
                ${SMTG_CODE_SIGN_IDENTITY_MAC}
            PATH
                "~/Library/Audio/Plug-Ins/Components/${ARG_BUNDLE_NAME}.component"
            FLAGS
            	--timestamp
        )

    endfunction(smtg_target_add_auv2)
else()
    message("[SMTG] * To add an AudioUnit v2 target, you need to use the Xcode generator and set SMTG_COREAUDIO_SDK_PATH to the path of your installation of the CoreAudio SDK!")
endif(XCODE AND SMTG_ENABLE_AUV2_BUILDS)
