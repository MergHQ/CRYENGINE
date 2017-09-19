// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QFrame>

#include "AudioAssets.h"
#include <CryAudio/IAudioInterfacesCommonData.h>

// Forward declarations
class QVBoxLayout;
class QSortFilterProxyModel;
class QSearchBox;
class QAbstractItemModel;
class CMountingProxyModel;

namespace ACE
{
class CAudioAssetsManager;
class QFilterButton;
class CSystemControlsModel;
class CSystemControlsFilterProxyModel;
class CAudioLibraryModel;
class CAdvancedTreeView;

class CSystemControlsWidget final : public QFrame
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

private:

	// QObject
	virtual bool eventFilter(QObject* pObject, QEvent* pEvent) override;
	// ~QObject

	// Filtering
	void                InitFilterWidget();
	void                ResetFilters();
	void                ShowControlType(EItemType type, bool bShow);

	CAudioControl*      CreateControl(string const& name, EItemType type, CAudioAsset* pParent);
	CAudioAsset*        CreateFolder(CAudioAsset* pParent);

	QAbstractItemModel* CreateLibraryModelFromIndex(QModelIndex const& sourceIndex);
	CAudioAsset*        GetSelectedAsset() const;

	void                SelectNewAsset(QModelIndex const& parent, int const row);

private slots:

	void DeleteSelectedControl();
	void OnContextMenu(QPoint const& pos);

	// Audio Preview
	void ExecuteControl();
	void StopControlExecution();

signals:

	void SelectedControlChanged();
	void ControlTypeFiltered(EItemType type, bool bShow);
	void StartTextFiltering();
	void StopTextFiltering();

private:

	CAudioAssetsManager* const m_pAssetsManager;

	// Filtering
	QString                          m_filter;
	QWidget*                         m_pFilterWidget;
	QVBoxLayout*                     m_pFiltersLayout;

	QSearchBox*                      m_pSearchBox;
	CAdvancedTreeView*               m_pControlsTree;
	CSystemControlsModel*       m_pAssetsModel;
	CSystemControlsFilterProxyModel*            m_pProxyModel;

	CMountingProxyModel*             m_pMountedModel;
	std::vector<CAudioLibraryModel*> m_libraryModels;
	bool                             m_bReloading = false;
	bool                             m_bCreatedFromMenu = false;
};
} // namespace ACE
