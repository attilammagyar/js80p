//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/aaxwrapper/aaxwrapper_description.h
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

#pragma once

#include "base/source/fstring.h"

using namespace Steinberg;

struct AAX_Aux_Desc
{
	const char* mName;
	int32 mChannels; // -1 for same as output channel
};

struct AAX_Meter_Desc
{
	const char* mName;
	uint32 mID;
	uint32 mOrientation; // see AAX_EMeterOrientation
	uint32 mType; // see AAX_EMeterType
};

struct AAX_MIDI_Desc
{
	const char* mName;
	uint32 mMask;
};

struct AAX_Plugin_Desc
{
	const char* mEffectID; // unique for each channel layout as in "com.steinberg.aaxwrapper.mono"
	const char* mName;
	uint32 mPlugInIDNative; // unique for each channel layout
	uint32 mPlugInIDAudioSuite; // unique for each channel layout

	int32 mInputChannels;
	int32 mOutputChannels;
	int32 mSideChainInputChannels;

	AAX_MIDI_Desc* mMIDIports;
	AAX_Aux_Desc* mAuxOutputChannels; // zero terminated
	AAX_Meter_Desc* mMeters;

	uint32 mLatency;
};

struct AAX_Effect_Desc
{
	const char* mManufacturer;
	const char* mProduct;

	uint32 mManufacturerID;
	uint32 mProductID;
	const char* mCategory;
	TUID mVST3PluginID;
	uint32 mVersion;

	const char* mPageFile;

	AAX_Plugin_Desc* mPluginDesc;
};

// reference this in the Plug-In to force inclusion of the wrapper in the link
extern int AAXWrapper_linkAnchor;

AAX_Effect_Desc* AAXWrapper_GetDescription (); // to be defined by the Plug-In

/// \endcond
