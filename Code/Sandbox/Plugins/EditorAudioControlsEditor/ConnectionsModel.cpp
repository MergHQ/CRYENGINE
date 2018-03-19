// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ConnectionsModel.h"

#include "Assets.h"
#include "AudioControlsEditorPlugin.h"
#include "ImplementationManager.h"
#include "ModelUtils.h"

#include <IEditorImpl.h>
#include <IImplItem.h>
#include <CrySystem/File/CryFile.h>
#include <QtUtil.h>
#include <CryIcon.h>
#include <DragDrop.h>

#include <QApplication>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CConnectionsModel::CConnectionsModel(QObject* const pParent)
	: QAbstractItemModel(pParent)
	, m_pControl(nullptr)
{
	ConnectSignals();
}

//////////////////////////////////////////////////////////////////////////
CConnectionsModel::~CConnectionsModel()
{
	DisconnectSignals();
}

//////////////////////////////////////////////////////////////////////////
void CConnectionsModel::ConnectSignals()
{
	g_assetsManager.SignalConnectionAdded.Connect([this]()
		{
			if (!g_assetsManager.IsLoading())
			{
			  ResetModelAndCache();
			}
	  }, reinterpret_cast<uintptr_t>(this));

	g_assetsManager.SignalConnectionRemoved.Connect([this]()
		{
			if (!g_assetsManager.IsLoading())
			{
			  ResetModelAndCache();
			}
	  }, reinterpret_cast<uintptr_t>(this));

	g_implementationManager.SignalImplementationAboutToChange.Connect([this]()
		{
			beginResetModel();
			m_pControl = nullptr;
			m_connectionsCache.clear();
			endResetModel();
	  }, reinterpret_cast<uintptr_t>(this));

	g_implementationManager.SignalImplementationChanged.Connect([this]()
		{
			beginResetModel();
			ResetCache();
			endResetModel();
	  }, reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CConnectionsModel::DisconnectSignals()
{
	g_assetsManager.SignalConnectionAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	g_assetsManager.SignalConnectionRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	g_implementationManager.SignalImplementationAboutToChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	g_implementationManager.SignalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CConnectionsModel::Init(CControl* const pControl)
{
	beginResetModel();
	m_pControl = pControl;
	ResetCache();
	endResetModel();
}

//////////////////////////////////////////////////////////////////////////
CItemModelAttribute* CConnectionsModel::GetAttributeForColumn(EColumns const column)
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
QVariant CConnectionsModel::GetHeaderData(int const section, Qt::Orientation const orientation, int const role)
{
	QVariant variant;

	if (orientation == Qt::Horizontal)
	{
		CItemModelAttribute* const pAttribute = GetAttributeForColumn(static_cast<EColumns>(section));

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
int CConnectionsModel::rowCount(QModelIndex const& parent) const
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
int CConnectionsModel::columnCount(QModelIndex const& parent) const
{
	return static_cast<int>(EColumns::Count);
}

//////////////////////////////////////////////////////////////////////////
QVariant CConnectionsModel::data(QModelIndex const& index, int role) const
{
	QVariant variant;

	if ((g_pEditorImpl != nullptr) && (m_pControl != nullptr) && index.isValid())
	{
		if (index.row() < m_connectionsCache.size())
		{
			ConnectionPtr const pConnection = m_connectionsCache[index.row()];

			if (pConnection != nullptr)
			{
				IImplItem const* const pIImplItem = g_pEditorImpl->GetItem(pConnection->GetID());

				if (pIImplItem != nullptr)
				{
					if (role == static_cast<int>(ERoles::Name))
					{
						variant = static_cast<char const*>(pIImplItem->GetName());
					}
					else
					{
						switch (index.column())
						{
						case static_cast<int>(EColumns::Notification):
							{
								switch (role)
								{
								case Qt::DecorationRole:
									if (pIImplItem->IsPlaceholder())
									{
										variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::Placeholder));
									}
									else if (pIImplItem->IsLocalized())
									{
										variant = CryIcon(ModelUtils::GetItemNotificationIcon(ModelUtils::EItemStatus::Localized));
									}
									break;
								case Qt::ToolTipRole:
									if (pIImplItem->IsPlaceholder())
									{
										variant = tr("Item not found in middleware project");
									}
									else if (pIImplItem->IsLocalized())
									{
										variant = tr("Item is localized");
									}
									break;
								case static_cast<int>(ERoles::Id):
									variant = pIImplItem->GetId();
									break;
								}
							}
							break;
						case static_cast<int>(EColumns::Name):
							{
								switch (role)
								{
								case Qt::DecorationRole:
									variant = CryIcon(g_pEditorImpl->GetTypeIcon(pIImplItem));
									break;
								case Qt::DisplayRole:
								case Qt::ToolTipRole:
									variant = static_cast<char const*>(pIImplItem->GetName());
									break;
								case static_cast<int>(ERoles::Id):
									variant = pIImplItem->GetId();
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
										IImplItem const* pIImplItemParent = pIImplItem->GetParent();

										while (pIImplItemParent != nullptr)
										{
											QString parentName = QString(static_cast<char const*>(pIImplItemParent->GetName()));

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
											pIImplItemParent = pIImplItemParent->GetParent();
										}

										variant = path;
									}
									break;
								}
							}
							break;
						}
					}
				}
			}
		}
	}

	return variant;
}

//////////////////////////////////////////////////////////////////////////
QVariant CConnectionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	return GetHeaderData(section, orientation, role);
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CConnectionsModel::flags(QModelIndex const& index) const
{
	return QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled;
}

//////////////////////////////////////////////////////////////////////////
bool CConnectionsModel::setData(QModelIndex const& index, QVariant const& value, int role)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CConnectionsModel::index(int row, int column, QModelIndex const& parent /*= QModelIndex()*/) const
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
					IImplItem const* const pIImplItem = g_pEditorImpl->GetItem(pConnection->GetID());

					if (pIImplItem != nullptr)
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
QModelIndex CConnectionsModel::parent(QModelIndex const& index) const
{
	return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
bool CConnectionsModel::canDropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const
{
	bool canDrop = true;

	if ((g_pEditorImpl != nullptr) && (m_pControl != nullptr))
	{
		ImplItemIds ids;
		DecodeMimeData(pData, ids);
		QString dragText = tr("Connect to ") + QtUtil::ToQString(m_pControl->GetName());

		for (auto const id : ids)
		{
			IImplItem const* const pIImplItem = g_pEditorImpl->GetItem(id);

			if (pIImplItem != nullptr)
			{
				// is the type being dragged compatible?
				if (!(g_pEditorImpl->IsTypeCompatible(m_pControl->GetType(), pIImplItem)))
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
QStringList CConnectionsModel::mimeTypes() const
{
	QStringList types = QAbstractItemModel::mimeTypes();
	types << CDragDropData::GetMimeFormatForType(ModelUtils::s_szImplMimeType);
	return types;
}

//////////////////////////////////////////////////////////////////////////
bool CConnectionsModel::dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent)
{
	bool wasDropped = false;

	if ((g_pEditorImpl != nullptr) && (m_pControl != nullptr))
	{
		ImplItemIds ids;
		ControlId lastConnectedId = s_aceInvalidId;
		DecodeMimeData(pData, ids);

		for (auto const id : ids)
		{
			IImplItem* const pIImplItem = g_pEditorImpl->GetItem(id);

			if (pIImplItem != nullptr)
			{
				ConnectionPtr pConnection = m_pControl->GetConnection(pIImplItem);

				if (pConnection == nullptr)
				{
					pConnection = g_pEditorImpl->CreateConnectionToControl(m_pControl->GetType(), pIImplItem);

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
Qt::DropActions CConnectionsModel::supportedDropActions() const
{
	return Qt::CopyAction;
}

//////////////////////////////////////////////////////////////////////////
void CConnectionsModel::ResetCache()
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
				m_connectionsCache.push_back(pConnection);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CConnectionsModel::ResetModelAndCache()
{
	beginResetModel();
	ResetCache();
	endResetModel();
}

//////////////////////////////////////////////////////////////////////////
void CConnectionsModel::DecodeMimeData(QMimeData const* pData, ImplItemIds& ids) const
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
				ids.push_back(id);
			}
		}
	}
}
} // namespace ACE

