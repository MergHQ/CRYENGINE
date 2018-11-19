// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <QWidget>

class CProjectsModel;
class CProjectSortProxyModel;
class CSelectProjectDialog;
class QAdvancedTreeView;
class QButtonGroup;
class QItemSelection;
class QModelIndex;
class QPushButton;
class QThumbnailsView;
class QVBoxLayout;

class COpenProjectPanel : public QWidget
{
public:
	COpenProjectPanel(CSelectProjectDialog* pParent, bool runOnSandboxInit);

private:
	void CreateSearchPanel();
	void CreateViews();
	void CreateListViewButton(QButtonGroup* pGroup);
	void CreateThumbnailButton(QButtonGroup* pGroup);
	void CreateDialogButtons(bool runOnSandboxInit);

	void SetViewMode(bool thumbnailMode);

	void OpenProject(const QModelIndex& index);
	void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

	void OnLoadProjectPressed();

	CSelectProjectDialog*   m_pParent;

	CProjectsModel*         m_pModel;
	CProjectSortProxyModel* m_pSortedModel;

	QVBoxLayout*            m_pMainLayout;
	QAdvancedTreeView*      m_pTreeView;
	QThumbnailsView*        m_pThumbnailView;
	QPushButton*            m_pOpenProjectBtn;
};
