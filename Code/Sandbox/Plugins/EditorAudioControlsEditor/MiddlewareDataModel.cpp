// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "MiddlewareDataModel.h"
#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"
#include "ModelUtils.h"

#include <IEditorImpl.h>
#include <IImplItem.h>
#include <CrySystem/File/CryFile.h>
#include <CrySandbox/CrySignal.h>
#include <CryIcon.h>

#include <DragDrop.h>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CMiddlewareDataModel::CMiddlewareDataModel(QObject* const pParent)
	: QAbstractItemModel(pParent)
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
	g_implementationManager.SignalImplementationAboutToChange.Connect([this]()
		{
			beginResetModel();
			endResetModel();
	  }, reinterpret_cast<uintptr_t>(this));

	g_implementationManager.SignalImplementationChanged.Connect([this]()
		{
			beginResetModel();
			endResetModel();
	  }, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataModel::DisconnectSignals()
{
	g_implementationManager.SignalImplementationAboutToChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	g_implementationManager.SignalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
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
	QVariant variant;

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

	if (g_pEditorImpl != nullptr)
	{
		IImplItem const* pIImplItem = ItemFromIndex(parent);

		if (pIImplItem == nullptr)
		{
			// If not valid it must be a top level item, so get root.
			pIImplItem = g_pEditorImpl->GetRoot();
		}

		if (pIImplItem != nullptr)
		{
			rowCount = pIImplItem->GetNumChildren();
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
	QVariant variant;

	if (g_pEditorImpl != nullptr)
	{
		if (index.isValid())
		{
			IImplItem const* const pIImplItem = ItemFromIndex(index);

			if (pIImplItem != nullptr)
			{
				if (role == static_cast<int>(ERoles::Name))
				{
					variant = static_cast<char const*>(pIImplItem->GetName());
				}
				else
				{
					switch (index.column())
					{
					case static_cast<int>(EColumns::Notification):
						{
							switch (role)
							{
							case Qt::DecorationRole:
								if (!pIImplItem->IsConnected() && !pIImplItem->IsContainer())
								{
									variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::NoConnection));
								}
								else if (pIImplItem->IsLocalized())
								{
									variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::Localized));
								}
								break;
							case Qt::ToolTipRole:
								if (!pIImplItem->IsConnected() && !pIImplItem->IsContainer())
								{
									variant = tr("Item is not connected to any audio system control");
								}
								else if (pIImplItem->IsLocalized())
								{
									variant = tr("Item is localized");
								}
								break;
							case static_cast<int>(ERoles::Id):
								variant = pIImplItem->GetId();
								break;
							default:
								break;
							}
						}
						break;
					case static_cast<int>(EColumns::Connected):
						if ((role == Qt::CheckStateRole) && !pIImplItem->IsContainer())
						{
							variant = pIImplItem->IsConnected() ? Qt::Checked : Qt::Unchecked;
						}
						break;
					case static_cast<int>(EColumns::Localized):
						if ((role == Qt::CheckStateRole) && !pIImplItem->IsContainer())
						{
							variant = pIImplItem->IsLocalized() ? Qt::Checked : Qt::Unchecked;
						}
						break;
					case static_cast<int>(EColumns::Name):
						{
							switch (role)
							{
							case Qt::DecorationRole:
								variant = CryIcon(g_pEditorImpl->GetTypeIcon(pIImplItem));
								break;
							case Qt::DisplayRole:
							case Qt::ToolTipRole:
								variant = static_cast<char const*>(pIImplItem->GetName());
								break;
							case static_cast<int>(ERoles::Id):
								variant = pIImplItem->GetId();
								break;
							case static_cast<int>(ERoles::SortPriority):
								variant = pIImplItem->GetSortPriority();
								break;
							case static_cast<int>(ERoles::IsPlaceholder):
								variant = pIImplItem->IsPlaceholder();
								break;
							default:
								break;
							}
						}
						break;
					}
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

	if (index.isValid() && (g_pEditorImpl != nullptr))
	{
		IImplItem const* const pIImplItem = ItemFromIndex(index);

		if ((pIImplItem != nullptr) && !pIImplItem->IsPlaceholder() && (g_pEditorImpl->ImplTypeToAssetType(pIImplItem) != EAssetType::NumTypes))
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

	if (g_pEditorImpl != nullptr)
	{
		if ((row >= 0) && (column >= 0))
		{
			IImplItem const* pParent = ItemFromIndex(parent);

			if (pParent == nullptr)
			{
				pParent = g_pEditorImpl->GetRoot();
			}

			if ((pParent != nullptr) && pParent->GetNumChildren() > row)
			{
				IImplItem const* const pIImplItem = pParent->GetChildAt(row);

				if (pIImplItem != nullptr)
				{
					modelIndex = createIndex(row, column, reinterpret_cast<quintptr>(pIImplItem));
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
		IImplItem const* const pIImplItem = ItemFromIndex(index);

		if (pIImplItem != nullptr)
		{
			modelIndex = IndexFromItem(pIImplItem->GetParent());
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
		IImplItem const* const pIImplItem = ItemFromIndex(index);

		if (pIImplItem != nullptr)
		{
			stream << pIImplItem->GetId();
		}
	}

	pDragDropData->SetCustomData(ModelUtils::s_szImplMimeType, byteArray);
	return pDragDropData;
}

//////////////////////////////////////////////////////////////////////////
IImplItem* CMiddlewareDataModel::ItemFromIndex(QModelIndex const& index) const
{
	IImplItem* pIImplItem = nullptr;

	if (index.isValid())
	{
		pIImplItem = static_cast<IImplItem*>(index.internalPointer());
	}

	return pIImplItem;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CMiddlewareDataModel::IndexFromItem(IImplItem const* const pIImplItem) const
{
	QModelIndex modelIndex = QModelIndex();

	if (pIImplItem != nullptr)
	{
		IImplItem const* pParent = pIImplItem->GetParent();

		if (pParent == nullptr)
		{
			pParent = g_pEditorImpl->GetRoot();
		}

		if (pParent != nullptr)
		{
			int const size = pParent->GetNumChildren();

			for (int i = 0; i < size; ++i)
			{
				if (pParent->GetChildAt(i) == pIImplItem)
				{
					modelIndex = createIndex(i, 0, reinterpret_cast<quintptr>(pIImplItem));
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
		int const sortPriorityRole = static_cast<int>(CMiddlewareDataModel::ERoles::SortPriority);
		int const leftPriority = sourceModel()->data(left, sortPriorityRole).toInt();
		int const rightPriority = sourceModel()->data(right, sortPriorityRole).toInt();

		if (leftPriority != rightPriority)
		{
			isLessThan = leftPriority > rightPriority;
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

