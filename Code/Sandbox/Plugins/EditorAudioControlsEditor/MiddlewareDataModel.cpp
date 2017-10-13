// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "MiddlewareDataModel.h"
#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"
#include "ItemStatusHelper.h"

#include <IAudioSystemEditor.h>
#include <IAudioSystemItem.h>
#include <CrySystem/File/CryFile.h>  // Includes CryPath.h in correct order.
#include <QtUtil.h>
#include <CrySandbox/CrySignal.h>
#include <CryIcon.h>

#include <QMimeData>
#include <QDataStream>

namespace ACE
{
char const* const CMiddlewareDataModel::ms_szMimeType = "application/cryengine-audioimplementationitem";

//////////////////////////////////////////////////////////////////////////
CMiddlewareDataModel::CMiddlewareDataModel()
	: m_pAudioSystem(CAudioControlsEditorPlugin::GetAudioSystemEditorImpl())
{
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.Connect([&]()
	{
		m_pAudioSystem = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
		beginResetModel();
		endResetModel();
	});

	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationAboutToChange.Connect([&]()
	{
		beginResetModel();
		m_pAudioSystem = nullptr;
		endResetModel();
	});
}

//////////////////////////////////////////////////////////////////////////
int CMiddlewareDataModel::rowCount(QModelIndex const& parent) const
{
	if (m_pAudioSystem != nullptr)
	{
		IAudioSystemItem const* pItem = ItemFromIndex(parent);

		if (pItem == nullptr)
		{
			// if not valid it must be a top level item so get root
			pItem = m_pAudioSystem->GetRoot();
		}

		if (pItem != nullptr)
		{
			return pItem->ChildCount();
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
int CMiddlewareDataModel::columnCount(QModelIndex const& parent) const
{
	return static_cast<int>(EMiddlewareDataColumns::Count);
}

//////////////////////////////////////////////////////////////////////////
QVariant CMiddlewareDataModel::data(QModelIndex const& index, int role) const
{
	if (m_pAudioSystem != nullptr)
	{
		if (!index.isValid())
		{
			return QVariant();
		}

		IAudioSystemItem const* const pItem = ItemFromIndex(index);

		if (pItem != nullptr)
		{
			switch (index.column())
			{
			case static_cast<int>(EMiddlewareDataColumns::Name):
				{
					switch (role)
					{
					case Qt::DisplayRole:
						return (const char*)pItem->GetName();
						break;
					case Qt::DecorationRole:
						return CryIcon((QtUtil::ToQString(PathUtil::GetEnginePath()) + CRY_NATIVE_PATH_SEPSTR) + m_pAudioSystem->GetTypeIcon(pItem->GetType()));
						break;
					case Qt::ForegroundRole:
						if (pItem->IsLocalised())
						{
							return GetItemStatusColor(EItemStatus::Localized);
						}
						else if (!pItem->IsConnected() && (m_pAudioSystem->ImplTypeToSystemType(pItem->GetType()) != EItemType::Invalid))
						{
							// Tint non connected controls that can actually be connected to something (ie. exclude folders)
							return GetItemStatusColor(EItemStatus::NoConnection);
						}
						break;
					case Qt::ToolTipRole:
						if (pItem->IsLocalised())
						{
							return tr("Item is localized");
						}
						else if (!pItem->IsConnected())
						{
							return tr("Item is not connected to any audio system control");
						}
						break;
					case static_cast<int>(EMiddlewareDataAttributes::Type):
						return pItem->GetType();
						break;
					case static_cast<int>(EMiddlewareDataAttributes::Connected):
						return pItem->IsConnected();
						break;
					case static_cast<int>(EMiddlewareDataAttributes::Placeholder):
						return pItem->IsPlaceholder();
						break;
					case static_cast<int>(EMiddlewareDataAttributes::Localized):
						return pItem->IsLocalised();
						break;
					case static_cast<int>(EMiddlewareDataAttributes::Id) :
						return pItem->GetId();
						break;
					}
				}
				break;
			}
		}
	}
	return QVariant();
}

//////////////////////////////////////////////////////////////////////////
QVariant CMiddlewareDataModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
	if ((orientation == Qt::Horizontal) && (role == Qt::DisplayRole))
	{
		return tr("Name");
	}

	return QVariant();
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CMiddlewareDataModel::flags(QModelIndex const& index) const
{
	Qt::ItemFlags flag = QAbstractItemModel::flags(index);

	if (index.isValid() && (m_pAudioSystem != nullptr))
	{
		IAudioSystemItem const* const pItem = ItemFromIndex(index);

		if ((pItem != nullptr) && !pItem->IsPlaceholder() && (m_pAudioSystem->ImplTypeToSystemType(pItem->GetType()) != EItemType::NumTypes))
		{
			flag |= Qt::ItemIsDragEnabled;
		}
	}

	return flag;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CMiddlewareDataModel::index(int row, int column, QModelIndex const& parent /*= QModelIndex()*/) const
{
	if (m_pAudioSystem != nullptr)
	{
		if ((row >= 0) && (column >= 0))
		{
			IAudioSystemItem const* pParent = ItemFromIndex(parent);

			if (pParent == nullptr)
			{
				pParent = m_pAudioSystem->GetRoot();
			}

			if ((pParent != nullptr) && pParent->ChildCount() > row)
			{
				IAudioSystemItem const* const pItem = pParent->GetChildAt(row);

				if (pItem != nullptr)
				{
					return createIndex(row, column, reinterpret_cast<quintptr>(pItem));
				}
			}
		}
	}

	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CMiddlewareDataModel::parent(QModelIndex const& index) const
{
	if (index.isValid())
	{
		IAudioSystemItem const* const pItem = ItemFromIndex(index);

		if (pItem != nullptr)
		{
			return IndexFromItem(pItem->GetParent());
		}
	}

	return QModelIndex();

}

//////////////////////////////////////////////////////////////////////////
Qt::DropActions CMiddlewareDataModel::supportedDragActions() const
{
	return Qt::CopyAction;
}

//////////////////////////////////////////////////////////////////////////
QStringList CMiddlewareDataModel::mimeTypes() const
{
	QStringList list = QAbstractItemModel::mimeTypes();
	list << ms_szMimeType;
	return list;
}

//////////////////////////////////////////////////////////////////////////
QMimeData* CMiddlewareDataModel::mimeData(QModelIndexList const& indexes) const
{
	QMimeData* const pData = QAbstractItemModel::mimeData(indexes);
	QByteArray byteArray;
	QDataStream stream(&byteArray, QIODevice::ReadWrite);

	for (auto const& index : indexes)
	{
		IAudioSystemItem const* const pItem = ItemFromIndex(index);

		if (pItem != nullptr)
		{
			stream << pItem->GetId();
		}
	}

	pData->setData(ms_szMimeType, byteArray);
	return pData;
}

//////////////////////////////////////////////////////////////////////////
IAudioSystemItem* CMiddlewareDataModel::ItemFromIndex(QModelIndex const& index) const
{
	if ((index.row() < 0) || (index.column() < 0))
	{
		return nullptr;
	}

	return static_cast<IAudioSystemItem*>(index.internalPointer());
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CMiddlewareDataModel::IndexFromItem(IAudioSystemItem const* const pItem) const
{
	if (pItem != nullptr)
	{
		IAudioSystemItem const* pParent = pItem->GetParent();

		if (pParent == nullptr)
		{
			pParent = m_pAudioSystem->GetRoot();
		}

		if (pParent != nullptr)
		{
			int const size = pParent->ChildCount();

			for (int i = 0; i < size; ++i)
			{
				if (pParent->GetChildAt(i) == pItem)
				{
					return createIndex(i, 0, reinterpret_cast<quintptr>(pItem));
				}
			}
		}
	}

	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataModel::Reset()
{
	beginResetModel();
	endResetModel();
}

//////////////////////////////////////////////////////////////////////////
CMiddlewareDataFilterProxyModel::CMiddlewareDataFilterProxyModel(QObject* parent)
	: QDeepFilterProxyModel(QDeepFilterProxyModel::BehaviorFlags(QDeepFilterProxyModel::AcceptIfChildMatches), parent)
	, m_hideConnected(false)
{
}

//////////////////////////////////////////////////////////////////////////
bool CMiddlewareDataFilterProxyModel::rowMatchesFilter(int source_row, const QModelIndex& source_parent) const
{
	if (QDeepFilterProxyModel::rowMatchesFilter(source_row, source_parent))
	{
		QModelIndex const index = sourceModel()->index(source_row, 0, source_parent);

		if (index.isValid())
		{
			// Hide placeholder
			if (sourceModel()->data(index, static_cast<int>(CMiddlewareDataModel::EMiddlewareDataAttributes::Placeholder)).toBool())
			{
				return false;
			}

			if (m_hideConnected)
			{
				if (sourceModel()->data(index, static_cast<int>(CMiddlewareDataModel::EMiddlewareDataAttributes::Connected)).toBool())
				{
					return false;
				}

				if (sourceModel()->hasChildren(index))
				{
					int const rowCount = sourceModel()->rowCount(index);

					for (int i = 0; i < rowCount; ++i)
					{
						if (filterAcceptsRow(i, index))
						{
							return true;
						}

						return false;
					}
				}
			}

			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataFilterProxyModel::SetHideConnected(bool const hideConnected)
{
	m_hideConnected = hideConnected;
	invalidate();
}

//////////////////////////////////////////////////////////////////////////
bool CMiddlewareDataFilterProxyModel::lessThan(QModelIndex const& left, QModelIndex const& right) const
{
	if (left.column() == right.column())
	{
		bool const hasLeftChildren = (sourceModel()->rowCount(left) > 0);
		bool const hasRightChildren = (sourceModel()->rowCount(right) > 0);

		if (hasLeftChildren == hasRightChildren)
		{
			QVariant const& valueLeft = sourceModel()->data(left, Qt::DisplayRole);
			QVariant const& valueRight = sourceModel()->data(right, Qt::DisplayRole);
			return valueLeft < valueRight;
		}
		else
		{
			return !hasRightChildren; //high priority to the one that has children
		}
	}

	return QSortFilterProxyModel::lessThan(left, right);
}

//////////////////////////////////////////////////////////////////////////
namespace AudioModelUtils
{
//////////////////////////////////////////////////////////////////////////
void DecodeImplMimeData(const QMimeData* pData, std::vector<IAudioSystemItem*>& outItems)
{
	IAudioSystemEditor const* const pAudioSystemEditorImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
	QByteArray encoded = pData->data(CMiddlewareDataModel::ms_szMimeType);
	QDataStream stream(&encoded, QIODevice::ReadOnly);
	while (!stream.atEnd())
	{
		CID id;
		stream >> id;

		if (id != ACE_INVALID_ID)
		{
			IAudioSystemItem* const pAudioSystemControl = pAudioSystemEditorImpl->GetControl(id);

			if (pAudioSystemControl != nullptr)
			{
				outItems.push_back(pAudioSystemControl);
			}
		}
	}
}
} // namespace AudioModelUtils
} // namespace ACE
