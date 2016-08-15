// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioObjectManager.h"
#include "AudioEventManager.h"
#include "AudioStandaloneFileManager.h"
#include "ATLAudioObject.h"
#include "AudioCVars.h"
#include <CryRenderer/IRenderAuxGeom.h>

using namespace CryAudio::Impl;

// IDs below that number are reserved for the various unique
// objects inside the AudioSystem a user may want to address
// (e.g. an AudioManager, a MusicSystem, AudioListeners?...)
AudioObjectId const CAudioObjectManager::s_minAudioObjectId = 100;

//////////////////////////////////////////////////////////////////////////
CAudioObjectManager::CAudioObjectManager(CAudioEventManager& _audioEventMgr, CAudioStandaloneFileManager& _audioStandaloneFileMgr)
	: m_audioObjectPool(g_audioCVars.m_audioObjectPoolSize, s_minAudioObjectId)
	, m_pImpl(nullptr)
	, m_timeSinceLastControlsUpdate(0.0f)
	, m_audioEventMgr(_audioEventMgr)
	, m_audioStandaloneFileMgr(_audioStandaloneFileMgr)
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	, m_pDebugNameStore(nullptr)
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
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
		for (auto const& audioObjectPair : m_registeredAudioObjects)
		{
			CATLAudioObject* const pAudioObject = audioObjectPair.second;
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
			AudioObjectId const id = m_audioObjectPool.GetNextID();
			POOL_NEW_CREATE(CATLAudioObject, pNewObject)(id);
			m_audioObjectPool.m_reserved.push_back(pNewObject);
		}
	}

	for (auto const pAudioObject : m_audioObjectPool.m_reserved)
	{
		CRY_ASSERT(pAudioObject->GetImplDataPtr() == nullptr);
		pAudioObject->SetImplDataPtr(m_pImpl->NewAudioObject(pAudioObject->GetId()));
	}

	EAudioRequestStatus result = eAudioRequestStatus_None;

	for (auto const& audioObjectPair : m_registeredAudioObjects)
	{
		CATLAudioObject* const pAudioObject = audioObjectPair.second;
		CRY_ASSERT(pAudioObject->GetImplDataPtr() == nullptr);
		pAudioObject->SetImplDataPtr(m_pImpl->NewAudioObject(pAudioObject->GetId()));

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		char const* const szAudioObjectName = m_pDebugNameStore->LookupAudioObjectName(pAudioObject->GetId());
		result = m_pImpl->RegisterAudioObject(pAudioObject->GetImplDataPtr(), szAudioObjectName);
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

	for (auto const& audioObjectPair : m_registeredAudioObjects)
	{
		CATLAudioObject* const pAudioObject = audioObjectPair.second;
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

	RegisteredAudioObjectsMap::const_iterator iterEnd = m_registeredAudioObjects.cend();
	for (RegisteredAudioObjectsMap::iterator iter = m_registeredAudioObjects.begin(); iter != iterEnd; )
	{
		CATLAudioObject* const pAudioObject = iter->second;

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
				m_registeredAudioObjects.erase(iter++);
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
bool CAudioObjectManager::ReserveId(AudioObjectId& audioObjectId)
{
	CATLAudioObject* const pAudioObject = GetInstance();

	bool bSuccess = false;
	audioObjectId = INVALID_AUDIO_OBJECT_ID;

	if (pAudioObject != nullptr)
	{
		EAudioRequestStatus const result = (m_pImpl->RegisterAudioObject(pAudioObject->GetImplDataPtr()));

		if (result == eAudioRequestStatus_Success)
		{
			pAudioObject->SetFlag(eAudioObjectFlags_DoNotRelease);
			audioObjectId = pAudioObject->GetId();
			m_registeredAudioObjects.insert(std::make_pair(audioObjectId, pAudioObject));
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
class CObjectIDPredicate
{
public:

	explicit CObjectIDPredicate(AudioObjectId const audioObjectId)
		: m_audioObjectId(audioObjectId)
	{}

	~CObjectIDPredicate() {}

	bool operator()(CATLAudioObject const* const pAudioObject)
	{
		return pAudioObject->GetId() == m_audioObjectId;
	}

private:

	DELETE_DEFAULT_CONSTRUCTOR(CObjectIDPredicate);
	AudioObjectId const m_audioObjectId;
};

//////////////////////////////////////////////////////////////////////////
bool CAudioObjectManager::ReserveThisId(AudioObjectId const audioObjectId)
{
	bool bSuccess = false;
	CATLAudioObject* pAudioObject = LookupId(audioObjectId);

	if (pAudioObject == nullptr)
	{
		//no active object uses nAudioObjectID, so we can create one
		TAudioObjectPool::TPointerContainer::iterator ipObject = m_audioObjectPool.m_reserved.begin();
		// not using const for .end() iterator, because find_if seems to expect two iterators of the same type
		TAudioObjectPool::TPointerContainer::iterator const ipObjectEnd = m_audioObjectPool.m_reserved.end();

		ipObject = std::find_if(ipObject, ipObjectEnd, CObjectIDPredicate(audioObjectId));

		if (ipObject != ipObjectEnd)
		{
			// there is a reserved instance with the required ID
			std::swap(*ipObject, m_audioObjectPool.m_reserved.back());
			pAudioObject = m_audioObjectPool.m_reserved.back();
			m_audioObjectPool.m_reserved.pop_back();
		}
		else
		{
			// none of the reserved instances have the required ID
			IAudioObject* const pObjectData = m_pImpl->NewAudioObject(audioObjectId);
			POOL_NEW(CATLAudioObject, pAudioObject)(audioObjectId, pObjectData);
		}

		m_registeredAudioObjects.insert(std::make_pair(audioObjectId, pAudioObject));

		bSuccess = true;
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
bool CAudioObjectManager::ReleaseId(AudioObjectId const audioObjectId)
{
	bool bSuccess = false;
	CATLAudioObject* const pAudioObject = LookupId(audioObjectId);

	if (pAudioObject != nullptr)
	{
		pAudioObject->RemoveFlag(eAudioObjectFlags_DoNotRelease);
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
CATLAudioObject* CAudioObjectManager::LookupId(AudioObjectId const audioObjectId) const
{
	RegisteredAudioObjectsMap::const_iterator iter;
	CATLAudioObject* pResult = nullptr;

	if (FindPlaceConst(m_registeredAudioObjects, audioObjectId, iter))
	{
		pResult = iter->second;
	}

	return pResult;
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::ReportStartedEvent(CATLEvent* const pEvent)
{
	if (pEvent != nullptr)
	{
		CATLAudioObject* const pAudioObject = LookupId(pEvent->m_audioObjectId);

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
			  m_pDebugNameStore->LookupAudioObjectName(pEvent->m_audioObjectId));
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
		CATLAudioObject* const pAudioObject = LookupId(pEvent->m_audioObjectId);

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
			  m_pDebugNameStore->LookupAudioObjectName(pEvent->m_audioObjectId));
		}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "NULL pEvent in CAudioObjectManager::ReportFinishedEvent");
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::GetStartedStandaloneFileRequestData(CATLStandaloneFile* const _pStandaloneFile, CAudioRequestInternal& _request)
{
	if (_pStandaloneFile != nullptr)
	{
		CATLAudioObject* const pAudioObject = LookupId(_pStandaloneFile->m_audioObjectId);

		if (pAudioObject != nullptr)
		{
			pAudioObject->GetStartedStandaloneFileRequestData(_pStandaloneFile, _request);
		}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		else
		{
			g_audioLogger.Log(
			  eAudioLogType_Warning,
			  "Requesting request data from standalone file %u on Object %s which no longer exists!",
			  _pStandaloneFile->GetId(),
			  m_pDebugNameStore->LookupAudioObjectName(_pStandaloneFile->m_audioObjectId));
		}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}
	else
	{
		g_audioLogger.Log(eAudioLogType_Warning, "NULL _pStandaloneFile in CAudioObjectManager::GetStartedStandaloneFileRequestData");
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::ReportFinishedStandaloneFile(CATLStandaloneFile* const _pStandaloneFile)
{
	if (_pStandaloneFile != nullptr)
	{
		CATLAudioObject* const pAudioObject = LookupId(_pStandaloneFile->m_audioObjectId);

		if (pAudioObject != nullptr)
		{
			pAudioObject->ReportFinishedStandaloneFile(_pStandaloneFile);
		}
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		else
		{
			g_audioLogger.Log(
			  eAudioLogType_Warning,
			  "Removing standalone file %u from Object %s: Object no longer exists!",
			  _pStandaloneFile->GetId(),
			  m_pDebugNameStore->LookupAudioObjectName(_pStandaloneFile->m_audioObjectId));
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
		AudioObjectId const id = m_audioObjectPool.GetNextID();
		IAudioObject* const pImplAudioObject = m_pImpl->NewAudioObject(id);
		POOL_NEW(CATLAudioObject, pAudioObject)(id, pImplAudioObject);

		if (pAudioObject == nullptr)
		{
			--m_audioObjectPool.m_idCounter;
			g_audioLogger.Log(eAudioLogType_Warning, "Failed to get a new instance of an AudioObject from the implementation");
		}
	}

	return pAudioObject;
}

//////////////////////////////////////////////////////////////////////////
bool CAudioObjectManager::ReleaseInstance(CATLAudioObject* const pOldObject)
{
	CRY_ASSERT(pOldObject);
	bool bSuccess = false;

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	m_pDebugNameStore->RemoveAudioObject(static_cast<AudioObjectId const>(pOldObject->GetId()));
	pOldObject->CheckBeforeRemoval(m_pDebugNameStore);
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
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
	for (auto const& audioObjectPair : m_registeredAudioObjects)
	{
		CATLAudioObject* const pAudioObject = audioObjectPair.second;

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
bool CAudioObjectManager::ReserveId(AudioObjectId& audioObjectId, char const* const szAudioObjectName)
{
	CATLAudioObject* const pNewObject = GetInstance();

	bool bSuccess = false;
	audioObjectId = INVALID_AUDIO_OBJECT_ID;

	if (pNewObject != nullptr)
	{
		EAudioRequestStatus const result = (m_pImpl->RegisterAudioObject(pNewObject->GetImplDataPtr(), szAudioObjectName));

		if (result == eAudioRequestStatus_Success)
		{
			pNewObject->SetFlag(eAudioObjectFlags_DoNotRelease);
			audioObjectId = pNewObject->GetId();
			m_registeredAudioObjects.insert(std::make_pair(audioObjectId, pNewObject));
			m_pDebugNameStore->AddAudioObject(audioObjectId, szAudioObjectName);
			bSuccess = true;
		}
		else
		{
			m_registeredAudioObjects.erase(static_cast<AudioObjectId const>(pNewObject->GetId()));
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
	for (auto const& audioObjectPair : m_registeredAudioObjects)
	{
		CATLAudioObject* const pAudioObject = audioObjectPair.second;

		if (IsActive(pAudioObject))
		{
			++numActiveAudioObjects;
		}
	}

	return numActiveAudioObjects;
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::DrawPerObjectDebugInfo(IRenderAuxGeom& auxGeom, Vec3 const& listenerPos) const
{
	for (auto const& audioObjectPair : m_registeredAudioObjects)
	{
		CATLAudioObject* const pAudioObject = audioObjectPair.second;

		if (IsActive(pAudioObject))
		{
			pAudioObject->DrawDebugInfo(auxGeom, listenerPos, m_pDebugNameStore);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const
{
	static float const fHeaderColor[4] = { 1.0f, 1.0f, 1.0f, 0.9f };
	static float const fItemActiveColor[4] = { 0.1f, 0.6f, 0.1f, 0.9f };
	static float const fItemInactiveColor[4] = { 0.8f, 0.8f, 0.8f, 0.9f };
	static float const fItemVirtualColor[4] = { 0.1f, 0.8f, 0.8f, 0.9f };

	size_t nObjects = 0;
	float const fHeaderPosY = posY;
	posX += 20.0f;
	posY += 17.0f;

	for (auto const& audioObjectPair : m_registeredAudioObjects)
	{
		CATLAudioObject* const pAudioObject = audioObjectPair.second;
		char const* const sOriginalName = m_pDebugNameStore->LookupAudioObjectName(pAudioObject->GetId());
		CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> sLowerCaseAudioObjectName(sOriginalName);
		sLowerCaseAudioObjectName.MakeLower();
		CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH> sLowerCaseSearchString(g_audioCVars.m_pAudioObjectsDebugFilter->GetString());
		sLowerCaseSearchString.MakeLower();
		bool const bHasActiveData = HasActiveData(pAudioObject);
		bool const bIsVirtual = (pAudioObject->GetFlags() & eAudioObjectFlags_Virtual) > 0;
		bool const bStringFound = (sLowerCaseSearchString.empty() || (sLowerCaseSearchString.compareNoCase("0") == 0)) || (sLowerCaseAudioObjectName.find(sLowerCaseSearchString) != CryFixedStringT<MAX_AUDIO_CONTROL_NAME_LENGTH>::npos);
		bool const bDraw = bStringFound && ((g_audioCVars.m_showActiveAudioObjectsOnly == 0) || (g_audioCVars.m_showActiveAudioObjectsOnly > 0 && bHasActiveData && !bIsVirtual));

		if (bDraw)
		{
			auxGeom.Draw2dLabel(posX, posY, 1.2f,
			                    bIsVirtual ? fItemVirtualColor : (bHasActiveData ? fItemActiveColor : fItemInactiveColor),
			                    false,
			                    "%u : %s : %.2f",
			                    pAudioObject->GetId(), sOriginalName, pAudioObject->GetMaxRadius());

			posY += 10.0f;
			++nObjects;
		}
	}

	auxGeom.Draw2dLabel(posX, fHeaderPosY, 1.6f, fHeaderColor, false, "Audio Objects [%" PRISIZE_T "]", nObjects);
}

//////////////////////////////////////////////////////////////////////////
void CAudioObjectManager::SetDebugNameStore(CATLDebugNameStore* const pDebugNameStore)
{
	m_pDebugNameStore = pDebugNameStore;
}

#endif // INCLUDE_AUDIO_PRODUCTION_CODE
