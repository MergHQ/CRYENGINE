// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ConnectionsModel.h"

#include "SystemAssets.h"
#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"
#include "ModelUtils.h"

#include <IEditorImpl.h>
#include <ImplItem.h>
#include <CrySystem/File/CryFile.h>
#include <CryIcon.h>

#include <QMimeData>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CConnectionModel::CConnectionModel(QObject* const pParent)
	: QAbstractItemModel(pParent)
	, m_pControl(nullptr)
	, m_pEditorImpl(CAudioControlsEditorPlugin::GetImplEditor())
	, m_pAssetsManager(CAudioControlsEditorPlugin::GetAssetsManager())
{
	ConnectSignals();
	ModelUtils::GetPlatformNames();
}

//////////////////////////////////////////////////////////////////////////
CConnectionModel::~CConnectionModel()
{
	DisconnectSignals();
}

//////////////////////////////////////////////////////////////////////////
void CConnectionModel::ConnectSignals()
{
	m_pAssetsManager->signalItemAdded.Connect([&]()
	{
		ResetModelAndCache();
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalItemRemoved.Connect([&]()
	{
		ResetModelAndCache();
	}, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->signalControlModified.Connect([&]()
	{
		ResetModelAndCache();
	}, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationAboutToChange.Connect([&]()
	{
		beginResetModel();
		m_pEditorImpl = nullptr;
		m_connectionsCache.clear();
		endResetModel();
	}, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.Connect([&]()
	{
		m_pEditorImpl = CAudioControlsEditorPlugin::GetImplEditor();
		beginResetModel();
		ResetCache();
		endResetModel();
	}, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CConnectionModel::DisconnectSignals()
{
	m_pAssetsManager->signalItemAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalItemRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->signalControlModified.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationAboutToChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CConnectionModel::Init(CSystemControl* const pControl)
{
	beginResetModel();
	m_pControl = pControl;
	ResetCache();
	endResetModel();
}

//////////////////////////////////////////////////////////////////////////
CItemModelAttribute* CConnectionModel::GetAttributeForColumn(EColumns const column)
{
	switch (column)
	{
	case EColumns::Notification:
		return &ModelUtils::s_notificationAttribute;
	case EColumns::Name:
		return &Attributes::s_nameAttribute;
	case EColumns::Path:
		return &ModelUtils::s_pathAttribute;
	default:
		return nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
QVariant CConnectionModel::GetHeaderData(int const section, Qt::Orientation const orientation, int const role)
{
	if (orientation != Qt::Horizontal)
	{
		return QVariant();
	}

	CItemModelAttribute* pAttribute;

	if (section >= static_cast<int>(EColumns::Count))
	{
		pAttribute = &ModelUtils::s_platformModellAttributes[section - static_cast<int>(EColumns::Count)];
	}
	else
	{
		pAttribute = GetAttributeForColumn(static_cast<EColumns>(section));
	}

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
int CConnectionModel::rowCount(QModelIndex const& parent) const
{
	if ((m_pControl != nullptr) && (m_pEditorImpl != nullptr))
	{
		if ((parent.row() < 0) || (parent.column() < 0))
		{
			return m_connectionsCache.size();
		}
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
int CConnectionModel::columnCount(QModelIndex const& parent) const
{
	return static_cast<int>(EColumns::Count) + static_cast<int>(ModelUtils::s_platformModellAttributes.size());
}

//////////////////////////////////////////////////////////////////////////
QVariant CConnectionModel::data(QModelIndex const& index, int role) const
{
	if ((m_pEditorImpl != nullptr) && (m_pControl != nullptr) && index.isValid())
	{
		if (index.row() < m_connectionsCache.size())
		{
			ConnectionPtr const pConnection = m_connectionsCache[index.row()];

			if (pConnection != nullptr)
			{
				CImplItem const* const pImplItem = m_pEditorImpl->GetControl(pConnection->GetID());

				if (pImplItem != nullptr)
				{
					if (index.column() < static_cast<int>(EColumns::Count))
					{
						switch (index.column())
						{
						case static_cast<int>(EColumns::Notification):
							{
								switch (role)
								{
								case Qt::DecorationRole:
									if (pImplItem->IsPlaceholder())
									{
										return CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::Placeholder));
									}
									else if (pImplItem->IsLocalised())
									{
										return CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::Localized));
									}
									break;
								case Qt::ToolTipRole:
									if (pImplItem->IsPlaceholder())
									{
										return tr("Control not found in the audio middleware project");
									}
									else if (pImplItem->IsLocalised())
									{
										return tr("Item is localized");
									}
									break;
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
						case static_cast<int>(EColumns::Path):
							{
								switch (role)
								{
								case Qt::DisplayRole:
								case Qt::ToolTipRole:
									{
										QString path;
										CImplItem const* pImplItemParent = pImplItem->GetParent();

										while (pImplItemParent != nullptr)
										{
											QString parentName = QString(static_cast<char const*>(pImplItemParent->GetName()));
											if (!parentName.isEmpty())
											{
												if (path.isEmpty())
												{
													path = parentName;
												}
												else
												{
													path = parentName + "/" + path;
												}
											}
											pImplItemParent = pImplItemParent->GetParent();
										}

										return path;
									}
								}
							}
							break;
						}
					}
					else if ((role == Qt::CheckStateRole) && (m_pControl->GetType() == ESystemItemType::Preload))
					{
						return pConnection->IsPlatformEnabled(index.column() - static_cast<int>(EColumns::Count)) ? Qt::Checked : Qt::Unchecked;
					}
				}
			}
		}
	}

	return QVariant();
}

//////////////////////////////////////////////////////////////////////////
QVariant CConnectionModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return GetHeaderData(section, orientation, role);
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CConnectionModel::flags(QModelIndex const& index) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags(index);

	if (index.isValid() && (index.column() >= static_cast<int>(EColumns::Count)))
	{
		flags |= Qt::ItemIsUserCheckable;
	}

	return flags | Qt::ItemIsDropEnabled;
}

//////////////////////////////////////////////////////////////////////////
bool CConnectionModel::setData(QModelIndex const& index, QVariant const& value, int role)
{
	if ((index.column() >= static_cast<int>(EColumns::Count)) && (role == Qt::CheckStateRole))
	{
		ConnectionPtr const pConnection = m_connectionsCache[index.row()];
		pConnection->EnableForPlatform(index.column() - static_cast<int>(EColumns::Count), value == Qt::Checked);
		QVector<int> roleVector(1, role);
		dataChanged(index, index, roleVector);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CConnectionModel::index(int row, int column, QModelIndex const& parent /*= QModelIndex()*/) const
{
	if ((m_pEditorImpl != nullptr) && (m_pControl != nullptr))
	{
		if ((row >= 0) && (column >= 0))
		{
			if (row < m_connectionsCache.size())
			{
				ConnectionPtr const pConnection = m_connectionsCache[row];

				if (pConnection != nullptr)
				{
					CImplItem const* const pImplItem = m_pEditorImpl->GetControl(pConnection->GetID());

					if (pImplItem != nullptr)
					{
						return createIndex(row, column);
					}
				}
			}
		}
	}

	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CConnectionModel::parent(QModelIndex const& index) const
{
	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
bool CConnectionModel::canDropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const
{
	if ((m_pEditorImpl != nullptr) && (m_pControl != nullptr))
	{
		std::vector<CID> ids;
		DecodeMimeData(pData, ids);

		for (auto const id : ids)
		{
			CImplItem const* const pImplItem = m_pEditorImpl->GetControl(id);

			if (pImplItem != nullptr)
			{
				// is the type being dragged compatible?
				if (!(m_pEditorImpl->IsTypeCompatible(m_pControl->GetType(), pImplItem)))
				{
					return false;
				}
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
QStringList CConnectionModel::mimeTypes() const
{
	QStringList list = QAbstractItemModel::mimeTypes();
	list << ModelUtils::s_szMiddlewareMimeType;
	return list;
}

//////////////////////////////////////////////////////////////////////////
bool CConnectionModel::dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent)
{
	if ((m_pEditorImpl != nullptr) && (m_pControl != nullptr))
	{
		std::vector<CID> ids;
		DecodeMimeData(pData, ids);
		bool wasConnectionAdded = false;

		for (auto const id : ids)
		{
			CImplItem* const pImplItem = m_pEditorImpl->GetControl(id);

			if (pImplItem != nullptr)
			{
				ConnectionPtr pConnection = m_pControl->GetConnection(pImplItem);

				if (pConnection == nullptr)
				{
					pConnection = m_pEditorImpl->CreateConnectionToControl(m_pControl->GetType(), pImplItem);

					if (pConnection != nullptr)
					{
						m_pControl->AddConnection(pConnection);
						wasConnectionAdded = true;
					}
				}
			}
		}

		return wasConnectionAdded;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
Qt::DropActions CConnectionModel::supportedDropActions() const
{
	return Qt::CopyAction;
}

//////////////////////////////////////////////////////////////////////////
void CConnectionModel::ResetCache()
{
	m_connectionsCache.clear();

	if (m_pControl != nullptr)
	{
		int const size = m_pControl->GetConnectionCount();

		for (int i = 0; i < size; ++i)
		{
			ConnectionPtr const pConnection = m_pControl->GetConnectionAt(i);

			if (pConnection != nullptr)
			{
				m_connectionsCache.emplace_back(pConnection);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CConnectionModel::ResetModelAndCache()
{
	beginResetModel();
	ResetCache();
	endResetModel();
}

//////////////////////////////////////////////////////////////////////////
void CConnectionModel::DecodeMimeData(QMimeData const* pData, std::vector<CID>& ids) const
{
	QString const format = ModelUtils::s_szMiddlewareMimeType;

	if (pData->hasFormat(format))
	{
		QByteArray encoded = pData->data(format);
		QDataStream stream(&encoded, QIODevice::ReadOnly);

		while (!stream.atEnd())
		{
			CID id;
			stream >> id;

			if (id != ACE_INVALID_ID)
			{
				ids.emplace_back(id);
			}
		}
	}
}
} // namespace ACE
