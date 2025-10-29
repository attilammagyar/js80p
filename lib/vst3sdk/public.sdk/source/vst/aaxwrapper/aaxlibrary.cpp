//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/aaxwrapper/aaxlibrary.cpp
// Created by  : Steinberg, 08/2017
// Description : VST 3 -> AAX Wrapper
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

/// \cond ignore

// instead of linking to a library, we just include the sources here to have
// full control over compile settings

#define I18N_LIB 1
#define PLUGIN_SDK_BUILD 1
#define DPA_PLUGIN_BUILD 1
#define INITACFIDS // Make sure all of the AVX2 uids are defined.
#define UNICODE 1

#ifdef _WIN32
#ifndef WIN32
#define WIN32 // for CMutex.cpp
#endif
#define WINDOWS_VERSION 1 // for AAXWrapper_GUI.h
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreorder"
#pragma clang diagnostic ignored "-Wundef-prefix"
#endif

#include "AAX_Atomic.h"

#include "../Interfaces/ACF/CACFClassFactory.cpp"
#include "../Libs/AAXLibrary/source/AAX_CACFUnknown.cpp"
#include "../Libs/AAXLibrary/source/AAX_CChunkDataParser.cpp"
#include "../Libs/AAXLibrary/source/AAX_CEffectDirectData.cpp"
#include "../Libs/AAXLibrary/source/AAX_CEffectGUI.cpp"
#include "../Libs/AAXLibrary/source/AAX_CEffectParameters.cpp"
#include "../Libs/AAXLibrary/source/AAX_CHostProcessor.cpp"
#include "../Libs/AAXLibrary/source/AAX_CHostServices.cpp"
#include "../Libs/AAXLibrary/source/AAX_CMutex.cpp"
#include "../Libs/AAXLibrary/source/AAX_CPacketDispatcher.cpp"
#include "../Libs/AAXLibrary/source/AAX_CParameter.cpp"
#include "../Libs/AAXLibrary/source/AAX_CParameterManager.cpp"
#include "../Libs/AAXLibrary/source/AAX_CString.cpp"
#include "../Libs/AAXLibrary/source/AAX_CUIDs.cpp"
#include "../Libs/AAXLibrary/source/AAX_CommonConversions.cpp"
#include "../Libs/AAXLibrary/source/AAX_IEffectDirectData.cpp"
#include "../Libs/AAXLibrary/source/AAX_IEffectGUI.cpp"
#include "../Libs/AAXLibrary/source/AAX_IEffectParameters.cpp"
#include "../Libs/AAXLibrary/source/AAX_IHostProcessor.cpp"
#include "../Libs/AAXLibrary/source/AAX_Init.cpp"
#include "../Libs/AAXLibrary/source/AAX_Properties.cpp"
#include "../Libs/AAXLibrary/source/AAX_VAutomationDelegate.cpp"
#include "../Libs/AAXLibrary/source/AAX_VCollection.cpp"
#include "../Libs/AAXLibrary/source/AAX_VComponentDescriptor.cpp"
#include "../Libs/AAXLibrary/source/AAX_VController.cpp"
#include "../Libs/AAXLibrary/source/AAX_VDescriptionHost.cpp"
#include "../Libs/AAXLibrary/source/AAX_VEffectDescriptor.cpp"
#include "../Libs/AAXLibrary/source/AAX_VFeatureInfo.cpp"
#include "../Libs/AAXLibrary/source/AAX_VHostProcessorDelegate.cpp"
#include "../Libs/AAXLibrary/source/AAX_VHostServices.cpp"
#include "../Libs/AAXLibrary/source/AAX_VPageTable.cpp"
#include "../Libs/AAXLibrary/source/AAX_VPrivateDataAccess.cpp"
#include "../Libs/AAXLibrary/source/AAX_VPropertyMap.cpp"
#include "../Libs/AAXLibrary/source/AAX_VTransport.cpp"
#include "../Libs/AAXLibrary/source/AAX_VViewContainer.cpp"

#ifdef _WIN32
#include "../Libs/AAXLibrary/source/AAX_CAutoreleasePool.Win.cpp"
#else
//#include "../Libs/AAXLibrary/source/AAX_CAutoreleasePool.OSX.mm"
#endif

#undef min
#undef max

// put at the very end, uses "using namespace std"
#include "../Libs/AAXLibrary/source/AAX_SliderConversions.cpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

/// \endcond
