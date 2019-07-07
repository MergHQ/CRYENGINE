// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"

#include <QDialog>
#include <QString>
#include <QObject>
#include <QVariant>
#include <QWidget>

#include "QPopupWidget.h"
#include <CrySandbox/CrySignal.h>
#include "QAdvancedTreeView.h"
#include "ProxyModels/AttributeFilterProxyModel.h"

class CAbstractDictionary;
class CDictionaryFilterProxyModel;
class CDictionaryModel;
class CItemModelAttribute;
class CMergingProxyModel;
class QDictionaryTreeView;

class QAdvancedTreeView;
class QFilteringPanel;
class QHideEvent;
class QModelIndex;
class QSearchBox;
class QShowEvent;

class EDITOR_COMMON_API CAbstractDictionaryEntry
{
public:
	enum EType : uint32
	{
		Type_Undefined = 0,
		Type_Folder,
		Type_Entry
	};

public:
	CAbstractDictionaryEntry() {}
	virtual ~CAbstractDictionaryEntry() {}

	virtual uint32                          GetType()                         const                 { return Type_Undefined; }

	virtual QVariant                        GetColumnValue(int32 columnIndex) const                 { return QVariant();     }
	virtual void                            SetColumnValue(int32 columnIndex, const QVariant& data) {}

	virtual const QIcon*                    GetColumnIcon(int32 columnIndex)  const                 { return nullptr;        }
		
	virtual QString                         GetToolTip(int32 columnIndex)     const                 { return QString();      }

	virtual int32                           GetNumChildEntries()              const                 { return 0;              }
	virtual const CAbstractDictionaryEntry* GetChildEntry(int32 index)        const                 { return nullptr;        }

	virtual const CAbstractDictionaryEntry* GetParentEntry()                  const                 { return nullptr;        }

	virtual QVariant                        GetIdentifier()                   const                 { return QVariant();     }
	
	virtual bool                            IsEnabled()                       const                 { return true;           }
	virtual bool                            IsEditable(int32 columnIndex)     const                 { return false;          }
	virtual bool                            IsSortable()                      const                 { return true;           }
};

class EDITOR_COMMON_API CAbstractDictionary : public QObject
{
	friend class CDictionaryWidget;

public:
	CAbstractDictionary();
	virtual ~CAbstractDictionary();

public:
	void Clear();
	void Reset();

public:
	virtual const char*                     GetName()                       const { return "<Unknown>"; }
	virtual int32                           GetNumEntries()                 const = 0;
	virtual const CAbstractDictionaryEntry* GetEntry(int32 index)           const = 0;

	virtual void                            ClearEntries()                        { CRY_ASSERT_MESSAGE(false, "Please provide implementation."); }
	virtual void                            ResetEntries()                        { CRY_ASSERT_MESSAGE(false, "Please provide implementation."); }

	virtual int32                           GetNumColumns()                 const { return 0; }
	virtual QString                         GetColumnName(int32 index)      const { return QString(); }

	virtual int32                           GetDefaultFilterColumn()        const { return -1; }
	virtual int32                           GetDefaultSortColumn()          const { return -1; }

	virtual const CItemModelAttribute*      GetFilterAttribute()            const;
	virtual const CItemModelAttribute*      GetColumnAttribute(int32 index) const;

private:
	CDictionaryModel* GetDictionaryModel()            const;

private:
	CDictionaryModel* m_pDictionaryModel;
};

class EDITOR_COMMON_API CDictionaryFilterProxyModel : public QAttributeFilterProxyModel
{
	using CSortingFunc = std::function<bool(const QModelIndex&, const QModelIndex&)>;

public:
	CDictionaryFilterProxyModel(BehaviorFlags behavior = AcceptIfChildMatches, QObject* pParent = nullptr, int role = Attributes::s_getAttributeRole);

	// QSortFilterProxyModel
	// Ensures folders and entries are always grouped together in the sorting order.
	virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
	virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
	// ~QSortFilterProxyModel

public:
	void SetupSortingFunc(const CSortingFunc& sortingFunc);
	void RestoreDefaultSortingFunc();

//private:
	bool DefaultSortingFunc(const QModelIndex& left, const QModelIndex& right);

private:
	CSortingFunc  m_sortingFunc;
	Qt::SortOrder m_sortingOrder;
};

class EDITOR_COMMON_API CDictionaryWidget : public QWidget
{
	Q_OBJECT

public:
	CDictionaryWidget(QWidget* pParent = nullptr, QFilteringPanel* pFilteringPanel = nullptr);
	virtual ~CDictionaryWidget();

	CAbstractDictionary*         GetDictionary(const QString& name) const;
	void                         AddDictionary(CAbstractDictionary& dictionary);
	void                         RemoveDictionary(CAbstractDictionary& dictionary);
	void                         RemoveAllDictionaries();

	void                         ShowHeader(bool flag);
	void                         SetFilterText(const QString& filterText);
	
	QDictionaryTreeView*         GetTreeView() const;
	CDictionaryFilterProxyModel* GetFilterProxy() const;

	void                         RemountDictionaries();

Q_SIGNALS:
	void                         OnEntryClicked(CAbstractDictionaryEntry& entry);
	void                         OnEntryDoubleClicked(CAbstractDictionaryEntry& entry);

	void                         OnHide();

private:
	void                         OnClicked(const QModelIndex& index);
	void                         OnDoubleClicked(const QModelIndex& index);

	void                         OnFiltered();

	void                         GatherItemModelAttributes(std::vector<CItemModelAttribute*>& columns, std::vector<const QAbstractItemModel*>& models);
	QVariant                     GeneralHeaderDataCallback(std::vector<CItemModelAttribute*>& columns, int section, Qt::Orientation orientation, int role);

	virtual void                 showEvent(QShowEvent* pEvent) override;
	virtual void                 hideEvent(QHideEvent* pEvent) override;

private:
	QSearchBox*                  m_pFilter;
	QFilteringPanel*             m_pFilteringPanel;
	QDictionaryTreeView*           m_pTreeView;
	CMergingProxyModel*          m_pMergingModel;
	CDictionaryFilterProxyModel* m_pFilterProxy;
};

class EDITOR_COMMON_API CModalPopupDictionary : public QObject
{
public:
	CModalPopupDictionary(QString name, CAbstractDictionary& dictionary);
	~CModalPopupDictionary();

	void                      ExecAt(const QPoint& globalPos, QPopupWidget::Origin origin = QPopupWidget::Unknown);

	CAbstractDictionaryEntry* GetResult() const { return m_pResult; }

protected:
	void OnEntryClicked(CAbstractDictionaryEntry& entry);
	void OnAborted();

private:
	QString                   m_dictName;
	QPopupWidget*             m_pPopupWidget;
	CAbstractDictionaryEntry* m_pResult;
	QEventLoop*               m_pEventLoop;
	CDictionaryWidget*        m_pDictionaryWidget;
};

class EDITOR_COMMON_API QDictionaryTreeView : public QAdvancedTreeView
{
public:
	CCrySignal<void(QModelIndex, const QPoint&)> signalMouseRightButton;

public:
	QDictionaryTreeView(CDictionaryWidget* pWidget, BehaviorFlags flags = BehaviorFlags(PreserveExpandedAfterReset | PreserveSelectionAfterReset), QWidget* pParent = nullptr);
	virtual ~QDictionaryTreeView();

	void SetSeparator(QModelIndex index);

protected:
	virtual void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

	virtual void mouseMoveEvent(QMouseEvent* pEvent) override;
	virtual void mouseReleaseEvent(QMouseEvent* pEvent) override;
	
private:
	QModelIndex        m_index;
	CDictionaryWidget* m_pWidget;

};
