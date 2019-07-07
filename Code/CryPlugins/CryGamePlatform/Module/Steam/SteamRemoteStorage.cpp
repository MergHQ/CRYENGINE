// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "SteamRemoteStorage.h"
#include "SteamSharedRemoteFile.h"
#include "SteamRemoteFile.h"
#include "UserGeneratedContentManager.h"

namespace Cry
{
	namespace GamePlatform
	{
		namespace Steam
		{
			CRemoteStorage::CRemoteStorage(CService& steamService)
				: m_service(steamService)
			{
				m_pUGCManager = stl::make_unique<CUserGeneratedContentManager>(m_service);
			}

			bool CRemoteStorage::IsEnabled() const
			{
				if (ISteamRemoteStorage* pSteamRemoteStorage = SteamRemoteStorage())
					return pSteamRemoteStorage->IsCloudEnabledForApp();

				return false;
			}

			std::shared_ptr<IRemoteFile> CRemoteStorage::GetFile(const char* name)
			{
				if (ISteamRemoteStorage* pSteamRemoteStorage = SteamRemoteStorage())
				{
					if (!IsEnabled())
						return nullptr;

					return std::make_shared<CRemoteFile>(m_service, name);
				}

				return nullptr;
			}

			std::shared_ptr<ISharedRemoteFile> CRemoteStorage::GetSharedFile(ISharedRemoteFile::Identifier id)
			{
				if (id == 0)
					return nullptr;

				return std::make_shared<CSharedRemoteFile>(m_service, id);
			}
		}
	}
}