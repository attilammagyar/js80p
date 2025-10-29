//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/auwrapper/NSDataIBStream.h
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

#pragma once

#import <Foundation/Foundation.h>
#import "pluginterfaces/base/ibstream.h"
#import "public.sdk/source/vst/hosting/hostclasses.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
class NSDataIBStream : public IBStream, Vst::IStreamAttributes
{
public:
	NSDataIBStream (NSData* data, bool hideAttributes = false);
	virtual ~NSDataIBStream ();

	//---from IBStream-------------------
	tresult PLUGIN_API read  (void* buffer, int32 numBytes, int32* numBytesRead = 0) SMTG_OVERRIDE;
	tresult PLUGIN_API write (void* buffer, int32 numBytes, int32* numBytesWritten = 0) SMTG_OVERRIDE;
	tresult PLUGIN_API seek  (int64 pos, int32 mode, int64* result = 0) SMTG_OVERRIDE;
	tresult PLUGIN_API tell  (int64* pos) SMTG_OVERRIDE;

	//---from Vst::IStreamAttributes-----
	tresult PLUGIN_API getFileName (String128 name) SMTG_OVERRIDE;
	IAttributeList* PLUGIN_API getAttributes () SMTG_OVERRIDE;

	//------------------------------------------------------------------------
	DECLARE_FUNKNOWN_METHODS
protected:
	NSData* data;
	int64 currentPos;
	IPtr<IAttributeList> attrList;
	bool hideAttributes;
};

//------------------------------------------------------------------------
class NSMutableDataIBStream : public NSDataIBStream
{
public:
	NSMutableDataIBStream (NSMutableData* data);
	virtual ~NSMutableDataIBStream ();

	tresult PLUGIN_API write (void* buffer, int32 numBytes, int32* numBytesWritten = 0) SMTG_OVERRIDE;
//------------------------------------------------------------------------
protected:
	NSMutableData* mdata;
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

/// \endcond
