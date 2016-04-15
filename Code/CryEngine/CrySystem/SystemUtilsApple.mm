//////////////////////////////////////////////////////////////////////
//
//	Crytek CryENGINE Source code
//
//	File:SystemUtilsApple.mm
//  Description: Utilities for iOS and Mac OS X. Needs to be separated
//    due to conflict with the system headers.
//
//	History:
//	-Apr 15,2014:Created by Leander Beernaert
//
//////////////////////////////////////////////////////////////////////


#include <Foundation/Foundation.h>
#include "SystemUtilsApple.h"

// Get the path to the user's document directory.
// Return length of the string or 0 on failure.
size_t AppleGetUserDocumentDirectory(char* buffer, const size_t bufferLen)
{
	NSArray* directories = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	if ([directories count] != 0)
	{
		const char* cdirName = [(NSString*)[directories objectAtIndex:0] UTF8String];
		size_t dirLen = strlen(cdirName);
		if (dirLen < bufferLen -1 )
		{
			strncpy(buffer, cdirName, dirLen);
			buffer[dirLen] = '\0';
			return dirLen;
		}
	}
	return 0;
}

// Get the path to the user's library directory.
// Return length of the string or 0 on failure.
size_t AppleGetUserLibraryDirectory(char* buffer, const size_t bufferLen)
{
	NSArray* directories = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
	if ([directories count] != 0)
	{
		const char* cdirName = [(NSString*)[directories objectAtIndex:0] UTF8String];
		size_t dirLen = strlen(cdirName);
		if (dirLen < bufferLen -1 )
		{
			strncpy(buffer, cdirName, dirLen);
			buffer[dirLen] = '\0';
			return dirLen;
		}
	}
	return 0;
}

// Get the User's name
// Return length of the string or 0 on failure.
size_t AppleGetUserName(char* buffer, const size_t bufferLen)
{
	const char* userName = [NSUserName() UTF8String];
	size_t userLen = strlen(userName);
	if (userLen < bufferLen - 1)
	{
		strncpy(buffer, userName, userLen);
		buffer[userLen] = '\0';
		return userLen;
	}
	return 0;
}