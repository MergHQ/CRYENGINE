// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "GlobalObject.h"
#include "CVars.h"
#include "Managers.h"
#include "System.h"
#include "ListenerManager.h"
#include "Environment.h"
#include "Parameter.h"
#include "Switch.h"
#include "SwitchState.h"
#include "Trigger.h"
#include "TriggerInstance.h"
#include "Common/IImpl.h"
#include "Common/IObject.h"
#include <CryEntitySystem/IEntitySystem.h>
#include <CryMath/Cry_Camera.h>

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	#include "LoseFocusTrigger.h"
	#include "GetFocusTrigger.h"
	#include "MuteAllTrigger.h"
	#include "UnmuteAllTrigger.h"
	#include "PauseAllTrigger.h"
	#include "ResumeAllTrigger.h"
	#include "Debug.h"
	#include "Common/Logger.h"
	#include "Common/DebugStyle.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
void CGlobalObject::Release()
{
	// Do not clear the object's name though!

	for (auto& triggerInstancePair : m_triggerInstances)
	{
		triggerInstancePair.second->Release();
	}

	m_pIObject = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::ConstructTriggerInstance(
	ControlId const triggerId,
	uint16 const numPlayingConnectionInstances,
	uint16 const numPendingConnectionInstances,
	ERequestFlags const flags,
	void* const pOwner,
	void* const pUserData,
	void* const pUserDataOwner)
{
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	m_triggerInstances.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(g_triggerInstanceIdCounter),
		std::forward_as_tuple(new CTriggerInstance(triggerId, numPlayingConnectionInstances, numPendingConnectionInstances, flags, pOwner, pUserData, pUserDataOwner, 0.0f)));
#else
	m_triggerInstances.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(g_triggerInstanceIdCounter),
		std::forward_as_tuple(new CTriggerInstance(triggerId, numPlayingConnectionInstances, numPendingConnectionInstances, flags, pOwner, pUserData, pUserDataOwner)));
#endif // CRY_AUDIO_USE_PRODUCTION_CODE

	g_triggerInstanceIdToGlobalObject[g_triggerInstanceIdCounter] = this;
	IncrementTriggerInstanceIdCounter();
}

///////////////////////////////////////////////////////////////////////////
void CGlobalObject::ReportStartedTriggerInstance(TriggerInstanceId const triggerInstanceId, ETriggerResult const result)
{
	TriggerInstances::iterator const iter(m_triggerInstances.find(triggerInstanceId));

	if (iter != m_triggerInstances.end())
	{
		iter->second->SetPendingToPlaying();
	}
}

///////////////////////////////////////////////////////////////////////////
void CGlobalObject::ReportFinishedTriggerInstance(TriggerInstanceId const triggerInstanceId, ETriggerResult const result)
{
	TriggerInstances::iterator const iter(m_triggerInstances.find(triggerInstanceId));

	if (iter != m_triggerInstances.end())
	{
		CTriggerInstance* const pTriggerInstance = iter->second;

		if (result != ETriggerResult::Pending)
		{
			if (pTriggerInstance->IsPlayingInstanceFinished())
			{
				g_triggerInstanceIdToGlobalObject.erase(triggerInstanceId);
				pTriggerInstance->SendFinishedRequest(INVALID_ENTITYID);

				m_triggerInstances.erase(iter);
				delete pTriggerInstance;
			}
		}
		else
		{
			if (pTriggerInstance->IsPendingInstanceFinished())
			{
				g_triggerInstanceIdToGlobalObject.erase(triggerInstanceId);
				pTriggerInstance->SendFinishedRequest(INVALID_ENTITYID);

				m_triggerInstances.erase(iter);
				delete pTriggerInstance;
			}
		}
	}
#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown trigger instance id %u during %s", triggerInstanceId, __FUNCTION__);
	}
#endif  // CRY_AUDIO_USE_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CGlobalObject::StopAllTriggers()
{
	m_pIObject->StopAllTriggers();
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::SetImplDataPtr(Impl::IObject* const pIObject)
{
	m_pIObject = pIObject;
}

///////////////////////////////////////////////////////////////////////////
void CGlobalObject::Update(float const deltaTime)
{
	if (!m_triggerInstances.empty())
	{
		m_pIObject->Update(deltaTime);
	}
}

#if defined(CRY_AUDIO_USE_PRODUCTION_CODE)
///////////////////////////////////////////////////////////////////////////
void CGlobalObject::ForceImplementationRefresh(bool const setTransformation)
{
	if (setTransformation)
	{
		m_pIObject->SetTransformation(m_transformation);
	}

	// Parameters
	for (auto const& parameterPair : m_parameters)
	{
		CParameter const* const pParameter = stl::find_in_map(g_parameters, parameterPair.first, nullptr);

		if (pParameter != nullptr)
		{
			pParameter->Set(*this, parameterPair.second);
		}
		else
		{
			Cry::Audio::Log(ELogType::Warning, "Parameter \"%u\" does not exist!", parameterPair.first);
		}
	}

	// Switches
	for (auto const& switchPair : m_switchStates)
	{
		CSwitch const* const pSwitch = stl::find_in_map(g_switches, switchPair.first, nullptr);

		if (pSwitch != nullptr)
		{
			CSwitchState const* const pState = stl::find_in_map(pSwitch->GetStates(), switchPair.second, nullptr);

			if (pState != nullptr)
			{
				pState->Set(*this);
			}
		}
	}

	uint16 triggerCounter = 0;
	// Last re-execute its active triggers.
	for (auto const& triggerInstancePair : m_triggerInstances)
	{
		CTrigger const* const pTrigger = stl::find_in_map(g_triggers, triggerInstancePair.second->GetTriggerId(), nullptr);

		if (pTrigger != nullptr)
		{
			pTrigger->Execute(*this, triggerInstancePair.first, triggerInstancePair.second, triggerCounter);
			++triggerCounter;
		}
		else if (!ExecuteDefaultTrigger(triggerInstancePair.second->GetTriggerId()))
		{
			Cry::Audio::Log(ELogType::Warning, "Trigger \"%u\" does not exist!", triggerInstancePair.second->GetTriggerId());
		}
	}
}

///////////////////////////////////////////////////////////////////////////
void CGlobalObject::HandleSetTransformation(CTransformation const& transformation)
{
	m_transformation = transformation;
	m_pIObject->SetTransformation(transformation);
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CGlobalObject::HandleSetName(char const* const szName)
{
	m_name = szName;
	return m_pIObject->SetName(szName);
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::StoreParameterValue(ControlId const id, float const value)
{
	m_parameters[id] = value;
}

//////////////////////////////////////////////////////////////////////////
void CGlobalObject::StoreSwitchValue(ControlId const switchId, SwitchStateId const switchStateId)
{
	m_switchStates[switchId] = switchStateId;
}

//////////////////////////////////////////////////////////////////////////
bool CGlobalObject::ExecuteDefaultTrigger(ControlId const id)
{
	bool wasSuccess = true;

	switch (id)
	{
	case g_loseFocusTriggerId:
		{
			g_loseFocusTrigger.Execute();
			break;
		}
	case g_getFocusTriggerId:
		{
			g_getFocusTrigger.Execute();
			break;
		}
	case g_muteAllTriggerId:
		{
			g_muteAllTrigger.Execute();
			break;
		}
	case g_unmuteAllTriggerId:
		{
			g_unmuteAllTrigger.Execute();
			break;
		}
	case g_pauseAllTriggerId:
		{
			g_pauseAllTrigger.Execute();
			break;
		}
	case g_resumeAllTriggerId:
		{
			g_resumeAllTrigger.Execute();
			break;
		}
	default:
		{
			wasSuccess = false;
			break;
		}
	}

	return wasSuccess;
}

///////////////////////////////////////////////////////////////////////////
void CGlobalObject::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY)
{
	auxGeom.Draw2dLabel(posX, posY, Debug::g_listHeaderFontSize, Debug::s_globalColorHeader, false, "Global Object");
	posY += Debug::g_listHeaderLineHeight;

	// Check if text filter is enabled.
	CryFixedStringT<MaxControlNameLength> lowerCaseSearchString(g_cvars.m_pDebugFilter->GetString());
	lowerCaseSearchString.MakeLower();
	bool const isTextFilterDisabled = (lowerCaseSearchString.empty() || (lowerCaseSearchString.compareNoCase("0") == 0));
	bool const filterAllObjectInfo = (g_cvars.m_drawDebug & Debug::EDrawFilter::FilterAllObjectInfo) != 0;

	// Check if any trigger matches text filter.
	bool doesTriggerMatchFilter = false;
	std::vector<CryFixedStringT<MaxMiscStringLength>> triggerInfo;

	if (!m_triggerInstances.empty() || filterAllObjectInfo)
	{
		Debug::TriggerCounts triggerCounts;

		for (auto const& triggerInstancePair : m_triggerInstances)
		{
			++(triggerCounts[triggerInstancePair.second->GetTriggerId()]);
		}

		for (auto const& triggerCountsPair : triggerCounts)
		{
			CTrigger const* const pTrigger = stl::find_in_map(g_triggers, triggerCountsPair.first, nullptr);

			if (pTrigger != nullptr)
			{
				char const* const szTriggerName = pTrigger->GetName();

				if (!isTextFilterDisabled)
				{
					CryFixedStringT<MaxControlNameLength> lowerCaseTriggerName(szTriggerName);
					lowerCaseTriggerName.MakeLower();

					if (lowerCaseTriggerName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos)
					{
						doesTriggerMatchFilter = true;
					}
				}

				CryFixedStringT<MaxMiscStringLength> debugText;
				uint8 const numInstances = triggerCountsPair.second;

				if (numInstances == 1)
				{
					debugText.Format("%s\n", szTriggerName);
				}
				else
				{
					debugText.Format("%s: %u\n", szTriggerName, numInstances);
				}

				triggerInfo.emplace_back(debugText);
			}
		}
	}

	// Check if any state or switch matches text filter.
	bool doesStateSwitchMatchFilter = false;
	std::map<CSwitch const* const, CSwitchState const* const> switchStateInfo;

	if (!m_switchStates.empty() || filterAllObjectInfo)
	{
		for (auto const& switchStatePair : m_switchStates)
		{
			CSwitch const* const pSwitch = stl::find_in_map(g_switches, switchStatePair.first, nullptr);

			if (pSwitch != nullptr)
			{
				CSwitchState const* const pSwitchState = stl::find_in_map(pSwitch->GetStates(), switchStatePair.second, nullptr);

				if (pSwitchState != nullptr)
				{
					if (!isTextFilterDisabled)
					{
						char const* const szSwitchName = pSwitch->GetName();
						CryFixedStringT<MaxControlNameLength> lowerCaseSwitchName(szSwitchName);
						lowerCaseSwitchName.MakeLower();
						char const* const szStateName = pSwitchState->GetName();
						CryFixedStringT<MaxControlNameLength> lowerCaseStateName(szStateName);
						lowerCaseStateName.MakeLower();

						if ((lowerCaseSwitchName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos) ||
						    (lowerCaseStateName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos))
						{
							doesStateSwitchMatchFilter = true;
						}
					}

					switchStateInfo.emplace(pSwitch, pSwitchState);
				}
			}
		}
	}

	// Check if any parameter matches text filter.
	bool doesParameterMatchFilter = false;
	std::map<char const* const, float const> parameterInfo;

	if (!m_parameters.empty() || filterAllObjectInfo)
	{
		for (auto const& parameterPair : m_parameters)
		{
			CParameter const* const pParameter = stl::find_in_map(g_parameters, parameterPair.first, nullptr);

			if (pParameter != nullptr)
			{
				char const* const szParameterName = pParameter->GetName();

				if (!isTextFilterDisabled)
				{
					CryFixedStringT<MaxControlNameLength> lowerCaseParameterName(szParameterName);
					lowerCaseParameterName.MakeLower();

					if (lowerCaseParameterName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos)
					{
						doesParameterMatchFilter = true;
					}
				}

				parameterInfo.emplace(szParameterName, parameterPair.second);
			}
		}
	}

	if ((g_cvars.m_hideInactiveObjects == 0) || ((g_cvars.m_hideInactiveObjects != 0) && !m_triggerInstances.empty()))
	{
		if (isTextFilterDisabled || doesTriggerMatchFilter)
		{
			for (auto const& debugText : triggerInfo)
			{
				auxGeom.Draw2dLabel(
					posX,
					posY,
					Debug::g_objectFontSize,
					Debug::s_objectColorTrigger,
					false,
					"%s", debugText.c_str());

				posY += Debug::g_objectLineHeight;
			}
		}

		if (isTextFilterDisabled || doesStateSwitchMatchFilter)
		{
			for (auto const& switchStatePair : switchStateInfo)
			{
				auto const pSwitch = switchStatePair.first;
				auto const pSwitchState = switchStatePair.second;

				Debug::CStateDrawData& drawData = m_stateDrawInfo.emplace(std::piecewise_construct, std::forward_as_tuple(pSwitch->GetId()), std::forward_as_tuple(pSwitchState->GetId())).first->second;
				drawData.Update(pSwitchState->GetId());
				ColorF const switchTextColor = { 0.8f, drawData.m_currentSwitchColor, 0.6f };

				auxGeom.Draw2dLabel(
					posX,
					posY,
					Debug::g_objectFontSize,
					switchTextColor,
					false,
					"%s: %s\n",
					pSwitch->GetName(),
					pSwitchState->GetName());

				posY += Debug::g_objectLineHeight;
			}
		}

		if (isTextFilterDisabled || doesParameterMatchFilter)
		{
			for (auto const& parameterPair : parameterInfo)
			{
				auxGeom.Draw2dLabel(
					posX,
					posY,
					Debug::g_objectFontSize,
					Debug::s_objectColorParameter,
					false,
					"%s: %2.2f\n",
					parameterPair.first,
					parameterPair.second);

				posY += Debug::g_objectLineHeight;
			}
		}

		if ((g_cvars.m_drawDebug & Debug::EDrawFilter::ObjectImplInfo) != 0)
		{
			m_pIObject->DrawDebugInfo(auxGeom, posX, posY, (isTextFilterDisabled ? nullptr : lowerCaseSearchString.c_str()));
		}
	}
}
#endif // CRY_AUDIO_USE_PRODUCTION_CODE
}      // namespace CryAudio
