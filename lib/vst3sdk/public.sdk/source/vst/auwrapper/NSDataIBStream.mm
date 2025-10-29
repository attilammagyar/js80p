//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/auwrapper/NSDataIBStream.mm
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

#include "NSDataIBStream.h"

#include "pluginterfaces/vst/ivstattributes.h"

#include <algorithm>

#if __clang__
#if __has_feature(objc_arc) && __clang_major__ >= 3
#define ARC_ENABLED 1
#endif // __has_feature(objc_arc)
#endif // __clang__

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
NSDataIBStream::NSDataIBStream (NSData* data, bool hideAttributes)
: data (data)
, currentPos (0)
, hideAttributes (hideAttributes)
{
	FUNKNOWN_CTOR
	if (!hideAttributes)
		attrList = HostAttributeList::make ();
#if !ARC_ENABLED
	[data retain];
#endif
}

//------------------------------------------------------------------------
NSDataIBStream::~NSDataIBStream ()
{
#if !ARC_ENABLED
	[data release];
#endif
	FUNKNOWN_DTOR
}

//------------------------------------------------------------------------
IMPLEMENT_REFCOUNT (NSDataIBStream)

//------------------------------------------------------------------------
tresult PLUGIN_API NSDataIBStream::queryInterface (const TUID iid, void** obj)
{
	QUERY_INTERFACE (iid, obj, FUnknown::iid, IBStream)
	QUERY_INTERFACE (iid, obj, IBStream::iid, IBStream)
	if (!hideAttributes)
		QUERY_INTERFACE (iid, obj, IStreamAttributes::iid, IStreamAttributes)
	*obj = 0;
	return kNoInterface;
}

//------------------------------------------------------------------------
tresult PLUGIN_API NSDataIBStream::read (void* buffer, int32 numBytes, int32* numBytesRead)
{
	int32 useBytes = std::min (numBytes, (int32)([data length] - currentPos));
	if (useBytes > 0)
	{
		[data getBytes: buffer range: NSMakeRange (currentPos, useBytes)];
		if (numBytesRead)
			*numBytesRead = useBytes;
		currentPos += useBytes;
		return kResultTrue;
	}
	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API NSDataIBStream::write (void* buffer, int32 numBytes, int32* numBytesWritten)
{
	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API NSDataIBStream::seek (int64 pos, int32 mode, int64* result)
{
	switch (mode)
	{
		case kIBSeekSet:
		{
			if (pos <= [data length])
			{
				currentPos = pos;
				if (result)
					tell (result);
				return kResultTrue;
			}
			break;
		}
		case kIBSeekCur:
		{
			if (currentPos + pos <= [data length])
			{
				currentPos += pos;
				if (result)
					tell (result);
				return kResultTrue;
			}
			break;
		}
		case kIBSeekEnd:
		{
			if ([data length] + pos <= [data length])
			{
				currentPos = [data length] + pos;
				if (result)
					tell (result);
				return kResultTrue;
			}
			break;
		}
	}
	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API NSDataIBStream::tell (int64* pos)
{
	if (pos)
	{
		*pos = currentPos;
		return kResultTrue;
	}
	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API NSDataIBStream::getFileName (String128 name)
{
	return kNotImplemented;
}

//------------------------------------------------------------------------
IAttributeList* PLUGIN_API NSDataIBStream::getAttributes ()
{
	return attrList;
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
NSMutableDataIBStream::NSMutableDataIBStream (NSMutableData* data)
: NSDataIBStream (data, true)
, mdata (data)
{
}

//------------------------------------------------------------------------
NSMutableDataIBStream::~NSMutableDataIBStream ()
{
	[mdata setLength:currentPos];
}

//------------------------------------------------------------------------
tresult PLUGIN_API NSMutableDataIBStream::write (void* buffer, int32 numBytes, int32* numBytesWritten)
{
	[mdata replaceBytesInRange:NSMakeRange (currentPos, numBytes) withBytes:buffer];
	if (numBytesWritten)
		*numBytesWritten = numBytes;
	currentPos += numBytes;
	return kResultTrue;
}

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

/// \endcond
