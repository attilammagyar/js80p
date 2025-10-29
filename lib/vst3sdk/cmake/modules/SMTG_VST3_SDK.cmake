
#------------------------------------------------------------------------
# Includes
#------------------------------------------------------------------------
include(SMTG_Global)
include(SMTG_AddVST3Library)
include(SMTG_Bundle)
include(SMTG_ExportedSymbols)
include(SMTG_PrefixHeader)
include(SMTG_PlatformIOS)
include(SMTG_PlatformToolset)
include(SMTG_CoreAudioSupport)
include(SMTG_AAXSupport)
include(SMTG_VstGuiSupport)
include(SMTG_UniversalBinary)
include(SMTG_AddVST3Options)

#------------------------------------------------------------------------
function(smtg_create_lib_base_target)

    add_library(base
        STATIC
            ${SDK_ROOT}/base/source/baseiids.cpp
            ${SDK_ROOT}/base/source/classfactoryhelpers.h
            ${SDK_ROOT}/base/source/fbuffer.cpp
            ${SDK_ROOT}/base/source/fbuffer.h
            ${SDK_ROOT}/base/source/fcleanup.h
            ${SDK_ROOT}/base/source/fcommandline.h
            ${SDK_ROOT}/base/source/fdebug.cpp
            ${SDK_ROOT}/base/source/fdebug.h
            ${SDK_ROOT}/base/source/fdynlib.cpp
            ${SDK_ROOT}/base/source/fdynlib.h
            ${SDK_ROOT}/base/source/fobject.cpp
            ${SDK_ROOT}/base/source/fobject.h
            ${SDK_ROOT}/base/source/fstreamer.cpp
            ${SDK_ROOT}/base/source/fstreamer.h
            ${SDK_ROOT}/base/source/fstring.cpp
            ${SDK_ROOT}/base/source/fstring.h
            ${SDK_ROOT}/base/source/timer.cpp
            ${SDK_ROOT}/base/source/timer.h
            ${SDK_ROOT}/base/source/updatehandler.cpp
            ${SDK_ROOT}/base/source/updatehandler.h
            ${SDK_ROOT}/base/thread/include/fcondition.h
            ${SDK_ROOT}/base/thread/include/flock.h
            ${SDK_ROOT}/base/thread/source/fcondition.cpp
            ${SDK_ROOT}/base/thread/source/flock.cpp
    )

    target_include_directories(base
        PUBLIC
            ${SDK_ROOT}
    )

    # Compiler switches are PUBLIC and will be populated.
    target_compile_options(base 
        PUBLIC
            "$<$<CONFIG:Debug>:-DDEVELOPMENT=1>"
            "$<$<CONFIG:Release>:-DRELEASE=1>"
            "$<$<CONFIG:RelWithDebInfo>:-DRELEASE=1>"
    )
    
    target_compile_features(base
        PUBLIC
            cxx_std_17
    )
    
    if(MSVC)
        set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /IGNORE:4221")
    endif(MSVC)
    
    smtg_target_setup_universal_binary(base)
    
    # iOS target
    if(SMTG_MAC AND XCODE AND SMTG_ENABLE_IOS_TARGETS)
        smtg_create_ios_target_from_target(base)
        set_target_properties(base_ios
            PROPERTIES
                ${SDK_IDE_LIBS_FOLDER}
        )
        # Compiler switches are PUBLIC and will be populated.
        target_compile_options(base_ios 
            PUBLIC
                "$<$<CONFIG:Debug>:-DDEVELOPMENT=1>"
                "$<$<CONFIG:Release>:-DRELEASE=1>"
                "$<$<CONFIG:RelWithDebInfo>:-DRELEASE=1>"
        )
    endif(SMTG_MAC AND XCODE AND SMTG_ENABLE_IOS_TARGETS)

endfunction()

#------------------------------------------------------------------------
function(smtg_create_pluginterfaces_target)

    add_library(pluginterfaces
        STATIC
            ${SDK_ROOT}/pluginterfaces/base/conststringtable.cpp
            ${SDK_ROOT}/pluginterfaces/base/conststringtable.h
            ${SDK_ROOT}/pluginterfaces/base/coreiids.cpp
            ${SDK_ROOT}/pluginterfaces/base/falignpop.h
            ${SDK_ROOT}/pluginterfaces/base/falignpush.h
            ${SDK_ROOT}/pluginterfaces/base/fplatform.h
            ${SDK_ROOT}/pluginterfaces/base/fstrdefs.h
            ${SDK_ROOT}/pluginterfaces/base/ftypes.h
            ${SDK_ROOT}/pluginterfaces/base/funknown.cpp
            ${SDK_ROOT}/pluginterfaces/base/funknown.h
            ${SDK_ROOT}/pluginterfaces/base/funknownimpl.h
            ${SDK_ROOT}/pluginterfaces/base/futils.h
            ${SDK_ROOT}/pluginterfaces/base/fvariant.h
            ${SDK_ROOT}/pluginterfaces/base/geoconstants.h
            ${SDK_ROOT}/pluginterfaces/base/ibstream.h
            ${SDK_ROOT}/pluginterfaces/base/icloneable.h
            ${SDK_ROOT}/pluginterfaces/base/ierrorcontext.h
            ${SDK_ROOT}/pluginterfaces/base/ipersistent.h
            ${SDK_ROOT}/pluginterfaces/base/ipluginbase.h
            ${SDK_ROOT}/pluginterfaces/base/istringresult.h
            ${SDK_ROOT}/pluginterfaces/base/iupdatehandler.h
            ${SDK_ROOT}/pluginterfaces/base/keycodes.h
            ${SDK_ROOT}/pluginterfaces/base/pluginbasefwd.h
            ${SDK_ROOT}/pluginterfaces/base/smartpointer.h
            ${SDK_ROOT}/pluginterfaces/base/typesizecheck.h
            ${SDK_ROOT}/pluginterfaces/base/ucolorspec.h
            ${SDK_ROOT}/pluginterfaces/base/ustring.cpp
            ${SDK_ROOT}/pluginterfaces/base/ustring.h
            ${SDK_ROOT}/pluginterfaces/gui/iplugview.h
            ${SDK_ROOT}/pluginterfaces/gui/iplugviewcontentscalesupport.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstattributes.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstaudioprocessor.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstautomationstate.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstchannelcontextinfo.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstcomponent.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstcontextmenu.h
            ${SDK_ROOT}/pluginterfaces/vst/ivsteditcontroller.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstevents.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstdataexchange.h
            ${SDK_ROOT}/pluginterfaces/vst/ivsthostapplication.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstinterappaudio.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstmessage.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstmidicontrollers.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstmidilearn.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstnoteexpression.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstparameterchanges.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstparameterfunctionname.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstphysicalui.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstpluginterfacesupport.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstplugview.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstprefetchablesupport.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstprocesscontext.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstremapparamid.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstrepresentation.h
            ${SDK_ROOT}/pluginterfaces/vst/ivstunits.h
            ${SDK_ROOT}/pluginterfaces/vst/vstpresetkeys.h
            ${SDK_ROOT}/pluginterfaces/vst/vstpshpack4.h
            ${SDK_ROOT}/pluginterfaces/vst/vstspeaker.h
            ${SDK_ROOT}/pluginterfaces/vst/vsttypes.h
    )
    
    # check for C11 atomic header
    include(CheckSourceCompiles)
    set(SMTG_CHECK_STDATOMIC_H_SRC
    "#include <stdatomic.h>
    int main () { 
        atomic_int_least32_t value = 0;
        atomic_fetch_add (&value, 1); 
        return 0; 
    }"
    )
    #set(CMAKE_REQUIRED_QUIET 1)
    check_source_compiles(CXX "${SMTG_CHECK_STDATOMIC_H_SRC}" SMTG_USE_STDATOMIC_H)
    if(SMTG_USE_STDATOMIC_H)
        target_compile_definitions(pluginterfaces
            PRIVATE
                "SMTG_USE_STDATOMIC_H=${SMTG_USE_STDATOMIC_H}"
        )
    endif(SMTG_USE_STDATOMIC_H)
    
    target_include_directories(pluginterfaces
        PUBLIC
            ${SDK_ROOT}
    )
    
    target_compile_features(pluginterfaces
        PUBLIC
            cxx_std_17
    )
    
    smtg_target_setup_universal_binary(pluginterfaces)
    
    # iOS target
    if(SMTG_MAC AND XCODE AND SMTG_ENABLE_IOS_TARGETS)
        smtg_create_ios_target_from_target(pluginterfaces)
        set_target_properties(pluginterfaces_ios
            PROPERTIES
                ${SDK_IDE_LIBS_FOLDER}
        )
    endif()

endfunction()

#------------------------------------------------------------------------
function(smtg_create_public_sdk_common_target)

    add_library(sdk_common
        STATIC
            ${SDK_ROOT}/public.sdk/source/common/commoniids.cpp
            ${SDK_ROOT}/public.sdk/source/common/commonstringconvert.cpp
            ${SDK_ROOT}/public.sdk/source/common/commonstringconvert.h
            ${SDK_ROOT}/public.sdk/source/common/openurl.cpp
            ${SDK_ROOT}/public.sdk/source/common/openurl.h
            ${SDK_ROOT}/public.sdk/source/common/systemclipboard.h
            ${SDK_ROOT}/public.sdk/source/common/systemclipboard_linux.cpp
            ${SDK_ROOT}/public.sdk/source/common/systemclipboard_win32.cpp
            ${SDK_ROOT}/public.sdk/source/common/threadchecker.h
            ${SDK_ROOT}/public.sdk/source/common/threadchecker_linux.cpp
            ${SDK_ROOT}/public.sdk/source/common/threadchecker_win32.cpp
            ${SDK_ROOT}/public.sdk/source/common/readfile.cpp
            ${SDK_ROOT}/public.sdk/source/common/readfile.h
            ${SDK_ROOT}/public.sdk/source/vst/vstpresetfile.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vstpresetfile.h
    )

    if(SMTG_MAC)
        target_sources(sdk_common 
            PRIVATE
                ${SDK_ROOT}/public.sdk/source/common/threadchecker_mac.mm
                ${SDK_ROOT}/public.sdk/source/common/systemclipboard_mac.mm
        )
    endif(SMTG_MAC)

    # add dependencies
    target_link_libraries(sdk_common 
        PUBLIC 
            base
            pluginterfaces
    )
    target_include_directories(sdk_common
        PUBLIC
            ${SDK_ROOT}
    )
    smtg_target_setup_universal_binary(sdk_common)

    if(SMTG_MAC AND XCODE AND SMTG_ENABLE_IOS_TARGETS)
        smtg_create_ios_target_from_target(sdk_common)
        set_target_properties(sdk_common_ios
            PROPERTIES
                ${SDK_IDE_LIBS_FOLDER}
        )
        target_link_libraries(sdk_common_ios 
            PUBLIC 
                base_ios
                pluginterfaces_ios
        )
    endif()

endfunction()

#------------------------------------------------------------------------
function(smtg_create_public_sdk_target)

    add_library(sdk
        STATIC 
            ${SDK_ROOT}/public.sdk/source/common/pluginview.cpp
            ${SDK_ROOT}/public.sdk/source/common/pluginview.h
            ${SDK_ROOT}/public.sdk/source/main/pluginfactory.cpp
            ${SDK_ROOT}/public.sdk/source/main/pluginfactory.h
            ${SDK_ROOT}/public.sdk/source/main/pluginfactory_constexpr.h
            ${SDK_ROOT}/public.sdk/source/main/moduleinit.cpp
            ${SDK_ROOT}/public.sdk/source/main/moduleinit.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/alignedalloc.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/audiobuffers.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/dataexchange.cpp
            ${SDK_ROOT}/public.sdk/source/vst/utility/dataexchange.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/memoryibstream.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/processcontextrequirements.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/processdataslicer.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/ringbuffer.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/rttransfer.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/sampleaccurate.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/stringconvert.cpp
            ${SDK_ROOT}/public.sdk/source/vst/utility/stringconvert.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/systemtime.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/systemtime.cpp
            ${SDK_ROOT}/public.sdk/source/vst/utility/testing.cpp
            ${SDK_ROOT}/public.sdk/source/vst/utility/testing.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/vst2persistence.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/vst2persistence.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vstaudioeffect.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vstaudioeffect.h
            ${SDK_ROOT}/public.sdk/source/vst/vstbus.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vstbus.h
            ${SDK_ROOT}/public.sdk/source/vst/vstbypassprocessor.h
            ${SDK_ROOT}/public.sdk/source/vst/vstcomponent.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vstcomponent.h
            ${SDK_ROOT}/public.sdk/source/vst/vstcomponentbase.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vstcomponentbase.h
            ${SDK_ROOT}/public.sdk/source/vst/vsteditcontroller.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vsteditcontroller.h
            ${SDK_ROOT}/public.sdk/source/vst/vsteventshelper.h
            ${SDK_ROOT}/public.sdk/source/vst/vsthelpers.h
            ${SDK_ROOT}/public.sdk/source/vst/vstinitiids.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vstnoteexpressiontypes.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vstnoteexpressiontypes.h
            ${SDK_ROOT}/public.sdk/source/vst/vstparameters.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vstparameters.h
            ${SDK_ROOT}/public.sdk/source/vst/vstrepresentation.cpp
            ${SDK_ROOT}/public.sdk/source/vst/vstrepresentation.h
    )

    target_compile_features(sdk
        PUBLIC
            cxx_std_17
    )
    
    # add dependencies
    target_link_libraries(sdk 
        PUBLIC 
            sdk_common
    )
    target_include_directories(sdk
        PUBLIC
            ${SDK_ROOT}
    )
    smtg_target_setup_universal_binary(sdk)
    get_target_property(sdk_SOURCES sdk SOURCES)
    
    # iOS target
    if(SMTG_MAC AND XCODE AND SMTG_ENABLE_IOS_TARGETS)
        smtg_create_ios_target_from_target(sdk)
        set_target_properties(sdk_ios
            PROPERTIES
                ${SDK_IDE_LIBS_FOLDER}
        )
        target_link_libraries(sdk_ios
            PUBLIC 
                sdk_common_ios
        )
    endif(SMTG_MAC AND XCODE AND SMTG_ENABLE_IOS_TARGETS)

endfunction()

#------------------------------------------------------------------------
function(smtg_create_public_sdk_hosting_target)

    add_library(sdk_hosting 
        STATIC
            ${SDK_ROOT}/public.sdk/source/vst/hosting/connectionproxy.cpp
            ${SDK_ROOT}/public.sdk/source/vst/hosting/connectionproxy.h
            ${SDK_ROOT}/public.sdk/source/vst/hosting/eventlist.cpp
            ${SDK_ROOT}/public.sdk/source/vst/hosting/eventlist.h
            ${SDK_ROOT}/public.sdk/source/vst/hosting/hostclasses.cpp
            ${SDK_ROOT}/public.sdk/source/vst/hosting/hostclasses.h
            ${SDK_ROOT}/public.sdk/source/vst/hosting/module.cpp
            ${SDK_ROOT}/public.sdk/source/vst/hosting/module.h
            ${SDK_ROOT}/public.sdk/source/vst/hosting/parameterchanges.cpp
            ${SDK_ROOT}/public.sdk/source/vst/hosting/parameterchanges.h
            ${SDK_ROOT}/public.sdk/source/vst/hosting/pluginterfacesupport.cpp
            ${SDK_ROOT}/public.sdk/source/vst/hosting/pluginterfacesupport.h
            ${SDK_ROOT}/public.sdk/source/vst/hosting/processdata.cpp
            ${SDK_ROOT}/public.sdk/source/vst/hosting/processdata.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/optional.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/stringconvert.cpp
            ${SDK_ROOT}/public.sdk/source/vst/utility/stringconvert.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/uid.h
            ${SDK_ROOT}/public.sdk/source/vst/utility/versionparser.h
            ${SDK_ROOT}/public.sdk/source/vst/vstinitiids.cpp
    )
    # add dependencies
    target_link_libraries(sdk_hosting 
        PUBLIC 
            sdk_common
    )
    target_include_directories(sdk_hosting
        PUBLIC
            ${SDK_ROOT}
    )
    smtg_target_setup_universal_binary(sdk_hosting)

    # iOS target for hosting
    if(SMTG_MAC AND XCODE AND SMTG_ENABLE_IOS_TARGETS)
        smtg_create_ios_target_from_target(sdk_hosting)
        set_target_properties(sdk_hosting_ios
            PROPERTIES
                ${SDK_IDE_LIBS_FOLDER}
        )
        target_link_libraries(sdk_hosting_ios 
            PUBLIC 
                sdk_common_ios
        )
        target_include_directories(sdk_hosting_ios
            PUBLIC
                ${SDK_ROOT}
        )
    endif(SMTG_MAC AND XCODE AND SMTG_ENABLE_IOS_TARGETS)

endfunction()
