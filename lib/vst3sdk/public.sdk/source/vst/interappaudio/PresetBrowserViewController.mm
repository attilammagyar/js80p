//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/interappaudio/PresetBrowserViewController.mm
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

#import "PresetBrowserViewController.h"
#import "pluginterfaces/base/funknown.h"

//------------------------------------------------------------------------
@interface PresetBrowserViewController ()
//------------------------------------------------------------------------
{
	IBOutlet UITableView* presetTableView;
	IBOutlet UIView* containerView;

	std::function<void (const char* presetPath)> callback;
	Steinberg::FUID uid;
}

@property (strong) NSArray* factoryPresets;
@property (strong) NSArray* userPresets;
@property (strong) NSArray* displayPresets;
@property (assign) BOOL editMode;

@end

//------------------------------------------------------------------------
@implementation PresetBrowserViewController
//------------------------------------------------------------------------

//------------------------------------------------------------------------
- (id)initWithCallback:(std::function<void (const char* presetPath)>)_callback
{
	self = [super initWithNibName:@"PresetBrowserView" bundle:nil];
	if (self)
	{
		callback = _callback;

		self.modalPresentationStyle = UIModalPresentationOverCurrentContext;
		self.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;

		UIViewController* rootViewController =
		    [[UIApplication sharedApplication].windows[0] rootViewController];

		[rootViewController presentViewController:self animated:YES completion:^{}];
	}
	return self;
}

//------------------------------------------------------------------------
- (void)setFactoryPresets:(NSArray*)factoryPresets userPresets:(NSArray*)userPresets
{
	self.factoryPresets = factoryPresets;
	self.userPresets = userPresets;
	[self updatePresetArray];
	dispatch_async (dispatch_get_main_queue (), ^{ [presetTableView reloadData]; });
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
- (void)didReceiveMemoryWarning
{
	[super didReceiveMemoryWarning];
}

//------------------------------------------------------------------------
- (void)updatePresetArray
{
	if (self.userPresets)
	{
		self.displayPresets = [[self.factoryPresets arrayByAddingObjectsFromArray:self.userPresets]
		    sortedArrayUsingComparator:^NSComparisonResult (NSURL* obj1, NSURL* obj2) {
			  return [[obj1 lastPathComponent] caseInsensitiveCompare:[obj2 lastPathComponent]];
		    }];
	}
	else
	{
		self.displayPresets = self.factoryPresets;
	}
}

//------------------------------------------------------------------------
- (void)removeSelf
{
	[self dismissViewControllerAnimated:YES completion:^{}];
}

//------------------------------------------------------------------------
- (void)tableView:(UITableView*)tableView didSelectRowAtIndexPath:(NSIndexPath*)indexPath
{
	NSURL* url = [self.displayPresets objectAtIndex:indexPath.row];
	if (url)
	{
		callback ([[url path] UTF8String]);
	}
	[self removeSelf];
}

//------------------------------------------------------------------------
- (IBAction)toggleEditMode:(id)sender
{
	self.editMode = !self.editMode;
	if (self.editMode)
	{
		NSMutableArray* indexPaths = [NSMutableArray new];
		for (NSURL* url in self.factoryPresets)
		{
			NSUInteger index = [self.displayPresets indexOfObjectIdenticalTo:url];
			[indexPaths addObject:[NSIndexPath indexPathForRow:index inSection:0]];
		}
		[presetTableView deleteRowsAtIndexPaths:indexPaths
		                       withRowAnimation:UITableViewRowAnimationFade];
	}
	else
	{
		[self updatePresetArray];
		NSMutableArray* indexPaths = [NSMutableArray new];
		for (NSURL* url in self.factoryPresets)
		{
			NSUInteger index = [self.displayPresets indexOfObjectIdenticalTo:url];
			[indexPaths addObject:[NSIndexPath indexPathForRow:index inSection:0]];
		}
		[presetTableView insertRowsAtIndexPaths:indexPaths
		                       withRowAnimation:UITableViewRowAnimationFade];
	}
	[presetTableView setEditing:self.editMode animated:YES];
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
- (NSInteger)tableView:(UITableView*)tableView numberOfRowsInSection:(NSInteger)section
{
	if (self.editMode)
	{
		return [self.userPresets count];
	}
	return [self.displayPresets count];
}

//------------------------------------------------------------------------
- (UITableViewCell*)tableView:(UITableView*)tableView cellForRowAtIndexPath:(NSIndexPath*)indexPath
{
	UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier:@"PresetBrowserCell"];
	if (cell == nil)
	{
		cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1
		                              reuseIdentifier:@"PresetBrowserCell"];
	}

	cell.backgroundColor = [UIColor clearColor];

	NSURL* presetUrl = nil;
	if (self.editMode)
	{
		presetUrl = [self.userPresets objectAtIndex:indexPath.row];
		cell.detailTextLabel.text = @"User";
	}
	else
	{
		presetUrl = [self.displayPresets objectAtIndex:indexPath.row];
		if ([self.factoryPresets indexOfObject:presetUrl] == NSNotFound)
		{
			cell.detailTextLabel.text = @"User";
		}
		else
		{
			cell.detailTextLabel.text = @"Factory";
		}
	}

	cell.textLabel.text = [[presetUrl lastPathComponent] stringByDeletingPathExtension];

	return cell;
}

//------------------------------------------------------------------------
- (BOOL)tableView:(UITableView*)tableView canEditRowAtIndexPath:(NSIndexPath*)indexPath
{
	if (self.editMode)
	{
		return YES;
	}
	return NO;
}

//------------------------------------------------------------------------
- (void)tableView:(UITableView*)tableView
    commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
     forRowAtIndexPath:(NSIndexPath*)indexPath
{
	NSURL* presetUrl = [self.userPresets objectAtIndex:indexPath.row];
	if (presetUrl)
	{
		NSFileManager* fs = [NSFileManager defaultManager];
		NSError* error = nil;
		if ([fs removeItemAtURL:presetUrl error:&error] == NO)
		{
			auto alertController =
			    [UIAlertController alertControllerWithTitle:[error localizedDescription]
			                                        message:[error localizedRecoverySuggestion]
			                                 preferredStyle:UIAlertControllerStyleAlert];
			[self presentViewController:alertController animated:YES completion:nil];
		}
		else
		{
			NSMutableArray* newArray = [NSMutableArray arrayWithArray:self.userPresets];
			[newArray removeObject:presetUrl];
			self.userPresets = newArray;
			[presetTableView deleteRowsAtIndexPaths:@[indexPath]
			                       withRowAnimation:UITableViewRowAnimationAutomatic];
		}
	}
}

//------------------------------------------------------------------------
- (BOOL)prefersStatusBarHidden
{
	return YES;
}

@end
