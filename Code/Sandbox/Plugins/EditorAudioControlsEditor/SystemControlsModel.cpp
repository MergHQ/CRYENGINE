// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SystemControlsModel.h"

#include "AudioControlsEditorPlugin.h"
#include "SystemControlsIcons.h"
#include "ModelUtils.h"

#include <ImplItem.h>
#include <QtUtil.h>

#include <DragDrop.h>

namespace ACE
{
namespace SystemModelUtils
{
//////////////////////////////////////////////////////////////////////////
CItemModelAttribute* GetAttributeForColumn(EColumns const column)
{
	switch (column)
	{
	case EColumns::Notification:
		return &ModelUtils::s_notificationAttribute;
	case EColumns::Type:
		return &ModelUtils::s_typeAttribute;
	case EColumns::Placeholder:
		return &ModelUtils::s_placeholderAttribute;
	case EColumns::NoConnection:
		return &ModelUtils::s_connectedAttribute;
	case EColumns::NoControl:
		return &ModelUtils::s_noControlAttribute;
	case EColumns::Scope:
		return &ModelUtils::s_scopeAttribute;
	case EColumns::Name:
		return &Attributes::s_nameAttribute;
	default:
		return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
QVariant GetHeaderData(int const section, Qt::Orientation const orientation, int const role)
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
		if (section != static_cast<int>(EColumns::Notification))
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
void GetAssetsFromIndexesSeparated(QModelIndexList const& list, std::vector<CSystemLibrary*>& outLibraries, std::vector<CSystemFolder*>& outFolders, std::vector<CSystemControl*>& outControls)
{
	for (auto const& index : list)
	{
		QModelIndex const& nameColumnIndex = index.sibling(index.row(), static_cast<int>(EColumns::Name));
		QVariant const internalPtr = nameColumnIndex.data(static_cast<int>(ERoles::InternalPointer));

		if (internalPtr.isValid())
		{
			QVariant const type = nameColumnIndex.data(static_cast<int>(ERoles::ItemType));

			switch (type.toInt())
			{
			case static_cast<int>(ESystemItemType::Library):
				outLibraries.emplace_back(reinterpret_cast<CSystemLibrary*>(internalPtr.value<intptr_t>()));
				break;
			case static_cast<int>(ESystemItemType::Folder):
				outFolders.emplace_back(reinterpret_cast<CSystemFolder*>(internalPtr.value<intptr_t>()));
				break;
			default:
				outControls.emplace_back(reinterpret_cast<CSystemControl*>(internalPtr.value<intptr_t>()));
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void GetAssetsFromIndexesCombined(QModelIndexList const& list, std::vector<CSystemAsset*>& outAssets)
{
	for (auto const& index : list)
	{
		QModelIndex const& nameColumnIndex = index.sibling(index.row(), static_cast<int>(EColumns::Name));
		QVariant const internalPtr = nameColumnIndex.data(static_cast<int>(ERoles::InternalPointer));

		if (internalPtr.isValid())
		{
			outAssets.emplace_back(reinterpret_cast<CSystemAsset*>(internalPtr.value<intptr_t>()));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CSystemAsset* GetAssetFromIndex(QModelIndex const& index, int const column)
{
	QModelIndex const& nameColumnIndex = index.sibling(index.row(), column);
	QVariant const internalPtr = nameColumnIndex.data(static_cast<int>(ERoles::InternalPointer));

	if (internalPtr.isValid())
	{
		return reinterpret_cast<CSystemAsset*>(internalPtr.value<intptr_t>());
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void DecodeImplMimeData(const QMimeData* pData, std::vector<CImplItem*>& outItems)
{
	IEditorImpl const* const pEditorImpl = CAudioControlsEditorPlugin::GetImplEditor();
	QByteArray encoded = pData->data(ModelUtils::s_szMiddlewareMimeType);
	QDataStream stream(&encoded, QIODevice::ReadOnly);
	while (!stream.atEnd())
	{
		CID id;
		stream >> id;

		if (id != ACE_INVALID_ID)
		{
			CImplItem* const pImplControl = pEditorImpl->GetControl(id);

			if (pImplControl != nullptr)
			{
				outItems.emplace_back(pImplControl);
			}
		}
	}
}
} // namespace SystemModelUtils

//////////////////////////////////////////////////////////////////////////
bool IsParentValid(CSystemAsset const& parent, ESystemItemType const type)
{
	switch (parent.GetType())
	{
	case ESystemItemType::Folder:
	case ESystemItemType::Library: // Intentional fall-through.
		return type != ESystemItemType::State;
	case ESystemItemType::Switch:
		return type == ESystemItemType::State;
	default:
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////
void DecodeMimeData(QMimeData const* const pData, std::vector<CSystemAsset*>& outItems)
{
	CDragDropData const* const dragDropData = CDragDropData::FromMimeData(pData);

	if (dragDropData->HasCustomData("AudioLibraryItems"))
	{
		QByteArray const byteArray = dragDropData->GetCustomData("AudioLibraryItems");
		QDataStream stream(byteArray);

		while (!stream.atEnd())
		{
			intptr_t ptr;
			stream >> ptr;
			outItems.emplace_back(reinterpret_cast<CSystemAsset*>(ptr));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CanDropMimeData(QMimeData const* const pData, CSystemAsset const& parent)
{
	// Handle first if mime data is an external (from the implementation side) source
	std::vector<CImplItem*> implItems;
	SystemModelUtils::DecodeImplMimeData(pData, implItems);

	if (!implItems.empty())
	{
		IEditorImpl const* const pEditorImpl = CAudioControlsEditorPlugin::GetImplEditor();

		for (CImplItem const* const pImplItem : implItems)
		{
			if (!IsParentValid(parent, pEditorImpl->ImplTypeToSystemType(pImplItem)))
			{
				return false;
			}
		}

		return true;
	}
	else
	{
		// Handle if mime data is an internal move (rearranging controls)
		std::vector<CSystemAsset*> droppedItems;
		DecodeMimeData(pData, droppedItems);

		for (auto const pItem : droppedItems)
		{
			if (!IsParentValid(parent, pItem->GetType()))
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
void DropMimeData(QMimeData const* const pData, CSystemAsset* const pParent)
{
	// Handle first if mime data is an external (from the implementation side) source
	std::vector<CImplItem*> implItems;
	SystemModelUtils::DecodeImplMimeData(pData, implItems);

	if (!implItems.empty())
	{
		CSystemAssetsManager* const pAssetsManager = CAudioControlsEditorPlugin::GetAssetsManager();

		for (CImplItem* const pImplControl : implItems)
		{
			pAssetsManager->CreateAndConnectImplItems(pImplControl, pParent);
		}
	}
	else
	{
		std::vector<CSystemAsset*> droppedItems;
		DecodeMimeData(pData, droppedItems);
		std::vector<CSystemAsset*> topLevelDroppedItems;
		Utils::SelectTopLevelAncestors(droppedItems, topLevelDroppedItems);
		CAudioControlsEditorPlugin::GetAssetsManager()->MoveItems(pParent, topLevelDroppedItems);
	}
}

//////////////////////////////////////////////////////////////////////////
CSystemSourceModel::CSystemSourceModel(CSystemAssetsManager* const pAssetsManager, QObject* const pParent)
	: QAbstractItemModel(pParent)
	, m_pAssetsManager(pAssetsManager)
	, m_ignoreLibraryUpdates(false)
{
	ConnectSignals();
}

//////////////////////////////////////////////////////////////////////////
CSystemSourceModel::~CSystemSourceModel()
{
	DisconnectSignals();
}

//////////////////////////////////////////////////////////////////////////
void CSystemSourceModel::ConnectSignals()
{
	CAudioControlsEditorPlugin::signalAboutToLoad.Connect([&]()
	{
		beginResetModel();
		m_ignoreLibraryUpdates = true;
		endResetModel();
	}, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::signalLoaded.Connect([&]()
	{
		beginResetModel();
		m_ignoreLibraryUpdates = false;
		endResetModel();
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalLibraryAboutToBeAdded.Connect([&]()
	{
		if (!m_ignoreLibraryUpdates)
		{
			int const row = m_pAssetsManager->GetLibraryCount();
			beginInsertRows(QModelIndex(), row, row);
		}
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalLibraryAdded.Connect([&](CSystemLibrary* const pLibrary)
	{
		if (!m_ignoreLibraryUpdates)
		{
			endInsertRows();
		}
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalLibraryAboutToBeRemoved.Connect([&](CSystemLibrary* const pLibrary)
	{
		if (!m_ignoreLibraryUpdates)
		{
			int const libCount = m_pAssetsManager->GetLibraryCount();

			for (int i = 0; i < libCount; ++i)
			{
				if (m_pAssetsManager->GetLibrary(i) == pLibrary)
				{
					beginRemoveRows(QModelIndex(), i, i);
					break;
				}
			}
		}
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalLibraryRemoved.Connect([&]()
	{
		if (!m_ignoreLibraryUpdates)
		{
			endRemoveRows();
		}
	}, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CSystemSourceModel::DisconnectSignals()
{
	CAudioControlsEditorPlugin::signalAboutToLoad.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::signalLoaded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalLibraryAboutToBeAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalLibraryAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalLibraryAboutToBeRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalLibraryRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
int CSystemSourceModel::rowCount(QModelIndex const& parent) const
{
	if (!m_ignoreLibraryUpdates && !parent.isValid())
	{
		return m_pAssetsManager->GetLibraryCount();
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
int CSystemSourceModel::columnCount(QModelIndex const& parent) const
{
	return static_cast<int>(SystemModelUtils::EColumns::Count);
}

//////////////////////////////////////////////////////////////////////////
QVariant CSystemSourceModel::data(QModelIndex const& index, int role) const
{
	CSystemLibrary const* const pLibrary = static_cast<CSystemLibrary*>(index.internalPointer());

	if (pLibrary != nullptr)
	{
		switch (index.column())
		{
		case static_cast<int>(SystemModelUtils::EColumns::Notification):
			{
				switch (role)
				{
				case Qt::DecorationRole:
					if (pLibrary->HasPlaceholderConnection())
					{
						return CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::Placeholder));
					}
					else if (!pLibrary->HasConnection())
					{
						return CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::NoConnection));
					}
					else if (!pLibrary->HasControl())
					{
						return CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::NoControl));
					}
					break;
				case Qt::ToolTipRole:
					if (pLibrary->HasPlaceholderConnection())
					{
						return tr("Contains item that has an invalid connection");
					}
					else if (!pLibrary->HasConnection())
					{
						return tr("Contains item that is not connected to any audio control");
					}
					else if (!pLibrary->HasControl())
					{
						return tr("Contains no audio control");
					}
					break;
				}
			}
			break;
		case static_cast<int>(SystemModelUtils::EColumns::Placeholder):
			{
				if (role == Qt::CheckStateRole)
				{
					return (!pLibrary->HasPlaceholderConnection()) ? Qt::Checked : Qt::Unchecked;
				}
			}
			break;
		case static_cast<int>(SystemModelUtils::EColumns::NoConnection):
			{
				if (role == Qt::CheckStateRole)
				{
					return (pLibrary->HasConnection()) ? Qt::Checked : Qt::Unchecked;
				}
			}
			break;
		case static_cast<int>(SystemModelUtils::EColumns::NoControl):
			{
				if (role == Qt::CheckStateRole)
				{
					return (!pLibrary->HasControl()) ? Qt::Checked : Qt::Unchecked;
				}
			}
			break;
		case static_cast<int>(SystemModelUtils::EColumns::Name):
			{
				switch (role)
				{
				case Qt::DecorationRole:
					return GetItemTypeIcon(ESystemItemType::Library);
					break;
				case Qt::DisplayRole:

					if (pLibrary->IsModified())
					{
						return static_cast<char const*>(pLibrary->GetName() + " *");
					}
					else
					{
						return static_cast<char const*>(pLibrary->GetName());
					}
					break;
				case Qt::EditRole:
					return static_cast<char const*>(pLibrary->GetName());
					break;
				case Qt::ToolTipRole:
					if (!pLibrary->GetDescription().IsEmpty())
					{
						return tr(pLibrary->GetName() + ": " + pLibrary->GetDescription());
					}
					else
					{
						return static_cast<char const*>(pLibrary->GetName());
					}
					break;
				case static_cast<int>(SystemModelUtils::ERoles::ItemType) :
					return static_cast<int>(ESystemItemType::Library);
					break;
				case static_cast<int>(SystemModelUtils::ERoles::Name) :
					return static_cast<char const*>(pLibrary->GetName());
					break;
				case static_cast<int>(SystemModelUtils::ERoles::InternalPointer) :
					return reinterpret_cast<intptr_t>(pLibrary);
					break;
				}
			}
			break;
		}

		
	}

	return QVariant();
}

//////////////////////////////////////////////////////////////////////////
bool CSystemSourceModel::setData(QModelIndex const& index, QVariant const& value, int role)
{
	if (index.isValid() && (index.column() == static_cast<int>(SystemModelUtils::EColumns::Name)))
	{
		CSystemAsset* const pItem = static_cast<CSystemAsset*>(index.internalPointer());

		if (pItem != nullptr)
		{
			switch (role)
			{
			case Qt::EditRole:
				{
					if (value.canConvert<QString>())
					{
						string const& oldName = pItem->GetName();
						string const& newName = QtUtil::ToString(value.toString());

						if (!newName.empty() && newName.compareNoCase(oldName) != 0)
						{
							pItem->SetName(Utils::GenerateUniqueLibraryName(newName, *m_pAssetsManager));
						}

						return true;
					}
				}
			default:
				CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, R"([Audio Controls Editor] The role '%d' is not handled!)", role);
				break;
			}
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
QVariant CSystemSourceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return SystemModelUtils::GetHeaderData(section, orientation, role);
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CSystemSourceModel::flags(QModelIndex const& index) const
{
	if (index.isValid())
	{
		if (index.column() == static_cast<int>(SystemModelUtils::EColumns::Name))
		{
			return QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
		}
		else
		{
			return QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled;
		}
	}

	return Qt::NoItemFlags;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CSystemSourceModel::index(int row, int column, QModelIndex const& parent /*= QModelIndex()*/) const
{
	if ((row >= 0) && (column >= 0))
	{
		return createIndex(row, column, reinterpret_cast<quintptr>(m_pAssetsManager->GetLibrary(row)));
	}

	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CSystemSourceModel::parent(QModelIndex const& index) const
{
	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
bool CSystemSourceModel::canDropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const
{
	if (parent.isValid())
	{
		CSystemLibrary const* const pParent = static_cast<CSystemLibrary*>(parent.internalPointer());
		CRY_ASSERT_MESSAGE(pParent != nullptr, "Parent is null pointer.");
		return CanDropMimeData(pData, *pParent);
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CSystemSourceModel::dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent)
{
	if (parent.isValid())
	{
		CSystemLibrary* const pLibrary = static_cast<CSystemLibrary*>(parent.internalPointer());
		CRY_ASSERT_MESSAGE(pLibrary != nullptr, "Library is null pointer.");
		DropMimeData(pData, pLibrary);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
Qt::DropActions CSystemSourceModel::supportedDropActions() const
{
	return Qt::MoveAction | Qt::CopyAction;
}

//////////////////////////////////////////////////////////////////////////
QStringList CSystemSourceModel::mimeTypes() const
{
	QStringList result;
	result << CDragDropData::GetMimeFormatForType("AudioLibraryItems");
	result << ModelUtils::s_szMiddlewareMimeType;
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////
CSystemLibraryModel::CSystemLibraryModel(CSystemAssetsManager* const pAssetsManager, CSystemLibrary* const pLibrary, QObject* const pParent)
	: QAbstractItemModel(pParent)
	, m_pAssetsManager(pAssetsManager)
	, m_pLibrary(pLibrary)
{
	ConnectSignals();
}

//////////////////////////////////////////////////////////////////////////
CSystemLibraryModel::~CSystemLibraryModel()
{
	DisconnectSignals();
}

//////////////////////////////////////////////////////////////////////////
void CSystemLibraryModel::ConnectSignals()
{
	m_pAssetsManager->signalItemAboutToBeAdded.Connect([&](CSystemAsset* const pParent)
	{
		if (Utils::GetParentLibrary(pParent) == m_pLibrary)
		{
			int const row = pParent->ChildCount();

			if (pParent->GetType() == ESystemItemType::Library)
			{
				CRY_ASSERT_MESSAGE(pParent == m_pLibrary, "Parent is not the current library.");
				beginInsertRows(QModelIndex(), row, row);
			}
			else
			{
				QModelIndex const& parent = IndexFromItem(pParent);
				beginInsertRows(parent, row, row);
			}
		}
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalItemAdded.Connect([&](CSystemAsset* const pAsset)
	{
		if (Utils::GetParentLibrary(pAsset) == m_pLibrary)
		{
			endInsertRows();
		}
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalItemAboutToBeRemoved.Connect([&](CSystemAsset* const pAsset)
	{
		if (Utils::GetParentLibrary(pAsset) == m_pLibrary)
		{
			CSystemAsset const* const pParent = pAsset->GetParent();

			if (pParent != nullptr)
			{
				int const childCount = pParent->ChildCount();

				for (int i = 0; i < childCount; ++i)
				{
					if (pParent->GetChild(i) == pAsset)
					{
						if (pParent->GetType() == ESystemItemType::Library)
						{
							CRY_ASSERT_MESSAGE(pParent == m_pLibrary, "Parent is not the current library.");
							beginRemoveRows(QModelIndex(), i, i);
						}
						else
						{
							QModelIndex const& parent = IndexFromItem(pParent);
							beginRemoveRows(parent, i, i);
						}

						break;
					}
				}
			}
		}
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalItemRemoved.Connect([&](CSystemAsset* const pParent, CSystemAsset* const pAsset)
	{
		if (Utils::GetParentLibrary(pParent) == m_pLibrary)
		{
			endRemoveRows();
		}
	}, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CSystemLibraryModel::DisconnectSignals()
{
	m_pAssetsManager->signalItemAboutToBeAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalItemAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalItemAboutToBeRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalItemRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CSystemLibraryModel::IndexFromItem(CSystemAsset const* const pItem) const
{
	if (pItem != nullptr)
	{
		CSystemAsset const* const pParent = pItem->GetParent();

		if (pParent != nullptr)
		{
			int const size = pParent->ChildCount();

			for (int i = 0; i < size; ++i)
			{
				if (pParent->GetChild(i) == pItem)
				{
					return createIndex(i, 0, reinterpret_cast<quintptr>(pItem));
				}
			}
		}
	}

	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
int CSystemLibraryModel::rowCount(QModelIndex const& parent) const
{
	if (!parent.isValid())
	{
		return m_pLibrary->ChildCount();
	}

	CSystemAsset const* const pAsset = static_cast<CSystemAsset*>(parent.internalPointer());

	if (pAsset != nullptr)
	{
		return pAsset->ChildCount();
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
int CSystemLibraryModel::columnCount(QModelIndex const& parent) const
{
	return static_cast<int>(SystemModelUtils::EColumns::Count);
}

//////////////////////////////////////////////////////////////////////////
QVariant CSystemLibraryModel::data(QModelIndex const& index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	CSystemAsset* const pItem = static_cast<CSystemAsset*>(index.internalPointer());

	if (pItem != nullptr)
	{
		ESystemItemType const itemType = pItem->GetType();

		switch (index.column())
		{
		case static_cast<int>(SystemModelUtils::EColumns::Notification):
			{
				switch (role)
				{
				case Qt::DecorationRole:
					if (pItem->HasPlaceholderConnection())
					{
						return CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::Placeholder));
					}
					else if (!pItem->HasConnection())
					{
						return CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::NoConnection));
					}
					else if (((itemType == ESystemItemType::Folder) || (itemType == ESystemItemType::Switch)) && !pItem->HasControl())
					{
						return CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::NoControl));
					}
					break;
				case Qt::ToolTipRole:
					if (pItem->HasPlaceholderConnection())
					{
						if ((itemType == ESystemItemType::Folder) || (itemType == ESystemItemType::Switch))
						{
							return tr("Contains item that has an invalid connection");
						}
						else
						{
							return tr("Item has an invalid connection");
						}
					}
					else if (!pItem->HasConnection())
					{
						if ((itemType == ESystemItemType::Folder) || (itemType == ESystemItemType::Switch))
						{
							return tr("Contains item that is not connected to any audio control");
						}
						else
						{
							return tr("Item is not connected to any audio control");
						}
					}
					else if ((itemType == ESystemItemType::Folder) && !pItem->HasControl())
					{
						return tr("Contains no audio control");
					}
					else if ((itemType == ESystemItemType::Switch) && !pItem->HasControl())
					{
						return tr("Contains no state");
					}
					break;
				case static_cast<int>(SystemModelUtils::ERoles::Id):
					if (itemType != ESystemItemType::Folder)
					{
						CSystemControl const* const pControl = static_cast<CSystemControl*>(pItem);

						if (pControl != nullptr)
						{
							return pControl->GetId();
						}
					}
					break;
				}
			}
			break;
		case static_cast<int>(SystemModelUtils::EColumns::Type):
			{
				if ((role == Qt::DisplayRole) && (itemType != ESystemItemType::Folder))
				{
					return QString(pItem->GetTypeName());
				}
			}
			break;
		case static_cast<int>(SystemModelUtils::EColumns::Placeholder):
			{
				if (role == Qt::CheckStateRole)
				{
					return (!pItem->HasPlaceholderConnection()) ? Qt::Checked : Qt::Unchecked;
				}
			}
			break;
		case static_cast<int>(SystemModelUtils::EColumns::NoConnection):
			{
				if (role == Qt::CheckStateRole)
				{
					return (pItem->HasConnection()) ? Qt::Checked : Qt::Unchecked;
				}
			}
			break;
		case static_cast<int>(SystemModelUtils::EColumns::NoControl):
			{
				if ((role == Qt::CheckStateRole) && ((itemType == ESystemItemType::Folder) || (itemType == ESystemItemType::Switch)))
				{
					return (!pItem->HasControl()) ? Qt::Checked : Qt::Unchecked;
				}
			}
			break;
		case static_cast<int>(SystemModelUtils::EColumns::Scope) :
			if ((role == Qt::DisplayRole) && (itemType != ESystemItemType::Folder))
			{
				CSystemControl const* const pControl = static_cast<CSystemControl*>(pItem);

				if (pControl != nullptr)
				{
					return QString(m_pAssetsManager->GetScopeInfo(pControl->GetScope()).name);
				}
			}
			break;
		case static_cast<int>(SystemModelUtils::EColumns::Name):
			{
				switch (role)
				{
				case Qt::DecorationRole:
					return GetItemTypeIcon(itemType);
					break;
				case Qt::DisplayRole:
					if (pItem->IsModified())
					{
						return static_cast<char const*>(pItem->GetName() + " *");
					}
					else
					{
						return static_cast<char const*>(pItem->GetName());
					}
					break;
				case Qt::EditRole:
					return static_cast<char const*>(pItem->GetName());
					break;
				case Qt::ToolTipRole:
					if (!pItem->GetDescription().IsEmpty())
					{
						return tr(pItem->GetName() + ": " + pItem->GetDescription());
					}
					else
					{
						return static_cast<char const*>(pItem->GetName());
					}
					break;
				case static_cast<int>(SystemModelUtils::ERoles::ItemType):
					return static_cast<int>(itemType);
					break;

				case static_cast<int>(SystemModelUtils::ERoles::Name):
					return static_cast<char const*>(pItem->GetName());
					break;

				case static_cast<int>(SystemModelUtils::ERoles::InternalPointer):
					return reinterpret_cast<intptr_t>(pItem);
					break;
				}
			}
			break;
		}
	}

	return QVariant();
}

//////////////////////////////////////////////////////////////////////////
bool CSystemLibraryModel::setData(QModelIndex const& index, QVariant const& value, int role)
{
	if (index.isValid() && (index.column() == static_cast<int>(SystemModelUtils::EColumns::Name)))
	{
		CSystemAsset* const pItem = static_cast<CSystemAsset*>(index.internalPointer());

		if (pItem != nullptr)
		{
			switch (role)
			{
			case Qt::EditRole:
				{
					if (value.canConvert<QString>())
					{
						string const& oldName = pItem->GetName();
						string const& newName = QtUtil::ToString(value.toString());

						if (!newName.empty() && newName.compareNoCase(oldName) != 0)
						{
							ESystemItemType const itemType = pItem->GetType();

							switch (itemType)
							{
							case ESystemItemType::Preload:
							case ESystemItemType::Parameter:
							case ESystemItemType::State:
							case ESystemItemType::Switch:
							case ESystemItemType::Trigger:
							case ESystemItemType::Environment:
								pItem->SetName(Utils::GenerateUniqueControlName(newName, itemType, *m_pAssetsManager));
								break;
							case ESystemItemType::Folder:
								pItem->SetName(Utils::GenerateUniqueName(newName, itemType, pItem->GetParent()));
								break;
							default:
								CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, R"([Audio Controls Editor] The item type '%d' is not handled!)", itemType);
								break;
							}
						}

						return true;
					}
				}
			default:
				CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, R"([Audio Controls Editor] The role '%d' is not handled!)", role);
				break;
			}
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
QVariant CSystemLibraryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return SystemModelUtils::GetHeaderData(section, orientation, role);
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CSystemLibraryModel::flags(QModelIndex const& index) const
{
	if (index.isValid())
	{
		if (index.column() == static_cast<int>(SystemModelUtils::EColumns::Name))
		{
			return QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
		}
		else
		{
			return QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;
		}
	}

	return Qt::NoItemFlags;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CSystemLibraryModel::index(int row, int column, QModelIndex const& parent) const
{
	if ((row >= 0) && (column >= 0))
	{
		if (parent.isValid())
		{
			CSystemAsset const* const pParent = static_cast<CSystemAsset*>(parent.internalPointer());

			if ((pParent != nullptr) && (row < pParent->ChildCount()))
			{
				CSystemAsset const* const pChild = pParent->GetChild(row);

				if (pChild != nullptr)
				{
					return createIndex(row, column, reinterpret_cast<quintptr>(pChild));
				}
			}
		}
		else if (row < m_pLibrary->ChildCount())
		{
			return createIndex(row, column, reinterpret_cast<quintptr>(m_pLibrary->GetChild(row)));
		}
	}

	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CSystemLibraryModel::parent(QModelIndex const& index) const
{
	if (index.isValid())
	{
		CSystemAsset const* const pItem = static_cast<CSystemAsset*>(index.internalPointer());

		if (pItem != nullptr)
		{
			CSystemAsset const* const pParent = pItem->GetParent();

			if ((pParent != nullptr) && (pParent->GetType() != ESystemItemType::Library))
			{
				return IndexFromItem(pParent);
			}
		}
	}

	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
bool CSystemLibraryModel::canDropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const
{
	if (parent.isValid())
	{
		CSystemAsset const* const pParent = static_cast<CSystemAsset*>(parent.internalPointer());
		CRY_ASSERT_MESSAGE(pParent != nullptr, "Parent is null pointer.");
		return CanDropMimeData(pData, *pParent);
	}
	else
	{
		CRY_ASSERT_MESSAGE(m_pLibrary != nullptr,"Library is null pointer.");
		return CanDropMimeData(pData, *m_pLibrary);
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CSystemLibraryModel::dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent)
{
	if (parent.isValid())
	{
		CSystemAsset* const pAsset = static_cast<CSystemAsset*>(parent.internalPointer());
		CRY_ASSERT_MESSAGE(pAsset != nullptr,"Asset is null pointer.");
		DropMimeData(pData, pAsset);
		return true;
	}
	else
	{
		CRY_ASSERT_MESSAGE(m_pLibrary != nullptr , "Library is null pointer.");
		DropMimeData(pData, m_pLibrary);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
QMimeData* CSystemLibraryModel::mimeData(QModelIndexList const& indexes) const
{
	CDragDropData* const pDragDropData = new CDragDropData();
	QByteArray byteArray;
	QDataStream stream(&byteArray, QIODevice::ReadWrite);

	QModelIndexList nameIndexes;

	for (QModelIndex const& index : indexes)
	{
		nameIndexes.append(index.sibling(index.row(), static_cast<int>(SystemModelUtils::EColumns::Name)));
	}

	for (auto const& index : nameIndexes)
	{
		stream << reinterpret_cast<intptr_t>(index.internalPointer());
	}

	pDragDropData->SetCustomData("AudioLibraryItems", byteArray);
	return pDragDropData;
}

//////////////////////////////////////////////////////////////////////////
Qt::DropActions CSystemLibraryModel::supportedDropActions() const
{
	return Qt::MoveAction | Qt::CopyAction;
}

//////////////////////////////////////////////////////////////////////////
QStringList CSystemLibraryModel::mimeTypes() const
{
	QStringList result;
	result << CDragDropData::GetMimeFormatForType("AudioLibraryItems");
	result << ModelUtils::s_szMiddlewareMimeType;
	return result;
}

//////////////////////////////////////////////////////////////////////////
CSystemFilterProxyModel::CSystemFilterProxyModel(QObject* const pParent)
	: QAttributeFilterProxyModel(QDeepFilterProxyModel::Behavior::AcceptIfChildMatches, pParent)
{
}

//////////////////////////////////////////////////////////////////////////
bool CSystemFilterProxyModel::rowMatchesFilter(int sourceRow, QModelIndex const& sourceParent) const
{
	if (QAttributeFilterProxyModel::rowMatchesFilter(sourceRow, sourceParent))
	{
		QModelIndex const& index = sourceModel()->index(sourceRow, 0, sourceParent);

		if (index.isValid())
		{
			CSystemAsset const* const pAsset = SystemModelUtils::GetAssetFromIndex(index, static_cast<int>(SystemModelUtils::EColumns::Name));

			if ((pAsset != nullptr) && pAsset->IsHiddenDefault())
			{
				// Hide default controls that should not be connected to any middleware control.
				return false;
			}

			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CSystemFilterProxyModel::lessThan(QModelIndex const& left, QModelIndex const& right) const
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
