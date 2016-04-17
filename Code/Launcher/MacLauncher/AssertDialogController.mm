//
//  AssertDialogController.m
//  CryEngine
//
//  Created by Leander Beernaert on 02/09/13.
//
//
#define __APPLESPECIFIC_H__
#include <MacSpecific.h>
#import "AssertDialogController.h"

@interface AssertDialogController ()

@end

@implementation AssertDialogController


- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self) {
        // Initialization code here.
        
        
    }
    return self;
}

-(id)initWithDataAndNib:(NSString*) czCondition file:(NSString*) czFile
                   line:(int) nLine reason:(NSString*) czReason nib:(NSString*) nib
{
    self = [super initWithWindowNibName:nib];
    if (self)
    {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(receiveAssertNotif:) name:@"AssertDetails" object:nil];
        [self SetDialogData:czCondition file:czFile line:nLine reason:czReason];
    }
    return self;
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    [self->tfCondition setNeedsDisplay: YES];
    [self->tfFile setNeedsDisplay: YES];
    [self->tfReason setNeedsDisplay: YES];
    [self->tfLine setNeedsDisplay: YES];
    [[self window] orderOut:nil];
    
    
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
}

// Free resources
-(void) dealloc
{
    [tfLine dealloc];
    [tfCondition dealloc];
    [tfFile dealloc];
    [tfReason dealloc];
    [super dealloc];
}


-(void) receiveAssertNotif:(NSNotification *) notification
{
    if([[notification name] isEqualToString:@"AssertDetails"])
    {
        NSDictionary* userData = [notification userInfo];
        [self SetDialogData:[userData objectForKey:@"cond"] file:[userData objectForKey:@"file"]
                       line:[[userData objectForKey:@"line"] intValue] reason:[userData objectForKey:@"reas"]];
      
			[NSApp runModalForWindow: self.window];
			[NSApp activateIgnoringOtherApps:YES];
			[self.window orderOut:nil];
    }
}
-(void) SetDialogData:(NSString*) czCondition file:(NSString*) czFile
                 line:(int) nLine reason:(NSString*) czReason
{
    
    [self->tfCondition setStringValue: czCondition];
    [self->tfFile setStringValue: czFile];
    [self->tfReason setStringValue: czReason];
    [self->tfLine setIntegerValue: nLine];
    [self->tfCondition setNeedsDisplay: YES];
    [self->tfFile setNeedsDisplay: YES];
    [self->tfReason setNeedsDisplay: YES];
    [self->tfLine setNeedsDisplay: YES];
}



// Close window when done with selection

-(void) closeWindow
{
    [NSApp stopModal];
}

// Handle button input

-(IBAction)onContinue:(id)sender
{
    self.dialogAction = eDAContinue;
    [self closeWindow];
}

-(IBAction)onIgnore:(id)sender
{
    self.dialogAction = eDAIgnore;
    [self closeWindow];
}

-(IBAction)onIgnoreAll:(id)sender
{
    self.dialogAction = eDAIgnoreAll;
    [self closeWindow];
}

-(IBAction)onBreak:(id)sender
{
    self.dialogAction = eDABreak;
    [self closeWindow];
}

-(IBAction)onStop:(id)sender
{
    self.dialogAction = eDAStop;
    [self closeWindow];
}

-(IBAction)onReportAsBug:(id)sender
{
    self.dialogAction = eDAReportAsBug;
    [self closeWindow];
}

@end
