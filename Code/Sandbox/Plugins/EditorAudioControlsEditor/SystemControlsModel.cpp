// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "SystemControlsModel.h"

#include "AudioControlsEditorPlugin.h"
#include "MiddlewareDataModel.h"
#include "SystemControlsEditorIcons.h"

#include <IAudioSystemItem.h>
#include <QtUtil.h>
#include <EditorStyleHelper.h>

#include <DragDrop.h>

namespace ACE
{
static QColor s_placeholderColor = GetStyleHelper()->errorColor();
static QColor s_noConnectionColor = QColor(255, 150, 50);
static QColor s_noControlColor = QColor(50, 150, 255);

//////////////////////////////////////////////////////////////////////////
bool IsParentValid(CAudioAsset const& parent, EItemType const type)
{
	switch (parent.GetType())
	{
	case EItemType::Folder:
	case EItemType::Library: // intentional fall through
		return type != EItemType::State;
	default: // assumes the rest are actual controls
		{
			CAudioControl const* pControl = static_cast<CAudioControl const*>(&parent);

			if (pControl && (pControl->GetType() == EItemType::Switch))
			{
				return type == EItemType::State;
			}
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
void DecodeMimeData(const QMimeData* pData, std::vector<CAudioAsset*>& outItems)
{
	const CDragDropData* dragDropData = CDragDropData::FromMimeData(pData);

	if (dragDropData->HasCustomData("AudioLibraryItems"))
	{
		QByteArray const byteArray = dragDropData->GetCustomData("AudioLibraryItems");
		QDataStream stream(byteArray);

		while (!stream.atEnd())
		{
			intptr_t ptr;
			stream >> ptr;
			outItems.push_back(reinterpret_cast<CAudioAsset*>(ptr));
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CanDropMimeData(const QMimeData* pData, CAudioAsset const& parent)
{
	// Handle first if mime data is an external (from the implementation side) source
	std::vector<IAudioSystemItem*> implItems;
	AudioModelUtils::DecodeImplMimeData(pData, implItems);

	if (!implItems.empty())
	{
		IAudioSystemEditor* pImpl = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();

		for (IAudioSystemItem* pImplItem : implItems)
		{
			if (!IsParentValid(parent, pImpl->ImplTypeToATLType(pImplItem->GetType())))
			{
				return false;
			}
		}

		return true;
	}
	else
	{
		// Handle if mime data is an internal move (rearranging controls)
		std::vector<CAudioAsset*> droppedItems;
		DecodeMimeData(pData, droppedItems);

		for (CAudioAsset* pItem : droppedItems)
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
void DropMimeData(const QMimeData* pData, CAudioAsset& parent)
{
	// Handle first if mime data is an external (from the implementation side) source
	std::vector<IAudioSystemItem*> implItems;
	AudioModelUtils::DecodeImplMimeData(pData, implItems);

	if (!implItems.empty())
	{
		CAudioAssetsManager* pAssetsManager = CAudioControlsEditorPlugin::GetAssetsManager();

		for (IAudioSystemItem* pImplControl : implItems)
		{
			pAssetsManager->CreateAndConnectImplItems(pImplControl, &parent);
		}
	}
	else
	{
		std::vector<CAudioAsset*> droppedItems;
		DecodeMimeData(pData, droppedItems);
		std::vector<CAudioAsset*> topLevelDroppedItems;
		Utils::SelectTopLevelAncestors(droppedItems, topLevelDroppedItems);
		CAudioControlsEditorPlugin::GetAssetsManager()->MoveItems(&parent, topLevelDroppedItems);
	}
}

//////////////////////////////////////////////////////////////////////////
CSystemControlsModel::CSystemControlsModel(CAudioAssetsManager* pAssetsManager)
	: m_pAssetsManager(pAssetsManager)
{
	ConnectToSystem();
}

//////////////////////////////////////////////////////////////////////////
CSystemControlsModel::~CSystemControlsModel()
{
	DisconnectFromSystem();
}

//////////////////////////////////////////////////////////////////////////
int CSystemControlsModel::rowCount(QModelIndex const& parent) const
{
	if (!parent.isValid())
	{
		return m_pAssetsManager->GetLibraryCount();
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
int CSystemControlsModel::columnCount(QModelIndex const& parent) const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////
QVariant CSystemControlsModel::data(QModelIndex const& index, int role) const
{
	CAudioLibrary* pLibrary = static_cast<CAudioLibrary*>(index.internalPointer());

	if (pLibrary)
	{
		switch (role)
		{
		case Qt::DisplayRole:

			if (pLibrary->IsModified())
			{
				return QtUtil::ToQStringSafe(pLibrary->GetName()) + " *";
			}

			return QtUtil::ToQStringSafe(pLibrary->GetName());
			break;

		case Qt::EditRole:
			return QtUtil::ToQStringSafe(pLibrary->GetName());
			break;

		case Qt::ForegroundRole:

			if (pLibrary->HasPlaceholderConnection())
			{
				return s_placeholderColor;
			}
			else if (!pLibrary->HasConnection())
			{
				return s_noConnectionColor;
			}
			else if (!pLibrary->HasControl())
			{
				return s_noControlColor;
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

		case Qt::DecorationRole:
			return GetItemTypeIcon(EItemType::Library);
			break;

		case static_cast<int>(EDataRole::ItemType):
			return static_cast<int>(EItemType::Library);
			break;

		case static_cast<int>(EDataRole::InternalPointer):
			return reinterpret_cast<intptr_t>(pLibrary);
			break;
		}
	}

	return QVariant();
}

//////////////////////////////////////////////////////////////////////////
bool CSystemControlsModel::setData(QModelIndex const& index, QVariant const& value, int role)
{
	if (index.isValid())
	{
		CAudioAsset* const pItem = static_cast<CAudioAsset*>(index.internalPointer());

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
							pItem->SetModified(true);
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
QVariant CSystemControlsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole)
	{
		return "Name";
	}

	return QVariant();
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CSystemControlsModel::flags(QModelIndex const& index) const
{
	if (index.isValid())
	{
		return QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
	}

	return Qt::NoItemFlags;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CSystemControlsModel::index(int row, int column, QModelIndex const& parent /*= QModelIndex()*/) const
{
	if ((row >= 0) && (column >= 0))
	{
		return createIndex(row, column, reinterpret_cast<quintptr>(m_pAssetsManager->GetLibrary(row)));
	}

	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CSystemControlsModel::parent(QModelIndex const& index) const
{
	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
bool CSystemControlsModel::canDropMimeData(const QMimeData* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const
{
	if (parent.isValid())
	{
		CAudioAsset* pAsset = static_cast<CAudioAsset*>(parent.internalPointer());
		CRY_ASSERT(pAsset != nullptr);
		return CanDropMimeData(pData, *pAsset);
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CSystemControlsModel::dropMimeData(const QMimeData* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent)
{
	if (parent.isValid())
	{
		CAudioLibrary* pLibrary = static_cast<CAudioLibrary*>(parent.internalPointer());
		CRY_ASSERT(pLibrary != nullptr);
		DropMimeData(pData, *pLibrary);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
Qt::DropActions CSystemControlsModel::supportedDropActions() const
{
	return Qt::MoveAction | Qt::CopyAction;
}

//////////////////////////////////////////////////////////////////////////
QStringList CSystemControlsModel::mimeTypes() const
{
	QStringList result;
	result << CDragDropData::GetMimeFormatForType("AudioLibraryItems");
	result << CMiddlewareDataModel::ms_szMimeType;
	return result;
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsModel::ConnectToSystem()
{
	m_pAssetsManager->signalLibraryAboutToBeAdded.Connect([&]()
	{
		int const row = m_pAssetsManager->GetLibraryCount();
		beginInsertRows(QModelIndex(), row, row);
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalLibraryAdded.Connect([&](CAudioLibrary* pLibrary) { endInsertRows(); }, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalLibraryAboutToBeRemoved.Connect([&](CAudioLibrary* pLibrary)
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
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalLibraryRemoved.Connect([&]() { endRemoveRows(); }, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsModel::DisconnectFromSystem()
{
	m_pAssetsManager->signalLibraryAboutToBeAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalLibraryAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalLibraryAboutToBeRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalLibraryRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

/////////////////////////////////////////////////////////////////////////////////////////
CAudioLibraryModel::CAudioLibraryModel(CAudioAssetsManager* pAssetsManager, CAudioLibrary* pLibrary)
	: m_pAssetsManager(pAssetsManager)
	, m_pLibrary(pLibrary)
{
	ConnectToSystem();
}

//////////////////////////////////////////////////////////////////////////
CAudioLibraryModel::~CAudioLibraryModel()
{
	DisconnectFromSystem();
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CAudioLibraryModel::IndexFromItem(const CAudioAsset* pItem) const
{
	if (pItem)
	{
		CAudioAsset* pParent = pItem->GetParent();

		if (pParent)
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
int CAudioLibraryModel::rowCount(QModelIndex const& parent) const
{
	if (!parent.isValid())
	{
		return m_pLibrary->ChildCount();
	}

	CAudioAsset* pAsset = static_cast<CAudioAsset*>(parent.internalPointer());

	if (pAsset)
	{
		return pAsset->ChildCount();
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
int CAudioLibraryModel::columnCount(QModelIndex const& parent) const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////
QVariant CAudioLibraryModel::data(QModelIndex const& index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	CAudioAsset* pItem = static_cast<CAudioAsset*>(index.internalPointer());

	if (pItem)
	{
		EItemType const itemType = pItem->GetType();

		switch (role)
		{
		case Qt::DisplayRole:

			if (pItem->IsModified())
			{
				return QtUtil::ToQStringSafe(pItem->GetName()) + " *";
			}

			return QtUtil::ToQStringSafe(pItem->GetName());
			break;

		case Qt::EditRole:
			return QtUtil::ToQStringSafe(pItem->GetName());
			break;

		case Qt::ForegroundRole:

			if (pItem->HasPlaceholderConnection())
			{
				return s_placeholderColor;
			}
			else if (!pItem->HasConnection())
			{
				return s_noConnectionColor;
			}
			else if (((pItem->GetType() == EItemType::Folder) || (pItem->GetType() == EItemType::Switch)) && !pItem->HasControl())
			{
				return s_noControlColor;
			}

			break;

		case Qt::ToolTipRole:

			if (pItem->HasPlaceholderConnection())
			{
				if ((pItem->GetType() == EItemType::Folder) || (pItem->GetType() == EItemType::Switch))
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
				if ((pItem->GetType() == EItemType::Folder) || (pItem->GetType() == EItemType::Switch))
				{
					return tr("Contains item that is not connected to any audio control");
				}
				else
				{
					return tr("Item is not connected to any audio control");
				}
			}
			else if ((pItem->GetType() == EItemType::Folder) && !pItem->HasControl())
			{
				return tr("Contains no audio control");
			}
			else if ((pItem->GetType() == EItemType::Switch) && !pItem->HasControl())
			{
				return tr("Contains no state");
			}

			break;

		case Qt::DecorationRole:
			return GetItemTypeIcon(itemType);
			break;

		case static_cast<int>(EDataRole::ItemType):
			return static_cast<int>(pItem->GetType());
			break;

		case static_cast<int>(EDataRole::InternalPointer):
			return reinterpret_cast<intptr_t>(pItem);
			break;
		}
	}

	return QVariant();
}

//////////////////////////////////////////////////////////////////////////
bool CAudioLibraryModel::setData(QModelIndex const& index, QVariant const& value, int role)
{
	if (index.isValid())
	{
		CAudioAsset* const pItem = static_cast<CAudioAsset*>(index.internalPointer());

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
							EItemType const itemType = pItem->GetType();

							switch (itemType)
							{
							case EItemType::Preload:
							case EItemType::Parameter:
							case EItemType::State:
							case EItemType::Switch:
							case EItemType::Trigger:
							case EItemType::Environment:
								pItem->SetName(Utils::GenerateUniqueControlName(newName, itemType, *m_pAssetsManager));
								pItem->SetModified(true);
								break;
							case EItemType::Folder:
								pItem->SetName(Utils::GenerateUniqueName(newName, itemType, pItem->GetParent()));
								pItem->SetModified(true);
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
QVariant CAudioLibraryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole)
	{
		return "Name";
	}

	return QVariant();
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CAudioLibraryModel::flags(QModelIndex const& index) const
{
	if (index.isValid())
	{
		return QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
	}

	return Qt::NoItemFlags;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CAudioLibraryModel::index(int row, int column, QModelIndex const& parent) const
{
	if ((row >= 0) && (column >= 0))
	{
		if (parent.isValid())
		{
			CAudioAsset* pParent = static_cast<CAudioAsset*>(parent.internalPointer());

			if (pParent && row < pParent->ChildCount())
			{
				CAudioAsset* pChild = pParent->GetChild(row);

				if (pChild)
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
QModelIndex CAudioLibraryModel::parent(QModelIndex const& index) const
{
	if (index.isValid())
	{
		CAudioAsset* pItem = static_cast<CAudioAsset*>(index.internalPointer());

		if (pItem)
		{
			if (pItem->GetParent()->GetType() != EItemType::Library)
			{
				return IndexFromItem(pItem->GetParent());
			}
		}
	}

	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
bool CAudioLibraryModel::canDropMimeData(const QMimeData* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const
{
	if (parent.isValid())
	{
		CAudioAsset* pAsset = static_cast<CAudioAsset*>(parent.internalPointer());
		CRY_ASSERT(pAsset != nullptr);
		return CanDropMimeData(pData, *pAsset);
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAudioLibraryModel::dropMimeData(const QMimeData* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent)
{
	if (parent.isValid())
	{
		CAudioAsset* pAsset = static_cast<CAudioAsset*>(parent.internalPointer());
		CRY_ASSERT(pAsset != nullptr);
		DropMimeData(pData, *pAsset);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
QMimeData* CAudioLibraryModel::mimeData(QModelIndexList const& indexes) const
{
	CDragDropData* pDragDropData = new CDragDropData();
	QByteArray byteArray;
	QDataStream stream(&byteArray, QIODevice::ReadWrite);

	for (auto const index : indexes)
	{
		stream << reinterpret_cast<intptr_t>(index.internalPointer());
	}

	pDragDropData->SetCustomData("AudioLibraryItems", byteArray);
	return pDragDropData;
}

//////////////////////////////////////////////////////////////////////////
Qt::DropActions CAudioLibraryModel::supportedDropActions() const
{
	return Qt::MoveAction | Qt::CopyAction;
}

//////////////////////////////////////////////////////////////////////////
QStringList CAudioLibraryModel::mimeTypes() const
{
	QStringList result;
	result << CDragDropData::GetMimeFormatForType("AudioLibraryItems");
	result << CMiddlewareDataModel::ms_szMimeType;
	return result;
}

//////////////////////////////////////////////////////////////////////////
void CAudioLibraryModel::ConnectToSystem()
{
	CAudioAssetsManager* pAssetsManager = CAudioControlsEditorPlugin::GetAssetsManager();
	pAssetsManager->signalItemAboutToBeAdded.Connect([&](CAudioAsset* pParent)
	{
		if (Utils::GetParentLibrary(pParent) == m_pLibrary)
		{
			int const row = pParent->ChildCount();

			if (pParent->GetType() == EItemType::Library)
			{
				CRY_ASSERT(pParent == m_pLibrary);
				beginInsertRows(QModelIndex(), row, row);
			}
			else
			{
				QModelIndex parent = IndexFromItem(pParent);
				beginInsertRows(parent, row, row);
			}
		}
	}, reinterpret_cast<uintptr_t>(this));

	pAssetsManager->signalItemAdded.Connect([&](CAudioAsset* pAsset)
	{
		if (Utils::GetParentLibrary(pAsset) == m_pLibrary)
		{
			endInsertRows();
		}
	}, reinterpret_cast<uintptr_t>(this));

	pAssetsManager->signalItemAboutToBeRemoved.Connect([&](CAudioAsset* pAsset)
	{
		if (Utils::GetParentLibrary(pAsset) == m_pLibrary)
		{
			CAudioAsset* pParent = pAsset->GetParent();
			int const childCount = pParent->ChildCount();

			for (int i = 0; i < childCount; ++i)
			{
				if (pParent->GetChild(i) == pAsset)
				{
					if (pParent->GetType() == EItemType::Library)
					{
						CRY_ASSERT(pParent == m_pLibrary);
						beginRemoveRows(QModelIndex(), i, i);
					}
					else
					{
						QModelIndex parent = IndexFromItem(pParent);
						beginRemoveRows(parent, i, i);
					}

					break;
				}
			}
		}
	}, reinterpret_cast<uintptr_t>(this));

	pAssetsManager->signalItemRemoved.Connect([&](CAudioAsset* pParent, CAudioAsset* pAsset)
	{
		if (Utils::GetParentLibrary(pParent) == m_pLibrary)
		{
			endRemoveRows();
		}
	}, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CAudioLibraryModel::DisconnectFromSystem()
{
	CAudioAssetsManager* pAssetsManager = CAudioControlsEditorPlugin::GetAssetsManager();
	pAssetsManager->signalItemAboutToBeAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	pAssetsManager->signalItemAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	pAssetsManager->signalItemAboutToBeRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	pAssetsManager->signalItemRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
CSystemControlsFilterProxyModel::CSystemControlsFilterProxyModel(QObject* parent)
	: QDeepFilterProxyModel(QDeepFilterProxyModel::BehaviorFlags(QDeepFilterProxyModel::AcceptIfChildMatches), parent)
{
}

//////////////////////////////////////////////////////////////////////////
bool CSystemControlsFilterProxyModel::rowMatchesFilter(int source_row, QModelIndex const& source_parent) const
{
	if (QDeepFilterProxyModel::rowMatchesFilter(source_row, source_parent))
	{
		QModelIndex const index = sourceModel()->index(source_row, 0, source_parent);

		if (index.isValid())
		{
			if (m_validControlsMask != std::numeric_limits<uint>::max())
			{
				// Has type filtering.
				EItemType const itemType = (EItemType)sourceModel()->data(index, static_cast<int>(EDataRole::ItemType)).toUInt();

				if (itemType < EItemType::Folder)
				{
					return m_validControlsMask & (1 << static_cast<int>(itemType));
				}
				else
				{
					return false;
				}
			}

			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
bool CSystemControlsFilterProxyModel::lessThan(QModelIndex const& left, QModelIndex const& right) const
{
	if (left.column() == right.column())
	{
		EItemType const leftType = static_cast<EItemType>(sourceModel()->data(left, static_cast<int>(EDataRole::ItemType)).toInt());
		EItemType const rightType = static_cast<EItemType>(sourceModel()->data(right, static_cast<int>(EDataRole::ItemType)).toInt());

		if (leftType != rightType)
		{
			return rightType < EItemType::Folder;
		}

		QVariant const valueLeft = sourceModel()->data(left, Qt::DisplayRole);
		QVariant const valueRight = sourceModel()->data(right, Qt::DisplayRole);

		return valueLeft < valueRight;
	}

	return QSortFilterProxyModel::lessThan(left, right);
}

//////////////////////////////////////////////////////////////////////////
void CSystemControlsFilterProxyModel::EnableControl(bool const bEnabled, EItemType const type)
{
	if (bEnabled)
	{
		m_validControlsMask |= (1 << (int)type);
	}
	else
	{
		m_validControlsMask &= ~(1 << (int)type);
	}

	invalidate();
}

namespace AudioModelUtils
{
//////////////////////////////////////////////////////////////////////////
void GetAssetsFromIndices(QModelIndexList const& list, std::vector<CAudioLibrary*>& outLibraries, std::vector<CAudioFolder*>& outFolders, std::vector<CAudioControl*>& outControls)
{
	for (auto const& index : list)
	{
		QVariant const internalPtr = index.data(static_cast<int>(EDataRole::InternalPointer));

		if (internalPtr.isValid())
		{
			QVariant const type = index.data(static_cast<int>(EDataRole::ItemType));

			switch (type.toInt())
			{
			case static_cast<int>(EItemType::Library):
				outLibraries.push_back(reinterpret_cast<CAudioLibrary*>(internalPtr.value<intptr_t>()));
				break;
			case static_cast<int>(EItemType::Folder):
				outFolders.push_back(reinterpret_cast<CAudioFolder*>(internalPtr.value<intptr_t>()));
				break;
			default:
				outControls.push_back(reinterpret_cast<CAudioControl*>(internalPtr.value<intptr_t>()));
				break;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CAudioAsset* GetAssetFromIndex(QModelIndex const& index)
{
	QVariant const internalPtr = index.data(static_cast<int>(EDataRole::InternalPointer));

	if (internalPtr.isValid())
	{
		return reinterpret_cast<CAudioAsset*>(internalPtr.value<intptr_t>());
	}

	return nullptr;
}
} // namespace AudioModelUtils
} // namespace ACE
