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
	#include "Common/DebugStyle.h"
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
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioSystem, 0, "CryAudio::CStandaloneFile");
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
	auxGeom.Draw2dLabel(posX, posY, Debug::s_listHeaderFontSize, Debug::s_globalColorHeader, false, "Standalone Files [%" PRISIZE_T "]", m_constructedStandaloneFiles.size());
	posY += Debug::s_listHeaderLineHeight;

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
			char const* const szObjectName = pStandaloneFile->m_pObject->GetName();
			CryFixedStringT<MaxControlNameLength> lowerCaseObjectName(szObjectName);
			lowerCaseObjectName.MakeLower();

			bool const draw = isFilterNotSet ||
			                  (lowerCaseStandaloneFileName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos) ||
			                  (lowerCaseObjectName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos);

			if (draw)
			{
				ColorF color = Debug::s_globalColorInactive;

				switch (pStandaloneFile->m_state)
				{
				case EStandaloneFileState::Playing:
					{
						color = Debug::s_listColorItemActive;
						break;
					}
				case EStandaloneFileState::Loading:
					{
						color = Debug::s_listColorItemLoading;
						break;
					}
				case EStandaloneFileState::Stopping:
					{
						color = Debug::s_listColorItemStopping;
						break;
					}
				default:
					{
						CRY_ASSERT_MESSAGE(false, "Standalone file is in an unknown state during %s", __FUNCTION__);
						break;
					}
				}

				auxGeom.Draw2dLabel(posX, posY, Debug::s_listFontSize,
				                    color,
				                    false,
				                    "%s on %s",
				                    szStandaloneFileName,
				                    szObjectName);

				posY += Debug::s_listLineHeight;
			}
		}
	}
}
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
}      // namespace CryAudio
