//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : public.sdk/source/common/threadchecker.h
// Created by  : Steinberg, 01/2019
// Description : thread checker
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/ftypes.h"
#include <memory>

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
class ThreadChecker
{
public:
	static std::unique_ptr<ThreadChecker> create ();
	
	virtual bool test (const char* failmessage = nullptr, bool exit = false) = 0;

	virtual ~ThreadChecker () noexcept = default;
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
