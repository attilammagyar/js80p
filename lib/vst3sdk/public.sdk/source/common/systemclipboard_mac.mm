//------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
//
// Project     : Steinberg Plug-In SDK
// Filename    : public.sdk/source/common/systemclipboard_mac.mm
// Created by  : Steinberg 04.2020
// Description : Simple helper allowing to copy/retrieve text to/from the system clipboard
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "systemclipboard.h"
#include "pluginterfaces/base/fplatform.h"

#if SMTG_OS_OSX
#import <Cocoa/Cocoa.h>

//------------------------------------------------------------------------
namespace Steinberg {
namespace SystemClipboard {

//-----------------------------------------------------------------------------
bool copyTextToClipboard (const std::string& text)
{
	auto pb = [NSPasteboard generalPasteboard];
	[pb clearContents];
	auto nsString = [NSString stringWithUTF8String:text.data ()];
	return [pb setString:nsString forType:NSPasteboardTypeString];
}

//-----------------------------------------------------------------------------
bool getTextFromClipboard (std::string& text)
{
	auto pb = [NSPasteboard generalPasteboard];
	if ([pb canReadItemWithDataConformingToTypes:@[NSPasteboardTypeString]])
	{
		if (auto items = [pb readObjectsForClasses:@[[NSString class]] options:nil])
		{
			if (items.count > 0)
			{
				text = [items[0] UTF8String];
				return true;
			}
		}
	}
	return false;
}

//------------------------------------------------------------------------
} // namespace SystemClipboard
} // namespace Steinberg

#elif SMTG_OS_IOS
//------------------------------------------------------------------------
bool copyTextToClipboard (const std::string& text)
{
	return false;
}

//------------------------------------------------------------------------
bool getTextFromClipboard (std::string& text)
{
	return false;
}

#endif // SMTG_OS_MACOS
