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
			class CSharedRemoteFile
				: public ISharedRemoteFile
			{
			public:
				CSharedRemoteFile(CService& steamService, ISharedRemoteFile::Identifier sharedId);
				virtual ~CSharedRemoteFile() = default;

				// ISharedRemoteFile
				virtual void AddListener(IListener& listener) override { m_listeners.push_back(&listener); }
				virtual void RemoveListener(IListener& listener) override { stl::find_and_erase(m_listeners, &listener); }

				virtual void Download(int downloadPriority) override;
				virtual ISharedRemoteFile::Identifier GetIdentifier() const override { return m_sharedHandle; }

				virtual bool Read(std::vector<char>& bufferOut) override;
				virtual int GetFileSize() const override { return m_data.size(); }
				// ~ISharedRemoteFile

				void OnDownloaded(RemoteStorageDownloadUGCResult_t* pResult, bool bIOFailure);
				CCallResult<CSharedRemoteFile, RemoteStorageDownloadUGCResult_t> m_callResultDownloaded;

			protected:
				CService& m_service;

				std::vector<IListener*> m_listeners;
				UGCHandle_t m_sharedHandle;

				std::vector<char> m_data;
			};
		}
	}
}