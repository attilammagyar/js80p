//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/aaxwrapper/aaxwrapper_gui.h
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

#include "AAX_CEffectGUI.h"
#include "pluginterfaces/base/fplatform.h"

//==============================================================================
class AAXWrapper_GUI : public AAX_CEffectGUI
{
public:
	static AAX_IEffectGUI* AAX_CALLBACK Create ();

	AAXWrapper_GUI () = default;
	virtual ~AAXWrapper_GUI () = default;

	void CreateViewContents () SMTG_OVERRIDE;
	void CreateViewContainer () SMTG_OVERRIDE;
	void DeleteViewContainer () SMTG_OVERRIDE;

	AAX_Result GetViewSize (AAX_Point* oEffectViewSize) const SMTG_OVERRIDE;

	AAX_Result SetControlHighlightInfo (AAX_CParamID /* iParameterID */,
	                                    AAX_CBoolean /* iIsHighlighted */,
	                                    AAX_EHighlightColor /* iColor */) SMTG_OVERRIDE;

	AAX_Result TimerWakeup () SMTG_OVERRIDE;

	bool setWindowSize (AAX_Point& size); // calback from AAXWrapper

private:
	bool mInOpen = false;
	bool mRefreshSize = false;
	void* mHWND = nullptr;
};

/// \endcond
