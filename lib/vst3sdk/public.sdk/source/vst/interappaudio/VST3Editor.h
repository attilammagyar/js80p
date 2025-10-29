//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/VST3Editor.h
// Created by  : Steinberg, 08/2013.
// Description : VST 3 InterAppAudio
// Flags       : clang-format SMTGSequencer
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

/// \cond ignore

#import "base/source/fobject.h"
#import "pluginterfaces/gui/iplugview.h"
#import <UIKit/UIKit.h>

namespace Steinberg {
namespace Vst {
class IEditController;

namespace InterAppAudio {

//------------------------------------------------------------------------
class VST3Editor : public FObject, public IPlugFrame
{
public:
//------------------------------------------------------------------------
	VST3Editor ();
	virtual ~VST3Editor ();

	bool init (const CGRect& frame);
	bool attach (IEditController* editController);

	UIViewController* getViewController () const { return viewController; }

	OBJ_METHODS (VST3Editor, FObject)
	REFCOUNT_METHODS (FObject)
	DEFINE_INTERFACES
		DEF_INTERFACE (IPlugFrame)
	END_DEFINE_INTERFACES (FObject)
protected:
	// IPlugFrame
	tresult PLUGIN_API resizeView (IPlugView* view, ViewRect* newSize) override;

	IPlugView* plugView {nullptr};
	UIViewController* viewController {nullptr};
};

//------------------------------------------------------------------------
} // namespace InterAppAudio
} // namespace Vst
} // namespace Steinberg

/// \endcond
