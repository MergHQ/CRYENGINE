// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QFrame>
#include <QMenu>

#include "AudioControl.h"
#include "QTreeWidgetFilter.h"
#include <CryAudio/IAudioInterfacesCommonData.h>
#include "ATLControlsModel.h"

// Forward declarations
class QStandardItem;
class QSortFilterProxyModel;
class QLineEdit;
class QAudioControlsTreeView;

namespace ACE
{
class CATLControlsModel;
class QFilterButton;
class QATLTreeModel;

class CATLControlsPanel : public QFrame, public IATLControlModelListener
{
	Q_OBJECT

public:
	CATLControlsPanel(CATLControlsModel* pATLModel, QATLTreeModel* pATLControlsModel);
	~CATLControlsPanel();

	ControlList GetSelectedControls();
	void        Reload();

private:

	// Filtering
	void ResetFilters();
	void ApplyFilter();
	bool ApplyFilter(const QModelIndex parent);
	bool IsValid(const QModelIndex index);
	void ShowControlType(EACEControlType type, bool bShow, bool bExclusive);

	// Helper Functions
	void           SelectItem(QStandardItem* pItem);
	QStandardItem* GetCurrentItem();
	CATLControl*   GetControlFromItem(QStandardItem* pItem);
	CATLControl*   GetControlFromIndex(QModelIndex index);

	void           HandleExternalDropEvent(QDropEvent* pDropEvent);

	// ------------- IATLControlModelListener ----------------
	virtual void OnControlAdded(CATLControl* pControl) override;
	// -------------------------------------------------------

	// ------------------ QWidget ----------------------------
	bool eventFilter(QObject* pObject, QEvent* pEvent);
	// -------------------------------------------------------

private slots:
	void           ItemModified(QStandardItem* pItem);

	QStandardItem* CreateFolder();
	QStandardItem* AddControl(CATLControl* pControl);

	// Create controls / folders
	void CreateRTPCControl();
	void CreateSwitchControl();
	void CreateStateControl();
	void CreateTriggerControl();
	void CreateEnvironmentsControl();
	void CreatePreloadControl();
	void DeleteSelectedControl();

	void ShowControlsContextMenu(const QPoint& pos);

	// Filtering
	void SetFilterString(const QString& filterText);
	void ShowTriggers(bool bShow);
	void ShowRTPCs(bool bShow);
	void ShowEnvironments(bool bShow);
	void ShowSwitches(bool bShow);
	void ShowPreloads(bool bShow);

	// Audio Preview
	void ExecuteControl();
	void StopControlExecution();

signals:
	void SelectedControlChanged();
	void ControlTypeFiltered(EACEControlType type, bool bShow);

private:
	QSortFilterProxyModel*   m_pProxyModel;
	QATLTreeModel*           m_pTreeModel;
	CATLControlsModel* const m_pATLModel;

	// Context Menu
	QMenu m_addItemMenu;

	// Filtering
	QString                 m_sFilter;
	QMenu                   m_filterMenu;
	QAction*                m_pFilterActions[eACEControlType_NumTypes];
	bool                    m_visibleTypes[EACEControlType::eACEControlType_NumTypes];

	QLineEdit*              m_pTextFilter;
	QAudioControlsTreeView* m_pATLControlsTree;
};
}
