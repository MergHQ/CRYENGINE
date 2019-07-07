// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "VersionControlMenuBuilder.h"
#include "VersionControl/VersionControl.h"
#include "AssetSystem/Browser/AssetBrowser.h"
#include "IObjectManager.h"
#include "Objects/IObjectLayerManager.h"

namespace Private_VersionControlMenuBuilder
{

void OnLevelExplorerContextMenu(CAbstractMenu& menu, const std::vector<CBaseObject*>&, const std::vector<IObjectLayer*>& layers, const std::vector<IObjectLayer*>& folders)
{
	const int vcsSection = menu.GetNextEmptySection();
	menu.SetSectionName(vcsSection, "Version Control");

	auto action = menu.CreateAction(QObject::tr("Refresh"), vcsSection);
	QObject::connect(action, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.refresh");
	});

	const bool isVCSOnline = CVersionControl::GetInstance().IsOnline();

	action = menu.CreateAction(QObject::tr("Get Latest"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.get_latest");
	});

	action = menu.CreateAction(QObject::tr("Check Out"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, []() mutable
	{
		GetIEditor()->ExecuteCommand("version_control_system.check_out");
	});

	action = menu.CreateAction(QObject::tr("Revert"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.revert");
	});

	action = menu.CreateAction(QObject::tr("Revert if Unchanged"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.revert_unchanged");
	});

	action = menu.CreateAction(QObject::tr("Submit..."), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, []() mutable
	{
		GetIEditor()->ExecuteCommand("version_control_system.submit");
	});
}

void AddWorkFilesMenu(CAbstractMenu& menu)
{
	auto pWorkFilesMenu = menu.FindMenu("Work Files");
	const int vcsSection = pWorkFilesMenu->FindOrCreateSectionByName(QObject::tr("Version Control").toLocal8Bit());
	auto pAction = pWorkFilesMenu->CreateAction(QObject::tr("Refresh"), vcsSection);
	QObject::connect(pAction, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.refresh_work_files");
	});

	pAction = pWorkFilesMenu->CreateAction(QObject::tr("Get Latest"), vcsSection);
	QObject::connect(pAction, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.get_latest_work_files");
	});

	pAction = pWorkFilesMenu->CreateAction(QObject::tr("Check Out"), vcsSection);
	QObject::connect(pAction, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.check_out_work_files");
	});

	pAction = pWorkFilesMenu->CreateAction(QObject::tr("Revert"), vcsSection);
	QObject::connect(pAction, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.revert_work_files");
	});

	pAction = pWorkFilesMenu->CreateAction(QObject::tr("Revert if unchanged"), vcsSection);
	QObject::connect(pAction, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.revert_unchanged_work_files");
	});

	pAction = pWorkFilesMenu->CreateAction(QObject::tr("Submit..."), vcsSection);
	QObject::connect(pAction, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.submit_work_files");
	});

	int otherSection = pWorkFilesMenu->GetNextEmptySection();

	pAction = pWorkFilesMenu->CreateAction(QObject::tr("Remove local files..."), otherSection);
	QObject::connect(pAction, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.remove_local_work_files");
	});
}

void OnAssetBrowserContextMenu(CAbstractMenu& menu, const std::vector<CAsset*>& assets, const std::vector<string>& folders)
{
	const int vcsSection = menu.GetNextEmptySection();
	menu.SetSectionName(vcsSection, "Version Control");

	auto action = menu.CreateAction(QObject::tr("Refresh"), vcsSection);
	QObject::connect(action, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.refresh");
	});

	const bool isVCSOnline = CVersionControl::GetInstance().IsOnline();

	action = menu.CreateAction(QObject::tr("Get Latest"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.get_latest");
	});

	action = menu.CreateAction(QObject::tr("Check Out"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, []() mutable
	{
		GetIEditor()->ExecuteCommand("version_control_system.check_out");
	});

	action = menu.CreateAction(QObject::tr("Revert"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.revert");
	});

	action = menu.CreateAction(QObject::tr("Revert if Unchanged"), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, []()
	{
		GetIEditor()->ExecuteCommand("version_control_system.revert_unchanged");
	});

	action = menu.CreateAction(QObject::tr("Submit..."), vcsSection);
	action->setEnabled(isVCSOnline);
	QObject::connect(action, &QAction::triggered, []() mutable
	{
		GetIEditor()->ExecuteCommand("version_control_system.submit");
	});

	if (assets.size() == 1 && folders.empty() && !assets[0]->GetWorkFiles().empty())
	{
		AddWorkFilesMenu(menu);
	}
}

}

void CVersionControlMenuBuilder::Activate()
{
	using namespace Private_VersionControlMenuBuilder;

	auto id = reinterpret_cast<uintptr_t>(this);

	CAssetBrowser::s_signalContextMenuRequested.Connect(&OnAssetBrowserContextMenu, id);

	GetIEditor()->GetObjectManager()->GetIObjectLayerManager()->signalContextMenuRequested.Connect(&OnLevelExplorerContextMenu, id);

}

void CVersionControlMenuBuilder::Deactivate()
{
	auto id = reinterpret_cast<uintptr_t>(this);

	CAssetBrowser::s_signalContextMenuRequested.DisconnectById(id);

	GetIEditor()->GetObjectManager()->GetIObjectLayerManager()->signalContextMenuRequested.DisconnectById(id);
}
