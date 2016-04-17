////////////////////////////////////////////////////////////////////////////
//
//  CryEngine Source File.
//  Copyright (C), Crytek, 1999-2013.
// -------------------------------------------------------------------------
//  File name:   AppKitWrapper.mm
//  Version:     v1.00
//  Created:     08/05/2013 by Leander Beernaert
//  Description: 	Wrappers around Cocoa/AppKit APIs. This file is separate
//                  from the main launcher in order to avoid conflicts with the
//                  built in Cocoa types.
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#import <Cocoa/Cocoa.h>

#define __APPLESPECIFIC_H__
#include <MacSpecific.h>

#import "AppKitWrapper.h"


void InitAppKit() {
	
	
}

void ShutdownAppKit()
{

}


bool GetBundleResourcePath(char *ptr, const size_t len) {
	
	CFBundleRef bundle = CFBundleGetMainBundle();
	if( !bundle )
		return false;
	
	CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL( bundle );
	
	CFStringRef last = CFURLCopyLastPathComponent( resourcesURL );
	if( CFStringCompare( CFSTR( "Resources" ), last, 0 ) != kCFCompareEqualTo )
	{
		CFRelease( last );
		CFRelease( resourcesURL );
		return false;
	}
	
	CFRelease( last );
	
	if( !CFURLGetFileSystemRepresentation( resourcesURL,
																				true,
																				(UInt8*) ptr,
																				len) )
	{
		CFRelease( resourcesURL );
		return false;
	}
	
	CFRelease( resourcesURL );
	
	return true;
}


EDialogAction MessageBox(const char* header, const char* message, unsigned long message_type )
{
	//convert the strings from char* to CFStringRef
	CFStringRef header_ref      = CFStringCreateWithCString( NULL, header,     strlen(header)    );
	CFStringRef message_ref  = CFStringCreateWithCString( NULL, message,  strlen(message) );
	
	CFOptionFlags result;  //result code from the message box
	
	//launch the message box
	CFUserNotificationDisplayAlert(
																 0, // no timeout
																 kCFUserNotificationNoteAlertLevel, //change it depending message_type flags ( MB_ICONASTERISK.... etc.)
																 NULL, //icon url, use default, you can change it depending message_type flags
																 NULL, //not used
																 NULL, //localization of strings
																 header_ref, //header text
																 message_ref, //message text
																 CFSTR("Break"), //default "ok" text in button
																 CFSTR("Continue"), //alternate button title
																 CFSTR("Ignore All"), //other button title, null--> no other button
																 &result //response flags
																 );
	
	//Clean up the strings
	CFRelease( header_ref );
	CFRelease( message_ref );
	
	//Convert the result
	if( result == kCFUserNotificationDefaultResponse )
		return eDABreak;
	else if (result == kCFUserNotificationOtherResponse)
		return eDAIgnoreAll;
	else
		return eDAContinue;
	
}


/*__attribute__((visibility("default"))) EDialogAction MacOSXHandleAssert(const char* condition, const char* file, int line,
																																				const char* reason, bool isRenderInited)
{
#if !defined(_RELEASE)
	MyScopedLock lock(&g_mutex);
	
	// We can't show any Cocoa modal dialogs before the APPkit is initialized.
	// This is now being done by SDL. So before SDL initialises we present a
	// simple message box with the most important options (break, continue or
	// ignore all)
	if (isRenderInited)
	{
		
		if (g_dialogController == nil)
		{
			//g_dialogController = [[AssertDialogController alloc] initWithWindowNibName:@"AssertDialog"];
			g_dialogController = [[AssertDialogController alloc] initWithDataAndNib:[NSString stringWithUTF8String:condition]
																																				 file:[NSString stringWithUTF8String:file]
																																				 line:line
																																			 reason:[NSString stringWithUTF8String:reason]
																																					nib:@"AssertDialog"];
		}
		NSWindow* assertDialog = [g_dialogController window];
		// Order front so that it can refresh the gui
		[assertDialog orderFront:nil];
		// Send notification with the data
		NSMutableDictionary* userInfo = [[NSMutableDictionary dictionaryWithCapacity:4] autorelease];
		[userInfo setObject:[NSNumber numberWithInt:line] forKey:@"line"];
		[userInfo setObject:[NSString stringWithUTF8String:condition] forKey:@"cond"];
		[userInfo setObject:[NSString stringWithUTF8String:file] forKey:@"file"];
		[userInfo setObject:[NSString stringWithUTF8String:reason] forKey:@"reas"];
		
		// show dialong on the main thread and wait for exit
		NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];
		NSNotification *notification = [NSNotification notificationWithName:@"AssertDetails" object:nil userInfo:userInfo];
		[nc performSelectorOnMainThread:@selector(postNotification:) withObject:nc waitUntilDone:TRUE];
		
		// hide again for the assert
		//[NSApp runModalForWindow: assertDialog];
		//[NSApp activateIgnoringOtherApps:YES];
		//[assertDialog orderOut:nil];
    
		
		return g_dialogController.dialogAction;
	}
	else
	{
		return MessageBox("Assertion Failed Dialog", "A more informative dialog will be presented as soon as the renderer initialises.", 0);
	}
#else
	return eDAContinue;
#endif
}*/