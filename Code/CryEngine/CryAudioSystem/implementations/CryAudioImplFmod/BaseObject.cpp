// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "BaseObject.h"
#include "Impl.h"
#include "BaseStandaloneFile.h"
#include "Event.h"
#include "EventInstance.h"
#include "Parameter.h"
#include "ParameterState.h"
#include "Return.h"

#include <CryAudio/IAudioSystem.h>

#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	#include <Logger.h>
#endif // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CBaseObject::CBaseObject()
	: m_flags(EObjectFlags::None)
{
	ZeroStruct(m_attributes);
	m_attributes.forward.z = 1.0f;
	m_attributes.up.y = 1.0f;

	m_eventInstances.reserve(2);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::Update(float const deltaTime)
{
	if (!m_pendingEventInstances.empty())
	{
		auto iter(m_pendingEventInstances.begin());
		auto iterEnd(m_pendingEventInstances.end());

		while (iter != iterEnd)
		{
			if (SetEventInstance(*iter))
			{
				if (iter != (iterEnd - 1))
				{
					(*iter) = m_pendingEventInstances.back();
				}

				m_pendingEventInstances.pop_back();
				iter = m_pendingEventInstances.begin();
				iterEnd = m_pendingEventInstances.end();
			}
			else
			{
				++iter;
			}
		}
	}

	if (!m_pendingFiles.empty())
	{
		auto iter(m_pendingFiles.begin());
		auto iterEnd(m_pendingFiles.end());

		while (iter != iterEnd)
		{
			CBaseStandaloneFile* const pStandaloneFile = *iter;

			if (pStandaloneFile->IsReady())
			{
				m_files.push_back(pStandaloneFile);

				pStandaloneFile->PlayFile(m_attributes);

				if (iter != (iterEnd - 1))
				{
					(*iter) = m_pendingFiles.back();
				}

				m_pendingFiles.pop_back();
				iter = m_pendingFiles.begin();
				iterEnd = m_pendingFiles.end();
			}
			else
			{
				++iter;
			}
		}
	}

	EObjectFlags const previousFlags = m_flags;
	bool removedEvent = false;

	if (!m_eventInstances.empty())
	{
		m_flags |= EObjectFlags::IsVirtual;
	}

	auto iter(m_eventInstances.begin());
	auto iterEnd(m_eventInstances.end());

	while (iter != iterEnd)
	{
		auto const pEventInstance = *iter;

		if (pEventInstance->IsToBeRemoved())
		{
			gEnv->pAudioSystem->ReportFinishedTriggerConnectionInstance(pEventInstance->GetTriggerInstanceId());
			g_pImpl->DestructEventInstance(pEventInstance);
			removedEvent = true;

			if (iter != (iterEnd - 1))
			{
				(*iter) = m_eventInstances.back();
			}

			m_eventInstances.pop_back();
			iter = m_eventInstances.begin();
			iterEnd = m_eventInstances.end();
		}
		else
		{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
			// Always update in production code for debug draw.
			pEventInstance->UpdateVirtualState();

			if (pEventInstance->GetState() != EEventState::Virtual)
			{
				m_flags &= ~EObjectFlags::IsVirtual;
			}
#else
			if (((m_flags& EObjectFlags::IsVirtual) != 0) && ((m_flags& EObjectFlags::UpdateVirtualStates) != 0))
			{
				pEventInstance->UpdateVirtualState();

				if (pEventInstance->GetState() != EEventState::Virtual)
				{
					m_flags &= ~EObjectFlags::IsVirtual;
				}
			}
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
			++iter;
		}
	}

	if ((previousFlags != m_flags) && (!m_eventInstances.empty() || !m_pendingEventInstances.empty()))
	{
		if (((previousFlags& EObjectFlags::IsVirtual) != 0) && ((m_flags& EObjectFlags::IsVirtual) == 0))
		{
			gEnv->pAudioSystem->ReportPhysicalizedObject(this);
		}
		else if (((previousFlags& EObjectFlags::IsVirtual) == 0) && ((m_flags& EObjectFlags::IsVirtual) != 0))
		{
			gEnv->pAudioSystem->ReportVirtualizedObject(this);
		}
	}

	if (removedEvent)
	{
		UpdateVelocityTracking();
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StopAllTriggers()
{
	for (auto const pEventInstance : m_eventInstances)
	{
		pEventInstance->StopImmediate();
	}
}
//////////////////////////////////////////////////////////////////////////
ERequestStatus CBaseObject::SetName(char const* const szName)
{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
	m_name = szName;
#endif  // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::StopEventInstance(uint32 const id)
{
	for (auto const pEventInstance : m_eventInstances)
	{
		if (pEventInstance->GetId() == id)
		{
			pEventInstance->StopAllowFadeOut();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetParameter(uint32 const id, float const value)
{
	FMOD_RESULT fmodResult = FMOD_ERR_UNINITIALIZED;

	for (auto const pEventInstance : m_eventInstances)
	{
		FMOD::Studio::EventInstance* const pFmodEventInstance = pEventInstance->GetFmodEventInstance();
		CRY_ASSERT_MESSAGE(pFmodEventInstance != nullptr, "Fmod event instance doesn't exist during %s", __FUNCTION__);
		CEvent const* const pEvent = pEventInstance->GetEvent();
		CRY_ASSERT_MESSAGE(pEvent != nullptr, "Event doesn't exist during %s", __FUNCTION__);

		FMOD::Studio::EventDescription* pEventDescription = nullptr;
		fmodResult = pFmodEventInstance->getDescription(&pEventDescription);
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

		if (g_eventToParameterIndexes.find(pEvent) != g_eventToParameterIndexes.end())
		{
			ParameterIdToIndex& parameters = g_eventToParameterIndexes[pEvent];

			if (parameters.find(id) != parameters.end())
			{
				fmodResult = pFmodEventInstance->setParameterValueByIndex(parameters[id], value);
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
			}
			else
			{
				int parameterCount = 0;
				fmodResult = pFmodEventInstance->getParameterCount(&parameterCount);
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

				for (int index = 0; index < parameterCount; ++index)
				{
					FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDescription;
					fmodResult = pEventDescription->getParameterByIndex(index, &parameterDescription);
					CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

					if (id == StringToId(parameterDescription.name))
					{
						parameters.emplace(id, index);
						fmodResult = pFmodEventInstance->setParameterValueByIndex(index, value);
						CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
						break;
					}
				}
			}
		}
		else
		{
			int parameterCount = 0;
			fmodResult = pFmodEventInstance->getParameterCount(&parameterCount);
			CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

			for (int index = 0; index < parameterCount; ++index)
			{
				FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDescription;
				fmodResult = pEventDescription->getParameterByIndex(index, &parameterDescription);
				CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

				if (id == StringToId(parameterDescription.name))
				{
					g_eventToParameterIndexes[pEvent].emplace(std::make_pair(id, index));
					fmodResult = pFmodEventInstance->setParameterValueByIndex(index, value);
					CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
					break;
				}
			}
		}
	}

	m_parameters[id] = value;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveParameter(uint32 const id)
{
	m_parameters.erase(id);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::SetReturn(CReturn const* const pReturn, float const amount)
{
	bool shouldUpdate = true;
	auto const iter(m_returns.find(pReturn));

	if (iter != m_returns.end())
	{
		if (shouldUpdate = (fabs(iter->second - amount) > 0.001f))
		{
			iter->second = amount;
		}
	}
	else
	{
		m_returns.emplace(pReturn, amount);
	}

	if (shouldUpdate)
	{
		for (auto const pEventInstance : m_eventInstances)
		{
			pEventInstance->SetReturnSend(pReturn, amount);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveReturn(CReturn const* const pReturn)
{
	m_returns.erase(pReturn);
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::RemoveFile(CBaseStandaloneFile const* const pFile)
{
	if (!stl::find_and_erase(m_files, pFile))
	{
		if (!stl::find_and_erase(m_pendingFiles, pFile))
		{
#if defined(CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE)
			Cry::Audio::Log(ELogType::Error, "Tried to remove an audio file from an object that is not playing that file");
#endif      // CRY_AUDIO_IMPL_FMOD_USE_PRODUCTION_CODE
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CBaseObject::SetEventInstance(CEventInstance* const pEventInstance)
{
	bool bSuccess = false;

	// Update the event with all parameter and environment values
	// that are currently set on the object before starting it.
	if (pEventInstance->PrepareForOcclusion())
	{
		m_eventInstances.push_back(pEventInstance);

		FMOD::Studio::EventInstance* const pFModEventInstance = pEventInstance->GetFmodEventInstance();
		CRY_ASSERT_MESSAGE(pFModEventInstance != nullptr, "Fmod event instance doesn't exist during %s", __FUNCTION__);
		CEvent const* const pEvent = pEventInstance->GetEvent();
		CRY_ASSERT_MESSAGE(pEvent != nullptr, "Event doesn't exist during %s", __FUNCTION__);

		FMOD::Studio::EventDescription* pEventDescription = nullptr;
		FMOD_RESULT fmodResult = pFModEventInstance->getDescription(&pEventDescription);
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

		if (g_eventToParameterIndexes.find(pEvent) != g_eventToParameterIndexes.end())
		{
			ParameterIdToIndex& parameters = g_eventToParameterIndexes[pEvent];

			for (auto const& parameterPair : m_parameters)
			{
				uint32 const parameterId = parameterPair.first;

				if (parameters.find(parameterId) != parameters.end())
				{
					fmodResult = pFModEventInstance->setParameterValueByIndex(parameters[parameterId], parameterPair.second);
					CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
				}
				else
				{
					int parameterCount = 0;
					fmodResult = pFModEventInstance->getParameterCount(&parameterCount);
					CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

					for (int index = 0; index < parameterCount; ++index)
					{
						FMOD_STUDIO_PARAMETER_DESCRIPTION parameterDescription;
						fmodResult = pEventDescription->getParameterByIndex(index, &parameterDescription);
						CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

						if (parameterId == StringToId(parameterDescription.name))
						{
							parameters.emplace(parameterId, index);
							fmodResult = pFModEventInstance->setParameterValueByIndex(index, parameterPair.second);
							CRY_AUDIO_IMPL_FMOD_ASSERT_OK;
							break;
						}
					}
				}
			}
		}

		for (auto const& returnPair : m_returns)
		{
			pEventInstance->SetReturnSend(returnPair.first, returnPair.second);
		}

		UpdateVelocityTracking();
		pEventInstance->SetOcclusion(m_occlusion);
		pEventInstance->SetAbsoluteVelocity(m_absoluteVelocity);

		fmodResult = pEventInstance->GetFmodEventInstance()->start();
		CRY_AUDIO_IMPL_FMOD_ASSERT_OK;

		bSuccess = true;
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
void CBaseObject::UpdateVelocityTracking()
{
	bool trackVelocity = false;

	for (auto const pEventInstance : m_eventInstances)
	{
		if (pEventInstance->HasAbsoluteVelocityParameter())
		{
			trackVelocity = true;
			break;
		}
	}

	trackVelocity ? (m_flags |= EObjectFlags::TrackAbsoluteVelocity) : (m_flags &= ~EObjectFlags::TrackAbsoluteVelocity);
}
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
