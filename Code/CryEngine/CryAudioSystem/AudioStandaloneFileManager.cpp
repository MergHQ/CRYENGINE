// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AudioStandaloneFileManager.h"
#include "ATLAudioObject.h"
#include "AudioCVars.h"
#include "Common.h"
#include <IAudioImpl.h>
#include <CryString/HashedString.h>

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	#include <CryRenderer/IRenderAuxGeom.h>
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

namespace CryAudio
{
//////////////////////////////////////////////////////////////////////////
CAudioStandaloneFileManager::~CAudioStandaloneFileManager()
{
	Release();
}

//////////////////////////////////////////////////////////////////////////
void CAudioStandaloneFileManager::Release()
{
	// Don't clear m_constructedStandaloneFiles here as we need the files to survive a middleware switch!
	for (auto const pStandaloneFile : m_constructedStandaloneFiles)
	{
		g_pIImpl->DestructStandaloneFile(pStandaloneFile->m_pImplData);
		pStandaloneFile->m_pImplData = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
CATLStandaloneFile* CAudioStandaloneFileManager::ConstructStandaloneFile(char const* const szFile, bool const isLocalized, Impl::ITrigger const* const pITrigger)
{
	auto const pStandaloneFile = new CATLStandaloneFile();
	pStandaloneFile->m_pImplData = g_pIImpl->ConstructStandaloneFile(*pStandaloneFile, szFile, isLocalized, pITrigger);
	pStandaloneFile->m_hashedFilename = CHashedString(szFile);

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	pStandaloneFile->m_isLocalized = isLocalized;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE

	m_constructedStandaloneFiles.push_back(pStandaloneFile);
	return pStandaloneFile;
}

//////////////////////////////////////////////////////////////////////////
void CAudioStandaloneFileManager::ReleaseStandaloneFile(CATLStandaloneFile* const pStandaloneFile)
{
	if (pStandaloneFile != nullptr)
	{
		m_constructedStandaloneFiles.remove(pStandaloneFile);
		g_pIImpl->DestructStandaloneFile(pStandaloneFile->m_pImplData);
		delete pStandaloneFile;
	}
}

#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void CAudioStandaloneFileManager::DrawDebugInfo(IRenderAuxGeom& auxGeom, Vec3 const& listenerPosition, float posX, float posY) const
{
	auxGeom.Draw2dLabel(posX, posY, Debug::g_managerHeaderFontSize, Debug::g_managerColorHeader.data(), false, "Standalone Files [%" PRISIZE_T "]", m_constructedStandaloneFiles.size());
	posY += Debug::g_managerHeaderLineHeight;

	CryFixedStringT<MaxControlNameLength> lowerCaseSearchString(g_cvars.m_pDebugFilter->GetString());
	lowerCaseSearchString.MakeLower();

	bool const isFilterNotSet = (lowerCaseSearchString.empty() || (lowerCaseSearchString == "0"));

	for (auto const pStandaloneFile : m_constructedStandaloneFiles)
	{
		Vec3 const& position = pStandaloneFile->m_pAudioObject->GetTransformation().GetPosition();
		float const distance = position.GetDistance(listenerPosition);

		if (g_cvars.m_debugDistance <= 0.0f || (g_cvars.m_debugDistance > 0.0f && distance < g_cvars.m_debugDistance))
		{
			char const* const szStandaloneFileName = pStandaloneFile->m_hashedFilename.GetText().c_str();
			CryFixedStringT<MaxControlNameLength> lowerCaseStandaloneFileName(szStandaloneFileName);
			lowerCaseStandaloneFileName.MakeLower();
			char const* const szObjectName = pStandaloneFile->m_pAudioObject->m_name.c_str();
			CryFixedStringT<MaxControlNameLength> lowerCaseObjectName(szObjectName);
			lowerCaseObjectName.MakeLower();

			bool const draw = isFilterNotSet ||
			                  (lowerCaseStandaloneFileName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos) ||
			                  (lowerCaseObjectName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos);

			if (draw)
			{
				float const* pColor = Debug::g_managerColorItemInactive.data();

				switch (pStandaloneFile->m_state)
				{
				case EAudioStandaloneFileState::Playing:
					pColor = Debug::g_managerColorItemActive.data();
					break;
				case EAudioStandaloneFileState::Loading:
					pColor = Debug::g_managerColorItemLoading.data();
					break;
				case EAudioStandaloneFileState::Stopping:
					pColor = Debug::g_managerColorItemStopping.data();
					break;
				default:
					CRY_ASSERT_MESSAGE(false, "Standalone file is in an unknown state.");
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
