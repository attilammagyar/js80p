//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/auwrapper/aucarbonview.h
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

#include "pluginterfaces/base/fplatform.h"

#if !SMTG_PLATFORM_64

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#include "AUPublic/AUCarbonViewBase/AUCarbonViewBase.h"
#pragma clang diagnostic pop

#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "base/source/fobject.h"
#include "pluginterfaces/gui/iplugview.h"

namespace Steinberg {
namespace Vst {
class AUCarbonPlugFrame;

//------------------------------------------------------------------------
class AUCarbonView : public AUCarbonViewBase, public IPlugFrame, public FObject
{
public:
	AUCarbonView (AudioUnitCarbonView auv);
	~AUCarbonView ();

	OSStatus CreateUI (Float32 xoffset, Float32 yoffset) override;

	OBJ_METHODS(AUCarbonView, FObject)
	DEF_INTERFACES_1(IPlugFrame, FObject)
	REFCOUNT_METHODS(FObject)

protected:
	tresult PLUGIN_API resizeView (IPlugView* view, ViewRect* vr) SMTG_OVERRIDE;

	static OSStatus HIViewAdded (EventHandlerCallRef inHandlerCallRef, EventRef inEvent, void* inUserData);

	IEditController* editController;
	AUCarbonPlugFrame* plugFrame;
	IPlugView* plugView;
	HIViewRef hiPlugView;
	EventHandlerRef eventHandler;
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

#endif // !SMTG_PLATFORM_64

/// \endcond
