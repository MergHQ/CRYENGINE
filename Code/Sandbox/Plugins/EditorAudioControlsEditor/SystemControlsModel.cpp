// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SystemControlsModel.h"

#include "AudioControlsEditorPlugin.h"
#include "SystemControlsIcons.h"
#include "ModelUtils.h"

#include <ImplItem.h>
#include <QtUtil.h>
#include <DragDrop.h>

#include <QApplication>

namespace ACE
{
namespace SystemModelUtils
{
//////////////////////////////////////////////////////////////////////////
CItemModelAttribute* GetAttributeForColumn(EColumns const column)
{
	CItemModelAttribute* pAttribute = nullptr;

	switch (column)
	{
	case EColumns::Notification:
		pAttribute = &ModelUtils::s_notificationAttribute;
		break;
	case EColumns::Type:
		pAttribute = &ModelUtils::s_typeAttribute;
		break;
	case EColumns::Placeholder:
		pAttribute = &ModelUtils::s_placeholderAttribute;
		break;
	case EColumns::NoConnection:
		pAttribute = &ModelUtils::s_connectedAttribute;
		break;
	case EColumns::NoControl:
		pAttribute = &ModelUtils::s_noControlAttribute;
		break;
	case EColumns::Scope:
		pAttribute = &ModelUtils::s_scopeAttribute;
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
QVariant GetHeaderData(int const section, Qt::Orientation const orientation, int const role)
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
				if (section != static_cast<int>(EColumns::Notification))
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
	CSystemAsset* pAsset = nullptr;
	QModelIndex const& nameColumnIndex = index.sibling(index.row(), column);
	QVariant const internalPtr = nameColumnIndex.data(static_cast<int>(ERoles::InternalPointer));

	if (internalPtr.isValid())
	{
		pAsset = reinterpret_cast<CSystemAsset*>(internalPtr.value<intptr_t>());
	}

	return pAsset;
}

//////////////////////////////////////////////////////////////////////////
QMimeData* GetDragDropData(QModelIndexList const& list)
{
	CDragDropData* const pDragDropData = new CDragDropData();
	QByteArray byteArray;
	QDataStream stream(&byteArray, QIODevice::ReadWrite);

	QModelIndexList nameIndexes;
	int const nameColumn = static_cast<int>(EColumns::Name);

	for (auto const& index : list)
	{
		nameIndexes.append(index.sibling(index.row(), nameColumn));
	}

	nameIndexes.erase(std::unique(nameIndexes.begin(), nameIndexes.end()), nameIndexes.end());

	for (auto const& index : nameIndexes)
	{
		stream << reinterpret_cast<intptr_t>(index.internalPointer());
	}

	pDragDropData->SetCustomData(ModelUtils::s_szSystemMimeType, byteArray);
	return pDragDropData;
}

//////////////////////////////////////////////////////////////////////////
void DecodeImplMimeData(const QMimeData* pData, std::vector<CImplItem*>& outItems)
{
	CDragDropData const* const pDragDropData = CDragDropData::FromMimeData(pData);

	if (pDragDropData->HasCustomData(ModelUtils::s_szImplMimeType))
	{
		IEditorImpl const* const pEditorImpl = CAudioControlsEditorPlugin::GetImplEditor();
		QByteArray const byteArray = pDragDropData->GetCustomData(ModelUtils::s_szImplMimeType);
		QDataStream stream(byteArray);

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
}
} // namespace SystemModelUtils

//////////////////////////////////////////////////////////////////////////
bool IsParentValid(CSystemAsset const& parent, ESystemItemType const type)
{
	bool isValid = false;

	switch (parent.GetType())
	{
	case ESystemItemType::Folder:
	case ESystemItemType::Library: // Intentional fall-through.
		isValid = (type != ESystemItemType::State);
		break;
	case ESystemItemType::Switch:
		isValid = (type == ESystemItemType::State) || (type == ESystemItemType::Parameter);
		break;
	default:
		isValid = false;
		break;
	}

	return isValid;
}

//////////////////////////////////////////////////////////////////////////
void DecodeMimeData(QMimeData const* const pData, std::vector<CSystemAsset*>& outItems)
{
	CDragDropData const* const pDragDropData = CDragDropData::FromMimeData(pData);

	if (pDragDropData->HasCustomData(ModelUtils::s_szSystemMimeType))
	{
		QByteArray const byteArray = pDragDropData->GetCustomData(ModelUtils::s_szSystemMimeType);
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
	bool canDrop = false;
	bool hasValidParent = true;

	// Handle first if mime data is an external (from the implementation side) source.
	std::vector<CImplItem*> implItems;
	SystemModelUtils::DecodeImplMimeData(pData, implItems);

	if (!implItems.empty())
	{
		IEditorImpl const* const pEditorImpl = CAudioControlsEditorPlugin::GetImplEditor();

		for (CImplItem const* const pImplItem : implItems)
		{
			if (!IsParentValid(parent, pEditorImpl->ImplTypeToSystemType(pImplItem)))
			{
				hasValidParent = false;
				break;
			}
		}

		if (hasValidParent)
		{
			canDrop = true;
		}
	}
	else
	{
		// Handle if mime data is an internal move (rearranging controls).
		std::vector<CSystemAsset*> droppedItems;
		DecodeMimeData(pData, droppedItems);

		for (auto const pItem : droppedItems)
		{
			if (!IsParentValid(parent, pItem->GetType()))
			{
				hasValidParent = false;
				break;
			}
			else
			{
				// Don't drop item on its current parent.
				CSystemAsset const* const pParent = pItem->GetParent();

				if (pParent != nullptr)
				{
					if (pParent == &parent)
					{
						hasValidParent = false;
						break;
					}
				}
			}
		}

		if (hasValidParent)
		{
			canDrop = true;
		}
	}

	return canDrop;
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
	, m_nameColumn(static_cast<int>(SystemModelUtils::EColumns::Name))
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
	CAudioControlsEditorPlugin::SignalAboutToLoad.Connect([&]()
		{
			beginResetModel();
			m_ignoreLibraryUpdates = true;
			endResetModel();
	  }, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::SignalLoaded.Connect([&]()
		{
			beginResetModel();
			m_ignoreLibraryUpdates = false;
			endResetModel();
	  }, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->SignalLibraryAboutToBeAdded.Connect([&]()
		{
			if (!m_ignoreLibraryUpdates)
			{
			  int const row = static_cast<int>(m_pAssetsManager->GetLibraryCount());
			  beginInsertRows(QModelIndex(), row, row);
			}
	  }, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->SignalLibraryAdded.Connect([&](CSystemLibrary* const pLibrary)
		{
			if (!m_ignoreLibraryUpdates)
			{
			  endInsertRows();
			}
	  }, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->SignalLibraryAboutToBeRemoved.Connect([&](CSystemLibrary* const pLibrary)
		{
			if (!m_ignoreLibraryUpdates)
			{
			  size_t const libCount = m_pAssetsManager->GetLibraryCount();

			  for (size_t i = 0; i < libCount; ++i)
			  {
			    if (m_pAssetsManager->GetLibrary(i) == pLibrary)
			    {
			      int const index = static_cast<int>(i);
			      beginRemoveRows(QModelIndex(), index, index);
			      break;
			    }
			  }
			}
	  }, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->SignalLibraryRemoved.Connect([&]()
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
	CAudioControlsEditorPlugin::SignalAboutToLoad.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::SignalLoaded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->SignalLibraryAboutToBeAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->SignalLibraryAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->SignalLibraryAboutToBeRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->SignalLibraryRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
int CSystemSourceModel::rowCount(QModelIndex const& parent) const
{
	int rowCount = 0;

	if (!m_ignoreLibraryUpdates && !parent.isValid())
	{
		rowCount = m_pAssetsManager->GetLibraryCount();
	}

	return rowCount;
}

//////////////////////////////////////////////////////////////////////////
int CSystemSourceModel::columnCount(QModelIndex const& parent) const
{
	return static_cast<int>(SystemModelUtils::EColumns::Count);
}

//////////////////////////////////////////////////////////////////////////
QVariant CSystemSourceModel::data(QModelIndex const& index, int role) const
{
	QVariant variant;
	CSystemLibrary const* const pLibrary = static_cast<CSystemLibrary*>(index.internalPointer());

	if (pLibrary != nullptr)
	{
		if (role == static_cast<int>(SystemModelUtils::ERoles::IsDefaultControl))
		{
			variant = pLibrary->IsDefaultControl();
		}
		else
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
							variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::Placeholder));
						}
						else if (!pLibrary->HasConnection())
						{
							variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::NoConnection));
						}
						else if (!pLibrary->HasControl())
						{
							variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::NoControl));
						}
						break;
					case Qt::ToolTipRole:
						if (pLibrary->HasPlaceholderConnection())
						{
							variant = tr("Contains item whose connection was not found in middleware project");
						}
						else if (!pLibrary->HasConnection())
						{
							variant = tr("Contains item that is not connected to any middleware control");
						}
						else if (!pLibrary->HasControl())
						{
							variant = tr("Contains no audio control");
						}
						break;
					}
				}
				break;
			case static_cast<int>(SystemModelUtils::EColumns::Placeholder):
				{
					if (role == Qt::CheckStateRole)
					{
						variant = (!pLibrary->HasPlaceholderConnection()) ? Qt::Checked : Qt::Unchecked;
					}
				}
				break;
			case static_cast<int>(SystemModelUtils::EColumns::NoConnection):
				{
					if (role == Qt::CheckStateRole)
					{
						variant = (pLibrary->HasConnection()) ? Qt::Checked : Qt::Unchecked;
					}
				}
				break;
			case static_cast<int>(SystemModelUtils::EColumns::NoControl):
				{
					if (role == Qt::CheckStateRole)
					{
						variant = (!pLibrary->HasControl()) ? Qt::Checked : Qt::Unchecked;
					}
				}
				break;
			case static_cast<int>(SystemModelUtils::EColumns::Name):
				{
					switch (role)
					{
					case Qt::DecorationRole:
						variant = GetItemTypeIcon(ESystemItemType::Library);
						break;
					case Qt::DisplayRole:

						if (pLibrary->IsModified())
						{
							variant = static_cast<char const*>(pLibrary->GetName() + " *");
						}
						else
						{
							variant = static_cast<char const*>(pLibrary->GetName());
						}
						break;
					case Qt::EditRole:
						variant = static_cast<char const*>(pLibrary->GetName());
						break;
					case Qt::ToolTipRole:
						if (!pLibrary->GetDescription().IsEmpty())
						{
							variant = tr(pLibrary->GetName() + ": " + pLibrary->GetDescription());
						}
						else
						{
							variant = static_cast<char const*>(pLibrary->GetName());
						}
						break;
					case static_cast<int>(SystemModelUtils::ERoles::ItemType):
						variant = static_cast<int>(ESystemItemType::Library);
						break;
					case static_cast<int>(SystemModelUtils::ERoles::Name):
						variant = static_cast<char const*>(pLibrary->GetName());
						break;
					case static_cast<int>(SystemModelUtils::ERoles::InternalPointer):
						variant = reinterpret_cast<intptr_t>(pLibrary);
						break;
					}
				}
				break;
			}
		}
	}

	return variant;
}

//////////////////////////////////////////////////////////////////////////
bool CSystemSourceModel::setData(QModelIndex const& index, QVariant const& value, int role)
{
	bool wasDataChanged = false;

	if (index.isValid() && (index.column() == m_nameColumn))
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

						wasDataChanged = true;
					}
				}
				break;
			default:
				CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, R"([Audio Controls Editor] The role '%d' is not handled!)", role);
				break;
			}
		}
	}

	return wasDataChanged;
}

//////////////////////////////////////////////////////////////////////////
QVariant CSystemSourceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return SystemModelUtils::GetHeaderData(section, orientation, role);
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CSystemSourceModel::flags(QModelIndex const& index) const
{
	Qt::ItemFlags flags = Qt::NoItemFlags;

	if (index.isValid())
	{
		if ((index.column() == m_nameColumn) && !(index.data(static_cast<int>(SystemModelUtils::ERoles::IsDefaultControl)).toBool()))
		{
			flags = QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
		}
		else
		{
			flags = QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled;
		}
	}

	return flags;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CSystemSourceModel::index(int row, int column, QModelIndex const& parent /*= QModelIndex()*/) const
{
	QModelIndex modelIndex = QModelIndex();

	if ((row >= 0) && (column >= 0))
	{
		modelIndex = createIndex(row, column, reinterpret_cast<quintptr>(m_pAssetsManager->GetLibrary(row)));
	}

	return modelIndex;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CSystemSourceModel::parent(QModelIndex const& index) const
{
	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
bool CSystemSourceModel::canDropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const
{
	bool canDrop = false;
	QString dragText = tr("Invalid operation");

	if (parent.isValid())
	{
		CSystemLibrary const* const pParent = static_cast<CSystemLibrary*>(parent.internalPointer());
		CRY_ASSERT_MESSAGE(pParent != nullptr, "Parent is null pointer.");

		if (pParent != nullptr)
		{
			canDrop = !pParent->IsDefaultControl() && CanDropMimeData(pData, *pParent);

			if (canDrop)
			{
				dragText = tr("Add to ") + QtUtil::ToQString(pParent->GetName());
			}
			else if (pParent->IsDefaultControl())
			{
				dragText = tr("Cannot add items to the default controls library.");
			}
		}
	}

	CDragDropData::ShowDragText(qApp->widgetAt(QCursor::pos()), dragText);
	return canDrop;
}

//////////////////////////////////////////////////////////////////////////
bool CSystemSourceModel::dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent)
{
	bool wasDropped = false;

	if (parent.isValid())
	{
		CSystemLibrary* const pLibrary = static_cast<CSystemLibrary*>(parent.internalPointer());
		CRY_ASSERT_MESSAGE(pLibrary != nullptr, "Library is null pointer.");

		if (pLibrary != nullptr)
		{
			DropMimeData(pData, pLibrary);
			wasDropped = true;
		}
	}

	return wasDropped;
}

//////////////////////////////////////////////////////////////////////////
Qt::DropActions CSystemSourceModel::supportedDropActions() const
{
	return Qt::MoveAction | Qt::CopyAction;
}

//////////////////////////////////////////////////////////////////////////
QStringList CSystemSourceModel::mimeTypes() const
{
	QStringList types;
	types << CDragDropData::GetMimeFormatForType(ModelUtils::s_szSystemMimeType);
	types << CDragDropData::GetMimeFormatForType(ModelUtils::s_szImplMimeType);
	return types;
}

/////////////////////////////////////////////////////////////////////////////////////////
CSystemLibraryModel::CSystemLibraryModel(CSystemAssetsManager* const pAssetsManager, CSystemLibrary* const pLibrary, QObject* const pParent)
	: QAbstractItemModel(pParent)
	, m_pAssetsManager(pAssetsManager)
	, m_pLibrary(pLibrary)
	, m_nameColumn(static_cast<int>(SystemModelUtils::EColumns::Name))
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
	m_pAssetsManager->SignalItemAboutToBeAdded.Connect([&](CSystemAsset* const pParent)
		{
			if (Utils::GetParentLibrary(pParent) == m_pLibrary)
			{
			  int const row = static_cast<int>(pParent->ChildCount());

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

	m_pAssetsManager->SignalItemAdded.Connect([&](CSystemAsset* const pAsset)
		{
			if (Utils::GetParentLibrary(pAsset) == m_pLibrary)
			{
			  endInsertRows();
			}
	  }, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->SignalItemAboutToBeRemoved.Connect([&](CSystemAsset* const pAsset)
		{
			if (Utils::GetParentLibrary(pAsset) == m_pLibrary)
			{
			  CSystemAsset const* const pParent = pAsset->GetParent();

			  if (pParent != nullptr)
			  {
			    size_t const childCount = pParent->ChildCount();

			    for (size_t i = 0; i < childCount; ++i)
			    {
			      if (pParent->GetChild(i) == pAsset)
			      {
			        int const index = static_cast<int>(i);

			        if (pParent->GetType() == ESystemItemType::Library)
			        {
			          CRY_ASSERT_MESSAGE(pParent == m_pLibrary, "Parent is not the current library.");
			          beginRemoveRows(QModelIndex(), index, index);
			        }
			        else
			        {
			          QModelIndex const& parent = IndexFromItem(pParent);
			          beginRemoveRows(parent, index, index);
			        }

			        break;
			      }
			    }
			  }
			}
	  }, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->SignalItemRemoved.Connect([&](CSystemAsset* const pParent, CSystemAsset* const pAsset)
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
	m_pAssetsManager->SignalItemAboutToBeAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->SignalItemAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->SignalItemAboutToBeRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->SignalItemRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CSystemLibraryModel::IndexFromItem(CSystemAsset const* const pItem) const
{
	QModelIndex modelIndex = QModelIndex();

	if (pItem != nullptr)
	{
		CSystemAsset const* const pParent = pItem->GetParent();

		if (pParent != nullptr)
		{
			size_t const size = pParent->ChildCount();

			for (size_t i = 0; i < size; ++i)
			{
				if (pParent->GetChild(i) == pItem)
				{
					modelIndex = createIndex(static_cast<int>(i), 0, reinterpret_cast<quintptr>(pItem));
					break;
				}
			}
		}
	}

	return modelIndex;
}

//////////////////////////////////////////////////////////////////////////
int CSystemLibraryModel::rowCount(QModelIndex const& parent) const
{
	int rowCount = 0;

	if (!parent.isValid())
	{
		rowCount = static_cast<int>(m_pLibrary->ChildCount());
	}
	else
	{
		CSystemAsset const* const pAsset = static_cast<CSystemAsset*>(parent.internalPointer());

		if (pAsset != nullptr)
		{
			rowCount = static_cast<int>(pAsset->ChildCount());
		}
	}

	return rowCount;
}

//////////////////////////////////////////////////////////////////////////
int CSystemLibraryModel::columnCount(QModelIndex const& parent) const
{
	return static_cast<int>(SystemModelUtils::EColumns::Count);
}

//////////////////////////////////////////////////////////////////////////
QVariant CSystemLibraryModel::data(QModelIndex const& index, int role) const
{
	QVariant variant;

	if (index.isValid())
	{
		CSystemAsset* const pItem = static_cast<CSystemAsset*>(index.internalPointer());

		if (pItem != nullptr)
		{
			if (role == static_cast<int>(SystemModelUtils::ERoles::IsDefaultControl))
			{
				variant = pItem->IsDefaultControl();
			}
			else
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
								variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::Placeholder));
							}
							else if (!pItem->HasConnection())
							{
								variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::NoConnection));
							}
							else if (((itemType == ESystemItemType::Folder) || (itemType == ESystemItemType::Switch)) && !pItem->HasControl())
							{
								variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::NoControl));
							}
							break;
						case Qt::ToolTipRole:
							if (pItem->HasPlaceholderConnection())
							{
								if ((itemType == ESystemItemType::Folder) || (itemType == ESystemItemType::Switch))
								{
									variant = tr("Contains item whose connection was not found in middleware project");
								}
								else
								{
									variant = tr("Item connection was not found in middleware project");
								}
							}
							else if (!pItem->HasConnection())
							{
								if ((itemType == ESystemItemType::Folder) || (itemType == ESystemItemType::Switch))
								{
									variant = tr("Contains item that is not connected to any middleware control");
								}
								else
								{
									variant = tr("Item is not connected to any middleware control");
								}
							}
							else if ((itemType == ESystemItemType::Folder) && !pItem->HasControl())
							{
								variant = tr("Contains no audio control");
							}
							else if ((itemType == ESystemItemType::Switch) && !pItem->HasControl())
							{
								variant = tr("Contains no state");
							}
							break;
						case static_cast<int>(SystemModelUtils::ERoles::Id):
							if (itemType != ESystemItemType::Folder)
							{
								CSystemControl const* const pControl = static_cast<CSystemControl*>(pItem);

								if (pControl != nullptr)
								{
									variant = pControl->GetId();
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
							variant = QString(pItem->GetTypeName());
						}
					}
					break;
				case static_cast<int>(SystemModelUtils::EColumns::Placeholder):
					{
						if (role == Qt::CheckStateRole)
						{
							variant = (!pItem->HasPlaceholderConnection()) ? Qt::Checked : Qt::Unchecked;
						}
					}
					break;
				case static_cast<int>(SystemModelUtils::EColumns::NoConnection):
					{
						if (role == Qt::CheckStateRole)
						{
							variant = (pItem->HasConnection()) ? Qt::Checked : Qt::Unchecked;
						}
					}
					break;
				case static_cast<int>(SystemModelUtils::EColumns::NoControl):
					{
						if ((role == Qt::CheckStateRole) && ((itemType == ESystemItemType::Folder) || (itemType == ESystemItemType::Switch)))
						{
							variant = (!pItem->HasControl()) ? Qt::Checked : Qt::Unchecked;
						}
					}
					break;
				case static_cast<int>(SystemModelUtils::EColumns::Scope):
					if ((role == Qt::DisplayRole) && (itemType != ESystemItemType::Folder))
					{
						CSystemControl const* const pControl = static_cast<CSystemControl*>(pItem);

						if (pControl != nullptr)
						{
							variant = QString(m_pAssetsManager->GetScopeInfo(pControl->GetScope()).name);
						}
					}
					break;
				case static_cast<int>(SystemModelUtils::EColumns::Name):
					{
						switch (role)
						{
						case Qt::DecorationRole:
							variant = GetItemTypeIcon(itemType);
							break;
						case Qt::DisplayRole:
							if (pItem->IsModified())
							{
								variant = static_cast<char const*>(pItem->GetName() + " *");
							}
							else
							{
								variant = static_cast<char const*>(pItem->GetName());
							}
							break;
						case Qt::EditRole:
							variant = static_cast<char const*>(pItem->GetName());
							break;
						case Qt::ToolTipRole:
							if (!pItem->GetDescription().IsEmpty())
							{
								variant = tr(pItem->GetName() + ": " + pItem->GetDescription());
							}
							else
							{
								variant = static_cast<char const*>(pItem->GetName());
							}
							break;
						case static_cast<int>(SystemModelUtils::ERoles::ItemType):
							variant = static_cast<int>(itemType);
							break;
						case static_cast<int>(SystemModelUtils::ERoles::Name):
							variant = static_cast<char const*>(pItem->GetName());
							break;
						case static_cast<int>(SystemModelUtils::ERoles::InternalPointer):
							variant = reinterpret_cast<intptr_t>(pItem);
							break;
						}
					}
					break;
				}
			}
		}
	}

	return variant;
}

//////////////////////////////////////////////////////////////////////////
bool CSystemLibraryModel::setData(QModelIndex const& index, QVariant const& value, int role)
{
	bool wasDataChanged = false;

	if (index.isValid() && (index.column() == m_nameColumn))
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
							case ESystemItemType::Switch:
							case ESystemItemType::Trigger:
							case ESystemItemType::Environment:
								pItem->SetName(Utils::GenerateUniqueControlName(newName, itemType, *m_pAssetsManager));
								break;
							case ESystemItemType::State:
							case ESystemItemType::Folder:
								pItem->SetName(Utils::GenerateUniqueName(newName, itemType, pItem->GetParent()));
								break;
							default:
								CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, R"([Audio Controls Editor] The item type '%d' is not handled!)", itemType);
								break;
							}
						}

						wasDataChanged = true;
					}
				}
				break;
			default:
				CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, R"([Audio Controls Editor] The role '%d' is not handled!)", role);
				break;
			}
		}
	}

	return wasDataChanged;
}

//////////////////////////////////////////////////////////////////////////
QVariant CSystemLibraryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return SystemModelUtils::GetHeaderData(section, orientation, role);
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CSystemLibraryModel::flags(QModelIndex const& index) const
{
	Qt::ItemFlags flags = Qt::NoItemFlags;

	if (index.isValid())
	{
		if (index.data(static_cast<int>(SystemModelUtils::ERoles::IsDefaultControl)).toBool())
		{
			flags = QAbstractItemModel::flags(index) | Qt::ItemIsSelectable;
		}
		else if (index.column() == m_nameColumn)
		{
			flags = QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
		}
		else
		{
			flags = QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;
		}
	}

	return flags;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CSystemLibraryModel::index(int row, int column, QModelIndex const& parent) const
{
	QModelIndex modelIndex = QModelIndex();

	if ((row >= 0) && (column >= 0))
	{
		size_t const childIndex = static_cast<size_t>(row);

		if (parent.isValid())
		{
			CSystemAsset const* const pParent = static_cast<CSystemAsset*>(parent.internalPointer());

			if ((pParent != nullptr) && (childIndex < pParent->ChildCount()))
			{
				CSystemAsset const* const pChild = pParent->GetChild(childIndex);

				if (pChild != nullptr)
				{
					modelIndex = createIndex(row, column, reinterpret_cast<quintptr>(pChild));
				}
			}
		}
		else if (childIndex < m_pLibrary->ChildCount())
		{
			modelIndex = createIndex(row, column, reinterpret_cast<quintptr>(m_pLibrary->GetChild(childIndex)));
		}
	}

	return modelIndex;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CSystemLibraryModel::parent(QModelIndex const& index) const
{
	QModelIndex modelIndex = QModelIndex();

	if (index.isValid())
	{
		CSystemAsset const* const pItem = static_cast<CSystemAsset*>(index.internalPointer());

		if (pItem != nullptr)
		{
			CSystemAsset const* const pParent = pItem->GetParent();

			if ((pParent != nullptr) && (pParent->GetType() != ESystemItemType::Library))
			{
				modelIndex = IndexFromItem(pParent);
			}
		}
	}

	return modelIndex;
}

//////////////////////////////////////////////////////////////////////////
bool CSystemLibraryModel::canDropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const
{
	bool canDrop = false;
	QString dragText = tr("Invalid operation");

	if (parent.isValid())
	{
		CSystemAsset const* const pParent = static_cast<CSystemAsset*>(parent.internalPointer());
		CRY_ASSERT_MESSAGE(pParent != nullptr, "Parent is null pointer.");

		if (pParent != nullptr)
		{
			canDrop = CanDropMimeData(pData, *pParent);

			if (canDrop)
			{
				dragText = tr("Add to ") + QtUtil::ToQString(pParent->GetName());
			}
		}
	}
	else
	{
		CRY_ASSERT_MESSAGE(m_pLibrary != nullptr, "Library is null pointer.");

		if (m_pLibrary != nullptr)
		{
			canDrop = !m_pLibrary->IsDefaultControl() && CanDropMimeData(pData, *m_pLibrary);

			if (canDrop)
			{
				dragText = tr("Add to ") + QtUtil::ToQString(m_pLibrary->GetName());
			}
			else if (m_pLibrary->IsDefaultControl())
			{
				dragText = tr("Cannot add items to the default controls library.");
			}
		}
	}

	CDragDropData::ShowDragText(qApp->widgetAt(QCursor::pos()), dragText);
	return canDrop;
}

//////////////////////////////////////////////////////////////////////////
bool CSystemLibraryModel::dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent)
{
	bool wasDropped = false;

	if (parent.isValid())
	{
		CSystemAsset* const pAsset = static_cast<CSystemAsset*>(parent.internalPointer());
		CRY_ASSERT_MESSAGE(pAsset != nullptr, "Asset is null pointer.");
		DropMimeData(pData, pAsset);
		wasDropped = true;
	}
	else
	{
		CRY_ASSERT_MESSAGE(m_pLibrary != nullptr, "Library is null pointer.");
		DropMimeData(pData, m_pLibrary);
		wasDropped = true;
	}

	return wasDropped;
}

//////////////////////////////////////////////////////////////////////////
QMimeData* CSystemLibraryModel::mimeData(QModelIndexList const& indexes) const
{
	return SystemModelUtils::GetDragDropData(indexes);
}

//////////////////////////////////////////////////////////////////////////
Qt::DropActions CSystemLibraryModel::supportedDropActions() const
{
	return Qt::MoveAction | Qt::CopyAction;
}

//////////////////////////////////////////////////////////////////////////
QStringList CSystemLibraryModel::mimeTypes() const
{
	QStringList types = QAbstractItemModel::mimeTypes();
	types << CDragDropData::GetMimeFormatForType(ModelUtils::s_szSystemMimeType);
	types << CDragDropData::GetMimeFormatForType(ModelUtils::s_szImplMimeType);
	return types;
}

//////////////////////////////////////////////////////////////////////////
CSystemFilterProxyModel::CSystemFilterProxyModel(QObject* const pParent)
	: QAttributeFilterProxyModel(QDeepFilterProxyModel::Behavior::AcceptIfChildMatches, pParent)
{
}

//////////////////////////////////////////////////////////////////////////
bool CSystemFilterProxyModel::rowMatchesFilter(int sourceRow, QModelIndex const& sourceParent) const
{
	bool matchesFilter = false;

	if (QAttributeFilterProxyModel::rowMatchesFilter(sourceRow, sourceParent))
	{
		QModelIndex const& index = sourceModel()->index(sourceRow, 0, sourceParent);

		if (index.isValid())
		{
			CSystemAsset const* const pAsset = SystemModelUtils::GetAssetFromIndex(index, static_cast<int>(SystemModelUtils::EColumns::Name));

			if ((pAsset != nullptr) && pAsset->IsInternalControl())
			{
				// Hide internal controls.
				matchesFilter = false;
			}
			else
			{
				matchesFilter = true;
			}
		}
	}

	return matchesFilter;
}

//////////////////////////////////////////////////////////////////////////
bool CSystemFilterProxyModel::lessThan(QModelIndex const& left, QModelIndex const& right) const
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
