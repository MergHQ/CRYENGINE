// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioObjectManager.h"
#include "AudioEventManager.h"
#include "AudioStandaloneFileManager.h"
#include "ATLAudioObject.h"
#include "AudioCVars.h"
#include <CryRenderer/IRenderAuxGeom.h>

using namespace CryAudio::Impl;

//////////////////////////////////////////////////////////////////////////
CAudioObjectManager::CAudioObjectManager(CAudioEventManager& audioEventMgr, CAudioStandaloneFileManager& audioStandaloneFileMgr)
	: m_audioObjectPool(g_audioCVars.m_audioObjectPoolSize, 0)
	, m_pImpl(nullptr)
	, m_timeSinceLastControlsUpdate(0.0f)
	, m_audioEventMgr(audioEventMgr)
	, m_audioStandaloneFileMgr(audioStandaloneFileMgr)
{
}

//////////////////////////////////////////////////////////////////////////
CAudioObjectManager::~CAudioObjectManager()
{
	if (m_pImpl != nullptr)
	{
		Release();
	}

	if (!m_audioObjectPool.m_reserved.empty())
	{
		for (auto const pAudioObject : m_audioObjectPool.m_reserved)
		{
			CRY_ASSERT(pAudioObject->GetImplDataPtr() == nullptr);
			POOL_FREE(pAudioObject);
		}

		m_audioObjectPool.m_reserved.clear();
	}

	if (!m_registeredAudioObjects.empty())
	{
		for (auto pAudioObject : m_registeredAudioObjects)
		{
			CRY_ASSERT(pAudioObject->GetImplDataPtr() == nullptr);
			POOL_FREE(pAudioObject);
		}

		m_registeredAudioObjects.clear();
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::Init(IAudioImpl* const pImpl)
{
	m_pImpl = pImpl;

	size_t const numPooledAudioObjects = m_audioObjectPool.m_reserved.size();
	size_t const numRegisteredAudioObjects = m_registeredAudioObjects.size();
	size_t const numTotalObjects = numPooledAudioObjects + numRegisteredAudioObjects;
	CRY_ASSERT(!(numTotalObjects > 0 && numTotalObjects < m_audioObjectPool.m_reserveSize));

	if (m_audioObjectPool.m_reserveSize > numTotalObjects)
	{
		for (size_t i = 0; i < m_audioObjectPool.m_reserveSize - numRegisteredAudioObjects; ++i)
		{
			uint32 const id = m_audioObjectPool.GetNextID();
			POOL_NEW_CREATE(CATLAudioObject, pNewObject)();
			m_audioObjectPool.m_reserved.push_back(pNewObject);
		}
	}

	for (auto const pAudioObject : m_audioObjectPool.m_reserved)
	{
		CRY_ASSERT(pAudioObject->GetImplDataPtr() == nullptr);
		pAudioObject->SetImplDataPtr(m_pImpl->NewAudioObject());
	}

	EAudioRequestStatus result = eAudioRequestStatus_None;

	for (auto pAudioObject : m_registeredAudioObjects)
	{
		CRY_ASSERT(pAudioObject->GetImplDataPtr() == nullptr);
		pAudioObject->SetImplDataPtr(m_pImpl->NewAudioObject());

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		result = m_pImpl->RegisterAudioObject(pAudioObject->GetImplDataPtr(), pAudioObject->m_name.c_str());
		CRY_ASSERT(result == eAudioRequestStatus_Success);
#else
		result = m_pImpl->RegisterAudioObject(pAudioObject->GetImplDataPtr());
		CRY_ASSERT(result == eAudioRequestStatus_Success);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::Release()
{
	for (auto const pAudioObject : m_audioObjectPool.m_reserved)
	{
		m_pImpl->DeleteAudioObject(pAudioObject->GetImplDataPtr());
		pAudioObject->SetImplDataPtr(nullptr);
	}

	EAudioRequestStatus result = eAudioRequestStatus_None;

	for (auto pAudioObject : m_registeredAudioObjects)
	{
		IAudioObject* const pImplData = pAudioObject->GetImplDataPtr();
		result = m_pImpl->UnregisterAudioObject(pImplData);
		CRY_ASSERT(result == eAudioRequestStatus_Success);
		result = m_pImpl->ResetAudioObject(pImplData);
		CRY_ASSERT(result == eAudioRequestStatus_Success);
		m_pImpl->DeleteAudioObject(pImplData);
		pAudioObject->SetImplDataPtr(nullptr);
		pAudioObject->Release();
	}

	m_pImpl = nullptr;
}

float CAudioObjectManager::s_controlsUpdateInterval = 100.0f;

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::Update(float const deltaTime, SAudioObject3DAttributes const& listenerAttributes)
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CPropagationProcessor::s_totalAsyncPhysRays = 0;
	CPropagationProcessor::s_totalSyncPhysRays = 0;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	m_timeSinceLastControlsUpdate += deltaTime;
	bool const bUpdateControls = m_timeSinceLastControlsUpdate > s_controlsUpdateInterval;

	auto iterEnd = m_registeredAudioObjects.cend();
	for (auto iter = m_registeredAudioObjects.begin(); iter != iterEnd; )
	{
		CATLAudioObject* const pAudioObject = *iter;

		SAudioObject3DAttributes const& attributes = pAudioObject->Get3DAttributes();
		float const distance = (attributes.transformation.GetPosition() - listenerAttributes.transformation.GetPosition()).GetLength();
		float const radius = pAudioObject->GetMaxRadius();

		if (radius <= 0.0f || distance < radius)
		{
			if ((pAudioObject->GetFlags() & eAudioObjectFlags_Virtual) > 0)
			{
				pAudioObject->RemoveFlag(eAudioObjectFlags_Virtual);
			}
		}
		else
		{
			if ((pAudioObject->GetFlags() & eAudioObjectFlags_Virtual) == 0)
			{
				pAudioObject->SetFlag(eAudioObjectFlags_Virtual);
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

				m_pImpl->SetObstructionOcclusion(pAudioObject->GetImplDataPtr(), propagationData.obstruction, propagationData.occlusion);
			}

			if (bUpdateControls)
			{
				pAudioObject->UpdateControls(m_timeSinceLastControlsUpdate, listenerAttributes);
			}

			m_pImpl->UpdateAudioObject(pAudioObject->GetImplDataPtr());
		}
		else
		{
			if (pAudioObject->CanBeReleased())
			{
				iter = m_registeredAudioObjects.erase(iter);
				iterEnd = m_registeredAudioObjects.cend();
				ReleaseInstance(pAudioObject);
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
bool CAudioObjectManager::ReserveAudioObject(CATLAudioObject*& pAudioObjectId)
{
	CATLAudioObject* const pAudioObject = GetInstance();

	bool bSuccess = false;
	pAudioObjectId = nullptr;

	if (pAudioObject != nullptr)
	{
		EAudioRequestStatus const result = (m_pImpl->RegisterAudioObject(pAudioObject->GetImplDataPtr()));

		if (result == eAudioRequestStatus_Success)
		{
			pAudioObject->SetFlag(eAudioObjectFlags_DoNotRelease);
			pAudioObjectId = pAudioObject;
			m_registeredAudioObjects.push_back(pAudioObject);
			bSuccess = true;
		}
		else
		{
			ReleaseInstance(pAudioObject);
		}
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
class CObjectIDPredicate final
{
public:

	explicit CObjectIDPredicate(CATLAudioObject* const pAudioObject)
		: m_pAudioObject(pAudioObject)
	{}

	bool operator()(CATLAudioObject const* const pAudioObject)
	{
		return pAudioObject == m_pAudioObject;
	}

private:

	CATLAudioObject* const m_pAudioObject;
};

//////////////////////////////////////////////////////////////////////////
bool CAudioObjectManager::ReleaseAudioObject(CATLAudioObject* const pAudioObject)
{
	bool bSuccess = false;
	if (pAudioObject != nullptr)
	{
		pAudioObject->RemoveFlag(eAudioObjectFlags_DoNotRelease);
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::ReportStartedEvent(CATLEvent* const pEvent)
{
	if (pEvent != nullptr)
	{
		CATLAudioObject* const pAudioObject = pEvent->m_pAudioObject;

		if (pAudioObject != nullptr)
		{
			pAudioObject->ReportStartedEvent(pEvent);
		}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		else
		{
			g_audioLogger.Log(
			  eAudioLogType_Warning,
			  "Failed to report starting event %u on object %s as it does not exist!",
			  pEvent->GetId(),
			  pEvent->m_pAudioObject->m_name.c_str());
		}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
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

		if (pAudioObject != nullptr)
		{
			pAudioObject->ReportFinishedEvent(pEvent, bSuccess);
		}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		else
		{
			g_audioLogger.Log(
			  eAudioLogType_Warning,
			  "Removing Event %u from Object %s: Object no longer exists!",
			  pEvent->GetId(),
			  pEvent->m_pAudioObject->m_name.c_str());
		}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "NULL pEvent in CAudioObjectManager::ReportFinishedEvent");
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::GetStartedStandaloneFileRequestData(CATLStandaloneFile* const pStandaloneFile, CAudioRequestInternal& request)
{
	if (pStandaloneFile != nullptr)
	{
		CATLAudioObject* const pAudioObject = pStandaloneFile->m_pAudioObject;

		if (pAudioObject != nullptr)
		{
			pAudioObject->GetStartedStandaloneFileRequestData(pStandaloneFile, request);
		}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		else
		{
			g_audioLogger.Log(
			  eAudioLogType_Warning,
			  "Requesting request data from standalone file %u from a non existent AudioObject!",
			  pStandaloneFile->GetId());
		}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
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

		if (pAudioObject != nullptr)
		{
			pAudioObject->ReportFinishedStandaloneFile(pStandaloneFile);
		}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		else
		{
			g_audioLogger.Log(
			  eAudioLogType_Warning,
			  "Removing standalone file %u from a non existent AudioObject!",
			  pStandaloneFile->GetId());
		}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "NULL _pStandaloneFile in CAudioObjectManager::ReportFinishedStandaloneFile");
	}
}

//////////////////////////////////////////////////////////////////////////
CATLAudioObject* CAudioObjectManager::GetInstance()
{
	CATLAudioObject* pAudioObject = nullptr;

	if (!m_audioObjectPool.m_reserved.empty())
	{
		//have reserved instances
		pAudioObject = m_audioObjectPool.m_reserved.back();
		m_audioObjectPool.m_reserved.pop_back();
	}
	else
	{
		//need to get a new instance
		uint32 const id = m_audioObjectPool.GetNextID();
		IAudioObject* const pImplAudioObject = m_pImpl->NewAudioObject();
		POOL_NEW(CATLAudioObject, pAudioObject)(pImplAudioObject);

		if (pAudioObject == nullptr)
		{
			--m_audioObjectPool.m_idCounter;
			g_audioLogger.Log(eAudioLogType_Warning, "Failed to get a new instance of an AudioObject from the implementation");
		}
	} return pAudioObject;
}

//////////////////////////////////////////////////////////////////////////
bool CAudioObjectManager::ReleaseInstance(CATLAudioObject* const pOldObject)
{
	CRY_ASSERT(pOldObject);
	bool bSuccess = false;
	pOldObject->Clear();
	bSuccess = (m_pImpl->UnregisterAudioObject(pOldObject->GetImplDataPtr()) == eAudioRequestStatus_Success);
	m_pImpl->ResetAudioObject(pOldObject->GetImplDataPtr());

	if (m_audioObjectPool.m_reserved.size() < m_audioObjectPool.m_reserveSize)
	{
		// can return the instance to the reserved pool
		m_audioObjectPool.m_reserved.push_back(pOldObject);
	}
	else
	{
		// the reserve pool is full, can return the instance to the implementation to dispose
		m_pImpl->DeleteAudioObject(pOldObject->GetImplDataPtr());
		POOL_FREE(pOldObject);
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::ReleasePendingRays()
{
	for (auto pAudioObject : m_registeredAudioObjects)
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
	if ((pAudioObject->GetFlags() & eAudioObjectFlags_Virtual) == 0)
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
		CATLStandaloneFile const* const pStandaloneFile = m_audioStandaloneFileMgr.LookupId(standaloneFilePair.first);

		if (pStandaloneFile->IsPlaying())
		{
			return true;
		}
	}
	return false;
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
bool CAudioObjectManager::ReserveAudioObject(CATLAudioObject*& pAudioObject, char const* const szAudioObjectName)
{
	CATLAudioObject* const pNewObject = GetInstance();

	bool bSuccess = false;
	pAudioObject = nullptr;

	if (pNewObject != nullptr)
	{
		EAudioRequestStatus const result = (m_pImpl->RegisterAudioObject(pNewObject->GetImplDataPtr(), szAudioObjectName));

		if (result == eAudioRequestStatus_Success)
		{
			pNewObject->SetFlag(eAudioObjectFlags_DoNotRelease);
			pAudioObject = pNewObject;
			pAudioObject->m_name = szAudioObjectName;
			m_registeredAudioObjects.push_back(pAudioObject);
			bSuccess = true;
		}
		else
		{
			ReleaseInstance(pNewObject);
			bSuccess = false;
		}
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
size_t CAudioObjectManager::GetNumAudioObjects() const
{
	return m_registeredAudioObjects.size();
}

//////////////////////////////////////////////////////////////////////////
size_t CAudioObjectManager::GetNumActiveAudioObjects() const
{
	size_t numActiveAudioObjects = 0;
	for (auto pAudioObject : m_registeredAudioObjects)
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
  AudioRtpcLookup const& parameters,
  AudioSwitchLookup const& switches,
  AudioPreloadRequestLookup const& preloadRequests,
  AudioEnvironmentLookup const& environments,
  AudioStandaloneFileLookup const& audioStandaloneFiles) const
{
	for (auto pAudioObject : m_registeredAudioObjects)
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
			  environments,
			  audioStandaloneFiles);
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

	for (auto pAudioObject : m_registeredAudioObjects)
	{
		char const* const szOriginalName = pAudioObject->m_name.c_str();
		CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> lowerCaseAudioObjectName(szOriginalName);
		lowerCaseAudioObjectName.MakeLower();
		CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> lowerCaseSearchString(g_audioCVars.m_pAudioObjectsDebugFilter->GetString());
		lowerCaseSearchString.MakeLower();
		bool const bHasActiveData = HasActiveData(pAudioObject);
		bool const bIsVirtual = (pAudioObject->GetFlags() & eAudioObjectFlags_Virtual) > 0;
		bool const bStringFound = (lowerCaseSearchString.empty() || (lowerCaseSearchString.compareNoCase("0") == 0)) || (lowerCaseAudioObjectName.find(lowerCaseSearchString) != CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH>::npos);
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
