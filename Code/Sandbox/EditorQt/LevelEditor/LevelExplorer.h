// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QWidget>

#include "EditorFramework/Editor.h"

class QAction;
class QAdvancedTreeView;
class QFilteringPanel;
class QAttributeFilterProxyModel;
class QAbstractItemModel;
class QLabel;
struct CLayerChangeEvent;
class CLevelExplorer;
class QBoxLayout;
class QItemSelection;
class CBaseObject;
class CObjectLayer;
struct CObjectEvent;
class QToolButton;

class CLevelExplorer final : public CDockableEditor
{
	Q_OBJECT

public:
	CLevelExplorer(QWidget* pParent = nullptr);
	~CLevelExplorer();

	//////////////////////////////////////////////////////////////////////////
	// CEditor implementation
	virtual const char* GetEditorName() const override { return "Level Explorer"; }
	virtual void        SetLayout(const QVariantMap& state) override;
	virtual QVariantMap GetLayout() const override;
	virtual void        customEvent(QEvent* event) override;
	virtual void        resizeEvent(QResizeEvent* event) override;
	//////////////////////////////////////////////////////////////////////////

	enum ModelType
	{
		Objects,
		Layers,
		FullHierarchy,
		ActiveLayer
	};

	bool IsModelTypeShowingLayers() const
	{
		return m_modelType == Layers || m_modelType == FullHierarchy;
	}

	void SetModelType(ModelType aModelType);
	void SetSyncSelection(bool syncSelection);
	void FocusActiveLayer();
	void GrabFocusSearchBar() { OnFind(); }

private:
	QToolButton* CreateToolButton(QAction* pAction);
	void         InitMenuBar();

	virtual void OnContextMenu(const QPoint& pos) const;

	void         CreateContextMenuForLayers(CAbstractMenu& abstractMenu, const std::vector<CObjectLayer*>& layers) const;
	void         CreateContextForSingleFolderLayer(CAbstractMenu& abstractMenu, const std::vector<CObjectLayer*>& layerFolders) const;
	void         OnContextMenuForSingleLayer(CAbstractMenu& menu, CObjectLayer* layer) const;
	void         OnClick(const QModelIndex& index);
	void         OnDoubleClick(const QModelIndex& index);

	//////////////////////////////////////////////////////////////////////////
	// CEditor impl
	virtual bool OnFind() override;
	virtual bool OnDelete() override;
	//////////////////////////////////////////////////////////////////////////

	void        OnLayerChange(const CLayerChangeEvent& event);
	void        OnObjectsChanged(const std::vector<CBaseObject*>& objects, const CObjectEvent& event);
	void        OnViewportSelectionChanged(const std::vector<CBaseObject*>& selected, const std::vector<CBaseObject*>& deselected);
	void        OnLayerModelsUpdated();
	void        OnModelDestroyed();
	void        OnNewLayer(CObjectLayer* parent) const;
	void        OnNewFolder(CObjectLayer* parent) const;
	void        OnReloadLayers(const std::vector<CObjectLayer*>& layers) const;
	void        OnDeleteLayers(const std::vector<CObjectLayer*>& layers) const;
	void        OnImportLayer(CObjectLayer* pTargetLayer = nullptr) const;
	void        IsolateEditability(const QModelIndex& index) const;
	void        IsolateVisibility(const QModelIndex& index) const;
	void        OnFreezeAllInLayer(CObjectLayer* layer) const;
	void        OnUnfreezeAllInLayer(CObjectLayer* layer) const;
	void        OnHideAllInLayer(CObjectLayer* layer) const;
	void        OnUnhideAllInLayer(CObjectLayer* layer) const;
	bool        AllFrozenInLayer(CObjectLayer* layer) const;
	bool        AllHiddenInLayer(CObjectLayer* layer) const;
	void        OnExpandAllLayers() const;
	void        OnCollapseAllLayers() const;
	void        OnSelectColor(const std::vector<CObjectLayer*>& layers) const;
	void        OnHeaderSectionCountChanged();
	void        OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void        OnRename(const QModelIndex& index) const;
	//Register to any Reset event being called on a CLevelLayerModel
	void        OnLayerModelResetBegin();
	void        OnLayerModelResetEnd();
	void        EditLayer(CObjectLayer* pLayer) const;

	void        SetSourceModel(QAbstractItemModel* model);
	void        SyncSelection();

	void        UpdateCurrentIndex();
	void        CreateItemSelectionFrom(const std::vector<CBaseObject*>& objects, QItemSelection& selection) const;

	QModelIndex FindObjectIndex(const CBaseObject* object) const;
	QModelIndex FindLayerIndex(const CObjectLayer* layer) const;
	QModelIndex FindIndexByInternalId(intptr_t id) const;
	QModelIndex FindLayerIndexInModel(const CObjectLayer* layer) const;
	QModelIndex FindObjectInHierarchy(const QModelIndex& parentIndex, const CBaseObject* object) const;

	ModelType                   m_modelType;
	QAdvancedTreeView*          m_treeView;
	QFilteringPanel*            m_filterPanel;
	QAttributeFilterProxyModel* m_pAttributeFilterProxyModel;
	QBoxLayout*                 m_pMainLayout;
	QBoxLayout*                 m_pShortcutBarLayout;

	QAction*                    m_pShowActiveLayer;
	QAction*                    m_pShowAllObjects;
	QAction*                    m_pShowFullHierarchy;
	QAction*                    m_pShowLayers;
	QAction*                    m_pSyncSelection;

	bool                        m_syncSelection;
	bool                        m_ignoreSelectionEvents;
};
