// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "UserGeneratedContentManager.h"
#include "UserGeneratedContent.h"
#include "SteamService.h"
#include "SteamUserIdentifier.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{
			CUserGeneratedContentManager::CUserGeneratedContentManager(CService& steamService)
				: m_service(steamService)
			{
			}

			void CUserGeneratedContentManager::Create(ApplicationIdentifier appId, IUserGeneratedContent::EType type)
			{
				m_lastUsedId = ExtractSteamID(appId);

				if (ISteamUGC* pSteamUGC = SteamUGC())
					pSteamUGC->CreateItem(m_lastUsedId, (EWorkshopFileType)type);
			}

			void CUserGeneratedContentManager::CreateDirect(ApplicationIdentifier appId, IUserGeneratedContent::EType type,
				const char* title, const char* desc, IUserGeneratedContent::EVisibility visibility,
				const char* *pTags, int numTags, const char* contentFolderPath, const char* previewPath)
			{
				m_lastUsedId = ExtractSteamID(appId);

				m_pWaitingParameters = stl::make_unique<SItemParameters>();
				m_pWaitingParameters->title = title;
				m_pWaitingParameters->desc = desc;
				m_pWaitingParameters->visibility = visibility;

				for (int i = 0; i < numTags; i++)
					m_pWaitingParameters->tags.push_back(pTags[i]);

				m_pWaitingParameters->contentFolderPath = contentFolderPath;
				m_pWaitingParameters->previewPath = previewPath;

				if (ISteamUGC* pSteamUGC = SteamUGC())
				{
					SteamAPICall_t result = pSteamUGC->CreateItem(m_lastUsedId, (EWorkshopFileType)type);
					m_callResultContentCreated.Set(result, this, &CUserGeneratedContentManager::OnContentCreated);
				}
			}

			void CUserGeneratedContentManager::OnContentCreated(CreateItemResult_t* pResult, bool bIOError)
			{
				if (pResult->m_eResult == k_EResultOK)
				{
					if (pResult->m_bUserNeedsToAcceptWorkshopLegalAgreement)
					{
						m_service.OpenBrowser("http://steamcommunity.com/sharedfiles/workshoplegalagreement");
						CrySleep(10000);
					}

					m_content.emplace_back(stl::make_unique<CUserGeneratedContent>(m_service, m_lastUsedId, pResult->m_nPublishedFileId));
					IUserGeneratedContent* pContent = m_content.back().get();

					if (m_pWaitingParameters != nullptr)
					{
						pContent->SetTitle(m_pWaitingParameters->title);
						pContent->SetDescription(m_pWaitingParameters->desc);
						pContent->SetVisibility(m_pWaitingParameters->visibility);

						if (m_pWaitingParameters->tags.size() > 0)
						{
							auto* *pTags = new const char* [m_pWaitingParameters->tags.size()];
							for (auto it = m_pWaitingParameters->tags.begin(); it != m_pWaitingParameters->tags.end(); ++it)
							{
								pTags[it - m_pWaitingParameters->tags.begin()] =* it;
							}

							pContent->SetTags(pTags, m_pWaitingParameters->tags.size());
							delete[] pTags;
						}

						pContent->SetContent(m_pWaitingParameters->contentFolderPath);
						pContent->SetPreview(m_pWaitingParameters->previewPath);

						pContent->CommitChanges("Automated after creation");

						m_pWaitingParameters.reset();
					}

					for (IListener* pListener : m_listeners)
					{
						pListener->OnContentCreated(pContent, pResult->m_bUserNeedsToAcceptWorkshopLegalAgreement);
					}
				}
				else
					CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "[Steam] Content creation failed, result %i", pResult->m_eResult);
			}
		}
	}
}