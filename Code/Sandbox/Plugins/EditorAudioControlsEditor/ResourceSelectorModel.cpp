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
	if (orientation != Qt::Horizontal)
	{
		return QVariant();
	}

	if ((role == Qt::DisplayRole) || (role == Qt::ToolTipRole))
	{
		return "Name";
	}

	return QVariant();
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
	CSystemLibrary const* const pLibrary = static_cast<CSystemLibrary*>(index.internalPointer());

	if (pLibrary != nullptr)
	{
		switch (role)
		{
		case Qt::DisplayRole:
		case Qt::ToolTipRole:
			return QtUtil::ToQStringSafe(pLibrary->GetName());
			break;

		case Qt::DecorationRole:
			return GetItemTypeIcon(ESystemItemType::Library);
			break;

		case static_cast<int>(SystemModelUtils::ERoles::ItemType) :
			return static_cast<int>(ESystemItemType::Library);
			break;

		case static_cast<int>(SystemModelUtils::ERoles::InternalPointer) :
			return reinterpret_cast<intptr_t>(pLibrary);
			break;
		}
	}

	return QVariant();
}

//////////////////////////////////////////////////////////////////////////
bool CResourceSourceModel::setData(QModelIndex const& index, QVariant const& value, int role)
{
	if (index.isValid())
	{
		CSystemAsset const* const pItem = static_cast<CSystemAsset*>(index.internalPointer());

		if (pItem != nullptr)
		{
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
QVariant CResourceSourceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return ResourceModelUtils::GetHeaderData(section, orientation, role);
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CResourceSourceModel::flags(QModelIndex const& index) const
{
	if (index.isValid())
	{
		return QAbstractItemModel::flags(index) | Qt::ItemIsSelectable;
	}

	return Qt::NoItemFlags;
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
	if (!index.isValid())
	{
		return QVariant();
	}

	CSystemAsset const* const pItem = static_cast<CSystemAsset*>(index.internalPointer());

	if (pItem != nullptr)
	{
		ESystemItemType const itemType = pItem->GetType();

		switch (role)
		{
		case Qt::DisplayRole:
		case Qt::ToolTipRole:
			return QtUtil::ToQStringSafe(pItem->GetName());
			break;

		case Qt::DecorationRole:
			return GetItemTypeIcon(itemType);
			break;

		case static_cast<int>(SystemModelUtils::ERoles::ItemType) :
			return static_cast<int>(itemType);
			break;

		case static_cast<int>(SystemModelUtils::ERoles::InternalPointer) :
			return reinterpret_cast<intptr_t>(pItem);
			break;
		}
	}

	return QVariant();
}

//////////////////////////////////////////////////////////////////////////
bool CResourceLibraryModel::setData(QModelIndex const& index, QVariant const& value, int role)
{
	if (index.isValid())
	{
		CSystemAsset const* const pItem = static_cast<CSystemAsset*>(index.internalPointer());

		if (pItem != nullptr)
		{
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
QVariant CResourceLibraryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return ResourceModelUtils::GetHeaderData(section, orientation, role);
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CResourceLibraryModel::flags(QModelIndex const& index) const
{
	if (index.isValid())
	{
		return QAbstractItemModel::flags(index) | Qt::ItemIsSelectable;
	}

	return Qt::NoItemFlags;
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
	if (QDeepFilterProxyModel::rowMatchesFilter(sourceRow, sourceParent))
	{
		QModelIndex const& index = sourceModel()->index(sourceRow, 0, sourceParent);

		if (index.isValid())
		{
			CSystemAsset* const pAsset = SystemModelUtils::GetAssetFromIndex(index, 0);

			if (pAsset != nullptr)
			{
				if (pAsset->GetType() != m_type)
				{
					return false;
				}
				else
				{
					auto const pControl = static_cast<CSystemControl*>(pAsset);
					Scope const scope = pControl->GetScope();

					if (scope != Utils::GetGlobalScope() && scope != m_scope)
					{
						return false;
					}
				}
				
				return true;
			}
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CResourceFilterProxyModel::lessThan(QModelIndex const& left, QModelIndex const& right) const
{
	// First sort by type, then by name.
	if (left.column() == right.column())
	{
		ESystemItemType const leftType = static_cast<ESystemItemType>(sourceModel()->data(left, static_cast<int>(SystemModelUtils::ERoles::ItemType)).toInt());
		ESystemItemType const rightType = static_cast<ESystemItemType>(sourceModel()->data(right, static_cast<int>(SystemModelUtils::ERoles::ItemType)).toInt());

		if (leftType != rightType)
		{
			return rightType < ESystemItemType::Folder;
		}

		QVariant const& valueLeft = sourceModel()->data(left, Qt::DisplayRole);
		QVariant const& valueRight = sourceModel()->data(right, Qt::DisplayRole);

		return valueLeft < valueRight;
	}

	return QSortFilterProxyModel::lessThan(left, right);
}
} // namespace ACE
