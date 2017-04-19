// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioObjectManager.h"
#include "AudioEventManager.h"
#include "AudioStandaloneFileManager.h"
#include "AudioListenerManager.h"
#include "ATLAudioObject.h"
#include "AudioCVars.h"
#include "IAudioImpl.h"
#include "SharedAudioData.h"

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include <CryRenderer/IRenderAuxGeom.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

using namespace CryAudio;
using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
CAudioObjectManager::CAudioObjectManager(
	CAudioEventManager& audioEventMgr,
	CAudioStandaloneFileManager& audioStandaloneFileMgr,
	CAudioListenerManager const& listenerManager)
	: m_pImpl(nullptr)
	, m_timeSinceLastControlsUpdate(0.0f)
	, m_audioEventMgr(audioEventMgr)
	, m_audioStandaloneFileMgr(audioStandaloneFileMgr)
	, m_listenerManager(listenerManager)
{}

//////////////////////////////////////////////////////////////////////////
CAudioObjectManager::~CAudioObjectManager()
{
	if (m_pImpl != nullptr)
	{
		Release();
	}

	for (auto pAudioObject : m_constructedAudioObjects)
	{
		delete pAudioObject;
	}

	m_constructedAudioObjects.clear();
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::Init(IAudioImpl* const pImpl)
{
	m_pImpl = pImpl;

	for (auto const pAudioObject : m_constructedAudioObjects)
	{
		CRY_ASSERT(pAudioObject->GetImplDataPtr() == nullptr);
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		pAudioObject->SetImplDataPtr(m_pImpl->ConstructAudioObject(pAudioObject->m_name.c_str()));
#else
		pAudioObject->SetImplDataPtr(m_pImpl->ConstructAudioObject());
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::Release()
{
	for (auto const pAudioObject : m_constructedAudioObjects)
	{
		m_pImpl->DestructAudioObject(pAudioObject->GetImplDataPtr());
		pAudioObject->Release();
	}

	m_pImpl = nullptr;
}

float CAudioObjectManager::s_controlsUpdateInterval = 10.0f;

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::Update(float const deltaTime, SObject3DAttributes const& listenerAttributes)
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CPropagationProcessor::s_totalAsyncPhysRays = 0;
	CPropagationProcessor::s_totalSyncPhysRays = 0;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	m_timeSinceLastControlsUpdate += deltaTime;
	bool const bUpdateControls = m_timeSinceLastControlsUpdate > s_controlsUpdateInterval;

	auto iterEnd = m_constructedAudioObjects.cend();
	for (auto iter = m_constructedAudioObjects.begin(); iter != iterEnd; )
	{
		CATLAudioObject* const pAudioObject = *iter;

		CObjectTransformation const& transformation = pAudioObject->GetTransformation();

		float const distance = transformation.GetPosition().GetDistance(listenerAttributes.transformation.GetPosition());
		float const radius = pAudioObject->GetMaxRadius();

		if (radius <= 0.0f || distance < radius)
		{
			if ((pAudioObject->GetFlags() & EAudioObjectFlags::Virtual) > 0)
			{
				pAudioObject->RemoveFlag(EAudioObjectFlags::Virtual);
			}
		}
		else
		{
			if ((pAudioObject->GetFlags() & EAudioObjectFlags::Virtual) == 0)
			{
				pAudioObject->SetFlag(EAudioObjectFlags::Virtual);
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
				pAudioObject->ResetObstructionRays();
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE
			}
		}

		if (IsActive(pAudioObject))
		{
			pAudioObject->Update(deltaTime, distance, listenerAttributes.transformation.GetPosition());

			if (pAudioObject->CanRunObstructionOcclusion() && pAudioObject->HasNewOcclusionValues())
			{
				SATLSoundPropagationData propagationData;
				pAudioObject->GetPropagationData(propagationData);

				pAudioObject->GetImplDataPtr()->SetObstructionOcclusion(propagationData.obstruction, propagationData.occlusion);
			}

			if (bUpdateControls)
			{
				pAudioObject->UpdateControls(m_timeSinceLastControlsUpdate, listenerAttributes);
			}

			pAudioObject->GetImplDataPtr()->Update();
		}
		else
		{
			if (pAudioObject->CanBeReleased())
			{
				iter = m_constructedAudioObjects.erase(iter);
				iterEnd = m_constructedAudioObjects.cend();
				m_pImpl->DestructAudioObject(pAudioObject->GetImplDataPtr());
				pAudioObject->SetImplDataPtr(nullptr);

				delete pAudioObject;
				continue;
			}
		}
		++iter;
	}

	if (bUpdateControls)
	{
		m_timeSinceLastControlsUpdate = 0.0f;
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::RegisterObject(CATLAudioObject* const pObject)
{
	m_constructedAudioObjects.push_back(pObject);
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::ReportStartedEvent(CATLEvent* const pEvent)
{
	if (pEvent != nullptr)
	{
		CATLAudioObject* const pAudioObject = pEvent->m_pAudioObject;
		CRY_ASSERT_MESSAGE(pAudioObject, "Event reported as started has no audio object!");
		pAudioObject->ReportStartedEvent(pEvent);
	}
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "NULL pEvent in CAudioObjectManager::ReportStartedEvent");
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::ReportFinishedEvent(CATLEvent* const pEvent, bool const bSuccess)
{
	if (pEvent != nullptr)
	{
		CATLAudioObject* const pAudioObject = pEvent->m_pAudioObject;
		CRY_ASSERT_MESSAGE(pAudioObject, "Event reported as finished has no audio object!");
		pAudioObject->ReportFinishedEvent(pEvent, bSuccess);
	}
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "NULL pEvent in CAudioObjectManager::ReportFinishedEvent");
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::GetStartedStandaloneFileRequestData(CATLStandaloneFile* const pStandaloneFile, CAudioRequest& request)
{
	if (pStandaloneFile != nullptr)
	{
		CATLAudioObject* const pAudioObject = pStandaloneFile->m_pAudioObject;
		CRY_ASSERT_MESSAGE(pAudioObject, "Standalone file request without audio object!");
		pAudioObject->GetStartedStandaloneFileRequestData(pStandaloneFile, request);
	}
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "NULL _pStandaloneFile in CAudioObjectManager::GetStartedStandaloneFileRequestData");
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::ReportFinishedStandaloneFile(CATLStandaloneFile* const pStandaloneFile)
{
	if (pStandaloneFile != nullptr)
	{
		CATLAudioObject* const pAudioObject = pStandaloneFile->m_pAudioObject;
		CRY_ASSERT_MESSAGE(pAudioObject, "Standalone file reported as finished has no audio object!");
		pAudioObject->ReportFinishedStandaloneFile(pStandaloneFile);
	}
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "NULL _pStandaloneFile in CAudioObjectManager::ReportFinishedStandaloneFile");
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::ReleasePendingRays()
{
	for (auto pAudioObject : m_constructedAudioObjects)
	{
		if (pAudioObject != nullptr)
		{
			pAudioObject->ReleasePendingRays();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CAudioObjectManager::IsActive(CATLAudioObject const* const pAudioObject) const
{
	if ((pAudioObject->GetFlags() & EAudioObjectFlags::Virtual) == 0)
	{
		return HasActiveData(pAudioObject);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAudioObjectManager::HasActiveData(CATLAudioObject const* const pAudioObject) const
{
	for (auto const pEvent : pAudioObject->GetActiveEvents())
	{
		if (pEvent->IsPlaying())
		{
			return true;
		}
	}

	for (auto const& standaloneFilePair : pAudioObject->GetActiveStandaloneFiles())
	{
		CATLStandaloneFile const* const pStandaloneFile = standaloneFilePair.first;

		if (pStandaloneFile->IsPlaying())
		{
			return true;
		}
	}
	return false;
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
size_t CAudioObjectManager::GetNumAudioObjects() const
{
	return m_constructedAudioObjects.size();
}

//////////////////////////////////////////////////////////////////////////
size_t CAudioObjectManager::GetNumActiveAudioObjects() const
{
	size_t numActiveAudioObjects = 0;
	for (auto pAudioObject : m_constructedAudioObjects)
	{
		if (IsActive(pAudioObject))
		{
			++numActiveAudioObjects;
		}
	}

	return numActiveAudioObjects;
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::DrawPerObjectDebugInfo(
  IRenderAuxGeom& auxGeom,
  Vec3 const& listenerPos,
  AudioTriggerLookup const& triggers,
  AudioParameterLookup const& parameters,
  AudioSwitchLookup const& switches,
  AudioPreloadRequestLookup const& preloadRequests,
  AudioEnvironmentLookup const& environments) const
{
	for (auto pAudioObject : m_constructedAudioObjects)
	{
		if (IsActive(pAudioObject))
		{
			pAudioObject->DrawDebugInfo(
			  auxGeom,
			  listenerPos,
			  triggers,
			  parameters,
			  switches,
			  preloadRequests,
			  environments);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const
{
	static float const headerColor[4] = { 1.0f, 1.0f, 1.0f, 0.9f };
	static float const itemActiveColor[4] = { 0.1f, 0.6f, 0.1f, 0.9f };
	static float const itemInactiveColor[4] = { 0.8f, 0.8f, 0.8f, 0.9f };
	static float const itemVirtualColor[4] = { 0.1f, 0.8f, 0.8f, 0.9f };

	size_t numAudioObjects = 0;
	float const headerPosY = posY;
	posX += 20.0f;
	posY += 17.0f;

	for (auto pAudioObject : m_constructedAudioObjects)
	{
		char const* const szOriginalName = pAudioObject->m_name.c_str();
		CryFixedStringT<MaxControlNameLength> lowerCaseAudioObjectName(szOriginalName);
		lowerCaseAudioObjectName.MakeLower();
		CryFixedStringT<MaxControlNameLength> lowerCaseSearchString(g_audioCVars.m_pAudioObjectsDebugFilter->GetString());
		lowerCaseSearchString.MakeLower();
		bool const bHasActiveData = HasActiveData(pAudioObject);
		bool const bIsVirtual = (pAudioObject->GetFlags() & EAudioObjectFlags::Virtual) > 0;
		bool const bStringFound = (lowerCaseSearchString.empty() || (lowerCaseSearchString.compareNoCase("0") == 0)) || (lowerCaseAudioObjectName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos);
		bool const bDraw = bStringFound && ((g_audioCVars.m_showActiveAudioObjectsOnly == 0) || (g_audioCVars.m_showActiveAudioObjectsOnly > 0 && bHasActiveData && !bIsVirtual));

		if (bDraw)
		{
			auxGeom.Draw2dLabel(posX, posY, 1.2f,
			                    bIsVirtual ? itemVirtualColor : (bHasActiveData ? itemActiveColor : itemInactiveColor),
			                    false,
			                    "%s : %.2f",
			                    szOriginalName, pAudioObject->GetMaxRadius());

			posY += 10.0f;
			++numAudioObjects;
		}
	}

	auxGeom.Draw2dLabel(posX, headerPosY, 1.6f, headerColor, false, "Audio Objects [%" PRISIZE_T "]", numAudioObjects);
}

#endif // INCLUDE_AUDIO_PRODUCTION_CODE
