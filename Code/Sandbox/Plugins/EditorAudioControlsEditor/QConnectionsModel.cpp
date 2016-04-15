// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "QConnectionsModel.h"
#include <IAudioSystemEditor.h>
#include <IAudioSystemItem.h>
#include "AudioControl.h"
#include "AudioControlsEditorPlugin.h"
#include "AudioSystemModel.h"
#include "ImplementationManager.h"

#include <CrySystem/File/CryFile.h>  // Includes CryPath.h in correct order.
#include <QtUtil.h>

#include <QMimeData>

namespace ACE
{

QConnectionModel::QConnectionModel()
	: m_pControl(nullptr)
	, m_pAudioSystem(CAudioControlsEditorPlugin::GetAudioSystemEditorImpl())
	, m_group("")
{
	CAudioControlsEditorPlugin::GetATLModel()->AddListener(this);
	connect(CAudioControlsEditorPlugin::GetImplementationManger(), &CImplementationManager::ImplementationChanged, [&]()
		{
			beginResetModel();
			m_pAudioSystem = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
			ResetCache();
			endResetModel();
	  });
}

QConnectionModel::~QConnectionModel()
{
	CAudioControlsEditorPlugin::GetATLModel()->RemoveListener(this);
}

void QConnectionModel::Init(CATLControl* pControl, const string& group)
{
	beginResetModel();
	m_group = group;
	m_pControl = pControl;
	ResetCache();
	endResetModel();
}

int QConnectionModel::rowCount(const QModelIndex& parent) const
{
	if (m_pControl && m_pAudioSystem)
	{
		if ((parent.row() < 0) || (parent.column() < 0))
		{
			return m_connectionsCache.size();
		}
	}
	return 0;
}

QVariant QConnectionModel::data(const QModelIndex& index, int role) const
{
	if (m_pAudioSystem && m_pControl && index.isValid())
	{
		if (index.row() < m_connectionsCache.size())
		{
			ConnectionPtr pConnection = m_connectionsCache[index.row()];
			if (pConnection)
			{
				IAudioSystemItem* pItem = m_pAudioSystem->GetControl(pConnection->GetID());
				if (pItem)
				{
					switch (role)
					{
					case Qt::DisplayRole:
						switch (index.column())
						{
						case eConnectionModelColoums_Name:
							return (const char*)pItem->GetName();
						case eConnectionModelColoums_Path:
							{
								QString path;
								IAudioSystemItem* pParent = pItem->GetParent();
								while (pParent)
								{
									QString parentName = QString((const char*)pParent->GetName());
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
									pParent = pParent->GetParent();
								}

								return path;
							}
						}
						break;
					case Qt::DecorationRole:
						if (index.column() == eConnectionModelColoums_Name)
						{
							return QIcon((QtUtil::ToQString(PathUtil::GetEnginePath()) + PathUtil::GetSlash()) + m_pAudioSystem->GetTypeIcon(pItem->GetType()));
						}
						break;
					case Qt::ForegroundRole:
						if (pItem->IsPlaceholder())
						{
							return QColor(200, 100, 100);
						}
						break;
					case Qt::ToolTipRole:
						if (pItem->IsPlaceholder())
						{
							return tr("Control not found in the audio middleware project");
						}
						break;
					case eConnectionModelRoles_Id:
						if (index.column() == eConnectionModelColoums_Name)
						{
							return pItem->GetId();
						}
						break;
					}
				}
			}
		}
	}
	return QVariant();
}

QVariant QConnectionModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
	{
		switch (section)
		{
		case eConnectionModelColoums_Name:
			return tr("Name");
		case eConnectionModelColoums_Path:
			return tr("Path");
		}
	}
	return QVariant();
}

Qt::ItemFlags QConnectionModel::flags(const QModelIndex& index) const
{
	return QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled;
}

QModelIndex QConnectionModel::index(int row, int column, const QModelIndex& parent /*= QModelIndex()*/) const
{
	if (m_pAudioSystem && m_pControl)
	{
		if ((row >= 0) && (column >= 0))
		{
			if (row < m_connectionsCache.size())
			{
				ConnectionPtr pConnection = m_connectionsCache[row];
				if (pConnection)
				{
					IAudioSystemItem* pItem = m_pAudioSystem->GetControl(pConnection->GetID());
					if (pItem)
					{
						return createIndex(row, column);
					}
				}
			}
		}
	}
	return QModelIndex();
}

QModelIndex QConnectionModel::parent(const QModelIndex& index) const
{
	return QModelIndex();
}

bool QConnectionModel::canDropMimeData(const QMimeData* pData, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
	if (m_pAudioSystem && m_pControl)
	{
		std::vector<CID> ids;
		DecodeMimeData(pData, ids);
		for (auto id : ids)
		{
			IAudioSystemItem* pItem = m_pAudioSystem->GetControl(id);
			if (pItem)
			{
				// is the type being dragged compatible?
				if (!(m_pAudioSystem->GetCompatibleTypes(m_pControl->GetType()) & pItem->GetType()))
				{
					return false;
				}
			}
		}
	}
	return true;
}

QStringList QConnectionModel::mimeTypes() const
{
	QStringList list = QAbstractItemModel::mimeTypes();
	list << QAudioSystemModel::ms_szMimeType;
	return list;
}

bool QConnectionModel::dropMimeData(const QMimeData* pData, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
	if (m_pAudioSystem && m_pControl)
	{
		std::vector<CID> ids;
		DecodeMimeData(pData, ids);
		for (auto id : ids)
		{
			IAudioSystemItem* pItem = m_pAudioSystem->GetControl(id);
			if (pItem)
			{
				ConnectionPtr pConnection = m_pControl->GetConnection(pItem, m_group);
				if (!pConnection)
				{
					ConnectionPtr pConnection = m_pAudioSystem->CreateConnectionToControl(m_pControl->GetType(), pItem);
					if (pConnection)
					{
						beginResetModel();
						CUndo undo("Connected Audio Control to Audio System");
						pConnection->SetGroup(m_group);
						m_pControl->AddConnection(pConnection);
						ResetCache();
						endResetModel();
					}
				}
			}
		}
	}
	return QAbstractItemModel::dropMimeData(pData, action, row, column, parent);
}

Qt::DropActions QConnectionModel::supportedDropActions() const
{
	return Qt::CopyAction;
}

void QConnectionModel::OnConnectionAdded(CATLControl* pControl, IAudioSystemItem* pMiddlewareControl)
{
	if (pControl == m_pControl)
	{
		beginResetModel();
		ResetCache();
		endResetModel();
	}
}

void QConnectionModel::OnConnectionRemoved(CATLControl* pControl, IAudioSystemItem* pMiddlewareControl)
{
	if (pControl == m_pControl)
	{
		beginResetModel();
		ResetCache();
		endResetModel();
	}
}

void QConnectionModel::OnControlModified(CATLControl* pControl)
{
	if (pControl == m_pControl)
	{
		beginResetModel();
		ResetCache();
		endResetModel();
	}
}

void QConnectionModel::ResetCache()
{
	m_connectionsCache.clear();
	if (m_pControl)
	{
		const int size = m_pControl->GetConnectionCount();
		for (int i = 0; i < size; ++i)
		{
			ConnectionPtr pConnection = m_pControl->GetConnectionAt(i);
			if (pConnection && pConnection->GetGroup() == m_group)
			{
				m_connectionsCache.push_back(pConnection);
			}
		}
	}
}

void QConnectionModel::DecodeMimeData(const QMimeData* pData, std::vector<CID>& ids) const
{
	const QString format = QAudioSystemModel::ms_szMimeType;
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
				ids.push_back(id);
			}
		}
	}
}

}
