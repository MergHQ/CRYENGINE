// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "ObjectManager.h"
#include "Common.h"
#include "Managers.h"
#include "ListenerManager.h"
#include "Object.h"
#include "StandaloneFile.h"
#include "CVars.h"
#include "IImpl.h"
#include "Common/IObject.h"
#include "Common/Logger.h"

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include "Debug.h"
	#include "Common/DebugStyle.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CObjectManager::~CObjectManager()
{
	CRY_ASSERT_MESSAGE(m_constructedObjects.empty(), "There are still objects during %s", __FUNCTION__);
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
		CRY_ASSERT_MESSAGE(pObject->GetImplDataPtr() == nullptr, "An object cannot have valid impl data during %s", __FUNCTION__);
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
			CObject* const pObject = *iter;

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
void CObjectManager::RegisterObject(CObject* const pObject)
{
	m_constructedObjects.push_back(pObject);
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

//////////////////////////////////////////////////////////////////////////
void CObjectManager::SetObjectPhysical(Impl::IObject* const pIObject)
{
	for (auto const pObject : m_constructedObjects)
	{
		if (pObject->GetImplDataPtr() == pIObject)
		{
			pObject->RemoveFlag(EObjectFlags::Virtual);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
			pObject->ResetObstructionRays();
#endif  // INCLUDE_AUDIO_PRODUCTION_CODE

			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CObjectManager::SetObjectVirtual(Impl::IObject* const pIObject)
{
	for (auto const pObject : m_constructedObjects)
	{
		if (pObject->GetImplDataPtr() == pIObject)
		{
			pObject->SetFlag(EObjectFlags::Virtual);
			break;
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
	CObject const& object,
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
		bool const draw = stringFound && ((g_cvars.m_hideInactiveObjects == 0) || ((g_cvars.m_hideInactiveObjects != 0) && hasActiveData && !isVirtual));

		if (draw)
		{
			auxGeom.Draw2dLabel(posX, posY, Debug::s_listFontSize,
			                    !hasActiveData ? Debug::s_globalColorInactive : (isVirtual ? Debug::s_globalColorVirtual : Debug::s_listColorItemActive),
			                    false,
			                    szObjectName);

			posY += Debug::s_listLineHeight;
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

	posY += Debug::s_listHeaderLineHeight;

	DrawObjectDebugData(auxGeom, posX, posY, *g_pObject, lowerCaseSearchString, numObjects);
	DrawObjectDebugData(auxGeom, posX, posY, g_previewObject, lowerCaseSearchString, numObjects);

	for (auto const pObject : m_constructedObjects)
	{
		DrawObjectDebugData(auxGeom, posX, posY, *pObject, lowerCaseSearchString, numObjects);
	}

	auxGeom.Draw2dLabel(posX, headerPosY, Debug::s_listHeaderFontSize, Debug::s_globalColorHeader, false, "Audio Objects [%" PRISIZE_T "]", numObjects);
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
