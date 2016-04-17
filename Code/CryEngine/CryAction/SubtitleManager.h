// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   SubtitleManager.h
//  Version:     v1.00
//  Created:     29/01/2007 by AlexL.
//  Compilers:   Visual Studio.NET 2005
//  Description: Subtitle Manager Implementation
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SUBTITLEMANAGER_H__
#define __SUBTITLEMANAGER_H__
#pragma once

#include "ISubtitleManager.h"

class CSubtitleManager : public ISubtitleManager
{
public:
	CSubtitleManager();
	virtual ~CSubtitleManager();

	// ISubtitleManager
	virtual void SetHandler(ISubtitleHandler* pHandler) { m_pHandler = pHandler; }
	virtual void SetEnabled(bool bEnabled);
	virtual void SetAutoMode(bool bOn);
	virtual void ShowSubtitle(const char* subtitleLabel, bool bShow);
	// ~ISubtitleManager

	static void OnAudioTriggerStarted(const SAudioRequestInfo* const pAudioRequestInfo);
	static void OnAudioTriggerFinished(const SAudioRequestInfo* const pAudioRequestInfo);

protected:
	void ShowSubtitle(const SAudioRequestInfo* const pAudioRequestInfo, bool bShow);

	ISubtitleHandler*        m_pHandler;
	bool                     m_bEnabled;
	bool                     m_bAutoMode;

	static CSubtitleManager* s_Instance;
};

#endif // __SUBTITLEMANAGER_H__
