//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vstgui_win32_bundle_support.cpp
// Created by  : Steinberg, 10/2018
// Description : VSTGUI Win32 Bundle Support
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "vstgui_win32_bundle_support.h"
#include "vstgui/lib/platform/platform_win32.h"
#include "vstgui/lib/platform/win32/win32support.h"
#if ((VSTGUI_VERSION_MAJOR == 4) && (VSTGUI_VERSION_MINOR >= 10)) || (VSTGUI_VERSION_MAJOR > 4)
#include "vstgui/lib/platform/win32/win32factory.h"
#endif
#include "vstgui/lib/optional.h"
#include <string>

//-----------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
static VSTGUI::Optional<std::string> ascend (std::string& path, char delimiter = '\\')
{
	auto index = path.find_last_of (delimiter);
	if (index == std::string::npos)
		return {};
	path.erase (index);
	return VSTGUI::Optional<std::string> (std::move (path));
}

//-----------------------------------------------------------------------------
void setupVSTGUIBundleSupport (void* hInstance)
{
	using namespace VSTGUI;
	WCHAR path[MAX_PATH];
	if (SUCCEEDED (GetModuleFileNameW (static_cast<HMODULE> (hInstance), path, MAX_PATH)))
	{
		UTF8StringHelper helper (path);
		auto utf8Path = std::string (helper.getUTF8String ());
		if (auto p = ascend (utf8Path))
		{
			if (p = ascend (*p))
			{
				*p += "\\Resources";
#if ((VSTGUI_VERSION_MAJOR == 4) && (VSTGUI_VERSION_MINOR >= 10)) || (VSTGUI_VERSION_MAJOR > 4)
			    if (auto winFactory = VSTGUI::getPlatformFactory ().asWin32Factory ())
				{
					winFactory->setResourceBasePath (VSTGUI::UTF8String (*p));
				}
#else
				IWin32PlatformFrame::setResourceBasePath (UTF8String (*p));
#endif
			}
		}
	}

}

//-----------------------------------------------------------------------------
} // Vst
} // Steinberg
