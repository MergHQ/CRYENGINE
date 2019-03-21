// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include "EditorFramework/Editor.h"
#include "ProxyModels/ItemModelAttribute.h"
#include "CrySandbox/CrySignal.h"
#include "AssetSystem/AssetType.h"

#include <QWidget>

class CAssetDropHandler;
class CAssetFolderFilterModel;
class CAssetFoldersView;
class CBreadcrumbsBar;
class CLineEditDelegate;
class QAction;
class QAdvancedTreeView;
class QAttributeFilterProxyModel;
class QBoxLayout;
class QButtonGroup;
class QCommandAction;
class QFilteringPanel;
class QItemSelectionModel;
class QMenu;
class QSplitter;
class QThumbnailsView;
class QTimer;
class QToolButton;
struct IUIContext;

//Thumbnails should now be used, leaving for now in case it is temporarily useful
#define ASSET_BROWSER_USE_PREVIEW_WIDGET 0

//! The dockable class for the Asset Browser
class EDITOR_COMMON_API CAssetBrowser : public CDockableEditor
{
	Q_OBJECT
	Q_PROPERTY(int buttonsSpacing READ GetButtonsSpacing WRITE SetButtonsSpacing)
	Q_PROPERTY(int buttonGroupsSpacing READ GetButtonGroupsSpacing WRITE SetButtonGroupsSpacing)

public:
	static CCrySignal<void(CAbstractMenu&, const std::shared_ptr<IUIContext>&)> s_signalMenuCreated;
	static CCrySignal<void(CAbstractMenu&, const std::vector<CAsset*>&, const std::vector<string>& folders, const std::shared_ptr<IUIContext>&)> s_signalContextMenuRequested;

	CAssetBrowser(bool bHideEngineFolder = false, QWidget* pParent = nullptr);
	virtual ~CAssetBrowser();

	std::vector<CAsset*> GetSelectedAssets() const;
	CAsset*              GetLastSelectedAsset() const;

	void                 SelectAsset(const char* szPath) const;
	void                 SelectAsset(const CAsset& asset) const;

	std::vector<string>  GetSelectedFolders() const;

	//CEditor implementation
	virtual const char* GetEditorName() const override { return "Asset Browser"; }
	virtual void        SetLayout(const QVariantMap& state) override;
	virtual QVariantMap GetLayout() const override;

	void                GrabFocusSearchBar() { OnFind(); }
	QFilteringPanel*    GetFilterPanel();

	enum ViewMode
	{
		Details,
		Thumbnails,
		VSplit,
		HSplit,

		Max
	};

	bool         IsRecursiveView() const;
	bool         IsFoldersViewVisible() const;

	void         SetViewMode(ViewMode viewMode);
	void         SetRecursiveView(bool recursiveView);
	void         SetFoldersViewVisible(bool isVisible);

	CAsset*      QueryNewAsset(const CAssetType& type, const CAssetType::SCreateParams* pCreateParams);

	void         NotifyContextMenuCreation(CAbstractMenu& menu, const std::vector<CAsset*>& assets, const std::vector<string>& folders);

	int          GetButtonsSpacing() const       { return m_buttonsSpacing; }
	void         SetButtonsSpacing(int val)      { m_buttonsSpacing = val; }
	int          GetButtonGroupsSpacing() const  { return m_buttonGroupsSpacing; }
	void         SetButtonGroupsSpacing(int val) { m_buttonGroupsSpacing = val; }

	virtual void customEvent(QEvent* pEvent) override;

signals:
	//! This signal is emitted whenever the selection of folders or assets changes.
	void SelectionChanged();

protected:
	virtual void                mouseReleaseEvent(QMouseEvent* pEvent) override;

	QAttributeFilterProxyModel* GetAttributeFilterProxyModel();
	QItemSelectionModel*        GetItemSelectionModel();
	QAdvancedTreeView*          GetDetailsView();
	QThumbnailsView*            GetThumbnailsView();

	// The widget has to be visible.
	void         ScrollToSelected();

	bool         ValidatePath(const QString);

	virtual void OnActivated(CAsset* pAsset);
	virtual void OnActivated(const QString& folder);

	virtual void UpdatePreview(const QModelIndex& currentIndex);
private:

	void               InitNewNameDelegates();
	void               InitViews(bool bHideEngineFolder);
	void               InitMenus();
	void               InitActions();
	void               InitAssetsView();
	void               InitDetailsView();
	void               InitThumbnailsView();
	void               AddViewModeButton(ViewMode viewMode, const char* szIconPath, const char* szToolTip, QMenu* pMenu = nullptr);
	QWidget*           CreateAssetsViewSelector();

	void               FillCreateAssetMenu(CAbstractMenu* menu, const QString& folder);

	void               BeginCreateAsset(const CAssetType& type, const CAssetType::SCreateParams* pCreateParams);
	void               EndCreateAsset();

	QAbstractItemView* GetFocusedView() const;

	virtual bool       eventFilter(QObject* object, QEvent* event) override;
	virtual void       resizeEvent(QResizeEvent* event) override;

	//extract actual content from the selection for further processing
	void ProcessSelection(std::vector<CAsset*>& assets, std::vector<string>& folders) const;

	void OnSelectionChanged();

	void UpdateSelectionDependantActions();
	void UpdatePasteActionState();

	void OnContextMenu();
	void AppendFilterDependenciesActions(CAbstractMenu* pAbstractMenu, const CAsset* pAsset);
	void OnFolderViewContextMenu();

	void CreateContextMenu(bool isFolderView = false);

	void BuildContextMenuForEmptiness(CAbstractMenu& abstractMenu);
	void BuildContextMenuForFolders(const std::vector<string>& folders, CAbstractMenu& abstractMenu);
	void BuildContextMenuForAssets(const std::vector<CAsset*>& assets, const std::vector<string>& folders, CAbstractMenu& abstractMenu);

	void AddWorkFilesMenu(CAbstractMenu& abstractMenu);

	bool HasSelectedModifiedAsset() const;

	void OnFolderSelectionChanged(const QStringList& selectedFolders);
	void OnActivated(const QModelIndex& index);
	void OnCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
	void OnDelete(const std::vector<CAsset*>& assets);

	void OnRenameAsset(CAsset& asset);
	void OnRenameFolder(const QString& folder);
	void OnOpenInExplorer(const QString& folder);
	void OnNavBack();
	void OnNavForward();
	void OnBreadcrumbClick(const QString& text, const QVariant& data);
	void OnBreadcrumbsTextChanged(const QString& text);

	void GenerateThumbnailsAsync(const string& folder, const std::function<void()>& finalize = std::function<void()>());

	void OnSearch(bool isNewSearch);
	void UpdateNavigation(bool clearHistory);
	void UpdateBreadcrumbsBar(const QString& path);

	void EditNewAsset();

	bool OnDiscardChanges();
	bool OnShowInFileExplorer();
	bool OnGenerateThumbmails();
	bool OnSaveAll();
	bool OnRecursiveView();
	bool OnFolderTreeView();
	bool OnManageWorkFiles();
	bool OnDetailsView();
	bool OnThumbnailsView();
	bool OnSplitHorizontalView();
	bool OnSplitVerticalView();
	bool OnGenerateRepairAllMetadata();
	bool OnReimport();

	// CEditor impl
	virtual bool OnFind() override;
	virtual bool OnDelete() override;
	virtual bool OnOpen() override;
	virtual bool OnCopy() override;
	virtual bool OnPaste() override;
	virtual bool OnDuplicate() override;
	virtual bool OnImport() override;
	virtual bool OnNewFolder() override;
	virtual bool OnRename() override;
	virtual bool OnSave() override;

	void         Paste(bool pasteNextToOriginal);

	//ui components
	CAssetFoldersView*                          m_pFoldersView = nullptr;
	CBreadcrumbsBar*                            m_pBreadcrumbs = nullptr;
	QCommandAction*                             m_pActionRecursiveView = nullptr;
	QCommandAction*                             m_pActionShowFoldersView = nullptr;
	QCommandAction*                             m_pActionManageWorkFiles = nullptr;
	QCommandAction*                             m_pActionShowInFileExplorer = nullptr;
	QCommandAction*                             m_pActionDelete = nullptr;
	QCommandAction*                             m_pActionRename = nullptr;
	QCommandAction*                             m_pActionCopy = nullptr;
	QCommandAction*                             m_pActionDuplicate = nullptr;
	QCommandAction*                             m_pActionSave = nullptr;
	QCommandAction*                             m_pActionPaste = nullptr;
	QCommandAction*                             m_pActionReimport = nullptr;
	QCommandAction*                             m_pActionDiscardChanges = nullptr;
	QAdvancedTreeView*                          m_pDetailsView = nullptr;
	QBoxLayout*                                 m_pAssetsViewLayout = nullptr;
	QBoxLayout*                                 m_pShortcutBarLayout = nullptr;
	QButtonGroup*                               m_pViewModeButtons = nullptr;
	QFilteringPanel*                            m_pFilterPanel = nullptr;
	QItemSelectionModel*                        m_pSelection = nullptr;
	QLabel*                                     m_pMultipleFoldersLabel = nullptr;
	QSplitter*                                  m_pFoldersSplitter = nullptr;
	QSplitter*                                  m_pMainViewSplitter = nullptr;
	QThumbnailsView*                            m_pThumbnailView = nullptr;
	QToolButton*                                m_pBackButton = nullptr;
	QToolButton*                                m_pForwardButton = nullptr;

	std::unique_ptr<CAbstractMenu>              m_pThumbnailSizeMenu;
	std::unique_ptr<CAssetDropHandler>          m_pAssetDropHandler;
	std::unique_ptr<CAssetFolderFilterModel>    m_pFolderFilterModel;
	std::unique_ptr<CLineEditDelegate>          m_pDetailsViewNewNameDelegate; // Note that delegates are not owned by view.
	std::unique_ptr<CLineEditDelegate>          m_pThumbnailViewNewNameDelegate;
	std::unique_ptr<QAttributeFilterProxyModel> m_pAttributeFilterProxyModel;
	std::unique_ptr<QTimer>                     m_pQuickEditTimer;

	//state variables
	QVector<QStringList> m_navigationHistory;
	ViewMode             m_viewMode = Max;
	int                  m_navigationIndex = -1;       //-1 is "all assets"
	bool                 m_dontPushNavHistory = false; //true when folder changes are triggered by back/forward buttons
	bool                 m_recursiveSearch = false;

	int                  m_buttonsSpacing = 4;
	int                  m_buttonGroupsSpacing = 24;

#if ASSET_BROWSER_USE_PREVIEW_WIDGET
	bool OnShowPreview();
	QContainer* m_previewWidget = nullptr;
#endif
};
