// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioObjectManager.h"
#include "Managers.h"
#include "AudioListenerManager.h"
#include "ATLAudioObject.h"
#include "AudioCVars.h"
#include "IAudioImpl.h"
#include "SharedAudioData.h"
#include "Common/Logger.h"

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include "DebugColor.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CObjectManager::~CObjectManager()
{
	CRY_ASSERT_MESSAGE(m_constructedObjects.empty(), "There are still objects during CObjectManager destruction!");
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::Initialize(uint32 const poolSize)
{
	m_constructedObjects.reserve(static_cast<std::size_t>(poolSize));
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::Terminate()
{
	for (auto const pObject : m_constructedObjects)
	{
		CRY_ASSERT_MESSAGE(pObject->GetImplDataPtr() == nullptr, "An object cannot have valid impl data during CObjectManager destruction!");
		delete pObject;
	}

	m_constructedObjects.clear();
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::OnAfterImplChanged()
{
	for (auto const pObject : m_constructedObjects)
	{
		CRY_ASSERT(pObject->GetImplDataPtr() == nullptr);
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
		pObject->SetImplDataPtr(g_pIImpl->ConstructObject(pObject->GetTransformation(), pObject->m_name.c_str()));
#else
		pObject->SetImplDataPtr(g_pIImpl->ConstructObject(pObject->GetTransformation()));
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::ReleaseImplData()
{
	// Don't delete objects here as we need them to survive a middleware switch!
	for (auto const pObject : m_constructedObjects)
	{
		g_pIImpl->DestructObject(pObject->GetImplDataPtr());
		pObject->Release();
	}
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::Update(float const deltaTime)
{
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	CPropagationProcessor::s_totalAsyncPhysRays = 0;
	CPropagationProcessor::s_totalSyncPhysRays = 0;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	if (deltaTime > 0.0f)
	{
		auto iter = m_constructedObjects.begin();
		auto iterEnd = m_constructedObjects.end();

		while (iter != iterEnd)
		{
			CATLAudioObject* const pObject = *iter;

			if (pObject->IsActive())
			{
				pObject->Update(deltaTime);
			}
			else if (pObject->CanBeReleased())
			{
				g_pIImpl->DestructObject(pObject->GetImplDataPtr());
				pObject->SetImplDataPtr(nullptr);
				delete pObject;

				if (iter != (iterEnd - 1))
				{
					(*iter) = m_constructedObjects.back();
				}

				m_constructedObjects.pop_back();
				iter = m_constructedObjects.begin();
				iterEnd = m_constructedObjects.end();
				continue;
			}

			++iter;
		}
	}
	else
	{
		for (auto const pObject : m_constructedObjects)
		{
			if (pObject->IsActive())
			{
				pObject->GetImplDataPtr()->Update(deltaTime);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::RegisterObject(CATLAudioObject* const pObject)
{
	m_constructedObjects.push_back(pObject);
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::ReportStartedEvent(CATLEvent* const pEvent)
{
	if (pEvent != nullptr)
	{
		CATLAudioObject* const pAudioObject = pEvent->m_pAudioObject;
		CRY_ASSERT_MESSAGE(pAudioObject, "Event reported as started has no audio object!");
		pAudioObject->ReportStartedEvent(pEvent);
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "NULL pEvent in CObjectManager::ReportStartedEvent");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::ReportFinishedEvent(CATLEvent* const pEvent, bool const bSuccess)
{
	if (pEvent != nullptr)
	{
		CATLAudioObject* const pAudioObject = pEvent->m_pAudioObject;
		CRY_ASSERT_MESSAGE(pAudioObject, "Event reported as finished has no audio object!");
		pAudioObject->ReportFinishedEvent(pEvent, bSuccess);
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "NULL pEvent in CObjectManager::ReportFinishedEvent");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::GetStartedStandaloneFileRequestData(CATLStandaloneFile* const pStandaloneFile, CAudioRequest& request)
{
	if (pStandaloneFile != nullptr)
	{
		CATLAudioObject* const pAudioObject = pStandaloneFile->m_pAudioObject;
		CRY_ASSERT_MESSAGE(pAudioObject, "Standalone file request without audio object!");
		pAudioObject->GetStartedStandaloneFileRequestData(pStandaloneFile, request);
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "NULL _pStandaloneFile in CObjectManager::GetStartedStandaloneFileRequestData");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::ReportFinishedStandaloneFile(CATLStandaloneFile* const pStandaloneFile)
{
	if (pStandaloneFile != nullptr)
	{
		CATLAudioObject* const pAudioObject = pStandaloneFile->m_pAudioObject;
		CRY_ASSERT_MESSAGE(pAudioObject, "Standalone file reported as finished has no audio object!");
		pAudioObject->ReportFinishedStandaloneFile(pStandaloneFile);
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "NULL _pStandaloneFile in CObjectManager::ReportFinishedStandaloneFile");
	}
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::ReleasePendingRays()
{
	for (auto const pObject : m_constructedObjects)
	{
		if (pObject != nullptr)
		{
			pObject->ReleasePendingRays();
		}
	}
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
size_t CObjectManager::GetNumAudioObjects() const
{
	return m_constructedObjects.size();
}

//////////////////////////////////////////////////////////////////////////
size_t CObjectManager::GetNumActiveAudioObjects() const
{
	size_t numActiveObjects = 0;

	for (auto const pObject : m_constructedObjects)
	{
		if (pObject->IsActive())
		{
			++numActiveObjects;
		}
	}

	return numActiveObjects;
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::DrawPerObjectDebugInfo(IRenderAuxGeom& auxGeom) const
{
	for (auto const pObject : m_constructedObjects)
	{
		if (pObject->IsActive())
		{
			pObject->DrawDebugInfo(auxGeom);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void DrawObjectDebugData(
	IRenderAuxGeom& auxGeom,
	float const posX,
	float& posY,
	CATLAudioObject const& object,
	CryFixedStringT<MaxControlNameLength> const& lowerCaseSearchString,
	size_t& numObjects)
{
	Vec3 const& position = object.GetTransformation().GetPosition();
	float const distance = position.GetDistance(g_listenerManager.GetActiveListenerTransformation().GetPosition());

	if (g_cvars.m_debugDistance <= 0.0f || (g_cvars.m_debugDistance > 0.0f && distance < g_cvars.m_debugDistance))
	{
		char const* const szObjectName = object.m_name.c_str();
		CryFixedStringT<MaxControlNameLength> lowerCaseObjectName(szObjectName);
		lowerCaseObjectName.MakeLower();
		bool const hasActiveData = object.IsActive();
		bool const isVirtual = (object.GetFlags() & EObjectFlags::Virtual) != 0;
		bool const stringFound = (lowerCaseSearchString.empty() || (lowerCaseSearchString.compareNoCase("0") == 0)) || (lowerCaseObjectName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos);
		bool const draw = stringFound && ((g_cvars.m_hideInactiveAudioObjects == 0) || ((g_cvars.m_hideInactiveAudioObjects != 0) && hasActiveData && !isVirtual));

		if (draw)
		{
			auxGeom.Draw2dLabel(posX, posY, Debug::g_managerFontSize,
			                    isVirtual ? Debug::g_globalColorVirtual.data() : (hasActiveData ? Debug::g_managerColorItemActive.data() : Debug::g_globalColorInactive.data()),
			                    false,
			                    szObjectName);

			posY += Debug::g_managerLineHeight;
			++numObjects;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float posY) const
{
	size_t numObjects = 0;
	float const headerPosY = posY;
	CryFixedStringT<MaxControlNameLength> lowerCaseSearchString(g_cvars.m_pDebugFilter->GetString());
	lowerCaseSearchString.MakeLower();

	posY += Debug::g_managerHeaderLineHeight;

	DrawObjectDebugData(auxGeom, posX, posY, *g_pObject, lowerCaseSearchString, numObjects);
	DrawObjectDebugData(auxGeom, posX, posY, g_previewObject, lowerCaseSearchString, numObjects);

	for (auto const pObject : m_constructedObjects)
	{
		DrawObjectDebugData(auxGeom, posX, posY, *pObject, lowerCaseSearchString, numObjects);
	}

	auxGeom.Draw2dLabel(posX, headerPosY, Debug::g_managerHeaderFontSize, Debug::g_globalColorHeader.data(), false, "Audio Objects [%" PRISIZE_T "]", numObjects);
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
