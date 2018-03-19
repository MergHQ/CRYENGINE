// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ResourceSelectorModel.h"

#include "AssetsManager.h"
#include "AssetIcons.h"

#include <QtUtil.h>

namespace ACE
{
namespace ResourceModelUtils
{
//////////////////////////////////////////////////////////////////////////
QVariant GetHeaderData(int const section, Qt::Orientation const orientation, int const role)
{
	QVariant variant;

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
	QVariant variant;

	CLibrary const* const pLibrary = static_cast<CLibrary*>(index.internalPointer());

	if (pLibrary != nullptr)
	{
		switch (role)
		{
		case Qt::DisplayRole:
			variant = QtUtil::ToQStringSafe(pLibrary->GetName());
			break;
		case Qt::ToolTipRole:
			if (!pLibrary->GetDescription().IsEmpty())
			{
				variant = QtUtil::ToQStringSafe(pLibrary->GetName() + ": " + pLibrary->GetDescription());
			}
			else
			{
				variant = QtUtil::ToQStringSafe(pLibrary->GetName());
			}
			break;
		case Qt::DecorationRole:
			variant = GetAssetIcon(EAssetType::Library);
			break;
		case static_cast<int>(SystemModelUtils::ERoles::ItemType):
			variant = static_cast<int>(EAssetType::Library);
			break;
		case static_cast<int>(SystemModelUtils::ERoles::InternalPointer):
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
	Qt::ItemFlags flags = Qt::NoItemFlags;

	if (index.isValid())
	{
		flags = QAbstractItemModel::flags(index) | Qt::ItemIsSelectable;
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
	QVariant variant;

	if (index.isValid())
	{
		CAsset const* const pAsset = static_cast<CAsset*>(index.internalPointer());

		if (pAsset != nullptr)
		{
			EAssetType const itemType = pAsset->GetType();

			switch (role)
			{
			case Qt::DisplayRole:
				variant = QtUtil::ToQStringSafe(pAsset->GetName());
				break;
			case Qt::ToolTipRole:
				if (!pAsset->GetDescription().IsEmpty())
				{
					variant = QtUtil::ToQStringSafe(pAsset->GetName() + ": " + pAsset->GetDescription());
				}
				else
				{
					variant = QtUtil::ToQStringSafe(pAsset->GetName());
				}
				break;
			case Qt::DecorationRole:
				variant = GetAssetIcon(itemType);
				break;
			case static_cast<int>(SystemModelUtils::ERoles::ItemType):
				variant = static_cast<int>(itemType);
				break;
			case static_cast<int>(SystemModelUtils::ERoles::InternalPointer):
				variant = reinterpret_cast<intptr_t>(pAsset);
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
CResourceFilterProxyModel::CResourceFilterProxyModel(EAssetType const type, Scope const scope, QObject* const pParent)
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
			CAsset* const pAsset = SystemModelUtils::GetAssetFromIndex(index, 0);

			if (pAsset != nullptr)
			{
				matchesFilter = (pAsset->GetType() == m_type);

				if (matchesFilter)
				{
					auto const pControl = static_cast<CControl*>(pAsset);
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
		EAssetType const leftType = static_cast<EAssetType>(sourceModel()->data(left, itemTypeRole).toInt());
		EAssetType const rightType = static_cast<EAssetType>(sourceModel()->data(right, itemTypeRole).toInt());

		if (leftType != rightType)
		{
			isLessThan = rightType < EAssetType::Folder;
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

