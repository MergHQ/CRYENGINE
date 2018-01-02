// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ResourceSelectorModel.h"

#include "SystemAssetsManager.h"
#include "SystemControlsIcons.h"

#include <QtUtil.h>

namespace ACE
{
namespace ResourceModelUtils
{
//////////////////////////////////////////////////////////////////////////
QVariant GetHeaderData(int const section, Qt::Orientation const orientation, int const role)
{
	QVariant variant = QVariant();

	if ((orientation == Qt::Horizontal) && ((role == Qt::DisplayRole) || (role == Qt::ToolTipRole)))
	{
		variant = "Name";
	}

	return variant;
}
} // namespace ResourceModelUtils

//////////////////////////////////////////////////////////////////////////
int CResourceSourceModel::columnCount(QModelIndex const& parent) const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////
QVariant CResourceSourceModel::data(QModelIndex const& index, int role) const
{
	QVariant variant = QVariant();

	CSystemLibrary const* const pLibrary = static_cast<CSystemLibrary*>(index.internalPointer());

	if (pLibrary != nullptr)
	{
		switch (role)
		{
		case Qt::DisplayRole:
		case Qt::ToolTipRole:
			variant = QtUtil::ToQStringSafe(pLibrary->GetName());
			break;
		case Qt::DecorationRole:
			variant = GetItemTypeIcon(ESystemItemType::Library);
			break;
		case static_cast<int>(SystemModelUtils::ERoles::ItemType) :
			variant = static_cast<int>(ESystemItemType::Library);
			break;
		case static_cast<int>(SystemModelUtils::ERoles::InternalPointer) :
			variant = reinterpret_cast<intptr_t>(pLibrary);
			break;
		default:
			break;
		}
	}

	return variant;
}

//////////////////////////////////////////////////////////////////////////
bool CResourceSourceModel::setData(QModelIndex const& index, QVariant const& value, int role)
{
	bool wasDataSet = false;

	if (index.isValid())
	{
		CSystemAsset const* const pItem = static_cast<CSystemAsset*>(index.internalPointer());

		if (pItem != nullptr)
		{
			wasDataSet = true;
		}
	}

	return wasDataSet;
}

//////////////////////////////////////////////////////////////////////////
QVariant CResourceSourceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return ResourceModelUtils::GetHeaderData(section, orientation, role);
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CResourceSourceModel::flags(QModelIndex const& index) const
{
	Qt::ItemFlags flags = Qt::NoItemFlags;

	if (index.isValid())
	{
		flags =  QAbstractItemModel::flags(index) | Qt::ItemIsSelectable;
	}

	return flags;
}

//////////////////////////////////////////////////////////////////////////
bool CResourceSourceModel::canDropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CResourceSourceModel::dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////
Qt::DropActions CResourceSourceModel::supportedDropActions() const
{
	return Qt::IgnoreAction;
}

//////////////////////////////////////////////////////////////////////////
int CResourceLibraryModel::columnCount(QModelIndex const& parent) const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////
QVariant CResourceLibraryModel::data(QModelIndex const& index, int role) const
{
	QVariant variant = QVariant();

	if (index.isValid())
	{
		CSystemAsset const* const pItem = static_cast<CSystemAsset*>(index.internalPointer());

		if (pItem != nullptr)
		{
			ESystemItemType const itemType = pItem->GetType();

			switch (role)
			{
			case Qt::DisplayRole:
			case Qt::ToolTipRole:
				variant = QtUtil::ToQStringSafe(pItem->GetName());
				break;
			case Qt::DecorationRole:
				variant = GetItemTypeIcon(itemType);
				break;
			case static_cast<int>(SystemModelUtils::ERoles::ItemType) :
				variant = static_cast<int>(itemType);
				break;
			case static_cast<int>(SystemModelUtils::ERoles::InternalPointer) :
				variant = reinterpret_cast<intptr_t>(pItem);
				break;
			default:
				break;
			}
		}
	}

	return variant;
}

//////////////////////////////////////////////////////////////////////////
bool CResourceLibraryModel::setData(QModelIndex const& index, QVariant const& value, int role)
{
	bool wasDataSet = false;

	if (index.isValid())
	{
		CSystemAsset const* const pItem = static_cast<CSystemAsset*>(index.internalPointer());

		if (pItem != nullptr)
		{
			wasDataSet = true;
		}
	}

	return wasDataSet;
}

//////////////////////////////////////////////////////////////////////////
QVariant CResourceLibraryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return ResourceModelUtils::GetHeaderData(section, orientation, role);
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CResourceLibraryModel::flags(QModelIndex const& index) const
{
	Qt::ItemFlags flags = Qt::NoItemFlags;

	if (index.isValid())
	{
		flags = QAbstractItemModel::flags(index) | Qt::ItemIsSelectable;
	}

	return flags;
}

//////////////////////////////////////////////////////////////////////////
bool CResourceLibraryModel::canDropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CResourceLibraryModel::dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////
Qt::DropActions CResourceLibraryModel::supportedDropActions() const
{
	return Qt::IgnoreAction;
}

//////////////////////////////////////////////////////////////////////////
CResourceFilterProxyModel::CResourceFilterProxyModel(ESystemItemType const type, Scope const scope, QObject* const pParent)
	: QDeepFilterProxyModel(QDeepFilterProxyModel::Behavior::AcceptIfChildMatches, pParent)
	, m_type(type)
	, m_scope(scope)
{
}

//////////////////////////////////////////////////////////////////////////
bool CResourceFilterProxyModel::rowMatchesFilter(int sourceRow, QModelIndex const& sourceParent) const
{
	bool matchesFilter = false;

	if (QDeepFilterProxyModel::rowMatchesFilter(sourceRow, sourceParent))
	{
		QModelIndex const& index = sourceModel()->index(sourceRow, 0, sourceParent);

		if (index.isValid())
		{
			CSystemAsset* const pAsset = SystemModelUtils::GetAssetFromIndex(index, 0);

			if (pAsset != nullptr)
			{
				matchesFilter = (pAsset->GetType() == m_type);

				if (matchesFilter)
				{
					auto const pControl = static_cast<CSystemControl*>(pAsset);
					Scope const scope = pControl->GetScope();

					if (scope != Utils::GetGlobalScope() && scope != m_scope)
					{
						matchesFilter = ((scope == Utils::GetGlobalScope()) || (scope == m_scope));
					}
				}
			}
		}
	}

	return matchesFilter;
}

//////////////////////////////////////////////////////////////////////////
bool CResourceFilterProxyModel::lessThan(QModelIndex const& left, QModelIndex const& right) const
{
	bool isLessThan = false;

	// First sort by type, then by name.
	if (left.column() == right.column())
	{
		int const itemTypeRole = static_cast<int>(SystemModelUtils::ERoles::ItemType);
		ESystemItemType const leftType = static_cast<ESystemItemType>(sourceModel()->data(left, itemTypeRole).toInt());
		ESystemItemType const rightType = static_cast<ESystemItemType>(sourceModel()->data(right, itemTypeRole).toInt());

		if (leftType != rightType)
		{
			isLessThan = rightType < ESystemItemType::Folder;
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
