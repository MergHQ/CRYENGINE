// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "FileManager.h"
#include "Common.h"
#include "StandaloneFile.h"
#include "Object.h"
#include "CVars.h"
#include <IImpl.h>
#include <CryString/HashedString.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include "Managers.h"
	#include "ListenerManager.h"
	#include "Debug.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CFileManager::~CFileManager()
{
	CRY_ASSERT_MESSAGE(m_constructedStandaloneFiles.empty(), "There are still files during %s", __FUNCTION__);
}

//////////////////////////////////////////////////////////////////////////
void CFileManager::ReleaseImplData()
{
	// Don't clear m_constructedStandaloneFiles here as we need the files to survive a middleware switch!
	for (auto const pFile : m_constructedStandaloneFiles)
	{
		g_pIImpl->DestructStandaloneFileConnection(pFile->m_pImplData);
		pFile->m_pImplData = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CFileManager::Release()
{
	for (auto const pFile : m_constructedStandaloneFiles)
	{
		delete pFile;
	}

	m_constructedStandaloneFiles.clear();
}

//////////////////////////////////////////////////////////////////////////
CStandaloneFile* CFileManager::ConstructStandaloneFile(char const* const szFile, bool const isLocalized, Impl::ITriggerConnection const* const pITriggerConnection)
{
	auto const pStandaloneFile = new CStandaloneFile();
	pStandaloneFile->m_pImplData = g_pIImpl->ConstructStandaloneFileConnection(*pStandaloneFile, szFile, isLocalized, pITriggerConnection);
	pStandaloneFile->m_hashedFilename = CHashedString(szFile);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	pStandaloneFile->m_isLocalized = isLocalized;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	m_constructedStandaloneFiles.push_back(pStandaloneFile);
	return pStandaloneFile;
}

//////////////////////////////////////////////////////////////////////////
void CFileManager::ReleaseStandaloneFile(CStandaloneFile* const pStandaloneFile)
{
	if (pStandaloneFile != nullptr)
	{
		m_constructedStandaloneFiles.erase(std::remove(m_constructedStandaloneFiles.begin(), m_constructedStandaloneFiles.end(), pStandaloneFile), m_constructedStandaloneFiles.end());
		g_pIImpl->DestructStandaloneFileConnection(pStandaloneFile->m_pImplData);
		delete pStandaloneFile;
	}
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CFileManager::DrawDebugInfo(IRenderAuxGeom& auxGeom, float posX, float posY) const
{
	auxGeom.Draw2dLabel(posX, posY, Debug::g_managerHeaderFontSize, Debug::g_globalColorHeader.data(), false, "Standalone Files [%" PRISIZE_T "]", m_constructedStandaloneFiles.size());
	posY += Debug::g_managerHeaderLineHeight;

	CryFixedStringT<MaxControlNameLength> lowerCaseSearchString(g_cvars.m_pDebugFilter->GetString());
	lowerCaseSearchString.MakeLower();

	bool const isFilterNotSet = (lowerCaseSearchString.empty() || (lowerCaseSearchString == "0"));

	for (auto const pStandaloneFile : m_constructedStandaloneFiles)
	{
		Vec3 const& position = pStandaloneFile->m_pObject->GetTransformation().GetPosition();
		float const distance = position.GetDistance(g_listenerManager.GetActiveListenerTransformation().GetPosition());

		if (g_cvars.m_debugDistance <= 0.0f || (g_cvars.m_debugDistance > 0.0f && distance < g_cvars.m_debugDistance))
		{
			char const* const szStandaloneFileName = pStandaloneFile->m_hashedFilename.GetText().c_str();
			CryFixedStringT<MaxControlNameLength> lowerCaseStandaloneFileName(szStandaloneFileName);
			lowerCaseStandaloneFileName.MakeLower();
			char const* const szObjectName = pStandaloneFile->m_pObject->m_name.c_str();
			CryFixedStringT<MaxControlNameLength> lowerCaseObjectName(szObjectName);
			lowerCaseObjectName.MakeLower();

			bool const draw = isFilterNotSet ||
			                  (lowerCaseStandaloneFileName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos) ||
			                  (lowerCaseObjectName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos);

			if (draw)
			{
				float const* pColor = Debug::g_globalColorInactive.data();

				switch (pStandaloneFile->m_state)
				{
				case EStandaloneFileState::Playing:
					pColor = Debug::g_managerColorItemActive.data();
					break;
				case EStandaloneFileState::Loading:
					pColor = Debug::g_managerColorItemLoading.data();
					break;
				case EStandaloneFileState::Stopping:
					pColor = Debug::g_managerColorItemStopping.data();
					break;
				default:
					CRY_ASSERT_MESSAGE(false, "Standalone file is in an unknown state during %s", __FUNCTION__);
					break;
				}

				auxGeom.Draw2dLabel(posX, posY, Debug::g_managerFontSize,
				                    pColor,
				                    false,
				                    "%s on %s",
				                    szStandaloneFileName,
				                    szObjectName);

				posY += Debug::g_managerLineHeight;
			}
		}
	}
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
