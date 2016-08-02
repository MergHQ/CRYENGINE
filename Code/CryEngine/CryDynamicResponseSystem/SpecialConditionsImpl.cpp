// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SpecialConditionsImpl.h"

#include <CryDynamicResponseSystem/IDynamicResponseCondition.h>
#include <CryMath/Random.h>
#include <CryString/StringUtils.h>
#include <CryGame/IGame.h>
#include <CryGame/IGameFramework.h>
#include <CryMemory/CrySizer.h>
#include <CrySerialization/Decorators/Range.h>
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>

#include "VariableCollection.h"
#include "ResponseSystem.h"
#include "Response.h"
#include "ResponseManager.h"
#include "ResponseInstance.h"

using namespace CryDRS;

//--------------------------------------------------------------------------------------------------
bool CRandomCondition::IsMet(DRS::IResponseInstance* pResponseInstance)
{
	return cry_random(0, 100) < m_randomFactor;
}

//--------------------------------------------------------------------------------------------------
void CRandomCondition::Serialize(Serialization::IArchive& ar)
{
	ar(Serialization::Range(m_randomFactor, 1, 99), "RandomFactor", "^ Probability");

#if !defined(_RELEASE)
	if (ar.isEdit())
	{
		if (m_randomFactor <= 0)
		{
			ar.warning(m_randomFactor, "Will never be true.");
		}
		else if (m_randomFactor >= 100)
		{
			ar.warning(m_randomFactor, "Will always be true.");
		}
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CGameTokenCondition::~CGameTokenCondition()
{
	if (m_pCachedToken)
	{
		IGameTokenSystem* pGTSys = gEnv->pGame->GetIGameFramework()->GetIGameTokenSystem();
		if (pGTSys)
		{
			pGTSys->UnregisterListener(m_pCachedToken->GetName(), this);
		}
	}
}

//--------------------------------------------------------------------------------------------------
void CGameTokenCondition::OnGameTokenEvent(EGameTokenEvent event, IGameToken* pGameToken)
{
	if (event == EGAMETOKEN_EVENT_DELETE)
	{
		m_pCachedToken = nullptr;
	}
}

//--------------------------------------------------------------------------------------------------
void CGameTokenCondition::GetMemoryUsage(class ICrySizer* pSizer) const
{
	pSizer->Add(*this);
}

//--------------------------------------------------------------------------------------------------
void CGameTokenCondition::Serialize(Serialization::IArchive& ar)
{
	ar(m_tokenName, "Token", "Token");

	m_pCachedToken = (gEnv->pGame) ? gEnv->pGame->GetIGameFramework()->GetIGameTokenSystem()->FindToken(m_tokenName.c_str()) : nullptr;

	if (ar.isEdit() && m_pCachedToken && (m_pCachedToken->GetType() == eFDT_Bool)) //for booleans only EQUAL tests do make sense, therefore we only need one comparison value. (actually also strings, but gametokes keep changing their type to string all the time, so we dont handle that here...)
	{
		ar(m_minValue, "MinVal", " EQUALS ");
		m_maxValue = m_minValue;
		ar(m_maxValue, "MaxVal", "");
	}
	else
	{
		ar(m_minValue, "MinVal", "GREATER than or equal to");
		ar(m_maxValue, "MaxVal", "and LESS than or equal to");
	}

	if (ar.isEdit())
	{
		if (m_tokenName.empty())
		{
			ar.warning(m_tokenName, "You need to specify a GameToken Name");
		}
		else if (m_maxValue == CVariableValue::POS_INFINITE && m_minValue == CVariableValue::NEG_INFINITE)
		{
			ar.warning(m_tokenName, "This Condition will always be true!");
		}
		else if (!m_minValue.DoTypesMatch(m_maxValue))
		{
			ar.warning(m_tokenName, "The type of min and max value do not match!");
		}
	}
#if !defined(_RELEASE)
	if (ar.isEdit() && ar.isOutput())
	{
		ar(GetVerboseInfo(), "ConditionDesc", "!^<");
	}
#endif
}

//--------------------------------------------------------------------------------------------------
bool CGameTokenCondition::IsMet(DRS::IResponseInstance* pResponseInstance)
{
	if (!m_pCachedToken)
	{
		IGameTokenSystem* pGTSys = gEnv->pGame->GetIGameFramework()->GetIGameTokenSystem();

		m_pCachedToken = pGTSys->FindToken(m_tokenName.c_str());
		if (!m_pCachedToken)
		{
			CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_ERROR, "Could not find GameToken for condition-check, token-name: %s", m_tokenName.c_str());
			return false;
		}
		else
		{
			pGTSys->RegisterListener(m_pCachedToken->GetName(), this);
		}
	}

	CVariableValue currentTokenValue = CVariableValue::NEG_INFINITE;

	switch (m_pCachedToken->GetType())
	{
	case eFDT_Int:
		{
			int value;
			if (m_pCachedToken->GetValueAs(value))
			{
				currentTokenValue = value;
			}
			break;
		}
	case eFDT_Float:
		{
			float value;
			if (m_pCachedToken->GetValueAs(value))
			{
				currentTokenValue = value;
			}
			break;
		}
	case eFDT_String:
		{
			currentTokenValue = CHashedString(m_pCachedToken->GetValueAsString());
			break;
		}
	case eFDT_Bool:
		{
			bool bValue;
			if (m_pCachedToken->GetValueAs(bValue))
			{
				currentTokenValue = bValue;
			}
			break;
		}
	default:
		{
			CryWarning(VALIDATOR_MODULE_DRS, VALIDATOR_ERROR, "Only Int/Float/String and Bool GameTokens can be used in DRS-Conditions. TokenName: %s", m_tokenName.c_str());
			return false;
		}
	}

	return currentTokenValue <= m_maxValue && currentTokenValue >= m_minValue;
}

//////////////////////////////////////////////////////////////////////////
string CGameTokenCondition::GetVerboseInfo() const
{
	if (m_minValue == m_maxValue)
	{
		return string("Token: '") + m_tokenName + "' EQUALS '" + m_minValue.GetValueAsString() + "'";
	}
	else
	{
		return string("Token: '") + m_tokenName + "' is BETWEEN '" + m_minValue.GetValueAsString() + "' AND '" + m_maxValue.GetValueAsString() + "'";
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CPlaceholderCondition::Serialize(Serialization::IArchive& ar)
{
	ar(m_Label, "Label", "^ Text:");
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct SDepthCounterHelper
{
	SDepthCounterHelper() { ++s_depthCounter; }
	~SDepthCounterHelper() { --s_depthCounter; }

	static int s_depthCounter;
};
int SDepthCounterHelper::s_depthCounter = 0;

bool CInheritConditionsCondition::IsMet(DRS::IResponseInstance* pResponseInstance)
{
	SDepthCounterHelper temp;
	if (temp.s_depthCounter >= 3)
	{
		DrsLogWarning("Max depth of 'inherited conditions' is 2. Example: Response1 inherites from Response2 which inherites from Response3, would be the maximum. Also make sure not to inherit from yourself.");
		return false;
	}

	if (!m_pCachedResponse)
	{
		m_pCachedResponse = CResponseSystem::GetInstance()->GetResponseManager()->GetResponse(m_responseToReuse);
	}
	if (m_pCachedResponse)
	{
		return m_pCachedResponse->GetBaseSegment().AreConditionsMet(static_cast<CResponseInstance*>(pResponseInstance));
	}

	return true;
}

//--------------------------------------------------------------------------------------------------
void CInheritConditionsCondition::Serialize(Serialization::IArchive& ar)
{
	ar(m_responseToReuse, "ResponseToCopyConditionsFrom", "^ Response to inherit from");
	m_pCachedResponse = CResponseSystem::GetInstance()->GetResponseManager()->GetResponse(m_responseToReuse);

#if defined(HASHEDSTRING_STORES_SOURCE_STRING)
	if (ar.isEdit())
	{
		if (!m_responseToReuse.IsValid())
		{
			ar.warning(m_responseToReuse.m_textCopy, "No Response to inherit conditions from specified");
		}
		else if (!m_pCachedResponse)
		{
			ar.warning(m_responseToReuse.m_textCopy, "No response specified from which the conditions should be inherited");
		}
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

bool CTimeSinceCondition::IsMet(DRS::IResponseInstance* pResponseInstance)
{
	CVariable* pVariable = GetCurrentVariable(static_cast<CResponseInstance*>(pResponseInstance));
	if (pVariable && pVariable->m_value.GetType() != eDRVT_Undefined)
	{
		const float timeSince = CResponseSystem::GetInstance()->GetCurrentDrsTime() - pVariable->GetValueAsFloat();
		return timeSince >= m_minTime && ((timeSince <= m_maxTime) || m_maxTime < 0.0f);
	}
	else
	{
		return true;  //if the timer-variable is not even existing, we assume it never happened, and therefore the time since then is infinite
	}
}

//--------------------------------------------------------------------------------------------------
void CTimeSinceCondition::Serialize(Serialization::IArchive& ar)
{
	IVariableUsingBase::_Serialize(ar, "^TimerVariable");
	ar(m_minTime, "minTime", "^MinElpasedTime");
	ar(m_maxTime, "maxTime", "^MaxElpasedTime");

#if defined (ENABLE_VARIABLE_VALUE_TYPE_CHECKINGS)
	CVariableCollection* pCollection = CResponseSystem::GetInstance()->GetCollection(m_collectionName);
	if (pCollection)
	{
		if (pCollection->GetVariableValue(m_variableName).GetType() != eDRVT_Float && pCollection->GetVariableValue(m_variableName).GetType() != eDRVT_Undefined)
		{
			ar.warning(m_collectionName.m_textCopy, "Variable to check needs to hold a time value as float");
		}
	}
#endif
}

//--------------------------------------------------------------------------------------------------
string CTimeSinceCondition::GetVerboseInfo() const
{
	if (IVariableUsingBase::s_bDoDisplayCurrentValueInDebugOutput)
	{
		return GetVariableVerboseName() + string(" more than ") + CryStringUtils::toString(m_minTime) + " seconds away from current time: "
		       + CryStringUtils::toString(CResponseSystem::GetInstance()->GetCurrentDrsTime()) + " and less then " + CryStringUtils::toString(m_maxTime);
	}
	else
	{
		return GetVariableVerboseName() + string(" more than ") + CryStringUtils::toString(m_minTime) + " seconds and less then " + CryStringUtils::toString(m_maxTime);
	}
}

//--------------------------------------------------------------------------------------------------
bool CExecutionLimitCondition::IsMet(DRS::IResponseInstance* pResponseInstance)
{
	CResponse* pResponse = nullptr;
	if (!m_ResponseToTest.IsValid())
	{
		pResponse = static_cast<CResponseInstance*>(pResponseInstance)->GetResponse();
	}
	else
	{
		ResponsePtr pOverrideResponse = CResponseSystem::GetInstance()->GetResponseManager()->GetResponse(m_ResponseToTest);
		if (pOverrideResponse)
		{
			pResponse = pOverrideResponse.get();
		}
	}
	if (pResponse)
	{
		const uint32 executionCounter = pResponse->GetExecutionCounter();
		return executionCounter >= m_minExecutions && ((executionCounter <= m_maxExecutions) || m_maxExecutions < 0);
	}
	else
	{
		return false;
	}
}

//--------------------------------------------------------------------------------------------------
void CExecutionLimitCondition::Serialize(Serialization::IArchive& ar)
{
	ar(m_minExecutions, "minExecutions", "^ MinExecutions");
	ar(m_maxExecutions, "maxExecutions", "^MaxExecutions");
	ar(m_ResponseToTest, "response", "^Response");

#if !defined(_RELEASE)
	if (ar.isEdit())
	{
		if (m_minExecutions > m_maxExecutions)
		{
			ar.warning(m_minExecutions, "Will never be true. Because Min is larger than Max");
		}
		else if (m_maxExecutions <= 0)
		{
			ar.warning(m_maxExecutions, "Will never be true. Because the maximum execution count is set to 0");
		}
	}
#endif
}

//--------------------------------------------------------------------------------------------------
string CryDRS::CExecutionLimitCondition::GetVerboseInfo() const
{
	return string("min: ") + CryStringUtils::toString(m_minExecutions) + string(" max: ") + CryStringUtils::toString(m_maxExecutions);
}

//--------------------------------------------------------------------------------------------------
bool CryDRS::CTimeSinceResponseCondition::IsMet(DRS::IResponseInstance* pResponseInstance)
{
	CResponse* pResponse = nullptr;
	if (!m_responseId.IsValid())
	{
		pResponse = static_cast<CResponseInstance*>(pResponseInstance)->GetResponse();
	}
	else
	{
		ResponsePtr pOverrideResponse = CResponseSystem::GetInstance()->GetResponseManager()->GetResponse(m_responseId);
		if (pOverrideResponse)
		{
			pResponse = pOverrideResponse.get();
		}
	}
	if (pResponse)
	{
		const float timeSinceLastExecution = (pResponse->GetLastEndTime() > 0) ? CResponseSystem::GetInstance()->GetCurrentDrsTime() - pResponse->GetLastEndTime() : std::numeric_limits<float>::max();
		return timeSinceLastExecution >= m_minTime && ((timeSinceLastExecution <= m_maxTime) || m_maxTime < 0.0f);
	}
	else
	{
		return false;
	}
}

//--------------------------------------------------------------------------------------------------
void CryDRS::CTimeSinceResponseCondition::Serialize(Serialization::IArchive& ar)
{
	ar(m_responseId, "response", "^ Response");
	ar(m_minTime, "minTime", "^MinElpasedTime");
	ar(m_maxTime, "maxTime", "^MaxElpasedTime");
}

//--------------------------------------------------------------------------------------------------
string CryDRS::CTimeSinceResponseCondition::GetVerboseInfo() const
{
	string responseName = (m_responseId.IsValid()) ? m_responseId.GetText() : "CurrentResponse";
	return "Execution of '" + responseName + string("' more than ") + CryStringUtils::toString(m_minTime) + " seconds and less then " + CryStringUtils::toString(m_maxTime);
}

REGISTER_DRS_CONDITION(CPlaceholderCondition, "Placeholder", "FF66CC");
REGISTER_DRS_CONDITION(CRandomCondition, "Random", DEFAULT_DRS_CONDITION_COLOR);
REGISTER_DRS_CONDITION(CTimeSinceCondition, "TimeSince", DEFAULT_DRS_CONDITION_COLOR);
REGISTER_DRS_CONDITION(CTimeSinceResponseCondition, "TimeSinceResponse", DEFAULT_DRS_CONDITION_COLOR);
REGISTER_DRS_CONDITION(CGameTokenCondition, "GameToken", DEFAULT_DRS_CONDITION_COLOR);
REGISTER_DRS_CONDITION(CInheritConditionsCondition, "Inherit Conditions", DEFAULT_DRS_CONDITION_COLOR);
REGISTER_DRS_CONDITION(CExecutionLimitCondition, "Execution Limit", DEFAULT_DRS_CONDITION_COLOR);
