//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/VST3Editor.mm
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

#import "VST3Editor.h"
#import "pluginterfaces/vst/ivsteditcontroller.h"

//------------------------------------------------------------------------
@interface VST3EditorViewController : UIViewController
//------------------------------------------------------------------------

@end

//------------------------------------------------------------------------
@implementation VST3EditorViewController
//------------------------------------------------------------------------

- (BOOL)prefersStatusBarHidden
{
	return YES;
}
- (BOOL)shouldAutorotate
{
	return YES;
}
- (NSUInteger)supportedInterfaceOrientations
{
	return UIInterfaceOrientationMaskLandscapeLeft | UIInterfaceOrientationMaskLandscapeRight;
}

@end

namespace Steinberg {
namespace Vst {
namespace InterAppAudio {

//------------------------------------------------------------------------
VST3Editor::VST3Editor () = default;

//------------------------------------------------------------------------
VST3Editor::~VST3Editor ()
{
	if (plugView)
	{
		plugView->release ();
	}
}

//------------------------------------------------------------------------
bool VST3Editor::init (const CGRect& frame)
{
	viewController = [VST3EditorViewController new];
	viewController.view = [[UIView alloc] initWithFrame:frame];
	return true;
}

//------------------------------------------------------------------------
bool VST3Editor::attach (IEditController* editController)
{
	auto ec2 = U::cast<IEditController2> (editController);
	if (ec2)
	{
		ec2->setKnobMode (kLinearMode);
	}
	plugView = editController->createView (ViewType::kEditor);
	if (plugView)
	{
		if (plugView->isPlatformTypeSupported (kPlatformTypeUIView) == kResultTrue)
		{
			plugView->setFrame (this);
			if (plugView->attached ((__bridge void*)viewController.view, kPlatformTypeUIView) ==
			    kResultTrue)
			{
				return true;
			}
		}
		plugView->release ();
		plugView = nullptr;
	}

	return false;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Editor::resizeView (IPlugView* view, ViewRect* newSize)
{
	if (newSize && plugView && plugView == view)
	{
		if (view->onSize (newSize) == kResultTrue)
			return kResultTrue;
		return kResultFalse;
	}
	return kInvalidArgument;
}

//------------------------------------------------------------------------
} // InterAppAudio
} // Vst
} // Steinberg
