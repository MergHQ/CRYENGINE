// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QWidget>
#include <SystemTypes.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

class QAction;
class QVBoxLayout;
class QAbstractItemModel;
class CMountingProxyModel;
class QFilteringPanel;

namespace ACE
{
class CSystemAsset;
class CSystemControl;
class CSystemAssetsManager;
class CSystemSourceModel;
class CSystemLibraryModel;
class CSystemFilterProxyModel;
class CTreeView;

class CSystemControlsWidget final : public QWidget
{
	Q_OBJECT

public:

	CSystemControlsWidget(CSystemAssetsManager* const pAssetsManager, QWidget* const pParent);
	virtual ~CSystemControlsWidget() override;

	bool                       IsEditing() const;
	std::vector<CSystemAsset*> GetSelectedAssets() const;
	void                       SelectConnectedSystemControl(CSystemControl& control, CID const itemId);
	void                       Reload();
	void                       BackupTreeViewStates();
	void                       RestoreTreeViewStates();

signals:

	void SignalSelectedControlChanged();

private slots:

	void OnDeleteSelectedControl();
	void OnContextMenu(QPoint const& pos);
	void OnUpdateCreateButtons();

	void ExecuteControl();
	void StopControlExecution();

private:

	// QObject
	virtual bool        eventFilter(QObject* pObject, QEvent* pEvent) override;
	// ~QObject

	void                InitAddControlWidget(QVBoxLayout* const pLayout);
	void                SelectNewAsset(QModelIndex const& parent, int const row);
	void                ClearFilters();
	void                DeleteModels();

	CSystemControl*     CreateControl(string const& name, ESystemItemType const type, CSystemAsset* const pParent);
	CSystemAsset*       CreateFolder(CSystemAsset* const pParent);
	void                CreateParentFolder();
	bool                IsParentFolderAllowed() const;
	bool                IsDefaultControlSelected(QStringList& controlNames = QStringList()) const;
	bool                HasDefaultControl(CSystemAsset* const pAsset, QStringList& controlNames) const;

	QAbstractItemModel* CreateLibraryModelFromIndex(QModelIndex const& sourceIndex);
	CSystemAsset*       GetSelectedAsset() const;

	CSystemAssetsManager* const       m_pAssetsManager;
	QFilteringPanel*                  m_pFilteringPanel;
	CTreeView* const                  m_pTreeView;
	CSystemFilterProxyModel* const    m_pSystemFilterProxyModel;
	CMountingProxyModel*              m_pMountingProxyModel;
	CSystemSourceModel* const         m_pSourceModel;
	std::vector<CSystemLibraryModel*> m_libraryModels;

	QAction*                          m_pCreateParentFolderAction;
	QAction*                          m_pCreateFolderAction;
	QAction*                          m_pCreateTriggerAction;
	QAction*                          m_pCreateParameterAction;
	QAction*                          m_pCreateSwitchAction;
	QAction*                          m_pCreateStateAction;
	QAction*                          m_pCreateEnvironmentAction;
	QAction*                          m_pCreatePreloadAction;

	bool                              m_isReloading;
	bool                              m_isCreatedFromMenu;
	int const                         m_nameColumn;
};
} // namespace ACE
