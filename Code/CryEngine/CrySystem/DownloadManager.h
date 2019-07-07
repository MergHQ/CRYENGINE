// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if CRY_PLATFORM_WINDOWS

class CHTTPDownloader;
struct ISystem;

class CDownloadManager
{
public:
	CDownloadManager();
	virtual ~CDownloadManager();

	void             Create(ISystem* pSystem);
	CHTTPDownloader* CreateDownload();
	void             RemoveDownload(CHTTPDownloader* pDownload);
	void             Update();
	void             Release();

private:

	ISystem*                    m_pSystem;
	std::list<CHTTPDownloader*> m_lDownloadList;
};

#endif
