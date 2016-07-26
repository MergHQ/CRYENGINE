// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ActionSpeakLine.h"
#include "DialogLineDatabase.h"
#include "ResponseSystem.h"
#include "SpeakerManager.h"
#include <CryAudio/IAudioSystem.h>
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>
#include <CryDynamicResponseSystem/IDynamicResponseAction.h>
#include "ResponseInstance.h"

using namespace CryDRS;

//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstanceUniquePtr CActionSpeakLine::Execute(DRS::IResponseInstance* pResponseInstance)
{
	if (!m_lineIDToSpeak.IsValid())
		return nullptr;

	CResponseSystem* pDrs = CResponseSystem::GetInstance();
	CResponseActor* pSpeaker = nullptr;
	if (m_speakerOverrideName.IsValid())
	{
		pSpeaker = pDrs->GetResponseActor(m_speakerOverrideName);
	}
	else
	{
		pSpeaker = static_cast<CResponseInstance*>(pResponseInstance)->GetCurrentActor();
	}
	if (!pSpeaker)
	{
		CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_WARNING, "Triggered an SpeakLineAction, without a valid Speaker! LineID was: '%s', Speaker override was '%s'", m_lineIDToSpeak.GetText().c_str(), m_speakerOverrideName.GetText().c_str());
		return nullptr;
	}

	if (pDrs->GetSpeakerManager()->StartSpeaking(pSpeaker, m_lineIDToSpeak))
	{
		return DRS::IResponseActionInstanceUniquePtr(new CActionSpeakLineInstance(pSpeaker, m_lineIDToSpeak));
	}
	else
	{
		//drs-todo: queue line
		return nullptr;
	}
}

//--------------------------------------------------------------------------------------------------
void CActionSpeakLine::Serialize(Serialization::IArchive& ar)
{
	ar(m_lineIDToSpeak, "lineToSpeak", "^ Line");
	ar(m_speakerOverrideName, "speakerOverride", "^ SpeakerOverride");

#if defined(HASHEDSTRING_STORES_SOURCE_STRING)
	if (ar.isEdit())
	{
		if (!m_lineIDToSpeak.IsValid())
		{
			ar.warning(m_lineIDToSpeak.m_textCopy, "No line specified");
		}
		else if (!gEnv->pDynamicResponseSystem->GetDialogLineDatabase()->GetLineSetById(m_lineIDToSpeak))
		{
			ar.warning(m_lineIDToSpeak.m_textCopy, "Specified line not found in the database");
		}
	}
#endif
}

//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstance::eCurrentState CActionSpeakLineInstance::Update()
{
	if (!m_pSpeaker)
	{
		return CS_CANCELED;
	}

	const CSpeakerManager* pSpeakerMgr = CResponseSystem::GetInstance()->GetSpeakerManager();
	if (pSpeakerMgr->IsSpeaking(m_pSpeaker))
	{
		if (pSpeakerMgr->IsSpeaking(m_pSpeaker, m_lineID))
		{
			return CS_RUNNING;
		}
		else
		{
			return CS_CANCELED;  //so he is still speaking, but not our line, so it seems like someone canceled our talk
		}
	}
	else
	{
		return CS_FINISHED;  //not speaking? so he is done
	}
}

//--------------------------------------------------------------------------------------------------
void CActionSpeakLineInstance::Cancel()
{
	CSpeakerManager* pSpeakerMgr = CResponseSystem::GetInstance()->GetSpeakerManager();
	if (pSpeakerMgr->IsSpeaking(m_pSpeaker, m_lineID))
	{
		pSpeakerMgr->CancelSpeaking(m_pSpeaker);
	}
	m_pSpeaker = nullptr;
}

//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstanceUniquePtr CActionCancelSpeaking::Execute(DRS::IResponseInstance* pResponseInstance)
{
	CResponseSystem* pDrs = CResponseSystem::GetInstance();
	CResponseActor* pSpeaker = nullptr;
	if (m_speakerOverrideName.IsValid())
	{
		pSpeaker = pDrs->GetResponseActor(m_speakerOverrideName);
	}
	else
	{
		pSpeaker = static_cast<CResponseInstance*>(pResponseInstance)->GetCurrentActor();
	}

	if (!pSpeaker && m_speakerOverrideName != "all")
	{
		CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_WARNING, "Triggered an CancelSpeaking Action, without a valid Speaker! Speaker override was '%s'", m_speakerOverrideName.GetText().c_str());
		return nullptr;
	}

	if (pDrs->GetSpeakerManager()->CancelSpeaking(pSpeaker, m_maxPrioToCancel) && pSpeaker)
	{
		return DRS::IResponseActionInstanceUniquePtr(new CActionCancelSpeakingInstance(pSpeaker));
	}
	else
	{
		return nullptr;
	}
}

//--------------------------------------------------------------------------------------------------
void CActionCancelSpeaking::Serialize(Serialization::IArchive& ar)
{
	ar(m_speakerOverrideName, "speakerOverride", "^ SpeakerOverride");
	ar(m_maxPrioToCancel, "maxPrioToCancel", "^ MaxPrioToCancel");
}

//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstance::eCurrentState CActionCancelSpeakingInstance::Update()
{
	CResponseSystem* pDrs = CResponseSystem::GetInstance();

	if (!pDrs->GetSpeakerManager()->IsSpeaking(m_pSpeaker) || !m_pSpeaker)
	{
		return CS_FINISHED;
	}
	return CS_RUNNING;
}

//--------------------------------------------------------------------------------------------------
void CActionCancelSpeakingInstance::Cancel()
{
	m_pSpeaker = nullptr;
}

//--------------------------------------------------------------------------------------------------

REGISTER_DRS_ACTION(CActionSpeakLine, "SpeakLine", "00FF00");
REGISTER_DRS_ACTION(CActionCancelSpeaking, "CancelSpeaking", DEFAULT_DRS_ACTION_COLOR);
