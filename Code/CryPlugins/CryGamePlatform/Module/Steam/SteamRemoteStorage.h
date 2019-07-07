// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IPlatformRemoteStorage.h"
#include "SteamTypes.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{
			class CRemoteStorage
				: public IRemoteStorage
			{
			public:
				CRemoteStorage(CService& steamService);
				virtual ~CRemoteStorage() = default;

				// IRemoteStorage
				virtual bool IsEnabled() const override;

				virtual IUserGeneratedContentManager* GetUserGeneratedContentManager() const override { return m_pUGCManager.get(); }

				virtual std::shared_ptr<IRemoteFile> GetFile(const char* name) override;
				virtual std::shared_ptr<ISharedRemoteFile> GetSharedFile(ISharedRemoteFile::Identifier id) override;
				// ~IRemoteStorage

			protected:
				CService& m_service;
				std::unique_ptr<IUserGeneratedContentManager> m_pUGCManager;
			};
		}
	}
}