// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "MiddlewareDataModel.h"
#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"
#include "ModelUtils.h"

#include <IEditorImpl.h>
#include <ImplItem.h>
#include <CrySystem/File/CryFile.h>
#include <CrySandbox/CrySignal.h>
#include <CryIcon.h>

#include <DragDrop.h>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CMiddlewareDataModel::CMiddlewareDataModel(QObject* const pParent)
	: QAbstractItemModel(pParent)
	, m_pEditorImpl(CAudioControlsEditorPlugin::GetImplEditor())
{
	ConnectSignals();
}

//////////////////////////////////////////////////////////////////////////
CMiddlewareDataModel::~CMiddlewareDataModel()
{
	DisconnectSignals();
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataModel::ConnectSignals()
{
	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationAboutToChange.Connect([&]()
	{
		beginResetModel();
		m_pEditorImpl = nullptr;
		endResetModel();
	}, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationChanged.Connect([&]()
	{
		m_pEditorImpl = CAudioControlsEditorPlugin::GetImplEditor();
		beginResetModel();
		endResetModel();
	}, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataModel::DisconnectSignals()
{
	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationAboutToChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataModel::Reset()
{
	beginResetModel();
	endResetModel();
}

//////////////////////////////////////////////////////////////////////////
CItemModelAttribute* CMiddlewareDataModel::GetAttributeForColumn(EColumns const column)
{
	CItemModelAttribute* pAttribute = nullptr;

	switch (column)
	{
	case EColumns::Notification:
		pAttribute = &ModelUtils::s_notificationAttribute;
		break;
	case EColumns::Connected:
		pAttribute = &ModelUtils::s_connectedAttribute;
		break;
	case EColumns::Localized:
		pAttribute = &ModelUtils::s_localizedAttribute;
		break;
	case EColumns::Name:
		pAttribute = &Attributes::s_nameAttribute;
		break;
	default:
		pAttribute = nullptr;
		break;
	}

	return pAttribute;
}

//////////////////////////////////////////////////////////////////////////
QVariant CMiddlewareDataModel::GetHeaderData(int const section, Qt::Orientation const orientation, int const role)
{
	QVariant variant = QVariant();

	if (orientation == Qt::Horizontal)
	{
		auto const pAttribute = GetAttributeForColumn(static_cast<EColumns>(section));

		if (pAttribute != nullptr)
		{
			switch (role)
			{
			case Qt::DecorationRole:
				if (section == static_cast<int>(EColumns::Notification))
				{
					variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::NotificationHeader));
				}
				break;
			case Qt::DisplayRole:
				// For Notification we use an icons instead.
				if (section == static_cast<int>(EColumns::Name))
				{
					variant = pAttribute->GetName();
				}
				break;
			case Qt::ToolTipRole:
				variant = pAttribute->GetName();
				break;
			case Attributes::s_getAttributeRole:
				variant = QVariant::fromValue(pAttribute);
				break;
			default:
				break;
			}
		}
	}

	return variant;
}

//////////////////////////////////////////////////////////////////////////
int CMiddlewareDataModel::rowCount(QModelIndex const& parent) const
{
	int rowCount = 0;

	if (m_pEditorImpl != nullptr)
	{
		CImplItem const* pImplItem = ItemFromIndex(parent);

		if (pImplItem == nullptr)
		{
			// If not valid it must be a top level item, so get root.
			pImplItem = m_pEditorImpl->GetRoot();
		}

		if (pImplItem != nullptr)
		{
			rowCount =  pImplItem->ChildCount();
		}
	}

	return rowCount;
}

//////////////////////////////////////////////////////////////////////////
int CMiddlewareDataModel::columnCount(QModelIndex const& parent) const
{
	return static_cast<int>(EColumns::Count);
}

//////////////////////////////////////////////////////////////////////////
QVariant CMiddlewareDataModel::data(QModelIndex const& index, int role) const
{
	QVariant variant = QVariant();

	if (m_pEditorImpl != nullptr)
	{
		if (index.isValid())
		{
			CImplItem const* const pImplItem = ItemFromIndex(index);

			if (pImplItem != nullptr)
			{
				switch (index.column())
				{
				case static_cast<int>(EColumns::Notification):
					{
						switch (role)
						{
						case Qt::DecorationRole:
							if (!pImplItem->IsConnected() && !pImplItem->IsContainer())
							{
								variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::NoConnection));
							}
							else if (pImplItem->IsLocalised())
							{
								variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::Localized));
							}
							break;
						case Qt::ToolTipRole:
							if (!pImplItem->IsConnected() && !pImplItem->IsContainer())
							{
								variant = tr("Item is not connected to any audio system control");
							}
							else if (pImplItem->IsLocalised())
							{
								variant = tr("Item is localized");
							}
							break;
						case static_cast<int>(ERoles::Id):
							variant = pImplItem->GetId();
							break;
						default:
							break;
						}
					}
					break;
				case static_cast<int>(EColumns::Connected):
					if ((role == Qt::CheckStateRole) && !pImplItem->IsContainer())
					{
						variant = pImplItem->IsConnected() ? Qt::Checked : Qt::Unchecked;
					}
					break;
				case static_cast<int>(EColumns::Localized):
					if ((role == Qt::CheckStateRole) && !pImplItem->IsContainer())
					{
						variant = pImplItem->IsLocalised() ? Qt::Checked : Qt::Unchecked;
					}
					break;
				case static_cast<int>(EColumns::Name):
					{
						switch (role)
						{
						case Qt::DecorationRole:
							variant = CryIcon(m_pEditorImpl->GetTypeIcon(pImplItem));
							break;
						case Qt::DisplayRole:
						case Qt::ToolTipRole:
						case static_cast<int>(ERoles::Name):
							variant = static_cast<char const*>(pImplItem->GetName());
							break;
						case static_cast<int>(ERoles::Id):
							variant = pImplItem->GetId();
							break;
						case static_cast<int>(ERoles::ItemType):
							variant = pImplItem->GetType();
							break;
						case static_cast<int>(ERoles::IsPlaceholder):
							variant = pImplItem->IsPlaceholder();
							break;
						default:
							break;
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}

	return variant;
}

//////////////////////////////////////////////////////////////////////////
QVariant CMiddlewareDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return GetHeaderData(section, orientation, role);
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CMiddlewareDataModel::flags(QModelIndex const& index) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags(index);

	if (index.isValid() && (m_pEditorImpl != nullptr))
	{
		CImplItem const* const pImplItem = ItemFromIndex(index);

		if ((pImplItem != nullptr) && !pImplItem->IsPlaceholder() && (m_pEditorImpl->ImplTypeToSystemType(pImplItem) != ESystemItemType::NumTypes))
		{
			flags |= Qt::ItemIsDragEnabled;
		}
	}

	return flags;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CMiddlewareDataModel::index(int row, int column, QModelIndex const& parent /*= QModelIndex()*/) const
{
	QModelIndex modelIndex = QModelIndex();

	if (m_pEditorImpl != nullptr)
	{
		if ((row >= 0) && (column >= 0))
		{
			CImplItem const* pParent = ItemFromIndex(parent);

			if (pParent == nullptr)
			{
				pParent = m_pEditorImpl->GetRoot();
			}

			if ((pParent != nullptr) && pParent->ChildCount() > row)
			{
				CImplItem const* const pImplItem = pParent->GetChildAt(row);

				if (pImplItem != nullptr)
				{
					modelIndex = createIndex(row, column, reinterpret_cast<quintptr>(pImplItem));
				}
			}
		}
	}

	return modelIndex;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CMiddlewareDataModel::parent(QModelIndex const& index) const
{
	QModelIndex modelIndex = QModelIndex();

	if (index.isValid())
	{
		CImplItem const* const pItem = ItemFromIndex(index);

		if (pItem != nullptr)
		{
			modelIndex = IndexFromItem(pItem->GetParent());
		}
	}

	return modelIndex;

}

//////////////////////////////////////////////////////////////////////////
Qt::DropActions CMiddlewareDataModel::supportedDragActions() const
{
	return Qt::CopyAction;
}

//////////////////////////////////////////////////////////////////////////
QStringList CMiddlewareDataModel::mimeTypes() const
{
	QStringList types = QAbstractItemModel::mimeTypes();
	types << CDragDropData::GetMimeFormatForType(ModelUtils::s_szImplMimeType);
	return types;
}

//////////////////////////////////////////////////////////////////////////
QMimeData* CMiddlewareDataModel::mimeData(QModelIndexList const& indexes) const
{
	CDragDropData* const pDragDropData = new CDragDropData();
	QByteArray byteArray;
	QDataStream stream(&byteArray, QIODevice::ReadWrite);

	QModelIndexList nameIndexes;

	for (QModelIndex const& index : indexes)
	{
		nameIndexes.append(index.sibling(index.row(), static_cast<int>(EColumns::Name)));
	}

	nameIndexes.erase(std::unique(nameIndexes.begin(), nameIndexes.end()), nameIndexes.end());

	for (auto const& index : nameIndexes)
	{
		CImplItem const* const pImplItem = ItemFromIndex(index);

		if (pImplItem != nullptr)
		{
			stream << pImplItem->GetId();
		}
	}

	pDragDropData->SetCustomData(ModelUtils::s_szImplMimeType, byteArray);
	return pDragDropData;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CMiddlewareDataModel::ItemFromIndex(QModelIndex const& index) const
{
	CImplItem* pImplItem = nullptr;

	if (index.isValid())
	{
		pImplItem = static_cast<CImplItem*>(index.internalPointer());
	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CMiddlewareDataModel::IndexFromItem(CImplItem const* const pImplItem) const
{
	QModelIndex modelIndex = QModelIndex();

	if (pImplItem != nullptr)
	{
		CImplItem const* pParent = pImplItem->GetParent();

		if (pParent == nullptr)
		{
			pParent = m_pEditorImpl->GetRoot();
		}

		if (pParent != nullptr)
		{
			int const size = pParent->ChildCount();

			for (int i = 0; i < size; ++i)
			{
				if (pParent->GetChildAt(i) == pImplItem)
				{
					modelIndex = createIndex(i, 0, reinterpret_cast<quintptr>(pImplItem));
					break;
				}
			}
		}
	}

	return modelIndex;
}

//////////////////////////////////////////////////////////////////////////
CMiddlewareFilterProxyModel::CMiddlewareFilterProxyModel(QObject* const pParent)
	: QAttributeFilterProxyModel(QDeepFilterProxyModel::Behavior::AcceptIfChildMatches, pParent)
{
}

//////////////////////////////////////////////////////////////////////////
bool CMiddlewareFilterProxyModel::rowMatchesFilter(int sourceRow, QModelIndex const& sourceParent) const
{
	bool matchesFilter = QAttributeFilterProxyModel::rowMatchesFilter(sourceRow, sourceParent);

	if (matchesFilter)
	{
		QModelIndex const& index = sourceModel()->index(sourceRow, static_cast<int>(CMiddlewareDataModel::EColumns::Name), sourceParent);

		if (index.isValid())
		{
			// Hide placeholder.
			matchesFilter = !sourceModel()->data(index, static_cast<int>(CMiddlewareDataModel::ERoles::IsPlaceholder)).toBool();
		}
	}

	return matchesFilter;
}

//////////////////////////////////////////////////////////////////////////
bool CMiddlewareFilterProxyModel::lessThan(QModelIndex const& left, QModelIndex const& right) const
{
	bool isLessThan = false;

	// First sort by type, then by name.
	if (left.column() == right.column())
	{
		int const itemTypeRole = static_cast<int>(CMiddlewareDataModel::ERoles::ItemType);
		int const leftType = sourceModel()->data(left, itemTypeRole).toInt();
		int const rightType = sourceModel()->data(right, itemTypeRole).toInt();

		if (leftType != rightType)
		{
			isLessThan = leftType > rightType;
		}
		else
		{
			QVariant const& valueLeft = sourceModel()->data(left, Qt::DisplayRole);
			QVariant const& valueRight = sourceModel()->data(right, Qt::DisplayRole);
			isLessThan = valueLeft < valueRight;
		}
	}
	else
	{
		isLessThan = QSortFilterProxyModel::lessThan(left, right);
	}

	return isLessThan;
}
} // namespace ACE
