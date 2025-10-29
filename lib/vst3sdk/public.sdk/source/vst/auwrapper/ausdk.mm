//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/auwrapper/ausdk.mm
// Created by  : Steinberg, 12/2007
// Description : VST 3 -> AU Wrapper
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------
/// \cond ignore

#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wunused-value"
#pragma clang diagnostic ignored "-Wparentheses"
#pragma clang diagnostic ignored "-Woverloaded-virtual"

#ifndef MAC_OS_X_VERSION_10_7
	#define MAC_OS_X_VERSION_10_7 1070
#endif

#import "PublicUtility/CAAudioChannelLayout.cpp"
#import "PublicUtility/CABundleLocker.cpp"
#import "PublicUtility/CAHostTimeBase.cpp"
#import "PublicUtility/CAStreamBasicDescription.cpp"
#import "PublicUtility/CAVectorUnit.cpp"
#import "PublicUtility/CAAUParameter.cpp"

#import "AUPublic/AUBase/ComponentBase.cpp"
#import "AUPublic/AUBase/AUScopeElement.cpp"
#import "AUPublic/AUBase/AUOutputElement.cpp"
#import "AUPublic/AUBase/AUInputElement.cpp"
#import "AUPublic/AUBase/AUBase.cpp"

#if !__LP64__
	#ifndef verify_noerr
		#define verify_noerr(x) x
	#endif
	#ifndef verify
		#define verify(x)
	#endif
	#import "AUPublic/AUCarbonViewBase/AUCarbonViewBase.cpp"
	#import "AUPublic/AUCarbonViewBase/AUCarbonViewControl.cpp"
	#import "AUPublic/AUCarbonViewBase/AUCarbonViewDispatch.cpp"
	#import "AUPublic/AUCarbonViewBase/AUControlGroup.cpp"
	#import "AUPublic/AUCarbonViewBase/CarbonEventHandler.cpp"
#endif

#import "AUPublic/Utility/AUTimestampGenerator.cpp"
#import "AUPublic/Utility/AUBuffer.cpp"
#import "AUPublic/Utility/AUBaseHelper.cpp"

#if MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7
	#import "AUPublic/OtherBases/AUMIDIEffectBase.cpp"
	#import "AUPublic/Utility/AUDebugDispatcher.cpp"
#else
	#import "AUPublic/AUBase/AUPlugInDispatch.cpp"
#endif

#if !CA_USE_AUDIO_PLUGIN_ONLY
	#import "AUPublic/AUBase/AUDispatch.cpp"
	#import "AUPublic/OtherBases/MusicDeviceBase.cpp"
	#import "AUPublic/OtherBases/AUMIDIBase.cpp"
	#import "AUPublic/OtherBases/AUEffectBase.cpp"
#endif

/// \endcond
