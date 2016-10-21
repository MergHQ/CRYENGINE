// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QAbstractItemModel>
#include <QTreeView>
#include <QWidget>
#include <CrySerialization/Forward.h>
#include <Schematyc/Script/Schematyc_IScriptRegistry.h>
#include <Schematyc/Utils/Schematyc_EnumFlags.h>
#include <Schematyc/Utils/Schematyc_ScopedConnection.h>
#include <Schematyc/Utils/Schematyc_Signal.h>

// Forward declare classes.
class QBoxLayout;
class QItemSelection;
class QLineEdit;
class QMenu;
class QParentWndWidget;
class QPushButton;
class QSplitter;
class QToolBar;

namespace Schematyc
{
// Forward declare structures.
struct SSourceControlStatus;
// Forward declare classes.
class CScriptBrowserFilter;
class CScriptBrowserItem;
class CSourceControlManager;
// Forward declare shared pointers.
DECLARE_SHARED_POINTERS(CScriptBrowserItem)

struct EScriptBrowserColumn
{
	enum : int
	{
		Name = 0,
		File,
		Count
	};
};

enum class EScriptBrowserItemType
{
	None,
	Root,
	Filter,
	ScriptElement
};

enum class EScriptBrowserItemFlags
{
	None                  = 0,

	Modified              = BIT(0),   // #SchematycTODO : Should this be a flag on the script element itself?

	FileLocal             = BIT(1),
	FileInPak             = BIT(2),
	FileReadOnly          = BIT(3),
	FileManaged           = BIT(4),
	FileCheckedOut        = BIT(5),
	FileAdd               = BIT(6),

	ContainsWarnings      = BIT(7),
	ContainsErrors        = BIT(8),
	ChildContainsWarnings = BIT(9),
	ChildContainsErrors   = BIT(10),

	FileMask              = FileLocal | FileInPak | FileReadOnly | FileManaged | FileCheckedOut | FileAdd,
	ChildMask             = ChildContainsWarnings | ChildContainsErrors
};

typedef CEnumFlags<EScriptBrowserItemFlags> ScriptBrowserItemFlags;

class CScriptBrowserItem
{
private:

	typedef std::vector<CScriptBrowserItemPtr> Items;

public:

	CScriptBrowserItem(EScriptBrowserItemType type, const char* szName, const char* szIcon);
	CScriptBrowserItem(EScriptBrowserItemType type, const char* szName, EScriptElementType filter);
	CScriptBrowserItem(EScriptBrowserItemType type, const char* szName, const char* szIcon, IScriptElement* pScriptElement);

	EScriptBrowserItemType GetType() const;
	void                   SetName(const char* szName);
	const char*            GetName() const;
	const char*            GetPath() const;
	const char*            GetIcon() const;
	const char*            GetTooltip() const;
	void                   SetFlags(const ScriptBrowserItemFlags& flags);
	ScriptBrowserItemFlags GetFlags() const;
	EScriptElementType     GetFilter() const;
	const char*            GetFileText() const;
	const char*            GetFileIcon() const;
	const char*            GetFileToolTip() const;
	QVariant               GetColor() const;
	IScriptElement*        GetScriptElement() const;
	IScript*               GetScript() const;

	CScriptBrowserItem*    GetParent();
	void                   AddChild(const CScriptBrowserItemPtr& pChild);
	CScriptBrowserItemPtr  RemoveChild(CScriptBrowserItem* pChild);
	int                    GetChildCount() const;
	int                    GetChildIdx(CScriptBrowserItem* pChild);
	CScriptBrowserItem*    GetChildByIdx(int childIdx);
	CScriptBrowserItem*    GetChildByTypeAndName(EScriptBrowserItemType type, const char* szName);

	void                   Validate();
	void                   RefreshChildFlags();

private:

	EScriptBrowserItemType m_type;
	string                 m_name;
	string                 m_path;
	string                 m_icon;
	string                 m_tooltip;
	ScriptBrowserItemFlags m_flags;
	EScriptElementType     m_filter;
	IScriptElement*        m_pScriptElement;
	CScriptBrowserItem*    m_pParent;
	Items                  m_children;
};

class CScriptBrowserModel : public QAbstractItemModel
{
	Q_OBJECT

private:

	typedef std::unordered_map<SGUID, CScriptBrowserItem*> ItemsByGUID;
	typedef std::map<string, CScriptBrowserItem*>          ItemsByFileName;

public:

	CScriptBrowserModel(QObject* pParent, CSourceControlManager& sourceControlManager);

	// QAbstractItemModel
	QModelIndex   index(int row, int column, const QModelIndex& parent) const override;
	QModelIndex   parent(const QModelIndex& index) const override;
	int           rowCount(const QModelIndex& index) const override;
	int           columnCount(const QModelIndex& parent) const override;
	bool          hasChildren(const QModelIndex& parent) const override;
	QVariant      data(const QModelIndex& index, int role) const override;
	bool          setData(const QModelIndex& index, const QVariant& value, int role) override;
	QVariant      headerData(int section, Qt::Orientation orientation, int role) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	// ~QAbstractItemModel

	void                Reset();

	QModelIndex         ItemToIndex(CScriptBrowserItem* pItem, int column = 0) const;
	CScriptBrowserItem* ItemFromIndex(const QModelIndex& index) const;

	CScriptBrowserItem* GetRootItem();
	CScriptBrowserItem* GetItemByGUID(const SGUID& guid);
	CScriptBrowserItem* GetItemByFileName(const char* szFileName);

public slots:

private:

	void                  Populate();

	void                  OnScriptRegistryChange(const SScriptRegistryChange& change);
	void                  OnSourceControlStatusUpdate(const char* szFileName, const SSourceControlStatus& status);
	void                  OnScriptElementAdded(IScriptElement& scriptElement);
	void                  OnScriptElementModified(IScriptElement& scriptElement);
	void                  OnScriptElementRemoved(IScriptElement& scriptElement);
	void                  OnScriptElementSaved(IScriptElement& scriptElement);

	CScriptBrowserItem*   CreateScriptElementItem(IScriptElement& scriptElement, const ScriptBrowserItemFlags& flags, CScriptBrowserItem& parentItem);

	CScriptBrowserItem*   AddItem(CScriptBrowserItem& parentItem, const CScriptBrowserItemPtr& pItem);
	CScriptBrowserItemPtr RemoveItem(CScriptBrowserItem& item);

	void                  ValidateItem(CScriptBrowserItem& item);

private:

	CSourceControlManager& m_sourceControlManager;

	CScriptBrowserItemPtr  m_pRootItem;
	ItemsByGUID            m_itemsByGUID;
	ItemsByFileName        m_itemsByFileName;

	CConnectionScope       m_connectionScope;
};

class CScriptBrowserTreeView : public QTreeView
{
	Q_OBJECT

public:

	CScriptBrowserTreeView(QWidget* pParent);

signals:

	void keyPress(QKeyEvent* pKeyEvent, bool& bEventHandled);

protected:

	virtual void keyPressEvent(QKeyEvent* pKeyEvent) override;
};

struct SScriptBrowserSelection
{
	SScriptBrowserSelection(IScriptElement* _pScriptElement);

	IScriptElement* pScriptElement;
};

typedef CSignal<void, const SScriptBrowserSelection&> ScriptBrowserSelectionSignal;

class CScriptBrowserWidget : public QWidget
{
	Q_OBJECT

private:

	struct SSignals
	{
		ScriptBrowserSelectionSignal selection;
	};

public:

	CScriptBrowserWidget(QWidget* pParent, CSourceControlManager& sourceControlManager);

	~CScriptBrowserWidget();

	void                                 InitLayout();
	void                                 Reset();
	void                                 SelectItem(const SGUID& guid);
	void                                 Serialize(Serialization::IArchive& archive);
	ScriptBrowserSelectionSignal::Slots& GetSelectionSignalSlots();

public slots:

	void OnHomeButtonClicked();
	void OnBackButtonClicked();
	void OnForwardButtonClicked();
	void OnSearchFilterChanged(const QString& text);
	void OnAddMenuShow();
	void OnTreeViewDoubleClicked(const QModelIndex& index);
	void OnTreeViewCustomContextMenuRequested(const QPoint& position);
	void OnTreeViewKeyPress(QKeyEvent* pKeyEvent, bool& bEventHandled);
	void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void OnScopeToThis();
	void OnFindReferences();
	void OnAddItem();
	void OnCopyItem();
	void OnPasteItem();
	void OnRemoveItem();
	void OnRenameItem();
	void OnSave();
	void OnAddToSourceControl();
	void OnGetLatestRevision();
	void OnCheckOut();
	void OnRevert();
	void OnDiffAgainstLatestRevision();
	void OnShowInExplorer();
	void OnExtract();

protected:

	virtual void keyPressEvent(QKeyEvent* pKeyEvent) override;

private:

	void        ExpandAll();
	void        RefreshAddMenu(CScriptBrowserItem* pItem);
	void        SetScope(const QModelIndex& index);
	bool        HandleKeyPress(QKeyEvent* pKeyEvent);

	QModelIndex GetTreeViewSelection() const;
	QModelIndex TreeViewToModelIndex(const QModelIndex& index) const;
	QModelIndex TreeViewFromModelIndex(const QModelIndex& index) const;

private:

	CSourceControlManager&  m_sourceControlManager;

	QBoxLayout*             m_pMainLayout;
	QBoxLayout*             m_pButtonLayout;
	QLineEdit*              m_pSearchFilter;
	QPushButton*            m_pHomeButton;
	QPushButton*            m_pBackButton;
	QPushButton*            m_pForwardButton;
	QPushButton*            m_pAddButton;
	QMenu*                  m_pAddMenu;
	CScriptBrowserTreeView* m_pTreeView;
	CScriptBrowserModel*    m_pModel;
	CScriptBrowserFilter*   m_pFilter;

	SSignals                m_signals;
};
} // Schematyc
