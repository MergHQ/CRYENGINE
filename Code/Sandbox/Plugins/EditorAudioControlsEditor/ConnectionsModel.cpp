// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ConnectionsModel.h"

#include "SystemAssets.h"
#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"
#include "ModelUtils.h"

#include <IEditorImpl.h>
#include <ImplItem.h>
#include <CrySystem/File/CryFile.h>
#include <QtUtil.h>
#include <CryIcon.h>
#include <DragDrop.h>

#include <QApplication>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CConnectionModel::CConnectionModel(QObject* const pParent)
	: QAbstractItemModel(pParent)
	, m_pControl(nullptr)
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
	m_pAssetsManager->SignalItemAdded.Connect([&]()
		{
			if (!m_pAssetsManager->IsLoading())
			{
			  ResetModelAndCache();
			}
	  }, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->SignalItemRemoved.Connect([&]()
		{
			if (!m_pAssetsManager->IsLoading())
			{
			  ResetModelAndCache();
			}
	  }, reinterpret_cast<uintptr_t>(this));

	m_pAssetsManager->SignalControlModified.Connect([&]()
		{
			if (!m_pAssetsManager->IsLoading())
			{
			  ResetModelAndCache();
			}
	  }, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationAboutToChange.Connect([&]()
		{
			beginResetModel();
			m_pControl = nullptr;
			m_connectionsCache.clear();
			endResetModel();
	  }, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationChanged.Connect([&]()
		{
			beginResetModel();
			ResetCache();
			endResetModel();
	  }, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CConnectionModel::DisconnectSignals()
{
	m_pAssetsManager->SignalItemAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->SignalItemRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	m_pAssetsManager->SignalControlModified.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationAboutToChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->SignalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
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
	CItemModelAttribute* pAttribute = nullptr;

	switch (column)
	{
	case EColumns::Notification:
		pAttribute = &ModelUtils::s_notificationAttribute;
		break;
	case EColumns::Name:
		pAttribute = &Attributes::s_nameAttribute;
		break;
	case EColumns::Path:
		pAttribute = &ModelUtils::s_pathAttribute;
		break;
	default:
		pAttribute = nullptr;
		break;
	}

	return pAttribute;
}

//////////////////////////////////////////////////////////////////////////
QVariant CConnectionModel::GetHeaderData(int const section, Qt::Orientation const orientation, int const role)
{
	QVariant variant;

	if (orientation == Qt::Horizontal)
	{
		CItemModelAttribute* pAttribute;

		if (section >= static_cast<int>(EColumns::Count))
		{
			pAttribute = &ModelUtils::s_platformModellAttributes[section - static_cast<int>(EColumns::Count)];
		}
		else
		{
			pAttribute = GetAttributeForColumn(static_cast<EColumns>(section));
		}

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
int CConnectionModel::rowCount(QModelIndex const& parent) const
{
	int rowCount = 0;

	if ((m_pControl != nullptr) && (g_pEditorImpl != nullptr))
	{
		if ((parent.row() < 0) || (parent.column() < 0))
		{
			rowCount = m_connectionsCache.size();
		}
	}

	return rowCount;
}

//////////////////////////////////////////////////////////////////////////
int CConnectionModel::columnCount(QModelIndex const& parent) const
{
	return static_cast<int>(EColumns::Count) + static_cast<int>(ModelUtils::s_platformModellAttributes.size());
}

//////////////////////////////////////////////////////////////////////////
QVariant CConnectionModel::data(QModelIndex const& index, int role) const
{
	QVariant variant;

	if ((g_pEditorImpl != nullptr) && (m_pControl != nullptr) && index.isValid())
	{
		if (index.row() < m_connectionsCache.size())
		{
			ConnectionPtr const pConnection = m_connectionsCache[index.row()];

			if (pConnection != nullptr)
			{
				CImplItem const* const pImplItem = g_pEditorImpl->GetControl(pConnection->GetID());

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
										variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::Placeholder));
									}
									else if (pImplItem->IsLocalised())
									{
										variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::Localized));
									}
									break;
								case Qt::ToolTipRole:
									if (pImplItem->IsPlaceholder())
									{
										variant = tr("Item not found in middleware project");
									}
									else if (pImplItem->IsLocalised())
									{
										variant = tr("Item is localized");
									}
									break;
								case static_cast<int>(ERoles::Id):
									variant = pImplItem->GetId();
									break;
								}
							}
							break;
						case static_cast<int>(EColumns::Name):
							{
								switch (role)
								{
								case Qt::DecorationRole:
									variant = CryIcon(g_pEditorImpl->GetTypeIcon(pImplItem));
									break;
								case Qt::DisplayRole:
								case Qt::ToolTipRole:
								case static_cast<int>(ERoles::Name):
									variant = static_cast<char const*>(pImplItem->GetName());
									break;
								case static_cast<int>(ERoles::Id):
									variant = pImplItem->GetId();
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

										variant = path;
									}
									break;
								}
							}
							break;
						}
					}
					else if ((role == Qt::CheckStateRole) && (m_pControl->GetType() == ESystemItemType::Preload))
					{
						variant = pConnection->IsPlatformEnabled(index.column() - static_cast<int>(EColumns::Count)) ? Qt::Checked : Qt::Unchecked;
					}
				}
			}
		}
	}

	return variant;
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
	bool wasDataChanged = false;

	if ((index.column() >= static_cast<int>(EColumns::Count)) && (role == Qt::CheckStateRole))
	{
		ConnectionPtr const pConnection = m_connectionsCache[index.row()];
		pConnection->EnableForPlatform(index.column() - static_cast<int>(EColumns::Count), value == Qt::Checked);
		QVector<int> roleVector(1, role);
		dataChanged(index, index, roleVector);
		wasDataChanged = true;
	}

	return wasDataChanged;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CConnectionModel::index(int row, int column, QModelIndex const& parent /*= QModelIndex()*/) const
{
	QModelIndex modelIndex = QModelIndex();

	if ((g_pEditorImpl != nullptr) && (m_pControl != nullptr))
	{
		if ((row >= 0) && (column >= 0))
		{
			if (row < m_connectionsCache.size())
			{
				ConnectionPtr const pConnection = m_connectionsCache[row];

				if (pConnection != nullptr)
				{
					CImplItem const* const pImplItem = g_pEditorImpl->GetControl(pConnection->GetID());

					if (pImplItem != nullptr)
					{
						modelIndex = createIndex(row, column);
					}
				}
			}
		}
	}

	return modelIndex;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CConnectionModel::parent(QModelIndex const& index) const
{
	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
bool CConnectionModel::canDropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const
{
	bool canDrop = true;

	if ((g_pEditorImpl != nullptr) && (m_pControl != nullptr))
	{
		std::vector<CID> ids;
		DecodeMimeData(pData, ids);
		QString dragText = tr("Connect to ") + QtUtil::ToQString(m_pControl->GetName());

		for (auto const id : ids)
		{
			CImplItem const* const pImplItem = g_pEditorImpl->GetControl(id);

			if (pImplItem != nullptr)
			{
				// is the type being dragged compatible?
				if (!(g_pEditorImpl->IsTypeCompatible(m_pControl->GetType(), pImplItem)))
				{
					dragText = tr("Control types are not compatible.");
					canDrop = false;
					break;
				}
			}
		}

		CDragDropData::ShowDragText(qApp->widgetAt(QCursor::pos()), dragText);
	}

	return canDrop;
}

//////////////////////////////////////////////////////////////////////////
QStringList CConnectionModel::mimeTypes() const
{
	QStringList types = QAbstractItemModel::mimeTypes();
	types << CDragDropData::GetMimeFormatForType(ModelUtils::s_szImplMimeType);
	return types;
}

//////////////////////////////////////////////////////////////////////////
bool CConnectionModel::dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent)
{
	bool wasDropped = false;

	if ((g_pEditorImpl != nullptr) && (m_pControl != nullptr))
	{
		std::vector<CID> ids;
		CID lastConnectedId = ACE_INVALID_ID;
		DecodeMimeData(pData, ids);

		for (auto const id : ids)
		{
			CImplItem* const pImplItem = g_pEditorImpl->GetControl(id);

			if (pImplItem != nullptr)
			{
				ConnectionPtr pConnection = m_pControl->GetConnection(pImplItem);

				if (pConnection == nullptr)
				{
					pConnection = g_pEditorImpl->CreateConnectionToControl(m_pControl->GetType(), pImplItem);

					if (pConnection != nullptr)
					{
						m_pControl->AddConnection(pConnection);
						lastConnectedId = pConnection->GetID();
						wasDropped = true;
					}
				}
			}
		}

		if (wasDropped)
		{
			SignalConnectionAdded(lastConnectedId);
		}
	}

	return wasDropped;
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
		size_t const size = m_pControl->GetConnectionCount();

		for (size_t i = 0; i < size; ++i)
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
	CDragDropData const* const pDragDropData = CDragDropData::FromMimeData(pData);

	if (pDragDropData->HasCustomData(ModelUtils::s_szImplMimeType))
	{
		QByteArray const byteArray = pDragDropData->GetCustomData(ModelUtils::s_szImplMimeType);
		QDataStream stream(byteArray);

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
