// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ActionSpeakLineBasedOnVariable.h"
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>


DRS::IResponseActionInstanceUniquePtr CActionSpeakLineBasedOnVariable::Execute(DRS::IResponseInstance* pResponseInstance)
{
	int currentValue = 0;
	DRS::IVariableCollection* pCollection = gEnv->pDynamicResponseSystem->GetCollection(m_collectionName, pResponseInstance);
	if (pCollection)
	{
		DRS::IVariable* pVariable = pCollection->GetVariable(m_variableName);
		if (pVariable)
		{
			currentValue = pVariable->GetValueAsInt();
		}
	}

	CHashedString lineToSpeak; 
	if (currentValue <= m_variableValueForLine1)
		lineToSpeak = m_lineIDToSpeak1;
	else if (currentValue <= m_variableValueForLine2)
		lineToSpeak = m_lineIDToSpeak2;
	else
		lineToSpeak = m_lineIDToSpeak3;

	if (!lineToSpeak.IsValid())
	{
		return nullptr;
	}
	
	DRS::IResponseActor* pSpeaker;
	if (m_speakerOverrideName.IsValid())
	{
		pSpeaker = gEnv->pDynamicResponseSystem->GetResponseActor(m_speakerOverrideName);
	}
	else
	{
		pSpeaker = pResponseInstance->GetCurrentActor();
	}
	if (!pSpeaker)
	{
		CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_WARNING, "Triggered an SpeakLineAction, without a valid Speaker! LineID was: '%s', Speaker override was '%s'",
			lineToSpeak.GetText().c_str(), m_speakerOverrideName.GetText().c_str());
		return nullptr;
	}

	if (gEnv->pDynamicResponseSystem->GetSpeakerManager()->StartSpeaking(pSpeaker, lineToSpeak))
	{
		return DRS::IResponseActionInstanceUniquePtr(new CActionSpeakLineBasedOnVariableInstance(pSpeaker, lineToSpeak));
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
void CActionSpeakLineBasedOnVariable::Serialize(Serialization::IArchive& ar)
{
	if (ar.openBlock("general", " "))
	{
		ar(m_speakerOverrideName, "speakerOverride", "^^>120>SpeakerOverride");
		ar(m_collectionName, "collection", "^^>140> collection");
		ar(m_variableName, "variable", "^^ variable");
		ar.closeBlock();
	}

	if (ar.openBlock("line1", " "))
	{
		ar(m_variableValueForLine1, "lineToSpeak", "^^>90>Max value for line");
		ar(m_lineIDToSpeak1, "lineToSpeak", "^^ Line");
		ar.closeBlock();
	}
	if (ar.openBlock("line2", " "))
	{
		ar(m_variableValueForLine2, "lineToSpeak", "^^>90>Max value for line");
		ar(m_lineIDToSpeak2, "lineToSpeak", "^^ DefaultLine");
		ar.closeBlock();
	}

	ar(m_lineIDToSpeak3, "lineToSpeak", "Line");

#if !defined(_RELEASE)
	if (ar.isEdit() && ar.isOutput())
	{
		ar(GetVerboseInfo(), "ConditionDesc", "!^<");
	}
#endif
}

//--------------------------------------------------------------------------------------------------
string CActionSpeakLineBasedOnVariable::GetVerboseInfo() const
{
	return "'" + m_collectionName.GetText() + "::" + m_variableName.GetText() + "'";
}

//--------------------------------------------------------------------------------------------------
DRS::IResponseActionInstance::eCurrentState CActionSpeakLineBasedOnVariableInstance::Update()
{
	if (!m_pSpeaker)
	{
		return DRS::IResponseActionInstance::CS_CANCELED;
	}

	const DRS::ISpeakerManager* pSpeakerMgr = gEnv->pDynamicResponseSystem->GetSpeakerManager();
	if (pSpeakerMgr->IsSpeaking(m_pSpeaker))
	{
		if (pSpeakerMgr->IsSpeaking(m_pSpeaker, m_LineID))
		{
			return DRS::IResponseActionInstance::CS_RUNNING;
		}
		else
		{
			return DRS::IResponseActionInstance::CS_CANCELED;  //so he is still speaking, but not our line, so it seems like someone canceled our talk
		}
	}
	else
	{
		return DRS::IResponseActionInstance::CS_FINISHED;  //not speaking? so he is done
	}
}

//--------------------------------------------------------------------------------------------------
void CActionSpeakLineBasedOnVariableInstance::Cancel()
{
	DRS::ISpeakerManager* pSpeakerMgr = gEnv->pDynamicResponseSystem->GetSpeakerManager();
	if (pSpeakerMgr->IsSpeaking(m_pSpeaker, m_LineID))
	{
		pSpeakerMgr->CancelSpeaking(m_pSpeaker);
	}
	m_pSpeaker = nullptr;
}
