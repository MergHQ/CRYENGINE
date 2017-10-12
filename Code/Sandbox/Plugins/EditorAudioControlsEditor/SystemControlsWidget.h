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
	std::vector<CAudioControl*> GetSelectedControls() const;
	void                        SelectConnectedSystemControl(CAudioControl const* const pControl);
	void                        Reload();
	void                        BackupTreeViewStates();
	void                        RestoreTreeViewStates();

signals:

	void SelectedControlChanged();
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

	void InitAddControlWidget(QHBoxLayout* const pLayout);
	void InitFilterWidgets(QVBoxLayout* const pMainLayout);
	void InitTypeFilters();
	void ResetFilters();
	void ShowControlType(EItemType const type, bool const isVisible);
	void SelectNewAsset(QModelIndex const& parent, int const row);

	CAudioControl*      CreateControl(string const& name, EItemType type, CAudioAsset* const pParent);
	CAudioAsset*        CreateFolder(CAudioAsset* const pParent);
	void                CreateParentFolder();
	bool                IsParentFolderAllowed();

	QAbstractItemModel* CreateLibraryModelFromIndex(QModelIndex const& sourceIndex);
	CAudioAsset*        GetSelectedAsset() const;

	CAudioAssetsManager* const             m_pAssetsManager;
	QSearchBox* const                      m_pSearchBox;
	QToolButton* const                     m_pFilterButton;
	CAudioTreeView* const                  m_pTreeView;
	CSystemControlsModel* const            m_pAssetsModel;
	CSystemControlsFilterProxyModel* const m_pFilterProxyModel;
	CMountingProxyModel*                   m_pMountingProxyModel;
	std::vector<CAudioLibraryModel*>       m_libraryModels;

	QString                                m_filter;
	QWidget*                               m_pFilterWidget;
	QAction*                               m_pCreateParentFolderAction;
	QAction*                               m_pCreateFolderAction;
	QAction*                               m_pCreateTriggerAction;
	QAction*                               m_pCreateParameterAction;
	QAction*                               m_pCreateSwitchAction;
	QAction*                               m_pCreateStateAction;
	QAction*                               m_pCreateEnvironmentAction;
	QAction*                               m_pCreatePreloadAction;

	bool                                   m_isReloading = false;
	bool                                   m_isCreatedFromMenu = false;
};
} // namespace ACE
