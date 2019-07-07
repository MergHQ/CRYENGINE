// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include "DictionaryWidget.h"

#include "QSearchBox.h"
#include "QPopupWidget.h"
#include "QAdvancedTreeView.h"
#include "ProxyModels/AttributeFilterProxyModel.h"
#include "ProxyModels/ItemModelAttribute.h"
#include "ProxyModels/MergingProxyModel.h"

#include <QFilteringPanel.h>
#include <EditorStyleHelper.h>

#include <QAbstractItemModel>
#include <QBoxLayout>
#include <QEventLoop>
#include <QHelpEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QToolTip>

class CDictionaryModel : public QAbstractItemModel
{
public:
	enum ERole : int32
	{
		EditRole      = Qt::EditRole,
		DisplayRole   = Qt::DisplayRole,		
		IconRole      = Qt::DecorationRole,
		ColorRole     = Qt::TextColorRole,
		PointerRole   = Qt::UserRole + 0,
		MergingRole   = Qt::UserRole + 1,
		ToolTipRole   = Qt::UserRole + 2,
		CategoryRole  = Qt::UserRole + 3,
	};

public:
	CDictionaryModel(CAbstractDictionary& dictionary)
		: QAbstractItemModel(&dictionary)
		, m_dictionary(dictionary)
	{
	}

	~CDictionaryModel()
	{
	}

	CAbstractDictionary* GetDictionary() const
	{
		return &m_dictionary;
	}

	void Clear()
	{
		beginRemoveRows(QModelIndex(), 0, m_dictionary.GetNumEntries() - 1);
		m_dictionary.ClearEntries();
		endRemoveRows();
	}

	void Reset()
	{
		beginResetModel();
		m_dictionary.ResetEntries();
		endResetModel();
	}

	// QAbstractItemModel
	virtual int rowCount(const QModelIndex& parent) const override
	{
		if (!parent.isValid())
		{
			return m_dictionary.GetNumEntries();
		}
		else
		{
			const CAbstractDictionaryEntry* pEntry = static_cast<const CAbstractDictionaryEntry*>(parent.internalPointer());
			if (pEntry)
			{
				return pEntry->GetNumChildEntries();
			}
		}
		return 0;
	}

	virtual Qt::ItemFlags flags(const QModelIndex &index) const override
	{
		if (index.isValid())
		{
			const CAbstractDictionaryEntry* pEntry = static_cast<const CAbstractDictionaryEntry*>(index.internalPointer());
			if (pEntry)
			{				
				return (pEntry->IsEditable(index.column()) ? (Qt::ItemIsEditable | QAbstractItemModel::flags(index)) : QAbstractItemModel::flags(index));
			}
		}

		return 0;
	}

	virtual int CDictionaryModel::columnCount(const QModelIndex& parent) const override
	{
		return m_dictionary.GetNumColumns();
	}

	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override
	{
		return GetHeaderDataFromDictionary(m_dictionary, section, orientation, role);
	}

	virtual QVariant CDictionaryModel::data(const QModelIndex& index, int role) const override
	{
		if (index.isValid())
		{
			const CAbstractDictionaryEntry* pEntry = static_cast<const CAbstractDictionaryEntry*>(index.internalPointer());
			if (pEntry)
			{
				switch (role)
				{
				case CDictionaryModel::EditRole:
				case CDictionaryModel::DisplayRole:
				case CDictionaryModel::MergingRole:
				case CDictionaryModel::CategoryRole:
					{
						return QVariant::fromValue(pEntry->GetColumnValue(index.column()));
					}
					break;
				case CDictionaryModel::ToolTipRole:
					{
						const QString text = pEntry->GetToolTip(index.column());
						if (!text.isEmpty())
						   return QVariant::fromValue(text);
					}
					break;
				case CDictionaryModel::IconRole:
					{
						if (const QIcon* pIcon = pEntry->GetColumnIcon(index.column()))
							return QVariant::fromValue(*pIcon);
					}
					break;
				case CDictionaryModel::PointerRole:
					{
						return QVariant::fromValue(reinterpret_cast<quintptr>(pEntry));
					}
					break;
				case CDictionaryModel::ColorRole:
				{
					if (pEntry->IsEnabled())
					{
						return GetStyleHelper()->textColor();
					}
					else
					{
						return GetStyleHelper()->disabledTextColor();
					}
				}
				break;
				default:
					break;
				}
			}
		}
		return QVariant();
	}

	virtual bool setData(const QModelIndex& index, const QVariant& value, int role) override
	{
		if (index.isValid())
		{
			CAbstractDictionaryEntry* pEntry = static_cast<CAbstractDictionaryEntry*>(index.internalPointer());
			if (pEntry)
			{
				switch (role)
				{
					case CDictionaryModel::EditRole:
					{
						pEntry->SetColumnValue(index.column(), value);
					}
					break;
				}
				return true;
			}
		}
		return false;
	}

	virtual QModelIndex CDictionaryModel::index(int row, int column, const QModelIndex& parent) const override
	{
		if (!parent.isValid())
		{
			return QAbstractItemModel::createIndex(row, column, reinterpret_cast<quintptr>(m_dictionary.GetEntry(row)));
		}

		const CAbstractDictionaryEntry* pParentEntry = static_cast<const CAbstractDictionaryEntry*>(parent.internalPointer());
		if (pParentEntry)
		{
			return QAbstractItemModel::createIndex(row, column, reinterpret_cast<quintptr>(pParentEntry->GetChildEntry(row)));
		}

		return QModelIndex();
	}

	virtual QModelIndex CDictionaryModel::parent(const QModelIndex& index) const override
	{
		if (index.isValid())
		{
			const CAbstractDictionaryEntry* pEntry = static_cast<const CAbstractDictionaryEntry*>(index.internalPointer());
			if (pEntry)
			{
				const CAbstractDictionaryEntry* pParentEntry = pEntry->GetParentEntry();
				if (pParentEntry)
				{
					const CAbstractDictionaryEntry* pParentParentEntry = pParentEntry->GetParentEntry();
					if (pParentParentEntry)
					{
						for (int32 i = 0; i < pParentParentEntry->GetNumChildEntries(); ++i)
						{
							if (pParentParentEntry->GetChildEntry(i) == pParentEntry)
							{
								return QAbstractItemModel::createIndex(i, 0, reinterpret_cast<quintptr>(pParentEntry));
							}
						}
					}
					else
					{
						for (int32 i = 0; i < m_dictionary.GetNumEntries(); ++i)
						{
							if (m_dictionary.GetEntry(i) == pParentEntry)
							{
								return QAbstractItemModel::createIndex(i, 0, reinterpret_cast<quintptr>(pParentEntry));
							}
						}
					}
				}
			}

		}
		return QModelIndex();
	}
	// ~QAbstractItemModel

public:
	static QVariant GetHeaderDataFromDictionary(const CAbstractDictionary& dictionary, int section, Qt::Orientation orientation, int role)
	{
		if (orientation != Qt::Horizontal)
		{
			return QVariant();
		}

		const CItemModelAttribute* pAttribute = dictionary.GetColumnAttribute(section);
		if (pAttribute)
		{
			if (role == DisplayRole || role == ToolTipRole || role == MergingRole)
			{
				return pAttribute->GetName();
			}
			else if (role == Attributes::s_getAttributeRole)
			{
				return QVariant::fromValue(const_cast<CItemModelAttribute*>(pAttribute));
			}
			else if (role == Attributes::s_attributeMenuPathRole)
			{
				return "";
			}
		}

		return QVariant();
	}

private:
	CAbstractDictionary& m_dictionary;
};

CDictionaryFilterProxyModel::CDictionaryFilterProxyModel(BehaviorFlags behavior, QObject* pParent, int role)
	: QAttributeFilterProxyModel(behavior, pParent, role)
	, m_sortingOrder(Qt::AscendingOrder)
{
}

void CDictionaryFilterProxyModel::sort(int column, Qt::SortOrder order)
{
	m_sortingOrder = order;
	QDeepFilterProxyModel::sort(column, order);
}

bool CDictionaryFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
	return m_sortingFunc(left, right);
}

void CDictionaryFilterProxyModel::SetupSortingFunc(const CSortingFunc& sortingFunc)
{
	m_sortingFunc = sortingFunc;
}

void CDictionaryFilterProxyModel::RestoreDefaultSortingFunc()
{
	m_sortingFunc = std::bind(&CDictionaryFilterProxyModel::DefaultSortingFunc, this, std::placeholders::_1, std::placeholders::_2);
}

bool CDictionaryFilterProxyModel::DefaultSortingFunc(const QModelIndex& left, const QModelIndex& right)
{
	CAbstractDictionaryEntry* pLeft = reinterpret_cast<CAbstractDictionaryEntry*>(left.data(CDictionaryModel::PointerRole).value<quintptr>());
	CAbstractDictionaryEntry* pRight = reinterpret_cast<CAbstractDictionaryEntry*>(right.data(CDictionaryModel::PointerRole).value<quintptr>());

	if (pLeft && !pLeft->IsSortable())
		return m_sortingOrder = Qt::AscendingOrder;

	if (pRight && !pRight->IsSortable())
		return m_sortingOrder != Qt::AscendingOrder;

	if (pLeft && pRight && pLeft->GetType() != pRight->GetType())
		return pLeft->GetType() == CAbstractDictionaryEntry::Type_Folder;

	return QDeepFilterProxyModel::lessThan(left, right);
}



CAbstractDictionary::CAbstractDictionary()
	: m_pDictionaryModel(new CDictionaryModel(*this))
{
}

CAbstractDictionary::~CAbstractDictionary()
{	
}

void CAbstractDictionary::Clear()
{
	m_pDictionaryModel->Clear();
}

void CAbstractDictionary::Reset()
{
	m_pDictionaryModel->Reset();
}

const CItemModelAttribute* CAbstractDictionary::GetFilterAttribute() const
{
	static CItemModelAttribute filterAttribute(GetName(), &Attributes::s_stringAttributeType);
	return &filterAttribute;
}

const CItemModelAttribute* CAbstractDictionary::GetColumnAttribute(int32 index) const
{
	static CItemModelAttribute fallback("", &Attributes::s_stringAttributeType);

	QString& columnName = const_cast<QString&>(fallback.GetName());
	columnName = GetColumnName(index);

	return &fallback;
}

CDictionaryModel* CAbstractDictionary::GetDictionaryModel() const
{
	return m_pDictionaryModel;
}


CDictionaryWidget::CDictionaryWidget(QWidget* pParent, QFilteringPanel* pFilteringPanel)
	: m_pFilterProxy(nullptr)
	, m_pFilteringPanel(pFilteringPanel)
	, m_pMergingModel(new CMergingProxyModel(this))
	, m_pTreeView(new QDictionaryTreeView(this, static_cast<QAdvancedTreeView::Behavior>(QAdvancedTreeView::PreserveExpandedAfterReset | QAdvancedTreeView::PreserveSelectionAfterReset | QAdvancedTreeView::UseItemModelAttribute)))
{
	m_pFilter = pFilteringPanel ? pFilteringPanel->GetSearchBox() : new QSearchBox();

	m_pFilter->EnableContinuousSearch(true);
	m_pFilter->setPlaceholderText("Search");
	m_pFilter->signalOnSearch.Connect(this, &CDictionaryWidget::OnFiltered);

	m_pTreeView->setSelectionMode(QAbstractItemView::NoSelection);
	m_pTreeView->setHeaderHidden(true);
	m_pTreeView->setItemsExpandable(true);
	m_pTreeView->setRootIsDecorated(true);
	m_pTreeView->setMouseTracking(true);
	m_pTreeView->setSortingEnabled(true);

	QObject::connect(m_pTreeView, &QTreeView::clicked, this, &CDictionaryWidget::OnClicked);
	QObject::connect(m_pTreeView, &QTreeView::doubleClicked, this, &CDictionaryWidget::OnDoubleClicked);

	QVBoxLayout* pLayout = new QVBoxLayout();

	if (m_pFilteringPanel)
		pLayout->addWidget(m_pFilteringPanel);
	else
		pLayout->addWidget(m_pFilter);

	if (m_pFilteringPanel)
	{
		m_pFilteringPanel->SetContent(m_pTreeView);
		m_pFilteringPanel->GetSearchBox()->SetAutoExpandOnSearch(m_pTreeView);
	}
	else
	{
		pLayout->addWidget(m_pTreeView);
	}

	setLayout(pLayout);
}

CDictionaryWidget::~CDictionaryWidget()
{
	m_pFilter->deleteLater();
	m_pTreeView->deleteLater();
	m_pMergingModel->deleteLater();

	if (m_pFilterProxy)
		m_pFilterProxy->deleteLater();
}

CAbstractDictionary* CDictionaryWidget::GetDictionary(const QString& name) const
{
	CAbstractDictionary* pDictionary = nullptr;

	auto models = m_pMergingModel->GetSourceModels().toStdVector();
	auto it = std::find_if(models.begin(), models.end(), [name](const QAbstractItemModel* model)
	{
		return static_cast<const CDictionaryModel*>(model)->GetDictionary()->GetName() == name;
	});

	if (it != models.end())
	{
		pDictionary = static_cast<const CDictionaryModel*>(*it)->GetDictionary();
	}

	return pDictionary;
}

void CDictionaryWidget::AddDictionary(CAbstractDictionary& newDictionary)
{
	auto       models             = m_pMergingModel->GetSourceModels().toStdVector();
	const bool isDictAlreadyAdded = std::find(models.begin(), models.end(), newDictionary.GetDictionaryModel()) != models.end();

	CRY_ASSERT_MESSAGE(!isDictAlreadyAdded, "Dictionary was already added before.");
	if (isDictAlreadyAdded)
	{
		return;
	}

	std::vector<CItemModelAttribute*> columns;	

	models.push_back(newDictionary.GetDictionaryModel());
	GatherItemModelAttributes(columns, models);

	if (m_pFilterProxy)
		m_pFilterProxy->deleteLater();

	m_pMergingModel->SetHeaderDataCallbacks(columns.size(), std::bind(&CDictionaryWidget::GeneralHeaderDataCallback, this, columns, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), CDictionaryModel::MergingRole);
	m_pMergingModel->MountAppend(newDictionary.GetDictionaryModel());

	m_pFilterProxy = new CDictionaryFilterProxyModel(static_cast<CDictionaryFilterProxyModel::BehaviorFlags>(CDictionaryFilterProxyModel::AcceptIfChildMatches | CDictionaryFilterProxyModel::AcceptIfParentMatches), this, CDictionaryModel::CategoryRole);
	m_pFilterProxy->RestoreDefaultSortingFunc();
	m_pFilterProxy->setSourceModel(m_pMergingModel);

	if (m_pFilteringPanel)
		m_pFilteringPanel->SetModel(m_pFilterProxy);

	m_pFilter->SetModel(m_pFilterProxy);
	m_pTreeView->setModel(m_pFilterProxy);	

	const int32 filterColumn = newDictionary.GetDefaultFilterColumn();
	if (filterColumn >= 0)
		m_pFilterProxy->setFilterKeyColumn(filterColumn);

	const int32 sortColumn = newDictionary.GetDefaultSortColumn();
	if (sortColumn >= 0)
	{
		m_pTreeView->setSortingEnabled(true);
	}
	else
	{
		m_pTreeView->setSortingEnabled(false);
	}

	m_pFilterProxy->sort(0, Qt::SortOrder::AscendingOrder);

	const size_t columnCount = columns.size();
	for (size_t i = 0; i < columnCount; i++)
	{
		m_pTreeView->resizeColumnToContents(i);
	}
}

void CDictionaryWidget::RemoveDictionary(CAbstractDictionary& dictionary)
{
	m_pMergingModel->Unmount(dictionary.GetDictionaryModel());
}

void CDictionaryWidget::RemoveAllDictionaries()
{
	m_pMergingModel->UnmountAll();
}

void CDictionaryWidget::ShowHeader(bool isShown)
{
	m_pTreeView->setHeaderHidden(!isShown);
}

void CDictionaryWidget::SetFilterText(const QString& filterText)
{
	m_pFilter->setText(filterText);
}

QDictionaryTreeView* CDictionaryWidget::GetTreeView() const
{
	return m_pTreeView;
}

CDictionaryFilterProxyModel* CDictionaryWidget::GetFilterProxy() const
{
	return m_pFilterProxy;
}

void CDictionaryWidget::OnClicked(const QModelIndex& index)
{
	if (index.isValid())
	{
		CAbstractDictionaryEntry* pEntry = reinterpret_cast<CAbstractDictionaryEntry*>(index.data(CDictionaryModel::PointerRole).value<quintptr>());
		if (pEntry)
		{
			OnEntryClicked(*pEntry);
			if (pEntry->GetType() == CAbstractDictionaryEntry::Type_Folder)
			{
				if (m_pTreeView->isExpanded(index))
					m_pTreeView->collapse(index);
				else
					m_pTreeView->expand(index);
			}
		}
	}
}

void CDictionaryWidget::RemountDictionaries()
{
	//for (auto model : m_modelMap)
	//{
	//	m_pMergingModel->Unmount(model);
	//}

	//for (auto model : m_modelMap)
	//{
	//	m_pMergingModel->MountAppend(model);
	//}
}

void CDictionaryWidget::OnDoubleClicked(const QModelIndex& index)
{
	if (index.isValid())
	{
		CAbstractDictionaryEntry* pEntry = reinterpret_cast<CAbstractDictionaryEntry*>(index.data(CDictionaryModel::PointerRole).value<quintptr>());
		if (pEntry)
		{
			OnEntryDoubleClicked(*pEntry);
		}
	}
}

void CDictionaryWidget::OnFiltered()
{
	const QString text = m_pFilter->text();
	if (!text.isEmpty())
	{
		m_pTreeView->expandAll();
	}
	else
	{
		m_pTreeView->collapseAll();
	}
}

void CDictionaryWidget::GatherItemModelAttributes(std::vector<CItemModelAttribute*>& columns, std::vector<const QAbstractItemModel*>& models)
{
	for (auto it : models)
	{
		const CDictionaryModel* model = static_cast<const CDictionaryModel*>(it);
		CAbstractDictionary*    dictionary = model->GetDictionary();

		size_t count = dictionary->GetNumColumns();
		for (size_t i = 0; i < count; i++)
		{
			auto columnAttribute = const_cast<CItemModelAttribute*>(dictionary->GetColumnAttribute(i));
			if (std::find(columns.begin(), columns.end(), columnAttribute) == columns.end())
			{
				columns.push_back(columnAttribute);
			}
		}
	}
}

QVariant CDictionaryWidget::GeneralHeaderDataCallback(std::vector<CItemModelAttribute*>& columns, int section, Qt::Orientation orientation, int role)
{
	CRY_ASSERT(section < columns.size());

	if (orientation != Qt::Horizontal)
	{
		return QVariant();
	}

	const CItemModelAttribute* pAttribute = columns[section];
	if (pAttribute)
	{
		if (role == CDictionaryModel::DisplayRole || role == CDictionaryModel::ToolTipRole || role == CDictionaryModel::MergingRole)
		{
			return pAttribute->GetName();
		}
		else if (role == CDictionaryModel::CategoryRole)
		{
			return QVariant::fromValue(const_cast<CItemModelAttribute*>(pAttribute));
		}
		else if (role == Attributes::s_getAttributeRole)
		{
			return QVariant::fromValue(const_cast<CItemModelAttribute*>(pAttribute));
		}
		else if (role == Attributes::s_attributeMenuPathRole)
		{
			return "";
		}
	}

	return QVariant();
}

void CDictionaryWidget::showEvent(QShowEvent* pEvent)
{
	m_pFilter->setText("");
	QWidget::showEvent(pEvent);
	m_pFilter->setFocus(Qt::FocusReason::PopupFocusReason);
}

void CDictionaryWidget::hideEvent(QHideEvent* pEvent)
{
	OnHide();
	QWidget::hideEvent(pEvent);
}

CModalPopupDictionary::CModalPopupDictionary(QString name, CAbstractDictionary& dictionary)
	: m_pResult(nullptr)
	, m_pEventLoop(nullptr)
	, m_dictName(dictionary.GetName())
{
	m_pEventLoop = new QEventLoop();

	m_pDictionaryWidget = new CDictionaryWidget();
	m_pDictionaryWidget->AddDictionary(dictionary);

	m_pPopupWidget = new QPopupWidget(name, m_pDictionaryWidget);

	QObject::connect(m_pDictionaryWidget, &CDictionaryWidget::OnEntryClicked, this, &CModalPopupDictionary::OnEntryClicked);
	QObject::connect(m_pDictionaryWidget, &CDictionaryWidget::OnEntryDoubleClicked, this, &CModalPopupDictionary::OnEntryClicked);

	QObject::connect(m_pPopupWidget, &QPopupWidget::SignalHide, this, &CModalPopupDictionary::OnAborted);
}

CModalPopupDictionary::~CModalPopupDictionary()
{
	CAbstractDictionary* abstractDictionary = m_pDictionaryWidget->GetDictionary(m_dictName);
	CRY_ASSERT(abstractDictionary);

	m_pDictionaryWidget->RemoveDictionary(*abstractDictionary);
	m_pPopupWidget->deleteLater();
	delete m_pEventLoop;
}

void CModalPopupDictionary::ExecAt(const QPoint& globalPos, QPopupWidget::Origin origin /* = Unknown*/)
{
	m_pPopupWidget->ShowAt(globalPos, origin);
	m_pEventLoop->exec();
}

void CModalPopupDictionary::OnEntryClicked(CAbstractDictionaryEntry& entry)
{
	if (entry.GetType() == CAbstractDictionaryEntry::Type_Entry && entry.IsEnabled())
	{
		m_pResult = &entry;
		m_pPopupWidget->hide();
		if (m_pEventLoop)
			m_pEventLoop->exit();
	}
}

void CModalPopupDictionary::OnAborted()
{
	if (m_pEventLoop)
		m_pEventLoop->exit();
}

QDictionaryTreeView::QDictionaryTreeView(CDictionaryWidget* pWidget, BehaviorFlags flags, QWidget* pParent)
	: QAdvancedTreeView(flags, pParent)
	, m_pWidget(pWidget)
{
	CRY_ASSERT(m_pWidget);
}

QDictionaryTreeView::~QDictionaryTreeView()
{
}

void QDictionaryTreeView::SetSeparator(QModelIndex index)
{
	m_index = index;
}

void QDictionaryTreeView::drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QAdvancedTreeView::drawRow(painter, option, index);

	if (m_index.isValid() && (m_index == index))
	{
		painter->setPen(QColor(0x66, 0x66, 0x66));
		painter->drawLine(0, index.row()*rowHeight(index), viewport()->size().width(), index.row()*rowHeight(index));
	}
}

void QDictionaryTreeView::mouseMoveEvent(QMouseEvent* pEvent)
{
	QAdvancedTreeView::mouseMoveEvent(pEvent);

	QModelIndex index = indexAt(pEvent->pos());
	if (index.isValid())
	{
		QString text = model()->data(index, CDictionaryModel::ToolTipRole).value<QString>();
		if (text.size())
		{
			QToolTip::showText(pEvent->globalPos(), text);
		}
		else
		{
			QToolTip::hideText();
		}
	}
	else
	{
		QToolTip::hideText();
	}
}

void QDictionaryTreeView::mouseReleaseEvent(QMouseEvent* pEvent)
{
	QTreeView::mouseReleaseEvent(pEvent);

	if (pEvent->button() == Qt::RightButton)
	{		
		signalMouseRightButton(indexAt(pEvent->localPos().toPoint()), pEvent->screenPos().toPoint());
	}
}
