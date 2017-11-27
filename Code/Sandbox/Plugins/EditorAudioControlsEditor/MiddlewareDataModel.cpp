// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

#include "MiddlewareDataModel.h"
#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"
#include "ModelUtils.h"

#include <IEditorImpl.h>
#include <ImplItem.h>
#include <CrySystem/File/CryFile.h>  // Includes CryPath.h in correct order.
#include <QtUtil.h>
#include <CrySandbox/CrySignal.h>
#include <CryIcon.h>

#include <QMimeData>
#include <QDataStream>

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
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationAboutToChange.Connect([&]()
	{
		beginResetModel();
		m_pEditorImpl = nullptr;
		endResetModel();
	}, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.Connect([&]()
	{
		m_pEditorImpl = CAudioControlsEditorPlugin::GetImplEditor();
		beginResetModel();
		endResetModel();
	}, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CMiddlewareDataModel::DisconnectSignals()
{
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationAboutToChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
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
	switch (column)
	{
	case EColumns::Notification:
		return &ModelUtils::s_notificationAttribute;
	case EColumns::Connected:
		return &ModelUtils::s_connectedAttribute;
	case EColumns::Localized:
		return &ModelUtils::s_localizedAttribute;
	case EColumns::Name:
		return &Attributes::s_nameAttribute;
	default:
		return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
QVariant CMiddlewareDataModel::GetHeaderData(int const section, Qt::Orientation const orientation, int const role)
{
	if (orientation != Qt::Horizontal)
	{
		return QVariant();
	}

	auto const pAttribute = GetAttributeForColumn(static_cast<EColumns>(section));

	if (pAttribute == nullptr)
	{
		return QVariant();
	}

	switch (role)
	{
	case Qt::DecorationRole:
		if (section == static_cast<int>(EColumns::Notification))
		{
			return CryIcon("icons:General/Scripting.ico"); // This icon is a placeholder.
		}
		break;
	case Qt::DisplayRole:
		// For Notification we use Icons instead.
		if (section == static_cast<int>(EColumns::Name))
		{
			return pAttribute->GetName();
		}
		break;
	case Qt::ToolTipRole:
		return pAttribute->GetName();
	case Attributes::s_getAttributeRole:
		return QVariant::fromValue(pAttribute);
	default:
		break;
	}

	return QVariant();
}

//////////////////////////////////////////////////////////////////////////
int CMiddlewareDataModel::rowCount(QModelIndex const& parent) const
{
	if (m_pEditorImpl != nullptr)
	{
		CImplItem const* pImplItem = ItemFromIndex(parent);

		if (pImplItem == nullptr)
		{
			// if not valid it must be a top level item so get root
			pImplItem = m_pEditorImpl->GetRoot();
		}

		if (pImplItem != nullptr)
		{
			return pImplItem->ChildCount();
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
int CMiddlewareDataModel::columnCount(QModelIndex const& parent) const
{
	return static_cast<int>(EColumns::Count);
}

//////////////////////////////////////////////////////////////////////////
QVariant CMiddlewareDataModel::data(QModelIndex const& index, int role) const
{
	if (m_pEditorImpl != nullptr)
	{
		if (!index.isValid())
		{
			return QVariant();
		}

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
						if ((!pImplItem->IsConnected() && !pImplItem->IsContainer()))
						{
							return CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::NoConnection));
						}
						else if (pImplItem->IsLocalised())
						{
							return CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::Localized));
						}
						break;
					case Qt::ToolTipRole:
						if ((!pImplItem->IsConnected() && !pImplItem->IsContainer()))
						{
							return tr("Item is not connected to any audio system control");
						}
						else if (pImplItem->IsLocalised())
						{
							return tr("Item is localized");
						}
						break;
					}
				}
				break;
			case static_cast<int>(EColumns::Connected):
				{
					if ((role == Qt::CheckStateRole) && (!pImplItem->IsContainer()))
					{
						return pImplItem->IsConnected() ? Qt::Checked : Qt::Unchecked;
					}
				}
				break;
			case static_cast<int>(EColumns::Localized):
				{
					if ((role == Qt::CheckStateRole) && (!pImplItem->IsContainer()))
					{
						return pImplItem->IsLocalised() ? Qt::Checked : Qt::Unchecked;
					}
				}
				break;
			case static_cast<int>(EColumns::Name):
				{
					switch (role)
					{
					case Qt::DecorationRole:
						return CryIcon(m_pEditorImpl->GetTypeIcon(pImplItem));
						break;
					case Qt::DisplayRole:
					case Qt::ToolTipRole:
					case static_cast<int>(ERoles::Name):
						return static_cast<char const*>(pImplItem->GetName());
						break;
					case static_cast<int>(ERoles::Id):
						return pImplItem->GetId();
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
QVariant CMiddlewareDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return GetHeaderData(section, orientation, role);
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CMiddlewareDataModel::flags(QModelIndex const& index) const
{
	Qt::ItemFlags flag = QAbstractItemModel::flags(index);

	if (index.isValid() && (m_pEditorImpl != nullptr))
	{
		CImplItem const* const pImplItem = ItemFromIndex(index);

		if ((pImplItem != nullptr) && !pImplItem->IsPlaceholder() && (m_pEditorImpl->ImplTypeToSystemType(pImplItem) != ESystemItemType::NumTypes))
		{
			flag |= Qt::ItemIsDragEnabled;
		}
	}

	return flag;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CMiddlewareDataModel::index(int row, int column, QModelIndex const& parent /*= QModelIndex()*/) const
{
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
					return createIndex(row, column, reinterpret_cast<quintptr>(pImplItem));
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
		CImplItem const* const pItem = ItemFromIndex(index);

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
	list << ModelUtils::s_szMiddlewareMimeType;
	return list;
}

//////////////////////////////////////////////////////////////////////////
QMimeData* CMiddlewareDataModel::mimeData(QModelIndexList const& indexes) const
{
	QMimeData* const pData = QAbstractItemModel::mimeData(indexes);
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

	pData->setData(ModelUtils::s_szMiddlewareMimeType, byteArray);
	return pData;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CMiddlewareDataModel::ItemFromIndex(QModelIndex const& index) const
{
	if ((index.row() < 0) || (index.column() < 0))
	{
		return nullptr;
	}

	return static_cast<CImplItem*>(index.internalPointer());
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CMiddlewareDataModel::IndexFromItem(CImplItem const* const pImplItem) const
{
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
					return createIndex(i, 0, reinterpret_cast<quintptr>(pImplItem));
				}
			}
		}
	}

	return QModelIndex();
}
} // namespace ACE
