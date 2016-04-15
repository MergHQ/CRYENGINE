// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   ISubtitleManager.h
//  Version:     v1.00
//  Created:     29/01/2007 by AlexL.
//  Compilers:   Visual Studio.NET 2005
//  Description: Interface to the Subtitle Manager
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ISUBTITLEMANAGER_H__
#define __ISUBTITLEMANAGER_H__
#pragma once

//////////////////////////////////////////////////////////////////////////
struct ISubtitleHandler
{
	virtual ~ISubtitleHandler(){}
	virtual void ShowSubtitle(const SAudioRequestInfo* const pAudioRequestInfo, bool bShow) = 0;
	virtual void ShowSubtitle(const char* subtitleLabel, bool bShow) = 0;
};

//////////////////////////////////////////////////////////////////////////
struct ISubtitleManager
{
	virtual ~ISubtitleManager(){}
	virtual void SetHandler(ISubtitleHandler* pHandler) = 0;

	// enables/disables subtitles manager
	virtual void SetEnabled(bool bEnabled) = 0;

	// automatic mode. Will inform the subtitleHandler about every executed/stopped audio trigger.
	// You can use this mode, if you want to drive your subtitles by started sounds and not manually.
	virtual void SetAutoMode(bool bOn) = 0;

	virtual void ShowSubtitle(const char* subtitleLabel, bool bShow) = 0;
};

#endif // __ISUBTITLEMANAGER_H__
