//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/SettingsViewController.mm
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

#import "SettingsViewController.h"

#import "AudioIO.h"
#import "MidiIO.h"
#import <CoreMIDI/MIDINetworkSession.h>

using namespace Steinberg::Vst::InterAppAudio;

static const NSUInteger kMinTempo = 30;

//------------------------------------------------------------------------
@interface SettingsViewController ()
//------------------------------------------------------------------------
{
	IBOutlet UIView* containerView;
	IBOutlet UISwitch* midiOnSwitch;
	IBOutlet UIPickerView* tempoView;
}
@end

//------------------------------------------------------------------------
@implementation SettingsViewController
//------------------------------------------------------------------------

//------------------------------------------------------------------------
- (id)init
{
	self = [super initWithNibName:@"SettingsView" bundle:nil];
	if (self)
	{
		self.modalPresentationStyle = UIModalPresentationOverCurrentContext;
		self.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;
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

	midiOnSwitch.on = MidiIO::instance ().isEnabled ();

	Float64 tempo = AudioIO::instance ()->getStaticFallbackTempo ();
	[tempoView selectRow:tempo - kMinTempo inComponent:0 animated:YES];
}

//------------------------------------------------------------------------
- (IBAction)enableMidi:(id)sender
{
	BOOL state = midiOnSwitch.on;
	MidiIO::instance ().setEnabled (state);
}

//------------------------------------------------------------------------
- (IBAction)close:(id)sender
{
	[self dismissViewControllerAnimated:YES completion:^{}];
}

//------------------------------------------------------------------------
- (void)pickerView:(UIPickerView*)pickerView
      didSelectRow:(NSInteger)row
       inComponent:(NSInteger)component
{
	AudioIO::instance ()->setStaticFallbackTempo (row + kMinTempo);
}

//------------------------------------------------------------------------
- (NSInteger)numberOfComponentsInPickerView:(UIPickerView*)pickerView
{
	return 1;
}

//------------------------------------------------------------------------
- (NSInteger)pickerView:(UIPickerView*)pickerView numberOfRowsInComponent:(NSInteger)component
{
	return 301 - kMinTempo;
}

//------------------------------------------------------------------------
- (NSString*)pickerView:(UIPickerView*)pickerView
            titleForRow:(NSInteger)row
           forComponent:(NSInteger)component
{
	return [@(row + kMinTempo) stringValue];
}

//------------------------------------------------------------------------
- (BOOL)prefersStatusBarHidden
{
	return YES;
}

@end

//------------------------------------------------------------------------
void showIOSettings ()
{
	SettingsViewController* controller = [[SettingsViewController alloc] init];

	UIViewController* rootViewController =
	    [[UIApplication sharedApplication].windows[0] rootViewController];
	[rootViewController presentViewController:controller animated:YES completion:^{}];
}
