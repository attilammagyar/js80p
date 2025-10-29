//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vstguieditor.h
// Created by  : Steinberg, 04/2005
// Description : VSTGUI Editor
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses.
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#pragma once

#include "vsteditcontroller.h"
#include "vstgui/vstgui.h"

#if VSTGUI_VERSION_MAJOR < 4
#include "vstgui/cvstguitimer.h"
#define VSTGUI_INT32 long
#else
#define VSTGUI_INT32 int32_t
#endif

namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
/** Base class for an edit view using VSTGUI.
\ingroup vstClasses
*/
class VSTGUIEditor : public EditorView, public VSTGUI::VSTGUIEditorInterface, public VSTGUI::CBaseObject
{
public:
	/** Constructor. */
	VSTGUIEditor (void* controller, ViewRect* size = nullptr);

	/** Destructor. */
	~VSTGUIEditor () override;

//---Internal function-----
/** Called when the editor will be opened. */
#if VSTGUI_VERSION_MAJOR >= 4 && VSTGUI_VERSION_MINOR >= 1
	virtual bool PLUGIN_API open (void* parent, const VSTGUI::PlatformType& platformType) = 0;
#else
	virtual bool PLUGIN_API open (void* parent) = 0;
#endif
	/** Called when the editor will be closed. */
	virtual void PLUGIN_API close () = 0;

	/** Sets the idle rate controlling the parameter update rate. */
	void setIdleRate (int32 millisec);

	//---from CBaseObject---------------
	VSTGUI::CMessageResult notify (VSTGUI::CBaseObject* sender, const char* message) SMTG_OVERRIDE;
	void forget () SMTG_OVERRIDE { EditorView::release (); }
	void remember () SMTG_OVERRIDE { EditorView::addRef (); }
	VSTGUI_INT32 getNbReference () const SMTG_OVERRIDE { return refCount; }

	//---from IPlugView-------
	tresult PLUGIN_API isPlatformTypeSupported (FIDString type) SMTG_OVERRIDE;
	tresult PLUGIN_API onSize (ViewRect* newSize) SMTG_OVERRIDE;

	//---from VSTGUIEditorInterface-------
	/** Called from VSTGUI when a user begins editing.
	    The default implementation calls performEdit of the EditController. */
	void beginEdit (VSTGUI_INT32 index) SMTG_OVERRIDE;
	/** Called from VSTGUI when a user ends editing.
	    The default implementation calls endEdit of the EditController. */
	void endEdit (VSTGUI_INT32 index) SMTG_OVERRIDE;

	VSTGUI_INT32 getKnobMode () const SMTG_OVERRIDE;

	OBJ_METHODS (VSTGUIEditor, EditorView)
	DEFINE_INTERFACES
	END_DEFINE_INTERFACES (EditorView)
	REFCOUNT_METHODS (EditorView)

#if TARGET_OS_IPHONE
	static void setBundleRef (/*CFBundleRef*/ void* bundle);
#endif

protected:
	//---from IPlugView-------
	tresult PLUGIN_API attached (void* parent, FIDString type) SMTG_OVERRIDE;
	tresult PLUGIN_API removed () SMTG_OVERRIDE;
	tresult PLUGIN_API onKeyDown (char16 key, int16 keyMsg, int16 modifiers) SMTG_OVERRIDE;
	tresult PLUGIN_API onKeyUp (char16 key, int16 keyMsg, int16 modifiers) SMTG_OVERRIDE;
	tresult PLUGIN_API onWheel (float distance) SMTG_OVERRIDE;
	tresult PLUGIN_API setFrame (IPlugFrame* frame) SMTG_OVERRIDE;

private:
	VSTGUI::CVSTGUITimer* timer;
};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg
