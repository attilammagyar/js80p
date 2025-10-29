//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/auwrapper/auresource.r
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

#include <AudioUnit/AudioUnit.r>
#include <AudioUnit/AudioUnitCarbonView.r>

#include "audiounitconfig.h"
/*	----------------------------------------------------------------------------------------------------------------------------------------
//	audiounitconfig.h needs the following definitions:
	#define kAudioUnitVersion			0xFFFFFFFF							// Version Number, needs to be in hex
	#define kAudioUnitName				"Steinberg: MyVST3 as AudioUnit"	// Company Name + Effect Name
	#define kAudioUnitDescription		"My VST3 as AudioUnit"				// Effect Description
	#define kAudioUnitType				kAudioUnitType_Effect				// can be kAudioUnitType_Effect or kAudioUnitType_MusicDevice
	#define kAudioUnitComponentSubType	'test'								// unique id
	#define kAudioUnitComponentManuf	'SMTG'								// registered company id
	#define kAudioUnitCarbonView		1									// if 0 no Carbon view support will be added
*/


#define kAudioUnitResID_Processor				1000
#define kAudioUnitResID_CarbonView				9000

//----------------------Processor----------------------------------------------

#define RES_ID			kAudioUnitResID_Processor
#define COMP_TYPE		kAudioUnitType
#define COMP_SUBTYPE	kAudioUnitComponentSubType
#define COMP_MANUF		kAudioUnitComponentManuf	

#define VERSION			kAudioUnitVersion
#define NAME			kAudioUnitName
#define DESCRIPTION		kAudioUnitDescription
#define ENTRY_POINT		"AUWrapperEntry"

#include "AUResources.r"

#if kAudioUnitCarbonView
//----------------------View----------------------------------------------

#define RES_ID			kAudioUnitResID_CarbonView
#define COMP_TYPE		kAudioUnitCarbonViewComponentType
#define COMP_SUBTYPE	kAudioUnitComponentSubType
#define COMP_MANUF		kAudioUnitComponentManuf	

#define VERSION			kAudioUnitVersion
#define NAME			"CarbonView"
#define DESCRIPTION		"CarbonView"
#define ENTRY_POINT		"AUCarbonViewEntry"

#include "AUResources.r"

#endif
