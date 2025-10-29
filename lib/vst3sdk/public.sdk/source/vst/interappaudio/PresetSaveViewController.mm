//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/PresetSaveViewController.mm
// Created by  : Steinberg, 09/2013
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
#import "PresetSaveViewController.h"
#import "pluginterfaces/base/funknown.h"

//------------------------------------------------------------------------
@interface PresetSaveViewController ()
//------------------------------------------------------------------------
{
	IBOutlet UIView* containerView;
	IBOutlet UITextField* presetName;

	std::function<void (const char* presetPath)> callback;
	Steinberg::FUID uid;
}
@end

//------------------------------------------------------------------------
@implementation PresetSaveViewController
//------------------------------------------------------------------------

//------------------------------------------------------------------------
- (id)initWithCallback:(std::function<void (const char* presetPath)>)_callback
{
	self = [super initWithNibName:@"PresetSaveView" bundle:nil];
	if (self)
	{
		callback = _callback;

		self.modalPresentationStyle = UIModalPresentationOverCurrentContext;
		self.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;

		UIViewController* rootViewController =
		    [[UIApplication sharedApplication].windows[0] rootViewController];

		[rootViewController presentViewController:self
		                                 animated:YES
		                               completion:^{ [self showKeyboard]; }];
	}
	return self;
}

//------------------------------------------------------------------------
- (void)viewDidLoad
{
	[super viewDidLoad];

	containerView.layer.shadowOpacity = 0.5;
	containerView.layer.shadowOffset = CGSizeMake (5, 5);
	containerView.layer.shadowRadius = 5;
}

//------------------------------------------------------------------------
- (void)showKeyboard
{
	[presetName becomeFirstResponder];
}

//------------------------------------------------------------------------
- (void)removeSelf
{
	[self dismissViewControllerAnimated:YES completion:^{}];
}

//------------------------------------------------------------------------
- (NSURL*)presetURL
{
	NSFileManager* fs = [NSFileManager defaultManager];
	NSURL* documentsUrl = [fs URLForDirectory:NSDocumentDirectory
	                                 inDomain:NSUserDomainMask
	                        appropriateForURL:Nil
	                                   create:YES
	                                    error:NULL];
	if (documentsUrl)
	{
		NSURL* presetPath = [[documentsUrl URLByAppendingPathComponent:presetName.text]
		    URLByAppendingPathExtension:@"vstpreset"];
		return presetPath;
	}
	return nil;
}

//------------------------------------------------------------------------
- (BOOL)textFieldShouldReturn:(UITextField*)textField
{
	if ([textField.text length] > 0)
	{
		[self save:textField];
		return YES;
	}
	return NO;
}

//------------------------------------------------------------------------
- (IBAction)save:(id)sender
{
	if (callback)
	{
		NSURL* presetPath = [self presetURL];
		NSFileManager* fs = [NSFileManager defaultManager];
		if ([fs fileExistsAtPath:[presetPath path]])
		{
			// alert for overwrite
			auto alertController = [UIAlertController
			    alertControllerWithTitle:NSLocalizedString (
			                                 @"A Preset with this name already exists",
			                                 "Alert title")
			                     message:NSLocalizedString (@"Save it anyway ?", "Alert message")
			              preferredStyle:UIAlertControllerStyleAlert];
			[alertController
			    addAction:[UIAlertAction
			                  actionWithTitle:NSLocalizedString (@"Save", "Alert Save Button")
			                            style:UIAlertActionStyleDefault
			                          handler:^(UIAlertAction* _Nonnull action) {
				                        callback ([[[self presetURL] path] UTF8String]);
				                        [self removeSelf];
			                          }]];
			[alertController
			    addAction:[UIAlertAction
			                  actionWithTitle:NSLocalizedString (@"Cancel", "Alert Cancel Button")
			                            style:UIAlertActionStyleCancel
			                          handler:^(UIAlertAction* _Nonnull action) {}]];
			[self presentViewController:alertController animated:YES completion:nil];
			return;
		}
		callback ([[presetPath path] UTF8String]);
	}
	[self removeSelf];
}

//------------------------------------------------------------------------
- (IBAction)cancel:(id)sender
{
	if (callback)
	{
		callback (nullptr);
	}
	[self removeSelf];
}

//------------------------------------------------------------------------
- (BOOL)prefersStatusBarHidden
{
	return YES;
}

@end
