// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "MiddlewareDataModel.h"
#include "IAudioSystemEditor.h"
#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"

#include <IAudioSystemItem.h>
#include <CrySystem/File/CryFile.h>  // Includes CryPath.h in correct order.
#include <QtUtil.h>
#include <CrySandbox/CrySignal.h>
#include <CryIcon.h>

#include <QMimeData>
#include <QDataStream>
#include <QColor>

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
	if (m_pAudioSystem)
	{
		IAudioSystemItem* pItem = ItemFromIndex(parent);

		if (!pItem)
		{
			// if not valid it must be a top level item so get root
			pItem = m_pAudioSystem->GetRoot();
		}

		if (pItem)
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
	if (m_pAudioSystem)
	{
		if (!index.isValid())
		{
			return QVariant();
		}

		IAudioSystemItem* pItem = ItemFromIndex(index);

		if (pItem)
		{
			switch (index.column())
			{
			case static_cast<int>(EMiddlewareDataColumns::Name):
				{
					switch (role)
					{
					case Qt::DisplayRole:
						return (const char*)pItem->GetName();
					case Qt::DecorationRole:
						return CryIcon((QtUtil::ToQString(PathUtil::GetEnginePath()) + CRY_NATIVE_PATH_SEPSTR) + m_pAudioSystem->GetTypeIcon(pItem->GetType()));
					case Qt::ForegroundRole:
						if (pItem->IsLocalised())
						{
							return QColor(36, 180, 245);
						}
						else if (!pItem->IsConnected() && (m_pAudioSystem->ImplTypeToATLType(pItem->GetType()) != EItemType::Invalid))
						{
							// Tint non connected controls that can actually be connected to something (ie. exclude folders)
							return QColor(255, 150, 50);
						}
						break;
					case Qt::ToolTipRole:
						if (pItem->IsLocalised())
						{
							return tr("Item is localized");
						}
						else if (!pItem->IsConnected())
						{
							return tr("Item is not connected to any ATL control");
						}
						break;
					case static_cast<int>(EMiddlewareDataAttributes::Type):
						return pItem->GetType();
					case static_cast<int>(EMiddlewareDataAttributes::Connected):
						return pItem->IsConnected();
					case static_cast<int>(EMiddlewareDataAttributes::Placeholder):
						return pItem->IsPlaceholder();
					case static_cast<int>(EMiddlewareDataAttributes::Localized):
						return pItem->IsLocalised();
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
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		return tr("Name");
	}
	return QVariant();
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CMiddlewareDataModel::flags(QModelIndex const& index) const
{
	Qt::ItemFlags flag = QAbstractItemModel::flags(index);

	if (index.isValid() && m_pAudioSystem)
	{
		IAudioSystemItem* pItem = ItemFromIndex(index);

		if (pItem && !pItem->IsPlaceholder() && (m_pAudioSystem->ImplTypeToATLType(pItem->GetType()) != EItemType::NumTypes))
		{
			flag |= Qt::ItemIsDragEnabled;
		}
	}

	return flag;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CMiddlewareDataModel::index(int row, int column, QModelIndex const& parent /*= QModelIndex()*/) const
{
	if (m_pAudioSystem)
	{
		if ((row >= 0) && (column >= 0))
		{
			IAudioSystemItem* pParent = ItemFromIndex(parent);

			if (!pParent)
			{
				pParent = m_pAudioSystem->GetRoot();
			}

			if (pParent && pParent->ChildCount() > row)
			{
				IAudioSystemItem* pItem = pParent->GetChildAt(row);

				if (pItem)
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
		IAudioSystemItem* pItem = ItemFromIndex(index);

		if (pItem)
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
	QMimeData* pData = QAbstractItemModel::mimeData(indexes);
	QByteArray byteArray;
	QDataStream stream(&byteArray, QIODevice::ReadWrite);

	for (auto const index : indexes)
	{
		IAudioSystemItem* pItem = ItemFromIndex(index);

		if (pItem)
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
QModelIndex CMiddlewareDataModel::IndexFromItem(IAudioSystemItem* pItem) const
{
	if (pItem)
	{
		IAudioSystemItem* pParent = pItem->GetParent();

		if (!pParent)
		{
			pParent = m_pAudioSystem->GetRoot();
		}

		if (pParent)
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
	, m_allowedControlsMask(std::numeric_limits<uint>::max())
	, m_bHideConnected(false)
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

			if (m_bHideConnected)
			{
				if (sourceModel()->data(index, static_cast<int>(CMiddlewareDataModel::EMiddlewareDataAttributes::Connected)).toBool())
				{
					return false;
				}

				if (sourceModel()->hasChildren(index))
				{
					for (int i = 0; i < sourceModel()->rowCount(index); ++i)
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
void CMiddlewareDataFilterProxyModel::SetAllowedControlsMask(uint allowedControlsMask)
{
	m_allowedControlsMask = allowedControlsMask;
	invalidate();
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataFilterProxyModel::SetHideConnected(bool const bHideConnected)
{
	m_bHideConnected = bHideConnected;
	invalidate();
}

//////////////////////////////////////////////////////////////////////////
bool CMiddlewareDataFilterProxyModel::lessThan(QModelIndex const& left, QModelIndex const& right) const
{
	if (left.column() == right.column())
	{
		bool const bLeftHasChildren = (sourceModel()->rowCount(left) > 0);
		bool const bRightHasChildren = (sourceModel()->rowCount(right) > 0);

		if (bLeftHasChildren == bRightHasChildren)
		{
			QVariant const valueLeft = sourceModel()->data(left, Qt::DisplayRole);
			QVariant const valueRight = sourceModel()->data(right, Qt::DisplayRole);
			return valueLeft < valueRight;
		}
		else
		{
			return !bRightHasChildren; //high priority to the one that has children
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
	IAudioSystemEditor* pAudioSystemEditorImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
	QByteArray encoded = pData->data(CMiddlewareDataModel::ms_szMimeType);
	QDataStream stream(&encoded, QIODevice::ReadOnly);
	while (!stream.atEnd())
	{
		CID id;
		stream >> id;

		if (id != ACE_INVALID_ID)
		{
			IAudioSystemItem* pAudioSystemControl = pAudioSystemEditorImpl->GetControl(id);

			if (pAudioSystemControl)
			{
				outItems.push_back(pAudioSystemControl);
			}
		}
	}
}
} // namespace AudioModelUtils
} // namespace ACE
