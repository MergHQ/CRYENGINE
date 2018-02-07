// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "FileMonitor.h"

#include "SystemAssetsManager.h"
#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"

#include <IEditorImpl.h>
#include <CryString/CryPath.h>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CFileMonitor::CFileMonitor(int const delay, CSystemAssetsManager const& assetsManager, QObject* const pParent)
	: QTimer(pParent)
	, m_assetsManager(assetsManager)
	, m_delay(delay)
{
	setSingleShot(true);
	connect(this, &CFileMonitor::timeout, this, &CFileMonitor::SignalReloadData);
}

//////////////////////////////////////////////////////////////////////////
CFileMonitor::~CFileMonitor()
{
	stop();
	GetIEditor()->GetFileMonitor()->UnregisterListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CFileMonitor::OnFileChange(char const* filename, EChangeType eType)
{
	start(m_delay);
}

//////////////////////////////////////////////////////////////////////////
void CFileMonitor::Disable()
{
	stop();
	GetIEditor()->GetFileMonitor()->UnregisterListener(this);
}

//////////////////////////////////////////////////////////////////////////
CFileMonitorSystem::CFileMonitorSystem(int const delay, CSystemAssetsManager const& assetsManager, QObject* const pParent)
	: CFileMonitor(delay, assetsManager, pParent)
	, m_monitorFolder(assetsManager.GetConfigFolderPath())
	, m_delayTimer(new QTimer(this))
{
	m_delayTimer->setSingleShot(true);
	connect(m_delayTimer, &QTimer::timeout, this, &CFileMonitorSystem::Enable);
	Enable();
}

//////////////////////////////////////////////////////////////////////////
void CFileMonitorSystem::Enable()
{
	GetIEditor()->GetFileMonitor()->RegisterListener(this, m_monitorFolder, "xml");
}

//////////////////////////////////////////////////////////////////////////
void CFileMonitorSystem::EnableDelayed()
{
	m_delayTimer->start(m_delay);
}

//////////////////////////////////////////////////////////////////////////
CFileMonitorMiddleware::CFileMonitorMiddleware(int const delay, CSystemAssetsManager const& assetsManager, QObject* const pParent)
	: CFileMonitor(delay, assetsManager, pParent)
{
	Enable();
}

//////////////////////////////////////////////////////////////////////////
void CFileMonitorMiddleware::Enable()
{
	if (g_pEditorImpl != nullptr)
	{
		IImplSettings const* const pImplSettings = g_pEditorImpl->GetSettings();

		if (pImplSettings != nullptr)
		{
			stop();
			m_monitorFolders.clear();
			GetIEditor()->GetFileMonitor()->UnregisterListener(this);

			int const gameFolderPathLength = (PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR).GetLength();

			string const& soundBanksPath = pImplSettings->GetAssetsPath();
			string const& soundBanksPathSubstr = (soundBanksPath).substr(gameFolderPathLength);
			m_monitorFolders.emplace_back(soundBanksPathSubstr.c_str());

			string const& localizationPath = PathUtil::GetLocalizationFolder();
			m_monitorFolders.emplace_back(localizationPath.c_str());

			string const& projectPath = pImplSettings->GetProjectPath();

			if (projectPath != soundBanksPath)
			{
				string const& projectPathSubstr = projectPath.substr(gameFolderPathLength);
				m_monitorFolders.emplace_back(projectPathSubstr.c_str());
			}

			for (auto const& folder : m_monitorFolders)
			{
				GetIEditor()->GetFileMonitor()->RegisterListener(this, folder);
			}
		}
	}
}
} // namespace ACE
