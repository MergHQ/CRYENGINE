// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ScriptBrowserWidget.h"

#include <QBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QParentWndWidget.h>
#include <QPushButton.h>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QToolbar>
#include <QWidgetAction>
#include <Schematyc/Compiler/ICompiler.h>
#include <Schematyc/Env/IEnvRegistry.h>
#include <Schematyc/Env/Elements/IEnvClass.h>
#include <Schematyc/Script/IScript.h>
#include <Schematyc/Script/IScriptRegistry.h>
#include <Schematyc/Script/Elements/IScriptActionInstance.h>
#include <Schematyc/Script/Elements/IScriptClass.h>
#include <Schematyc/Script/Elements/IScriptComponentInstance.h>
#include <Schematyc/Script/Elements/IScriptEnum.h>
#include <Schematyc/Script/Elements/IScriptFunction.h>
#include <Schematyc/Script/Elements/IScriptInterface.h>
#include <Schematyc/Script/Elements/IScriptInterfaceFunction.h>
#include <Schematyc/Script/Elements/IScriptInterfaceImpl.h>
#include <Schematyc/Script/Elements/IScriptInterfaceTask.h>
#include <Schematyc/Script/Elements/IScriptModule.h>
#include <Schematyc/Script/Elements/IScriptSignal.h>
#include <Schematyc/Script/Elements/IScriptSignalReceiver.h>
#include <Schematyc/Script/Elements/IScriptState.h>
#include <Schematyc/Script/Elements/IScriptStateMachine.h>
#include <Schematyc/Script/Elements/IScriptStruct.h>
#include <Schematyc/Script/Elements/IScriptTimer.h>
#include <Schematyc/Script/Elements/IScriptVariable.h>
#include <Schematyc/SerializationUtils/ISerializationContext.h>
#include <Schematyc/SerializationUtils/IValidatorArchive.h>
#include <Schematyc/Utils/StackString.h>
#include <Serialization/Qt.h>

#include "ScriptBrowserUtils.h"
#include "PluginUtils.h"
#include <CryIcon.h>

namespace Schematyc
{
namespace
{
static const char* g_szAddIcon = "icons:schematyc/add.png";
static const char* g_szRemoveIcon = "icons:General/Remove_Item.ico";
static const char* g_szHomeIcon = "icons:schematyc/home.png";
static const char* g_szBackIcon = "icons:schematyc/back.png";
static const char* g_szForwardIcon = "icons:schematyc/forward.png";
static const char* g_szFileNewIcon = "icons:schematyc/file_new.png";
static const char* g_szFileLocalIcon = "icons:schematyc/file_local.png";
static const char* g_szFileManagedIcon = "icons:schematyc/file_managed.png";
static const char* g_szFileCheckedOutIcon = "icons:schematyc/file_checked_out.png";
static const char* g_szFileAddIcon = "icons:schematyc/file_add.png";
static const char* g_szScriptRootIcon = "icons:schematyc/script_module.png";

const QSize g_defaultColumnSize[EScriptBrowserColumn::Count] = { QSize(100, 20), QSize(20, 20) };

const char* g_szClipboardPrefix = "[schematyc_xml]";
}

class CScriptBrowserFilter : public QSortFilterProxyModel
{
public:

	CScriptBrowserFilter(QObject* pParent, CScriptBrowserModel& model)
		: QSortFilterProxyModel(pParent)
		, m_model(model)
		, m_pScopeItem(nullptr)
	{}

	void SetFilterText(const char* szFilterText)
	{
		m_filterText = szFilterText;
	}

	void SetScope(const QModelIndex& index)
	{
		m_pScopeItem = m_model.ItemFromIndex(index);
	}

	QModelIndex GetScope() const
	{
		return m_model.ItemToIndex(m_pScopeItem);
	}

	// QSortFilterProxyModel

	virtual bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
	{
		QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
		CScriptBrowserItem* pItem = m_model.ItemFromIndex(index);
		if (pItem)
		{
			return FilterItemByScope(*pItem) && FilterItemByTextRecursive(*pItem);
		}
		return false;
	}

	virtual bool lessThan(const QModelIndex& lhs, const QModelIndex& rhs) const override
	{
		CScriptBrowserItem* pLHSItem = m_model.ItemFromIndex(lhs);
		CScriptBrowserItem* pRHSItem = m_model.ItemFromIndex(rhs);
		if (pLHSItem && pRHSItem)
		{
			const EScriptBrowserItemType lhsItemType = pLHSItem->GetType();
			const EScriptBrowserItemType rhsItemType = pRHSItem->GetType();
			if (lhsItemType != rhsItemType)
			{
				return lhsItemType > rhsItemType;
			}

			const IScriptElement* pLHSScriptElement = pLHSItem->GetScriptElement();
			const IScriptElement* pRHSScriptElement = pRHSItem->GetScriptElement();
			if (pLHSScriptElement && pRHSScriptElement)
			{
				const uint32 lhsScriptElementPriority = GetScriptElementPriority(pLHSScriptElement->GetType());
				const uint32 rhsScriptElementPriority = GetScriptElementPriority(pRHSScriptElement->GetType());
				if (lhsScriptElementPriority != rhsScriptElementPriority)
				{
					return lhsScriptElementPriority > rhsScriptElementPriority;
				}
			}
		}

		return QSortFilterProxyModel::lessThan(lhs, rhs);
	}

	// ~QSortFilterProxyModel

private:

	inline bool FilterItemByScope(CScriptBrowserItem& item) const
	{
		return !m_pScopeItem || (&item == m_pScopeItem) || (item.GetParent() != m_pScopeItem->GetParent());
	}

	inline bool FilterItemByTextRecursive(CScriptBrowserItem& item) const   // #SchematycTODO : Keep an eye on performance of this test!
	{
		switch (item.GetType())
		{
		case EScriptBrowserItemType::Root:
			{
				return true;
			}
		case EScriptBrowserItemType::ScriptElement:
			{
				if (StringUtils::Filter(item.GetName(), m_filterText.c_str()))
				{
					return true;
				}
				break;
			}
		}
		for (int childIdx = 0, childCount = item.GetChildCount(); childIdx < childCount; ++childIdx)
		{
			if (FilterItemByTextRecursive(*item.GetChildByIdx(childIdx)))
			{
				return true;
			}
		}
		return false;
	}

	inline uint32 GetScriptElementPriority(EScriptElementType scriptElementType) const
	{
		const EScriptElementType priorities[] =
		{
			EScriptElementType::Base,
			EScriptElementType::SignalReceiver,
			EScriptElementType::Constructor,
			EScriptElementType::ComponentInstance,
			EScriptElementType::ActionInstance,
			EScriptElementType::Variable
		};

		const uint32 priorityCount = CRY_ARRAY_COUNT(priorities);
		for (int32 priorityIdx = 0; priorityIdx < priorityCount; ++priorityIdx)
		{
			if (priorities[priorityIdx] == scriptElementType)
			{
				return priorityCount - priorityIdx;
			}
		}

		return 0;
	}

private:

	CScriptBrowserModel& m_model;
	string               m_filterText;
	CScriptBrowserItem*  m_pScopeItem;
};

CScriptBrowserItem::CScriptBrowserItem(EScriptBrowserItemType type, const char* szName, const char* szIcon)
	: m_type(type)
	, m_name(szName)
	, m_iconName(szIcon)
	, m_filter(EScriptElementType::None)
	, m_pScriptElement(nullptr)
	, m_pParent(nullptr)
{}

CScriptBrowserItem::CScriptBrowserItem(EScriptBrowserItemType type, const char* szName, EScriptElementType filter)
	: m_type(type)
	, m_name(szName)
	, m_filter(filter)
	, m_pScriptElement(nullptr)
	, m_pParent(nullptr)
{}

CScriptBrowserItem::CScriptBrowserItem(EScriptBrowserItemType type, const char* szName, const char* szIcon, IScriptElement* pScriptElement)
	: m_type(type)
	, m_name(szName)
	, m_iconName(szIcon)
	, m_filter(EScriptElementType::None)
	, m_pScriptElement(pScriptElement)
	, m_pParent(nullptr)
{}

EScriptBrowserItemType CScriptBrowserItem::GetType() const
{
	return m_type;
}

void CScriptBrowserItem::SetName(const char* szName)
{
	if (m_pScriptElement)
	{
		if (ScriptBrowserUtils::RenameScriptElement(*m_pScriptElement, szName))
		{
			m_name = szName;
		}
	}
}

const char* CScriptBrowserItem::GetName() const
{
	return m_name.c_str();
}

const char* CScriptBrowserItem::GetPath() const
{
	return m_path.c_str();
}

const char* CScriptBrowserItem::GetIcon() const
{
	return m_iconName.c_str();
}

const char* CScriptBrowserItem::GetTooltip() const
{
	return m_tooltip.c_str();
}

void CScriptBrowserItem::SetFlags(const ScriptBrowserItemFlags& flags)
{
	m_flags = flags;
}

ScriptBrowserItemFlags CScriptBrowserItem::GetFlags() const
{
	return m_flags;
}

EScriptElementType CScriptBrowserItem::GetFilter() const
{
	return m_filter;
}

const char* CScriptBrowserItem::GetFileText() const
{
	if (m_flags.Check(EScriptBrowserItemFlags::FileLocal))
	{
		if (m_flags.Check(EScriptBrowserItemFlags::FileManaged))
		{
			if (m_flags.Check(EScriptBrowserItemFlags::FileAdd))
			{
				return "SourceControl | Add";
			}
			else if (m_flags.Check(EScriptBrowserItemFlags::FileCheckedOut))
			{
				return "SourceControl | Checked Out";
			}
			else
			{
				return "SourceControl";
			}
		}
		else if (m_flags.Check(EScriptBrowserItemFlags::FileReadOnly))
		{
			return "Local | Read Only";
		}
		return "Local";
	}
	else if (m_flags.Check(EScriptBrowserItemFlags::FileInPak))
	{
		return "In Pak";
	}
	return "";
}

const char* CScriptBrowserItem::GetFileIcon() const
{
	IScript* pScript = GetScript();
	if (pScript)
	{
		if (m_flags.Check(EScriptBrowserItemFlags::FileLocal))
		{
			if (m_flags.Check(EScriptBrowserItemFlags::FileManaged))
			{
				if (m_flags.Check(EScriptBrowserItemFlags::FileCheckedOut))
				{
					if (m_flags.Check(EScriptBrowserItemFlags::FileAdd))
					{
						return g_szFileAddIcon;
					}
					else
					{
						return g_szFileCheckedOutIcon;
					}
				}
				else
				{
					return g_szFileManagedIcon;
				}
			}
			else
			{
				return g_szFileLocalIcon;
			}
		}
		else
		{
			return g_szFileNewIcon;
		}
	}
	return "";
}

const char* CScriptBrowserItem::GetFileToolTip() const
{
	IScript* pScript = GetScript();
	return pScript ? pScript->GetName() : "";
}

QVariant CScriptBrowserItem::GetColor() const
{
	if (m_flags.CheckAny({ EScriptBrowserItemFlags::ContainsErrors, EScriptBrowserItemFlags::ChildContainsErrors }))
	{
		return QColor(Qt::red);
	}
	else if (m_flags.CheckAny({ EScriptBrowserItemFlags::ContainsWarnings, EScriptBrowserItemFlags::ChildContainsWarnings }))
	{
		return QColor(Qt::yellow);
	}
	return QVariant();
}

IScriptElement* CScriptBrowserItem::GetScriptElement() const
{
	return m_pScriptElement;
}

IScript* CScriptBrowserItem::GetScript() const
{
	return m_pScriptElement ? m_pScriptElement->GetScript() : nullptr;
}

CScriptBrowserItem* CScriptBrowserItem::GetParent()
{
	return m_pParent;
}

void CScriptBrowserItem::AddChild(const CScriptBrowserItemPtr& pChild)
{
	m_children.push_back(pChild);
	pChild->m_pParent = this;
}

CScriptBrowserItemPtr CScriptBrowserItem::RemoveChild(CScriptBrowserItem* pChild)
{
	CScriptBrowserItemPtr pResult;
	for (Items::iterator itChild = m_children.begin(), itEndChild = m_children.end(); itChild != itEndChild; ++itChild)
	{
		if (itChild->get() == pChild)
		{
			pResult = *itChild;
			m_children.erase(itChild);
		}
	}
	return pResult;
}

int CScriptBrowserItem::GetChildCount() const
{
	return static_cast<int>(m_children.size());
}

int CScriptBrowserItem::GetChildIdx(CScriptBrowserItem* pChild)
{
	for (int childIdx = 0, childCount = m_children.size(); childIdx < childCount; ++childIdx)
	{
		if (m_children[childIdx].get() == pChild)
		{
			return childIdx;
		}
	}
	return -1;
}

CScriptBrowserItem* CScriptBrowserItem::GetChildByIdx(int childIdx)
{
	return childIdx < m_children.size() ? m_children[childIdx].get() : nullptr;
}

CScriptBrowserItem* CScriptBrowserItem::GetChildByTypeAndName(EScriptBrowserItemType type, const char* szName)
{
	if (szName)
	{
		for (const CScriptBrowserItemPtr& pChild : m_children)
		{
			if ((pChild->GetType() == type) && (strcmp(pChild->GetName(), szName) == 0))
			{
				return pChild.get();
			}
		}
	}
	return nullptr;
}

void CScriptBrowserItem::Validate()
{
	if (m_pScriptElement)
	{
		m_tooltip.clear();

		IValidatorArchivePtr pArchive = gEnv->pSchematyc->CreateValidatorArchive(SValidatorArchiveParams());
		ISerializationContextPtr pSerializationContext = gEnv->pSchematyc->CreateSerializationContext(SSerializationContextParams(*pArchive, ESerializationPass::Validate));
		m_pScriptElement->Serialize(*pArchive);

		ScriptBrowserItemFlags flags = m_flags;
		if (pArchive->GetWarningCount())
		{
			auto collectWarnings = [this](EValidatorMessageType messageType, const char* szMessage)
			{
				if (messageType == EValidatorMessageType::Warning)
				{
					m_tooltip.append("Warning: ");
					m_tooltip.append(szMessage);
					m_tooltip.append("\n");
				}
			};
			pArchive->Validate(Validator::FromLambda(collectWarnings));

			flags.Add(EScriptBrowserItemFlags::ContainsWarnings);
		}
		else
		{
			flags.Remove(EScriptBrowserItemFlags::ContainsWarnings);
		}

		if (pArchive->GetErrorCount())
		{
			auto collectErrors = [this](EValidatorMessageType messageType, const char* szMessage)
			{
				if (messageType == EValidatorMessageType::Error)
				{
					m_tooltip.append("Error: ");
					m_tooltip.append(szMessage);
					m_tooltip.append("\n");
				}
			};
			pArchive->Validate(Validator::FromLambda(collectErrors));

			flags.Add(EScriptBrowserItemFlags::ContainsErrors);
		}
		else
		{
			flags.Remove(EScriptBrowserItemFlags::ContainsErrors);
		}

		m_flags = flags;
	}

	RefreshChildFlags();
}

void CScriptBrowserItem::RefreshChildFlags()
{
	m_flags.Remove(EScriptBrowserItemFlags::ChildMask);

	for (const CScriptBrowserItemPtr& pChild : m_children)
	{
		const ScriptBrowserItemFlags childFlags = pChild->GetFlags();
		if (childFlags.CheckAny({ EScriptBrowserItemFlags::ContainsWarnings, EScriptBrowserItemFlags::ChildContainsWarnings }))
		{
			m_flags.Add(EScriptBrowserItemFlags::ChildContainsWarnings);
		}
		if (childFlags.CheckAny({ EScriptBrowserItemFlags::ContainsErrors, EScriptBrowserItemFlags::ChildContainsErrors }))
		{
			m_flags.Add(EScriptBrowserItemFlags::ChildContainsErrors);
		}
	}
}

CScriptBrowserModel::CScriptBrowserModel(QObject* pParent)
	: QAbstractItemModel(pParent)
{
	gEnv->pSchematyc->GetScriptRegistry().GetChangeSignalSlots().Connect(SCHEMATYC_MEMBER_DELEGATE(&CScriptBrowserModel::OnScriptRegistryChange, *this), m_connectionScope);

	Populate();
}

QModelIndex CScriptBrowserModel::index(int row, int column, const QModelIndex& parent) const
{
	CScriptBrowserItem* pParentItem = ItemFromIndex(parent);
	if (pParentItem)
	{
		CScriptBrowserItem* pChildItem = pParentItem->GetChildByIdx(row);
		return pChildItem ? QAbstractItemModel::createIndex(row, column, pChildItem) : QModelIndex();
	}
	else
	{
		return QAbstractItemModel::createIndex(row, column, m_pRootItem.get());
	}
}

QModelIndex CScriptBrowserModel::parent(const QModelIndex& index) const
{
	CScriptBrowserItem* pItem = ItemFromIndex(index);
	if (pItem)
	{
		CScriptBrowserItem* pParentItem = pItem->GetParent();
		if (pParentItem)
		{
			return ItemToIndex(pParentItem, index.column());
		}
	}
	return QModelIndex();
}

int CScriptBrowserModel::rowCount(const QModelIndex& parent) const
{
	CScriptBrowserItem* pParentItem = ItemFromIndex(parent);
	if (pParentItem)
	{
		return pParentItem->GetChildCount();
	}
	else
	{
		return 1;
	}
}

int CScriptBrowserModel::columnCount(const QModelIndex& parent) const
{
	return /*EScriptBrowserColumn::Count*/ 1; // File management will be handled by asset browser so we might as well this for now.
}

bool CScriptBrowserModel::hasChildren(const QModelIndex& parent) const
{
	CScriptBrowserItem* pParentItem = ItemFromIndex(parent);
	if (pParentItem)
	{
		return pParentItem->GetChildCount() > 0;
	}
	else
	{
		return true;
	}
}

QVariant CScriptBrowserModel::data(const QModelIndex& index, int role) const
{
	CScriptBrowserItem* pItem = ItemFromIndex(index);
	if (pItem)
	{
		switch (role)
		{
		case Qt::DisplayRole:
			{
				switch (index.column())
				{
				case EScriptBrowserColumn::Name:
					{
						QString name = pItem->GetName();
						if (pItem->GetFlags().Check(EScriptBrowserItemFlags::Modified))
						{
							name.append("*");
						}
						return name;
					}
				case EScriptBrowserColumn::File:
					{
						return QString(pItem->GetFileText());
					}
				}
				break;
			}
		case Qt::DecorationRole:
			{
				switch (index.column())
				{
				case EScriptBrowserColumn::Name:
					{
						return CryIcon(pItem->GetIcon());
					}
				case EScriptBrowserColumn::File:
					{
						return CryIcon(pItem->GetFileIcon());
					}
				}
				break;
			}
		case Qt::EditRole:
			{
				switch (index.column())
				{
				case EScriptBrowserColumn::Name:
					{
						return QString(pItem->GetName());
					}
				}
				break;
			}
		case Qt::ToolTipRole:
			{
				switch (index.column())
				{
				case EScriptBrowserColumn::Name:
					{
						return QString(pItem->GetTooltip());
					}
				case EScriptBrowserColumn::File:
					{
						return QString(pItem->GetFileToolTip());
					}
				}
				break;
			}
		case Qt::ForegroundRole:
			{
				switch (index.column())
				{
				case EScriptBrowserColumn::Name:
					{
						return pItem->GetColor();
					}
				}
				break;
			}
		case Qt::SizeHintRole:
			{
				return g_defaultColumnSize[index.column()];
			}
		}
	}
	return QVariant();
}

bool CScriptBrowserModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (role == Qt::EditRole)
	{
		CScriptBrowserItem* pItem = ItemFromIndex(index);
		if (pItem)
		{
			pItem->SetName(value.toString().toStdString().c_str());
			return true;
		}
	}
	return false;
}

QVariant CScriptBrowserModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal)
	{
		switch (role)
		{
		case Qt::DisplayRole:
			{
				switch (section)
				{
				case EScriptBrowserColumn::Name:
					{
						return QString("Name");
					}
				case EScriptBrowserColumn::File:
					{
						return QString("File");
					}
				}
				break;
			}
		case Qt::SizeHintRole:
			{
				return g_defaultColumnSize[section];
			}
		}
	}
	return QVariant();
}

Qt::ItemFlags CScriptBrowserModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags(index);
	CScriptBrowserItem* pItem = ItemFromIndex(index);
	if (pItem)
	{
		switch (pItem->GetType())
		{
		case EScriptBrowserItemType::Filter:
			{
				flags &= ~Qt::ItemIsEnabled;
				break;
			}
		case EScriptBrowserItemType::ScriptElement:
			{
				IScriptElement* pScriptElement = pItem->GetScriptElement();
				if (pScriptElement && ScriptBrowserUtils::CanRenameScriptElement(*pScriptElement))
				{
					flags |= Qt::ItemIsEditable;
				}
				break;
			}
		}
	}
	return flags;
}

void CScriptBrowserModel::Reset()
{
	m_itemsByGUID.clear();
	m_itemsByFileName.clear();

	QAbstractItemModel::beginResetModel();
	{
		Populate();
	}
	QAbstractItemModel::endResetModel();
}

QModelIndex CScriptBrowserModel::ItemToIndex(CScriptBrowserItem* pItem, int column) const
{
	if (pItem)
	{
		CScriptBrowserItem* pParentItem = pItem->GetParent();
		if (pParentItem)
		{
			return QAbstractItemModel::createIndex(pParentItem->GetChildIdx(pItem), column, pItem);
		}
		else
		{
			return QAbstractItemModel::createIndex(0, column, pItem);
		}
	}
	return QModelIndex();
}

CScriptBrowserItem* CScriptBrowserModel::ItemFromIndex(const QModelIndex& index) const
{
	return static_cast<CScriptBrowserItem*>(index.internalPointer());
}

CScriptBrowserItem* CScriptBrowserModel::GetRootItem()
{
	return m_pRootItem.get();
}

CScriptBrowserItem* CScriptBrowserModel::GetItemByGUID(const SGUID& guid)
{
	ItemsByGUID::iterator itItem = m_itemsByGUID.find(guid);
	return itItem != m_itemsByGUID.end() ? itItem->second : nullptr;
}

CScriptBrowserItem* CScriptBrowserModel::GetItemByFileName(const char* szFileName)
{
	SCHEMATYC_EDITOR_ASSERT(szFileName);
	if (szFileName)
	{
		ItemsByFileName::iterator itItem = m_itemsByFileName.find(szFileName);
		if (itItem != m_itemsByFileName.end())
		{
			return itItem->second;
		}
	}
	return nullptr;
}

void CScriptBrowserModel::Populate()
{
	m_pRootItem = std::make_shared<CScriptBrowserItem>(EScriptBrowserItemType::Root, "Root", g_szScriptRootIcon);

	auto visitScriptElement = [this](IScriptElement& scriptElement) -> EVisitStatus
	{
		const IScriptElement* pParentScriptElement = scriptElement.GetParent();
		if (pParentScriptElement)
		{
			CScriptBrowserItem* pParentItem = GetItemByGUID(pParentScriptElement->GetGUID());
			if (!pParentItem)
			{
				pParentItem = m_pRootItem.get();
			}
			if (pParentItem)
			{
				CreateScriptElementItem(scriptElement, EScriptBrowserItemFlags::None, *pParentItem);
			}
		}
		return EVisitStatus::Recurse;
	};
	gEnv->pSchematyc->GetScriptRegistry().GetRootElement().VisitChildren(ScriptElementVisitor::FromLambda(visitScriptElement));
}

void CScriptBrowserModel::OnScriptRegistryChange(const SScriptRegistryChange& change)
{
	bool bRecompile = false;
	bool bValidate = true;

	switch (change.type)
	{
	case EScriptRegistryChangeType::ElementAdded:
		{
			OnScriptElementAdded(change.element);
			bRecompile = true;
			break;
		}
	case EScriptRegistryChangeType::ElementModified:
		{
			OnScriptElementModified(change.element);
			bRecompile = true;
			break;
		}
	case EScriptRegistryChangeType::ElementRemoved:
		{
			OnScriptElementRemoved(change.element);
			bRecompile = true;
			break;
		}
	case EScriptRegistryChangeType::ElementSaved:
		{
			OnScriptElementSaved(change.element);
			bValidate = false;
			break;
		}
	}

	if (bRecompile)
	{
		gEnv->pSchematyc->GetCompiler().CompileDependencies(change.element.GetGUID());
	}

	if (bValidate)
	{
		CScriptBrowserItem* pItem = GetItemByGUID(change.element.GetGUID());
		if (pItem)
		{
			ValidateItem(*pItem);
		}
	}
}

void CScriptBrowserModel::OnScriptElementAdded(IScriptElement& scriptElement)
{
	IScriptElement* pParentScriptElement = scriptElement.GetParent();
	if (pParentScriptElement)
	{
		CScriptBrowserItem* pParentItem = GetItemByGUID(pParentScriptElement->GetGUID());
		if (!pParentItem)
		{
			pParentItem = m_pRootItem.get();
		}
		if (pParentItem)
		{
			CreateScriptElementItem(scriptElement, EScriptBrowserItemFlags::Modified, *pParentItem);
		}
	}
}

void CScriptBrowserModel::OnScriptElementModified(IScriptElement& scriptElement)
{
	CScriptBrowserItem* pItem = GetItemByGUID(scriptElement.GetGUID());
	while (pItem)
	{
		IScriptElement* pScriptElement = pItem->GetScriptElement();
		if (pScriptElement)
		{
			ScriptBrowserItemFlags itemFlags = pItem->GetFlags();
			itemFlags.Add(EScriptBrowserItemFlags::Modified);
			pItem->SetFlags(itemFlags);

			QModelIndex index = ItemToIndex(pItem);
			QAbstractItemModel::dataChanged(index, index);
		}

		if (!pScriptElement || !pScriptElement->GetScript())
		{
			pItem = pItem->GetParent();
		}
		else
		{
			break;
		}
	}
}

void CScriptBrowserModel::OnScriptElementRemoved(IScriptElement& scriptElement)
{
	ItemsByGUID::iterator itItem = m_itemsByGUID.find(scriptElement.GetGUID());
	if (itItem != m_itemsByGUID.end())
	{
		CScriptBrowserItem* pItem = itItem->second;
		CScriptBrowserItem* pParentItem = pItem->GetParent();

		RemoveItem(*pItem);
		m_itemsByGUID.erase(itItem);

		if (pParentItem && (pParentItem->GetType() == EScriptBrowserItemType::Filter) && !pParentItem->GetChildCount())
		{
			RemoveItem(*pParentItem);
		}
	}
}

void CScriptBrowserModel::OnScriptElementSaved(IScriptElement& scriptElement)
{
	ItemsByGUID::iterator itItem = m_itemsByGUID.find(scriptElement.GetGUID());
	if (itItem != m_itemsByGUID.end())
	{
		CScriptBrowserItem* pItem = itItem->second;

		ScriptBrowserItemFlags itemFlags = pItem->GetFlags();
		itemFlags.Remove(EScriptBrowserItemFlags::Modified);
		pItem->SetFlags(itemFlags);

		QModelIndex index = ItemToIndex(pItem);
		QAbstractItemModel::dataChanged(index, index);
	}
}

CScriptBrowserItem* CScriptBrowserModel::CreateScriptElementItem(IScriptElement& scriptElement, const ScriptBrowserItemFlags& flags, CScriptBrowserItem& parentItem)
{
	// Select parent/filter item.
	const EScriptElementType scriptElementType = scriptElement.GetType();
	CScriptBrowserItem* pFilterItem = nullptr;

	CScriptBrowserItem* pParentParent = parentItem.GetParent();
	EScriptElementType parentParentType = (pParentParent) ? pParentParent->GetFilter() : EScriptElementType::Root;

	if (scriptElement.GetType() != EScriptElementType::Class)
	{
		if (parentItem.GetType() != EScriptBrowserItemType::ScriptElement
			|| parentParentType == EScriptElementType::Class
			|| parentParentType == EScriptElementType::Root
			|| parentParentType == EScriptElementType::None
			) //we don't want sub-filters on other levels
		{
			const char* szFilter = ScriptBrowserUtils::GetScriptElementFilterName(scriptElementType);
			if (szFilter && szFilter[0])
			{
				pFilterItem = parentItem.GetChildByTypeAndName(EScriptBrowserItemType::Filter, szFilter);
				if (!pFilterItem)
				{
					pFilterItem = AddItem(parentItem, std::make_shared<CScriptBrowserItem>(EScriptBrowserItemType::Filter, szFilter, scriptElementType));

					const uint32 childCount = parentItem.GetChildCount();

					if (childCount > 0)
					{
						std::vector<CScriptBrowserItem*> childItems;
						childItems.reserve(childCount);
						for (uint32 childIdx = 0; childIdx < childCount; ++childIdx)
						{
							CScriptBrowserItem* pChildItem = parentItem.GetChildByIdx(childIdx);
							const IScriptElement* pChildScriptElement = pChildItem->GetScriptElement();
							if (pChildScriptElement && (pChildScriptElement->GetType() == scriptElementType))
							{
								childItems.push_back(pChildItem);
							}
						}

						if (!childItems.empty())
						{
							for (CScriptBrowserItem* pChildItem : childItems)
							{
								AddItem(*pFilterItem, RemoveItem(*pChildItem));
							}
						}
					}
				}
			}
		}
	}

	// Create item.
	const SGUID guid = scriptElement.GetGUID();
	CScriptBrowserItemPtr pItem = std::make_shared<CScriptBrowserItem>(EScriptBrowserItemType::ScriptElement, scriptElement.GetName(), ScriptBrowserUtils::GetScriptElementIcon(scriptElement), &scriptElement);
	pItem->SetFlags(flags);
	if (pFilterItem)
	{
		AddItem(*pFilterItem, pItem);
	}
	else
	{
		AddItem(parentItem, pItem);
	}
	m_itemsByGUID.insert(ItemsByGUID::value_type(guid, pItem.get()));

	// Set initial state of item.
	ValidateItem(*pItem);
	IScript* pScript = scriptElement.GetScript();
	if (pScript)
	{
		CStackString fileName;
		CrySchematycEditor::Utils::ConstructAbsolutePath(fileName, pScript->GetName());
		m_itemsByFileName.insert(ItemsByFileName::value_type(fileName.c_str(), pItem.get()));
	}

	return pItem.get();
}

CScriptBrowserItem* CScriptBrowserModel::AddItem(CScriptBrowserItem& parentItem, const CScriptBrowserItemPtr& pItem)
{
	const QModelIndex parentIndex = ItemToIndex(&parentItem);
	const int row = parentItem.GetChildCount();
	QAbstractItemModel::beginInsertRows(parentIndex, row, row);
	parentItem.AddChild(pItem);
	QAbstractItemModel::endInsertRows();
	QAbstractItemModel::dataChanged(parentIndex, parentIndex);
	return pItem.get();
}

CScriptBrowserItemPtr CScriptBrowserModel::RemoveItem(CScriptBrowserItem& item)
{
	CScriptBrowserItemPtr pResult;
	CScriptBrowserItem* pParentItem = item.GetParent();
	if (pParentItem)
	{
		QModelIndex index = ItemToIndex(&item);
		QAbstractItemModel::beginRemoveRows(index.parent(), index.row(), index.row());
		pResult = pParentItem->RemoveChild(&item);
		QAbstractItemModel::endRemoveRows();

		ValidateItem(*pParentItem);
	}
	return pResult;
}

void CScriptBrowserModel::ValidateItem(CScriptBrowserItem& item)
{
	ScriptBrowserItemFlags prevFlags = item.GetFlags();

	item.Validate();

	if (item.GetFlags() != prevFlags)
	{
		const QModelIndex itemIndex = ItemToIndex(&item);
		QAbstractItemModel::dataChanged(itemIndex, itemIndex);

		for (CScriptBrowserItem* pParentItem = item.GetParent(); pParentItem; pParentItem = pParentItem->GetParent())
		{
			prevFlags = pParentItem->GetFlags();

			pParentItem->RefreshChildFlags();

			if (pParentItem->GetFlags() != prevFlags)
			{
				const QModelIndex parentItemIndex = ItemToIndex(pParentItem);
				QAbstractItemModel::dataChanged(parentItemIndex, parentItemIndex);
			}
			else
			{
				break;
			}
		}
	}
}

CScriptBrowserTreeView::CScriptBrowserTreeView(QWidget* pParent)
	: QTreeView(pParent)
{}

void CScriptBrowserTreeView::keyPressEvent(QKeyEvent* pKeyEvent)
{
	bool bEventHandled = false;
	keyPress(pKeyEvent, bEventHandled);
	if (!bEventHandled)
	{
		QTreeView::keyPressEvent(pKeyEvent);
	}
}

SScriptBrowserSelection::SScriptBrowserSelection(IScriptElement* _pScriptElement)
	: pScriptElement(_pScriptElement)
{}

CScriptBrowserWidget::CScriptBrowserWidget(QWidget* pParent)
	: QWidget(pParent)
	, m_pContextMenu(nullptr)
{
	m_pMainLayout = new QBoxLayout(QBoxLayout::TopToBottom);
	m_pHeaderLayout = new QBoxLayout(QBoxLayout::LeftToRight);
	m_pAddButton = new QPushButton(CryIcon(g_szAddIcon), "Add", this);
	m_pAddMenu = new QMenu("Add", this);
	m_pTreeView = new CScriptBrowserTreeView(this);
	m_pModel = new CScriptBrowserModel(this);
	m_pFilter = new CScriptBrowserFilter(this, *m_pModel);

	m_pSearchFilter = new QLineEdit(this);
	m_pSearchFilter->setPlaceholderText("Search");   // #SchematycTODO : Add drop down history to filter text?

	m_pAddButton->setMenu(m_pAddMenu);

	m_pFilter->setDynamicSortFilter(true);
	m_pFilter->setSourceModel(m_pModel);
	m_pTreeView->setModel(m_pFilter);

	m_pTreeView->sortByColumn(0, Qt::AscendingOrder);
	m_pTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
	m_pTreeView->setEditTriggers(QAbstractItemView::SelectedClicked);
	m_pTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
	m_pTreeView->setExpandsOnDoubleClick(false);

	m_pTreeView->selectionModel()->setCurrentIndex(TreeViewFromModelIndex(m_pModel->ItemToIndex(m_pModel->GetRootItem())), QItemSelectionModel::ClearAndSelect);

	connect(m_pSearchFilter, SIGNAL(textChanged(const QString &)), this, SLOT(OnSearchFilterChanged(const QString &)));
	connect(m_pAddMenu, SIGNAL(aboutToShow()), this, SLOT(OnAddMenuShow()));
	connect(m_pTreeView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(OnTreeViewDoubleClicked(const QModelIndex &)));
	connect(m_pTreeView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(OnTreeViewCustomContextMenuRequested(const QPoint &)));
	connect(m_pTreeView, SIGNAL(keyPress(QKeyEvent*, bool&)), this, SLOT(OnTreeViewKeyPress(QKeyEvent*, bool&)));
	connect(m_pTreeView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(OnSelectionChanged(const QItemSelection &, const QItemSelection &)));

	QWidget::startTimer(500);
}

CScriptBrowserWidget::~CScriptBrowserWidget()
{
	m_pTreeView->setModel(nullptr);
}

void CScriptBrowserWidget::InitLayout()
{
	QWidget::setLayout(m_pMainLayout);
	m_pMainLayout->setSpacing(1);
	m_pMainLayout->setMargin(1);

	m_pHeaderLayout->addWidget(m_pSearchFilter), 1;
	m_pHeaderLayout->addWidget(m_pAddButton), 1;

	m_pMainLayout->addLayout(m_pHeaderLayout, 1);
	m_pMainLayout->addWidget(m_pTreeView, 1);

	/*for (int column = 0; column < EScriptBrowserColumn::Count; ++column)
	   {
	   m_pTreeView->header()->resizeSection(column, g_defaultColumnSize[column].width());
	   }*/
}

void CScriptBrowserWidget::Reset()
{
	m_pModel->Reset();
}

void CScriptBrowserWidget::SelectItem(const SGUID& guid)
{
	CScriptBrowserItem* pItem = m_pModel->GetItemByGUID(guid);
	if (pItem)
	{
		const QModelIndex itemIndex = TreeViewFromModelIndex(m_pModel->ItemToIndex(pItem));
		m_pTreeView->selectionModel()->setCurrentIndex(itemIndex, QItemSelectionModel::ClearAndSelect);
	}
}

bool CScriptBrowserWidget::SetScope(bool classOnly)
{
	if (classOnly)
	{
		CScriptBrowserItem* pItem = m_pModel->ItemFromIndex(TreeViewToModelIndex(GetTreeViewSelection()));
		if (pItem)
		{
			while (pItem->GetType() != EScriptBrowserItemType::Root)
			{
				Schematyc::IScriptElement* pScriptElement = pItem->GetScriptElement();
				if (pScriptElement)
				{
					const Schematyc::EScriptElementType type = pScriptElement->GetType();
					if (type != Schematyc::EScriptElementType::Class)
					{
						pItem = pItem->GetParent();
						CRY_ASSERT(pItem);
						if (pItem == nullptr)
						{
							return false;
						}
						continue;
					}

					pItem = pItem->GetParent();
					if (pItem)
					{
						const QModelIndex index = TreeViewFromModelIndex(m_pModel->ItemToIndex(pItem));
						if (index.isValid())
						{
							m_pTreeView->setRootIndex(index);

							m_pFilter->SetScope(index);
							m_pFilter->invalidate();
							return true;
						}
					}
				}
				else
				{
					pItem = pItem->GetParent();
				}
			}
		}
	}
	else
	{
		CScriptBrowserItem* pRootItem = m_pModel->GetRootItem();
		if (pRootItem)
		{
			const QModelIndex index = TreeViewFromModelIndex(m_pModel->ItemToIndex(pRootItem));
			if (index.isValid())
			{
				m_pTreeView->setRootIndex(index);

				m_pFilter->SetScope(index);
				m_pFilter->invalidate();
				return true;
			}
		}
	}

	return false;
}

void CScriptBrowserWidget::Serialize(Serialization::IArchive& archive)
{
	QByteArray geometry;
	if (archive.isOutput())
	{
		geometry = QWidget::saveGeometry();
	}
	archive(geometry, "geometry");
	if (archive.isInput() && !geometry.isEmpty())
	{
		QWidget::restoreGeometry(geometry);
	}
}

ScriptBrowserSelectionSignal::Slots& CScriptBrowserWidget::GetSelectionSignalSlots()
{
	return m_signals.selection.GetSlots();
}

bool CScriptBrowserWidget::HasUnsavedScriptElements() const
{
	std::function<bool(CScriptBrowserItem*)> hasUnsavedChanges;
	hasUnsavedChanges = [&hasUnsavedChanges](CScriptBrowserItem* pItem) -> bool
	{
		if (pItem && !pItem->GetFlags().Check(EScriptBrowserItemFlags::Modified))
		{
			for (uint32 i = pItem->GetChildCount(); i--; )
			{
				if (hasUnsavedChanges(pItem->GetChildByIdx(i)))
					return true;
			}
			return false;
		}
		return true;
	};

	return hasUnsavedChanges(m_pModel->GetRootItem());
}

void CScriptBrowserWidget::OnSearchFilterChanged(const QString& text)
{
	if (m_pSearchFilter)
	{
		m_pFilter->SetFilterText(text.toStdString().c_str());
		m_pFilter->invalidate();
		ExpandAll();
	}
}

void CScriptBrowserWidget::OnAddMenuShow()
{
	CScriptBrowserItem* pItem = m_pModel->ItemFromIndex(TreeViewToModelIndex(GetTreeViewSelection()));
	RefreshAddMenu(pItem);
}

void CScriptBrowserWidget::OnTreeViewDoubleClicked(const QModelIndex& index) {}

void CScriptBrowserWidget::OnTreeViewCustomContextMenuRequested(const QPoint& position)
{
	QModelIndex index = m_pTreeView->indexAt(position);
	CScriptBrowserItem* pItem = m_pModel->ItemFromIndex(TreeViewToModelIndex(index));
	if (pItem)
	{
		QMenu contextMenu;
		m_pContextMenu = &contextMenu;

		switch (index.column())
		{
		case EScriptBrowserColumn::Name:
			{
				IScriptElement* pScriptElement = pItem->GetScriptElement();
				if (pScriptElement)
				{
					/*{
					   QAction* pScopeToThisAction = contextMenu.addAction(QString("Scope To This"));
					   connect(pScopeToThisAction, SIGNAL(triggered()), this, SLOT(OnScopeToThis()));
					   }*/

					/*{
						QAction* pFindReferencesAction = contextMenu.addAction(QString("Find references"));
						connect(pFindReferencesAction, SIGNAL(triggered()), this, SLOT(OnFindReferences()));
					}*/

					//contextMenu.addSeparator();

					if (!m_pAddMenu->isEmpty())
					{
						contextMenu.addMenu(m_pAddMenu);
					}

					/*if(ScriptBrowserUtils::CanCopyScriptElement(*pScriptElement))
					{
						QAction* pCopyAction = contextMenu.addAction(QString("Copy"));
						connect(pCopyAction, SIGNAL(triggered()), this, SLOT(OnCopyItem()));
					}*/

					// #SchematycTODO : Verify clipboard contents.
					/*{
						QAction* pPasteAction = contextMenu.addAction(QString("Paste"));
						pPasteAction->setEnabled(CrySchematycEditor::Utils::ValidateClipboardContents(g_szClipboardPrefix));
						connect(pPasteAction, SIGNAL(triggered()), this, SLOT(OnPasteItem()));
					}*/

					if (ScriptBrowserUtils::CanRemoveScriptElement(*pScriptElement))
					{
						QAction* pRemoveAction = contextMenu.addAction(QString("Remove"));
						connect(pRemoveAction, SIGNAL(triggered()), this, SLOT(OnRemoveItem()));
					}

					if (ScriptBrowserUtils::CanRenameScriptElement(*pScriptElement))
					{
						QAction* pRenameAction = contextMenu.addAction(QString("Rename"));
						connect(pRenameAction, SIGNAL(triggered()), this, SLOT(OnRenameItem()));
					}
				}
				else
				{
					if (!m_pAddMenu->isEmpty())
					{
						contextMenu.addMenu(m_pAddMenu);
					}
				}

				break;
			}
		case EScriptBrowserColumn::File:
			{
				const ScriptBrowserItemFlags itemFlags = pItem->GetFlags();
				if (itemFlags.Check(EScriptBrowserItemFlags::FileLocal))
				{
					if (!itemFlags.Check(EScriptBrowserItemFlags::FileReadOnly))
					{
						QAction* pSaveAction = contextMenu.addAction(QString("Save"));
						connect(pSaveAction, SIGNAL(triggered()), this, SLOT(OnSave()));
					}
					contextMenu.addSeparator();
				}
				else if (itemFlags.Check(EScriptBrowserItemFlags::FileInPak))
				{
					QAction* pExtractAction = contextMenu.addAction(QString("Extract"));
					connect(pExtractAction, SIGNAL(triggered()), this, SLOT(OnExtract()));
				}

				break;
			}
		}

		const QPoint globalPosition = m_pTreeView->viewport()->mapToGlobal(position);
		contextMenu.exec(globalPosition);

		m_pContextMenu = nullptr;
	}
}

void CScriptBrowserWidget::OnTreeViewKeyPress(QKeyEvent* pKeyEvent, bool& bEventHandled)
{
	bEventHandled = HandleKeyPress(pKeyEvent);
}

void CScriptBrowserWidget::OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	CScriptBrowserItem* pItem = m_pModel->ItemFromIndex(TreeViewToModelIndex(GetTreeViewSelection()));
	if (pItem)
	{
		RefreshAddMenu(pItem); // #SchematycTODO : Decide where best to call RefreshAddMenu(), either on 'about to show' or 'selection changed'.

		m_pAddButton->setEnabled(!m_pAddMenu->isEmpty());

		m_signals.selection.Send(SScriptBrowserSelection(pItem ? pItem->GetScriptElement() : nullptr));
	}
}

void CScriptBrowserWidget::OnScopeToThis()
{
	//SetScope(TreeViewToModelIndex(GetTreeViewSelection()));
}

void CScriptBrowserWidget::OnFindReferences()
{
	CScriptBrowserItem* pItem = m_pModel->ItemFromIndex(TreeViewToModelIndex(GetTreeViewSelection()));
	if (pItem)
	{
		IScriptElement* pScriptElement = pItem->GetScriptElement();
		if (pScriptElement)
		{
			ScriptBrowserUtils::FindReferences(*pScriptElement);
		}
	}
}

void CScriptBrowserWidget::OnAddItem()
{
	QAction* pAddItemAction = static_cast<QAction*>(sender());
	if (pAddItemAction)
	{
		CScriptBrowserItem* pItem = m_pModel->ItemFromIndex(TreeViewToModelIndex(GetTreeViewSelection()));
		if (pItem)
		{
			switch (pItem->GetType())
			{
			case EScriptBrowserItemType::Root:
			case EScriptBrowserItemType::ScriptElement:
				{
					IScriptRegistry& scriptRegistry = gEnv->pSchematyc->GetScriptRegistry();
					IScriptElement* pScriptScope = pItem->GetScriptElement();
					if (!pScriptScope)
					{
						pScriptScope = &scriptRegistry.GetRootElement();
					}

					IScriptElement* pScriptElement = nullptr;
					bool bRenameElement = false; // #SchematycTODO : Rather than deciding whether to rename based on element type we should probably introduce a 'EScriptElementFlags::IsRenameable' flag.
					// #SchematycTODO : Move this to ScriptBrowserUtils.h?
					switch (static_cast<EScriptElementType>(pAddItemAction->data().toInt()))
					{
					case EScriptElementType::Module:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptModule(pScriptScope);
							bRenameElement = true;
							break;
						}
					case EScriptElementType::Enum:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptEnum(pScriptScope);
							bRenameElement = true;
							break;
						}
					case EScriptElementType::Struct:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptStruct(pScriptScope);
							bRenameElement = true;
							break;
						}
					case EScriptElementType::Signal:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptSignal(pScriptScope);
							bRenameElement = true;
							break;
						}
					case EScriptElementType::Function:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptFunction(pScriptScope);
							bRenameElement = true;
							break;
						}
					case EScriptElementType::Interface:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptInterface(pScriptScope);
							bRenameElement = true;
							break;
						}
					case EScriptElementType::InterfaceFunction:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptInterfaceFunction(pScriptScope);
							bRenameElement = true;
							break;
						}
					case EScriptElementType::InterfaceTask:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptInterfaceTask(pScriptScope);
							bRenameElement = true;
							break;
						}
					case EScriptElementType::Class:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptClass(pScriptScope);
							bRenameElement = true;
							break;
						}
					case EScriptElementType::StateMachine:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptStateMachine(pScriptScope);
							bRenameElement = true;
							break;
						}
					case EScriptElementType::State:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptState(pScriptScope);
							bRenameElement = true;
							break;
						}
					case EScriptElementType::Variable:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptVariable(pScriptScope);
							bRenameElement = true;
							break;
						}
					case EScriptElementType::Timer:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptTimer(pScriptScope);
							bRenameElement = true;
							break;
						}
					case EScriptElementType::SignalReceiver:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptSignalReceiver(pScriptScope);
							bRenameElement = true;
							break;
						}
					case EScriptElementType::InterfaceImpl:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptInterfaceImpl(pScriptScope);
							bRenameElement = false;
							break;
						}
					case EScriptElementType::ComponentInstance:
						{
							if (m_pContextMenu)
							{
								QPoint pos = m_pContextMenu->pos();
								pScriptElement = ScriptBrowserUtils::AddScriptComponentInstance(pScriptScope, &pos);
							}
							else if (m_pAddMenu)
							{
								QPoint pos = m_pAddMenu->pos();
								pScriptElement = ScriptBrowserUtils::AddScriptComponentInstance(pScriptScope, &pos);
							}
							else
							{
								pScriptElement = ScriptBrowserUtils::AddScriptComponentInstance(pScriptScope);
							}
							bRenameElement = true;
							break;
						}
					case EScriptElementType::ActionInstance:
						{
							pScriptElement = ScriptBrowserUtils::AddScriptActionInstance(pScriptScope);
							bRenameElement = true;
							break;
						}
					}
					if (pScriptElement)
					{
						CScriptBrowserItem* pItem = m_pModel->GetItemByGUID(pScriptElement->GetGUID());
						if (pItem)
						{
							const QModelIndex itemIndex = TreeViewFromModelIndex(m_pModel->ItemToIndex(pItem));
							m_pTreeView->selectionModel()->setCurrentIndex(itemIndex, QItemSelectionModel::ClearAndSelect);
							if (bRenameElement)
							{
								m_pTreeView->edit(itemIndex);
							}
						}
					}
					break;
				}
			}
		}
	}
}

void CScriptBrowserWidget::OnCopyItem()
{
	CScriptBrowserItem* pItem = m_pModel->ItemFromIndex(TreeViewToModelIndex(GetTreeViewSelection()));
	if (pItem)
	{
		IScriptElement* pScriptElement = pItem->GetScriptElement();
		if (pScriptElement)
		{
			XmlNodeRef xml;
			if (gEnv->pSchematyc->GetScriptRegistry().CopyElementsToXml(xml, *pScriptElement))
			{
				CrySchematycEditor::Utils::WriteToClipboard(xml->getXMLData()->GetString(), g_szClipboardPrefix);
			}
		}
	}
}

void CScriptBrowserWidget::OnPasteItem()
{
	CScriptBrowserItem* pItem = m_pModel->ItemFromIndex(TreeViewToModelIndex(GetTreeViewSelection()));
	if (pItem)
	{
		IScriptElement* pScriptElement = pItem->GetScriptElement();
		if (pScriptElement)
		{
			string clipboardText;
			if (CrySchematycEditor::Utils::ReadFromClipboard(clipboardText, g_szClipboardPrefix))
			{
				XmlNodeRef xml = gEnv->pSystem->LoadXmlFromBuffer(clipboardText.c_str(), clipboardText.length());
				if (xml)
				{
					gEnv->pSchematyc->GetScriptRegistry().PasteElementsFromXml(xml, pScriptElement);
				}
			}
		}
	}
}

void CScriptBrowserWidget::OnRemoveItem()
{
	CScriptBrowserItem* pItem = m_pModel->ItemFromIndex(TreeViewToModelIndex(GetTreeViewSelection()));
	if (pItem)
	{
		IScriptElement* pScriptElement = pItem->GetScriptElement();
		if (pScriptElement)
		{
			ScriptBrowserUtils::RemoveScriptElement(*pScriptElement);
		}
	}
}

void CScriptBrowserWidget::OnRenameItem()
{
	m_pTreeView->edit(GetTreeViewSelection());
}

void CScriptBrowserWidget::OnSave()
{
	// #SchematycTODO : Implement this function!!!
}

void CScriptBrowserWidget::OnExtract()
{
	// #SchematycTODO : Implement this function!!!
}

void CScriptBrowserWidget::keyPressEvent(QKeyEvent* pKeyEvent)
{
	HandleKeyPress(pKeyEvent);
}

void CScriptBrowserWidget::ExpandAll()
{
	m_pTreeView->expandAll();
}

void CScriptBrowserWidget::RefreshAddMenu(CScriptBrowserItem* pItem)
{
	m_pAddMenu->clear();

	if (pItem)
	{
		switch (pItem->GetType())
		{
		case EScriptBrowserItemType::Root:
			{
				//QAction* pAddModuleAction = m_pAddMenu->addAction("Module");
				//pAddModuleAction->setData(static_cast<int>(EScriptElementType::Module));
				//connect(pAddModuleAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
				//break;
			}  //Intended falls through now:
		case EScriptBrowserItemType::ScriptElement:
			{
				IScriptElement* pScriptScope = pItem->GetScriptElement();
				EScriptElementType elementType = (pScriptScope) ? pScriptScope->GetType() : EScriptElementType::Root;
				switch (elementType)
				{
				case EScriptElementType::Root:
				case EScriptElementType::Module:
				case EScriptElementType::Interface:
				case EScriptElementType::Class:
				case EScriptElementType::StateMachine:
				case EScriptElementType::State:
				case EScriptElementType::ComponentInstance:
					{
						// #SchematycTODO : Iterate through possible types rather than explicitly adding each action for each type?
						if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::Module, pScriptScope))
						{
							QAction* pAddModuleAction = m_pAddMenu->addAction("Folder");
							pAddModuleAction->setData(static_cast<int>(EScriptElementType::Module));
							connect(pAddModuleAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						}
						/*if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::Import, pScriptScope))
						   {
						   QAction* pAddImportAction = m_pAddMenu->addAction("Import");
						   pAddImportAction->setData(static_cast<int>(EScriptElementType::Import));
						   connect(pAddImportAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						   }*/
						if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::Enum, pScriptScope))
						{
							QAction* pAddEnumAction = m_pAddMenu->addAction("Enumeration");
							pAddEnumAction->setData(static_cast<int>(EScriptElementType::Enum));
							connect(pAddEnumAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						}
						/*if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::Struct, pScriptScope))
						   {
						   QAction* pAddStructAction = m_pAddMenu->addAction("Structure");
						   pAddStructAction->setData(static_cast<int>(EScriptElementType::Struct));
						   connect(pAddStructAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						   }*/
						if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::Signal, pScriptScope))
						{
							QAction* pAddSignalAction = m_pAddMenu->addAction("Signal");
							pAddSignalAction->setData(static_cast<int>(EScriptElementType::Signal));
							connect(pAddSignalAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						}
						if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::Function, pScriptScope))
						{
							QAction* pAddFunctionAction = m_pAddMenu->addAction("Function");
							pAddFunctionAction->setData(static_cast<int>(EScriptElementType::Function));
							connect(pAddFunctionAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						}
						/*if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::Interface, pScriptScope))
						   {
						   QAction* pAddInterfaceAction = m_pAddMenu->addAction("Interface");
						   pAddInterfaceAction->setData(static_cast<int>(EScriptElementType::Interface));
						   connect(pAddInterfaceAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						   }*/
						/*if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::InterfaceFunction, pScriptScope))
						   {
						   QAction* pAddInterfaceFunction = m_pAddMenu->addAction("Function");
						   pAddInterfaceFunction->setData(static_cast<int>(EScriptElementType::InterfaceFunction));
						   connect(pAddInterfaceFunction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						   }*/
						/*if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::InterfaceTask, pScriptScope))
						   {
						   QAction* pAddInterfaceTask = m_pAddMenu->addAction("Task");
						   pAddInterfaceTask->setData(static_cast<int>(EScriptElementType::InterfaceTask));
						   connect(pAddInterfaceTask, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						   }*/
						if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::Class, pScriptScope))
						{
							QAction* pAddClassAction = m_pAddMenu->addAction("Class");
							pAddClassAction->setData(static_cast<int>(EScriptElementType::Class));
							connect(pAddClassAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						}
						if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::StateMachine, pScriptScope))
						{
							QAction* pAddStateMachineAction = m_pAddMenu->addAction("State Machine");
							pAddStateMachineAction->setData(static_cast<int>(EScriptElementType::StateMachine));
							connect(pAddStateMachineAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						}
						if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::State, pScriptScope))
						{
							QAction* pAddStateAction = m_pAddMenu->addAction("State");
							pAddStateAction->setData(static_cast<int>(EScriptElementType::State));
							connect(pAddStateAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						}
						if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::Variable, pScriptScope))
						{
							QAction* pAddVariableAction = m_pAddMenu->addAction("Variable");
							pAddVariableAction->setData(static_cast<int>(EScriptElementType::Variable));
							connect(pAddVariableAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						}
						//if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::Property, pScriptScope))
						//{
						//	QAction* pAddPropertyAction = m_pAddMenu->addAction("Property");
						//	pAddPropertyAction->setData(static_cast<int>(EScriptElementType::Property));
						//	connect(pAddPropertyAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						//}
						if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::Timer, pScriptScope))
						{
							QAction* pAddTimerAction = m_pAddMenu->addAction("Timer");
							pAddTimerAction->setData(static_cast<int>(EScriptElementType::Timer));
							connect(pAddTimerAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						}
						if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::SignalReceiver, pScriptScope))
						{
							QAction* pAddSignalReceiverAction = m_pAddMenu->addAction("Signal Receiver");
							pAddSignalReceiverAction->setData(static_cast<int>(EScriptElementType::SignalReceiver));
							connect(pAddSignalReceiverAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						}
						/*if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::InterfaceImpl, pScriptScope))
						   {
						   QAction* pAddInterfaceImplAction = m_pAddMenu->addAction("Interface");
						   pAddInterfaceImplAction->setData(static_cast<int>(EScriptElementType::InterfaceImpl));
						   connect(pAddInterfaceImplAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						   }*/
						if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::ComponentInstance, pScriptScope))
						{
							QAction* pAddComponentAction = m_pAddMenu->addAction("Component");
							pAddComponentAction->setData(static_cast<int>(EScriptElementType::ComponentInstance));
							connect(pAddComponentAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						}
						/*if (ScriptBrowserUtils::CanAddScriptElement(EScriptElementType::ActionInstance, pScriptScope))
						   {
						   QAction* pAddActionAction = m_pAddMenu->addAction("Action");
						   pAddActionAction->setData(static_cast<int>(EScriptElementType::ActionInstance));
						   connect(pAddActionAction, SIGNAL(triggered()), this, SLOT(OnAddItem()));
						   }*/
						break;
					}
				}
				break;
			}
		}
	}
}

bool CScriptBrowserWidget::HandleKeyPress(QKeyEvent* pKeyEvent)
{
	if (pKeyEvent->modifiers() == Qt::CTRL)
	{
		if (pKeyEvent->key() == Qt::Key_A)
		{
			// #SchematycTODO : Make sure selected item is a script element?
			m_pAddButton->click();
			return true;
		}
		else if (pKeyEvent->key() == Qt::Key_S)  // #SchematycTODO : Is this really the best shortcut key?
		{
			m_pSearchFilter->setFocus();
			return true;
		}
	}
	return false;
}

QModelIndex CScriptBrowserWidget::GetTreeViewSelection() const
{
	const QModelIndexList& indices = m_pTreeView->selectionModel()->selection().indexes();
	if (indices.size())
	{
		return indices.front();
	}
	return QModelIndex();
}

QModelIndex CScriptBrowserWidget::TreeViewToModelIndex(const QModelIndex& index) const
{
	return m_pFilter->mapToSource(index);
}

QModelIndex CScriptBrowserWidget::TreeViewFromModelIndex(const QModelIndex& index) const
{
	return m_pFilter->mapFromSource(index);
}
} // Schematyc
