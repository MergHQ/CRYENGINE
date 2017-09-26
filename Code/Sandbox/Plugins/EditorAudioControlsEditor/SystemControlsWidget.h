// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QWidget>
#include <ACETypes.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

class QAction;
class QHBoxLayout;
class QVBoxLayout;
class QToolButton;
class QSearchBox;
class QAbstractItemModel;
class CMountingProxyModel;

namespace ACE
{
class CAudioAsset;
class CAudioControl;
class CAudioAssetsManager;
class CSystemControlsModel;
class CSystemControlsFilterProxyModel;
class CAudioLibraryModel;
class CAudioTreeView;

class CSystemControlsWidget final : public QWidget
{
	Q_OBJECT

public:

	CSystemControlsWidget(CAudioAssetsManager* pAssetsManager);
	virtual ~CSystemControlsWidget() override;

	bool                        IsEditing() const;
	std::vector<CAudioControl*> GetSelectedControls();
	void                        Reload();
	void                        BackupTreeViewStates();
	void                        RestoreTreeViewStates();

signals:

	void SelectedControlChanged();
	void ControlTypeFiltered(EItemType const type, bool const bShow);
	void StartTextFiltering();
	void StopTextFiltering();

private slots:

	void DeleteSelectedControl();
	void OnContextMenu(QPoint const& pos);
	void UpdateCreateButtons();

	void ExecuteControl();
	void StopControlExecution();

private:

	// QObject
	virtual bool eventFilter(QObject* pObject, QEvent* pEvent) override;
	// ~QObject

	void InitAddControlWidget(QHBoxLayout* pLayout);
	void InitFilterWidgets(QVBoxLayout* pMainLayout);
	void InitTypeFilters();
	void ResetFilters();
	void ShowControlType(EItemType const type, bool const bShow);
	void SelectNewAsset(QModelIndex const& parent, int const row);

	CAudioControl*      CreateControl(string const& name, EItemType type, CAudioAsset* pParent);
	CAudioAsset*        CreateFolder(CAudioAsset* pParent);

	QAbstractItemModel* CreateLibraryModelFromIndex(QModelIndex const& sourceIndex);
	CAudioAsset*        GetSelectedAsset() const;

	CAudioAssetsManager* const       m_pAssetsManager;

	QString                          m_filter;
	QWidget*                         m_pFilterWidget;

	QAction*                         m_pCreateFolderAction;
	QAction*                         m_pCreateTriggerAction;
	QAction*                         m_pCreateParameterAction;
	QAction*                         m_pCreateSwitchAction;
	QAction*                         m_pCreateStateAction;
	QAction*                         m_pCreateEnvironmentAction;
	QAction*                         m_pCreatePreloadAction;

	QSearchBox*                      m_pSearchBox;
	QToolButton*                     m_pFilterButton;
	CAudioTreeView*                  m_pTreeView;
	CSystemControlsModel*            m_pAssetsModel;
	CSystemControlsFilterProxyModel* m_pFilterProxyModel;

	CMountingProxyModel*             m_pMountingProxyModel;
	std::vector<CAudioLibraryModel*> m_libraryModels;
	bool                             m_bReloading = false;
	bool                             m_bCreatedFromMenu = false;
};
} // namespace ACE
