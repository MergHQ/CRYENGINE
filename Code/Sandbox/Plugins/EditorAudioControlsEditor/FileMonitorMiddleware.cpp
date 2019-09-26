// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FileMonitorMiddleware.h"

#include "Common.h"
#include "Common/IImpl.h"

#include <IEditor.h>
#include <CryString/CryPath.h>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CFileMonitorMiddleware::CFileMonitorMiddleware(int const delay, QObject* const pParent)
	: CFileMonitorBase(delay, pParent)
{
	Enable();
}

//////////////////////////////////////////////////////////////////////////
void CFileMonitorMiddleware::Enable()
{
	if (g_pIImpl != nullptr)
	{
		stop();
		m_monitorFolders.clear();
		GetIEditor()->GetFileMonitor()->UnregisterListener(this);

		m_monitorFolders.push_back(g_implInfo.assetsPath);
		m_monitorFolders.push_back(g_implInfo.localizedAssetsPath);

		if ((g_implInfo.flags & EImplInfoFlags::SupportsProjects) != EImplInfoFlags::None)
		{
			m_monitorFolders.push_back(g_implInfo.projectPath);
		}

		for (auto const& folder : m_monitorFolders)
		{
			GetIEditor()->GetFileMonitor()->RegisterListener(this, folder);
		}
	}
}
} // namespace ACE
