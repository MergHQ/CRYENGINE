// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SteamTypes.h"
#include "UserGeneratedContent.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{
			struct SItemParameters
			{
				string title;
				string desc;
				IUserGeneratedContent::EVisibility visibility;
				std::vector<string> tags;
				string contentFolderPath;
				string previewPath;
			};

			class CUserGeneratedContentManager
				: public IUserGeneratedContentManager
			{
			public:
				explicit CUserGeneratedContentManager(CService& steamService);
				virtual ~CUserGeneratedContentManager() = default;

				// IUserGeneratedContentManager
				virtual void AddListener(IListener& listener) override { m_listeners.push_back(&listener); }
				virtual void RemoveListener(IListener& listener) override { stl::find_and_erase(m_listeners, &listener); }

				virtual void Create(ApplicationIdentifier appId, IUserGeneratedContent::EType type) override;

				virtual void CreateDirect(ApplicationIdentifier appId, IUserGeneratedContent::EType type,
					const char* title, const char* desc, IUserGeneratedContent::EVisibility visibility,
					const char* *pTags, int numTags, const char* contentFolderPath, const char* previewPath) override;
				// ~IUserGeneratedContentManager

			protected:
				CService& m_service;
				std::vector<IListener*> m_listeners;

				void OnContentCreated(CreateItemResult_t* pResult, bool bIOFailure);
				CCallResult<CUserGeneratedContentManager, CreateItemResult_t> m_callResultContentCreated;

				std::unique_ptr<SItemParameters> m_pWaitingParameters;

				AppId_t m_lastUsedId = k_uAppIdInvalid;
				std::vector<std::unique_ptr<IUserGeneratedContent>> m_content;
			};
		}
	}
}