//------------------------------------------------------------------------
// Flags       : clang-format SMTGSequencer
//
// Category    : Helpers
// Project     : Steinberg Plug-In SDK
// Filename    : public.sdk/source/common/openurl.cpp
// Created by  : Steinberg 04.2020
// Description : Simple helper allowing to open a URL in the default associated application
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "openurl.h"
#include "pluginterfaces/base/ftypes.h"

#if SMTG_OS_WINDOWS
// keep this order
#include <windows.h>

#include <shellapi.h>
#else
#include <cstdlib>
#endif

//-----------------------------------------------------------------------------
namespace Steinberg {

//-----------------------------------------------------------------------------
bool openURLInDefaultApplication (const String& address)
{
	bool res = false;

#if SMTG_OS_WINDOWS
	auto r = ShellExecuteA (nullptr, "open", address.text8 (), nullptr, nullptr, SW_SHOWNORMAL);
	res = (r != nullptr);
#elif SMTG_OS_OSX
	String cmd;
	cmd += "open \"";
	cmd += address.text8 ();
	cmd += "\"";
	res = (system (cmd) == 0);
#elif SMTG_OS_LINUX
	String cmd;
	cmd += "xdg-open \"";
	cmd += address.text8 ();
	cmd += "\"";
	res = (system (cmd) == 0);
#endif
	return res;
}

//------------------------------------------------------------------------
} // namespace Steinberg
