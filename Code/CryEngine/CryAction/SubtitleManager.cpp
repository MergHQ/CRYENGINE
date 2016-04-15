// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   SubtitleManager.cpp
//  Version:     v1.00
//  Created:     29/01/2007 by AlexL.
//  Compilers:   Visual Studio.NET 2005
//  Description: Subtitle Manager Implementation
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "SubtitleManager.h"
#include "ISubtitleManager.h"
#include "DialogSystem/DialogActorContext.h"

CSubtitleManager* CSubtitleManager::s_Instance = NULL;

//////////////////////////////////////////////////////////////////////////
CSubtitleManager::CSubtitleManager()
{
	m_bEnabled = false;
	m_bAutoMode = true;
	m_pHandler = NULL;

	s_Instance = this;
}

//////////////////////////////////////////////////////////////////////////
CSubtitleManager::~CSubtitleManager()
{
	SetEnabled(false);
	s_Instance = NULL;
}

//////////////////////////////////////////////////////////////////////////
void CSubtitleManager::SetEnabled(bool bEnabled)
{
	if (bEnabled != m_bEnabled)
	{
		m_bEnabled = bEnabled;

		if (m_bAutoMode)
		{
			if (bEnabled)
			{
				gEnv->pAudioSystem->AddRequestListener(&CSubtitleManager::OnAudioTriggerStarted, 0, eAudioRequestType_AudioObjectRequest, eAudioObjectRequestType_ExecuteTrigger);
				gEnv->pAudioSystem->AddRequestListener(&CSubtitleManager::OnAudioTriggerFinished, 0, eAudioRequestType_AudioCallbackManagerRequest, eAudioCallbackManagerRequestType_ReportFinishedTriggerInstance);
			}
			else
			{
				gEnv->pAudioSystem->RemoveRequestListener(&CSubtitleManager::OnAudioTriggerStarted, 0);
				gEnv->pAudioSystem->RemoveRequestListener(&CSubtitleManager::OnAudioTriggerFinished, 0);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSubtitleManager::SetAutoMode(bool bOn)
{
	if (bOn != m_bAutoMode)
	{
		if (m_bEnabled)
		{
			SetEnabled(false);  //force refresh for add/remove as audio listener
			m_bAutoMode = bOn;
			SetEnabled(true);
		}
		else
		{
			m_bAutoMode = bOn;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CSubtitleManager::ShowSubtitle(const char* subtitleLabel, bool bShow)
{
	if (m_bEnabled && m_pHandler)
	{
		m_pHandler->ShowSubtitle(subtitleLabel, bShow);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSubtitleManager::ShowSubtitle(const SAudioRequestInfo* const pAudioRequestInfo, bool bShow)
{
	if (m_bEnabled && m_pHandler)
	{
		m_pHandler->ShowSubtitle(pAudioRequestInfo, bShow);
	}
}

//////////////////////////////////////////////////////////////////////////
void CSubtitleManager::OnAudioTriggerStarted(const SAudioRequestInfo* const pAudioRequestInfo)
{
	CSubtitleManager::s_Instance->ShowSubtitle(pAudioRequestInfo, true);
}

//////////////////////////////////////////////////////////////////////////
void CSubtitleManager::OnAudioTriggerFinished(const SAudioRequestInfo* const pAudioRequestInfo)
{
	CSubtitleManager::s_Instance->ShowSubtitle(pAudioRequestInfo, false);
}
