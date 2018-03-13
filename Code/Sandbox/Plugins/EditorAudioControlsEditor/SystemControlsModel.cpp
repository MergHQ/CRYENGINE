// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SystemControlsModel.h"

#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"
#include "AssetIcons.h"
#include "ModelUtils.h"

#include <IImplItem.h>
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
	case EColumns::PakStatus:
		pAttribute = &ModelUtils::s_pakStatus;
		break;
	case EColumns::InPak:
		pAttribute = &ModelUtils::s_inPakAttribute;
		break;
	case EColumns::OnDisk:
		pAttribute = &ModelUtils::s_onDiskAttribute;
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
void GetAssetsFromIndexesSeparated(QModelIndexList const& list, std::vector<CLibrary*>& outLibraries, std::vector<CFolder*>& outFolders, std::vector<CControl*>& outControls)
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
			case static_cast<int>(EAssetType::Library):
				outLibraries.push_back(reinterpret_cast<CLibrary*>(internalPtr.value<intptr_t>()));
				break;
			case static_cast<int>(EAssetType::Folder):
				outFolders.push_back(reinterpret_cast<CFolder*>(internalPtr.value<intptr_t>()));
				break;
			default:
				outControls.push_back(reinterpret_cast<CControl*>(internalPtr.value<intptr_t>()));
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void GetAssetsFromIndexesCombined(QModelIndexList const& list, std::vector<CAsset*>& outAssets)
{
	for (auto const& index : list)
	{
		QModelIndex const& nameColumnIndex = index.sibling(index.row(), static_cast<int>(EColumns::Name));
		QVariant const internalPtr = nameColumnIndex.data(static_cast<int>(ERoles::InternalPointer));

		if (internalPtr.isValid())
		{
			outAssets.push_back(reinterpret_cast<CAsset*>(internalPtr.value<intptr_t>()));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CAsset* GetAssetFromIndex(QModelIndex const& index, int const column)
{
	CAsset* pAsset = nullptr;
	QModelIndex const& nameColumnIndex = index.sibling(index.row(), column);
	QVariant const internalPtr = nameColumnIndex.data(static_cast<int>(ERoles::InternalPointer));

	if (internalPtr.isValid())
	{
		pAsset = reinterpret_cast<CAsset*>(internalPtr.value<intptr_t>());
	}

	return pAsset;
}

//////////////////////////////////////////////////////////////////////////
QMimeData* GetDragDropData(QModelIndexList const& list)
{
	auto const pDragDropData = new CDragDropData();
	QByteArray byteArray;
	QDataStream stream(&byteArray, QIODevice::ReadWrite);

	QModelIndexList nameIndexes;
	auto const nameColumn = static_cast<int>(EColumns::Name);

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
void DecodeImplMimeData(const QMimeData* pData, std::vector<IImplItem*>& outItems)
{
	CDragDropData const* const pDragDropData = CDragDropData::FromMimeData(pData);

	if (pDragDropData->HasCustomData(ModelUtils::s_szImplMimeType))
	{
		QByteArray const byteArray = pDragDropData->GetCustomData(ModelUtils::s_szImplMimeType);
		QDataStream stream(byteArray);

		while (!stream.atEnd())
		{
			ControlId id;
			stream >> id;

			if (id != s_aceInvalidId)
			{
				IImplItem* const pIImplItem = g_pEditorImpl->GetItem(id);

				if (pIImplItem != nullptr)
				{
					outItems.push_back(pIImplItem);
				}
			}
		}
	}
}
} // namespace SystemModelUtils

//////////////////////////////////////////////////////////////////////////
bool IsParentValid(CAsset const& parent, EAssetType const type)
{
	bool isValid = false;

	switch (parent.GetType())
	{
	case EAssetType::Folder:
	case EAssetType::Library: // Intentional fall-through.
		isValid = (type != EAssetType::State);
		break;
	case EAssetType::Switch:
		isValid = (type == EAssetType::State) || (type == EAssetType::Parameter);
		break;
	default:
		isValid = false;
		break;
	}

	return isValid;
}

//////////////////////////////////////////////////////////////////////////
void DecodeMimeData(QMimeData const* const pData, std::vector<CAsset*>& outItems)
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
			outItems.push_back(reinterpret_cast<CAsset*>(ptr));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CanDropMimeData(QMimeData const* const pData, CAsset const& parent)
{
	bool canDrop = false;
	bool hasValidParent = true;

	// Handle first if mime data is an external (from the implementation side) source.
	std::vector<IImplItem*> implItems;
	SystemModelUtils::DecodeImplMimeData(pData, implItems);

	if (!implItems.empty())
	{
		for (IImplItem const* const pIImplItem : implItems)
		{
			if (!IsParentValid(parent, g_pEditorImpl->ImplTypeToAssetType(pIImplItem)))
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
		std::vector<CAsset*> droppedAssets;
		DecodeMimeData(pData, droppedAssets);

		for (auto const pAsset : droppedAssets)
		{
			if (!IsParentValid(parent, pAsset->GetType()))
			{
				hasValidParent = false;
				break;
			}
			else
			{
				// Don't drop item on its current parent.
				CAsset const* const pParent = pAsset->GetParent();

				if ((pParent != nullptr) && (pParent == &parent))
				{
					hasValidParent = false;
					break;
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
void DropMimeData(QMimeData const* const pData, CAsset* const pParent)
{
	// Handle first if mime data is an external (from the implementation side) source
	std::vector<IImplItem*> implItems;
	SystemModelUtils::DecodeImplMimeData(pData, implItems);

	if (!implItems.empty())
	{
		for (auto const pIImplItem : implItems)
		{
			g_assetsManager.CreateAndConnectImplItems(pIImplItem, pParent);
		}
	}
	else
	{
		std::vector<CAsset*> droppedItems;
		DecodeMimeData(pData, droppedItems);
		std::vector<CAsset*> topLevelDroppedItems;
		Utils::SelectTopLevelAncestors(droppedItems, topLevelDroppedItems);
		g_assetsManager.MoveItems(pParent, topLevelDroppedItems);
	}
}

//////////////////////////////////////////////////////////////////////////
inline char const* GetPakStatusIcon(CLibrary const& library)
{
	char const* szIconPath = "icons:Dialogs/dialog-error.ico";

	if (library.IsExactPakStatus(EPakStatus::InPak | EPakStatus::OnDisk))
	{
		szIconPath = "icons:General/Pakfile_Folder.ico";
	}
	else if (library.IsExactPakStatus(EPakStatus::InPak))
	{
		szIconPath = "icons:General/Pakfile.ico";
	}
	else if (library.IsExactPakStatus(EPakStatus::OnDisk))
	{
		szIconPath = "icons:General/Folder.ico";
	}

	return szIconPath;
}

//////////////////////////////////////////////////////////////////////////
CSystemSourceModel::CSystemSourceModel(QObject* const pParent)
	: QAbstractItemModel(pParent)
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

	g_assetsManager.SignalLibraryAboutToBeAdded.Connect([this]()
		{
			if (!m_ignoreLibraryUpdates)
			{
			  int const row = static_cast<int>(g_assetsManager.GetLibraryCount());
			  beginInsertRows(QModelIndex(), row, row);
			}
	  }, reinterpret_cast<uintptr_t>(this));

	g_assetsManager.SignalLibraryAdded.Connect([this](CLibrary* const pLibrary)
		{
			if (!m_ignoreLibraryUpdates)
			{
			  endInsertRows();
			}
	  }, reinterpret_cast<uintptr_t>(this));

	g_assetsManager.SignalLibraryAboutToBeRemoved.Connect([this](CLibrary* const pLibrary)
		{
			if (!m_ignoreLibraryUpdates)
			{
			  size_t const libCount = g_assetsManager.GetLibraryCount();

			  for (size_t i = 0; i < libCount; ++i)
			  {
			    if (g_assetsManager.GetLibrary(i) == pLibrary)
			    {
			      int const index = static_cast<int>(i);
			      beginRemoveRows(QModelIndex(), index, index);
			      break;
			    }
			  }
			}
	  }, reinterpret_cast<uintptr_t>(this));

	g_assetsManager.SignalLibraryRemoved.Connect([this]()
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
	g_assetsManager.SignalLibraryAboutToBeAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	g_assetsManager.SignalLibraryAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	g_assetsManager.SignalLibraryAboutToBeRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	g_assetsManager.SignalLibraryRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
int CSystemSourceModel::rowCount(QModelIndex const& parent) const
{
	int rowCount = 0;

	if (!m_ignoreLibraryUpdates && !parent.isValid())
	{
		rowCount = g_assetsManager.GetLibraryCount();
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
	CLibrary const* const pLibrary = static_cast<CLibrary*>(index.internalPointer());

	if (pLibrary != nullptr)
	{
		if (role == static_cast<int>(SystemModelUtils::ERoles::IsDefaultControl))
		{
			variant = pLibrary->IsDefaultControl();
		}
		else if (role == static_cast<int>(SystemModelUtils::ERoles::Name))
		{
			variant = static_cast<char const*>(pLibrary->GetName());
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
			case static_cast<int>(SystemModelUtils::EColumns::PakStatus):
				{
					switch (role)
					{
					case Qt::DecorationRole:
						variant = CryIcon(GetPakStatusIcon(*pLibrary));
						break;
					case Qt::ToolTipRole:
						{
							if (pLibrary->IsExactPakStatus(EPakStatus::InPak | EPakStatus::OnDisk))
							{
								variant = "Library is in pak and on disk";
							}
							else if (pLibrary->IsExactPakStatus(EPakStatus::InPak))
							{
								variant = "Library is only in pak file";
							}
							else if (pLibrary->IsExactPakStatus(EPakStatus::OnDisk))
							{
								variant = "Library is only on disk";
							}
							else
							{
								variant = "Library does not exist on disk. Save to write file.";
							}
						}
						break;
					}
				}
				break;
			case static_cast<int>(SystemModelUtils::EColumns::InPak):
				{
					if (role == Qt::CheckStateRole)
					{
						variant = pLibrary->IsAnyPakStatus(EPakStatus::InPak) ? Qt::Checked : Qt::Unchecked;
					}
				}
				break;
			case static_cast<int>(SystemModelUtils::EColumns::OnDisk):
				{
					if (role == Qt::CheckStateRole)
					{
						variant = pLibrary->IsAnyPakStatus(EPakStatus::OnDisk) ? Qt::Checked : Qt::Unchecked;
					}
				}
				break;
			case static_cast<int>(SystemModelUtils::EColumns::Name):
				{
					switch (role)
					{
					case Qt::DecorationRole:
						variant = GetAssetIcon(EAssetType::Library);
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
						{
							if (!pLibrary->GetDescription().IsEmpty())
							{
								variant = tr(pLibrary->GetName() + ": " + pLibrary->GetDescription());
							}
							else
							{
								variant = static_cast<char const*>(pLibrary->GetName());
							}
						}
						break;
					case static_cast<int>(SystemModelUtils::ERoles::ItemType):
						variant = static_cast<int>(EAssetType::Library);
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
		auto const pAsset = static_cast<CAsset*>(index.internalPointer());

		if (pAsset != nullptr)
		{
			switch (role)
			{
			case Qt::EditRole:
				{
					if (value.canConvert<QString>())
					{
						string const& oldName = pAsset->GetName();
						string const& newName = QtUtil::ToString(value.toString());

						if (!newName.empty() && newName.compareNoCase(oldName) != 0)
						{
							pAsset->SetName(Utils::GenerateUniqueLibraryName(newName));
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
		modelIndex = createIndex(row, column, reinterpret_cast<quintptr>(g_assetsManager.GetLibrary(row)));
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
		CLibrary const* const pParent = static_cast<CLibrary*>(parent.internalPointer());
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
		auto const pLibrary = static_cast<CLibrary*>(parent.internalPointer());
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
	return static_cast<Qt::DropActions>(Qt::MoveAction | Qt::CopyAction);
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
CSystemLibraryModel::CSystemLibraryModel(CLibrary* const pLibrary, QObject* const pParent)
	: QAbstractItemModel(pParent)
	, m_pLibrary(pLibrary)
	, m_nameColumn(static_cast<int>(SystemModelUtils::EColumns::Name))
{
	CRY_ASSERT_MESSAGE(pLibrary != nullptr, "Library is null pointer.");
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
	g_assetsManager.SignalItemAboutToBeAdded.Connect([this](CAsset* const pParent)
		{
			if (Utils::GetParentLibrary(pParent) == m_pLibrary)
			{
			  int const row = static_cast<int>(pParent->ChildCount());

			  if (pParent->GetType() == EAssetType::Library)
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

	g_assetsManager.SignalItemAdded.Connect([this](CAsset* const pAsset)
		{
			if (Utils::GetParentLibrary(pAsset) == m_pLibrary)
			{
			  endInsertRows();
			}
	  }, reinterpret_cast<uintptr_t>(this));

	g_assetsManager.SignalItemAboutToBeRemoved.Connect([this](CAsset* const pAsset)
		{
			if (Utils::GetParentLibrary(pAsset) == m_pLibrary)
			{
			  CAsset const* const pParent = pAsset->GetParent();

			  if (pParent != nullptr)
			  {
			    size_t const childCount = pParent->ChildCount();

			    for (size_t i = 0; i < childCount; ++i)
			    {
			      if (pParent->GetChild(i) == pAsset)
			      {
			        int const index = static_cast<int>(i);

			        if (pParent->GetType() == EAssetType::Library)
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

	g_assetsManager.SignalItemRemoved.Connect([this](CAsset* const pParent, CAsset* const pAsset)
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
	g_assetsManager.SignalItemAboutToBeAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	g_assetsManager.SignalItemAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	g_assetsManager.SignalItemAboutToBeRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	g_assetsManager.SignalItemRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CSystemLibraryModel::IndexFromItem(CAsset const* const pAsset) const
{
	QModelIndex modelIndex = QModelIndex();

	if (pAsset != nullptr)
	{
		CAsset const* const pParent = pAsset->GetParent();

		if (pParent != nullptr)
		{
			size_t const size = pParent->ChildCount();

			for (size_t i = 0; i < size; ++i)
			{
				if (pParent->GetChild(i) == pAsset)
				{
					modelIndex = createIndex(static_cast<int>(i), 0, reinterpret_cast<quintptr>(pAsset));
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
		CAsset const* const pAsset = static_cast<CAsset*>(parent.internalPointer());

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
		auto const pAsset = static_cast<CAsset*>(index.internalPointer());

		if (pAsset != nullptr)
		{
			if (role == static_cast<int>(SystemModelUtils::ERoles::IsDefaultControl))
			{
				variant = pAsset->IsDefaultControl();
			}
			else if (role == static_cast<int>(SystemModelUtils::ERoles::Name))
			{
				variant = static_cast<char const*>(pAsset->GetName());
			}
			else
			{
				EAssetType const itemType = pAsset->GetType();

				switch (index.column())
				{
				case static_cast<int>(SystemModelUtils::EColumns::Notification):
					{
						switch (role)
						{
						case Qt::DecorationRole:
							if (pAsset->HasPlaceholderConnection())
							{
								variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::Placeholder));
							}
							else if (!pAsset->HasConnection())
							{
								variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::NoConnection));
							}
							else if (((itemType == EAssetType::Folder) || (itemType == EAssetType::Switch)) && !pAsset->HasControl())
							{
								variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::NoControl));
							}
							break;
						case Qt::ToolTipRole:
							if (pAsset->HasPlaceholderConnection())
							{
								if ((itemType == EAssetType::Folder) || (itemType == EAssetType::Switch))
								{
									variant = tr("Contains item whose connection was not found in middleware project");
								}
								else
								{
									variant = tr("Item connection was not found in middleware project");
								}
							}
							else if (!pAsset->HasConnection())
							{
								if ((itemType == EAssetType::Folder) || (itemType == EAssetType::Switch))
								{
									variant = tr("Contains item that is not connected to any middleware control");
								}
								else
								{
									variant = tr("Item is not connected to any middleware control");
								}
							}
							else if ((itemType == EAssetType::Folder) && !pAsset->HasControl())
							{
								variant = tr("Contains no audio control");
							}
							else if ((itemType == EAssetType::Switch) && !pAsset->HasControl())
							{
								variant = tr("Contains no state");
							}
							break;
						case static_cast<int>(SystemModelUtils::ERoles::Id):
							if (itemType != EAssetType::Folder)
							{
								CControl const* const pControl = static_cast<CControl*>(pAsset);

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
						if ((role == Qt::DisplayRole) && (itemType != EAssetType::Folder))
						{
							variant = QString(pAsset->GetTypeName());
						}
					}
					break;
				case static_cast<int>(SystemModelUtils::EColumns::Placeholder):
					{
						if (role == Qt::CheckStateRole)
						{
							variant = (!pAsset->HasPlaceholderConnection()) ? Qt::Checked : Qt::Unchecked;
						}
					}
					break;
				case static_cast<int>(SystemModelUtils::EColumns::NoConnection):
					{
						if (role == Qt::CheckStateRole)
						{
							variant = (pAsset->HasConnection()) ? Qt::Checked : Qt::Unchecked;
						}
					}
					break;
				case static_cast<int>(SystemModelUtils::EColumns::NoControl):
					{
						if ((role == Qt::CheckStateRole) && ((itemType == EAssetType::Folder) || (itemType == EAssetType::Switch)))
						{
							variant = (!pAsset->HasControl()) ? Qt::Checked : Qt::Unchecked;
						}
					}
					break;
				case static_cast<int>(SystemModelUtils::EColumns::PakStatus):
					{
						switch (role)
						{
						case Qt::DecorationRole:
							variant = CryIcon(GetPakStatusIcon(*m_pLibrary));
							break;
						case Qt::ToolTipRole:
							{
								if (m_pLibrary->IsExactPakStatus(EPakStatus::InPak | EPakStatus::OnDisk))
								{
									variant = "Parent library is in pak and on disk";
								}
								else if (m_pLibrary->IsExactPakStatus(EPakStatus::InPak))
								{
									variant = "Parent library is only in pak file";
								}
								else if (m_pLibrary->IsExactPakStatus(EPakStatus::OnDisk))
								{
									variant = "Parent library is only on disk";
								}
								else
								{
									variant = "Parent library does not exist on disk. Save to write file.";
								}
							}
							break;
						}
					}
					break;
				case static_cast<int>(SystemModelUtils::EColumns::InPak):
					{
						if (role == Qt::CheckStateRole)
						{
							variant = m_pLibrary->IsAnyPakStatus(EPakStatus::InPak) ? Qt::Checked : Qt::Unchecked;
						}
					}
					break;
				case static_cast<int>(SystemModelUtils::EColumns::OnDisk):
					{
						if (role == Qt::CheckStateRole)
						{
							variant = m_pLibrary->IsAnyPakStatus(EPakStatus::OnDisk) ? Qt::Checked : Qt::Unchecked;
						}
					}
					break;
				case static_cast<int>(SystemModelUtils::EColumns::Scope):
					if ((role == Qt::DisplayRole) && (itemType != EAssetType::Folder))
					{
						CControl const* const pControl = static_cast<CControl*>(pAsset);

						if (pControl != nullptr)
						{
							variant = QString(g_assetsManager.GetScopeInfo(pControl->GetScope()).name);
						}
					}
					break;
				case static_cast<int>(SystemModelUtils::EColumns::Name):
					{
						switch (role)
						{
						case Qt::DecorationRole:
							variant = GetAssetIcon(itemType);
							break;
						case Qt::DisplayRole:
							if (pAsset->IsModified())
							{
								variant = static_cast<char const*>(pAsset->GetName() + " *");
							}
							else
							{
								variant = static_cast<char const*>(pAsset->GetName());
							}
							break;
						case Qt::EditRole:
							variant = static_cast<char const*>(pAsset->GetName());
							break;
						case Qt::ToolTipRole:
							if (!pAsset->GetDescription().IsEmpty())
							{
								variant = tr(pAsset->GetName() + ": " + pAsset->GetDescription());
							}
							else
							{
								variant = static_cast<char const*>(pAsset->GetName());
							}
							break;
						case static_cast<int>(SystemModelUtils::ERoles::ItemType):
							variant = static_cast<int>(itemType);
							break;
						case static_cast<int>(SystemModelUtils::ERoles::InternalPointer):
							variant = reinterpret_cast<intptr_t>(pAsset);
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
		auto const pAsset = static_cast<CAsset*>(index.internalPointer());

		if (pAsset != nullptr)
		{
			switch (role)
			{
			case Qt::EditRole:
				{
					if (value.canConvert<QString>())
					{
						string const& oldName = pAsset->GetName();
						string const& newName = QtUtil::ToString(value.toString());

						if (!newName.empty() && newName.compareNoCase(oldName) != 0)
						{
							EAssetType const itemType = pAsset->GetType();

							switch (itemType)
							{
							case EAssetType::Preload:
							case EAssetType::Parameter:
							case EAssetType::Switch:
							case EAssetType::Trigger:
							case EAssetType::Environment:
								pAsset->SetName(Utils::GenerateUniqueControlName(newName, itemType));
								break;
							case EAssetType::State:
							case EAssetType::Folder:
								pAsset->SetName(Utils::GenerateUniqueName(newName, itemType, pAsset->GetParent()));
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
		auto const childIndex = static_cast<size_t>(row);

		if (parent.isValid())
		{
			CAsset const* const pParent = static_cast<CAsset*>(parent.internalPointer());

			if ((pParent != nullptr) && (childIndex < pParent->ChildCount()))
			{
				CAsset const* const pChild = pParent->GetChild(childIndex);

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
		CAsset const* const pAsset = static_cast<CAsset*>(index.internalPointer());

		if (pAsset != nullptr)
		{
			CAsset const* const pParent = pAsset->GetParent();

			if ((pParent != nullptr) && (pParent->GetType() != EAssetType::Library))
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
		CAsset const* const pParent = static_cast<CAsset*>(parent.internalPointer());
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
		auto const pAsset = static_cast<CAsset*>(parent.internalPointer());
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
	return static_cast<Qt::DropActions>(Qt::MoveAction | Qt::CopyAction);
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
			CAsset const* const pAsset = SystemModelUtils::GetAssetFromIndex(index, static_cast<int>(SystemModelUtils::EColumns::Name));

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
		auto const itemTypeRole = static_cast<int>(SystemModelUtils::ERoles::ItemType);
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
