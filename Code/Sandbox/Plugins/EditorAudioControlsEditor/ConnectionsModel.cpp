// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ConnectionsModel.h"
#include <IAudioSystemEditor.h>
#include <IAudioSystemItem.h>
#include "AudioAssets.h"
#include "AudioControlsEditorPlugin.h"
#include "MiddlewareDataModel.h"
#include "ImplementationManager.h"
#include "IUndoObject.h"
#include "EditorStyleHelper.h"
#include <CrySystem/File/CryFile.h>  // Includes CryPath.h in correct order.
#include <CryIcon.h>
#include <QtUtil.h>

#include <QMimeData>
#include <ConfigurationManager.h>

namespace ACE
{
//////////////////////////////////////////////////////////////////////////
CConnectionModel::CConnectionModel()
	: m_pControl(nullptr)
	, m_pAudioSystem(CAudioControlsEditorPlugin::GetAudioSystemEditorImpl())
{
	auto resetFunction = [&]()
	{
		beginResetModel();
		ResetCache();
		endResetModel();
	};

	CAudioAssetsManager* pAssetsManager = CAudioControlsEditorPlugin::GetAssetsManager();
	pAssetsManager->signalItemAdded.Connect(resetFunction, reinterpret_cast<uintptr_t>(this));
	pAssetsManager->signalItemRemoved.Connect(resetFunction, reinterpret_cast<uintptr_t>(this));
	pAssetsManager->signalControlModified.Connect(resetFunction, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationAboutToChange.Connect([&]()
		{
			beginResetModel();
			m_pAudioSystem = nullptr;
			m_connectionsCache.clear();
			endResetModel();
	  }, reinterpret_cast<uintptr_t>(this));

	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.Connect([&]()
		{
			m_pAudioSystem = CAudioControlsEditorPlugin::GetAudioSystemEditorImpl();
			beginResetModel();
			ResetCache();
			endResetModel();
	  }, reinterpret_cast<uintptr_t>(this));

	std::vector<dll_string> const& platforms = GetIEditor()->GetConfigurationManager()->GetPlatformNames();

	for (auto const& platform : platforms)
	{
		m_platformNames.push_back(QtUtil::ToQStringSafe(platform.c_str()));
	}
}

//////////////////////////////////////////////////////////////////////////
CConnectionModel::~CConnectionModel()
{
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationAboutToChange.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioControlsEditorPlugin::GetImplementationManger()->signalImplementationChanged.DisconnectById(reinterpret_cast<uintptr_t>(this));
	CAudioAssetsManager* pAssetsManager = CAudioControlsEditorPlugin::GetAssetsManager();
	pAssetsManager->signalItemAdded.DisconnectById(reinterpret_cast<uintptr_t>(this));
	pAssetsManager->signalItemRemoved.DisconnectById(reinterpret_cast<uintptr_t>(this));
	pAssetsManager->signalControlModified.DisconnectById(reinterpret_cast<uintptr_t>(this));
}

//////////////////////////////////////////////////////////////////////////
void CConnectionModel::Init(CAudioControl* const pControl)
{
	beginResetModel();
	m_pControl = pControl;
	ResetCache();
	endResetModel();
}

//////////////////////////////////////////////////////////////////////////
int CConnectionModel::rowCount(QModelIndex const& parent) const
{
	if ((m_pControl != nullptr) && (m_pAudioSystem != nullptr))
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
	return static_cast<int>(EConnectionModelColumns::Size) + static_cast<int>(m_platformNames.size());
}

//////////////////////////////////////////////////////////////////////////
QVariant CConnectionModel::data(QModelIndex const& index, int role) const
{
	if ((m_pAudioSystem != nullptr) && (m_pControl != nullptr) && index.isValid())
	{
		if (index.row() < m_connectionsCache.size())
		{
			ConnectionPtr const pConnection = m_connectionsCache[index.row()];

			if (pConnection != nullptr)
			{
				IAudioSystemItem const* const pItem = m_pAudioSystem->GetControl(pConnection->GetID());

				if (pItem != nullptr)
				{
					switch (role)
					{
					case Qt::DisplayRole:
						switch (index.column())
						{
						case static_cast<int>(EConnectionModelColumns::Name):
							return (const char*)pItem->GetName();
						case static_cast<int>(EConnectionModelColumns::Path):
							{
								QString path;
								IAudioSystemItem const* pParent = pItem->GetParent();

								while (pParent != nullptr)
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
						if (index.column() == static_cast<int>(EConnectionModelColumns::Name))
						{
							return CryIcon((QtUtil::ToQString(PathUtil::GetEnginePath()) + CRY_NATIVE_PATH_SEPSTR) + m_pAudioSystem->GetTypeIcon(pItem->GetType()));
						}
						break;
					case Qt::ForegroundRole:
						if (pItem->IsPlaceholder())
						{
							return GetStyleHelper()->errorColor();
						}
						break;
					case Qt::ToolTipRole:
						if (pItem->IsPlaceholder())
						{
							return tr("Control not found in the audio middleware project");
						}
						break;
					case Qt::CheckStateRole:
						{
							if ((m_pControl->GetType() == EItemType::Preload) && (index.column() >= static_cast<int>(EConnectionModelColumns::Size)))
							{
								return pConnection->IsPlatformEnabled(index.column() - static_cast<int>(EConnectionModelColumns::Size)) ? Qt::Checked : Qt::Unchecked;
							}
							break;
						}
					case static_cast<int>(EConnectionModelRoles::Id):
						if (index.column() == static_cast<int>(EConnectionModelColumns::Name))
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

//////////////////////////////////////////////////////////////////////////
QVariant CConnectionModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
	if ((orientation == Qt::Horizontal) && (role == Qt::DisplayRole))
	{
		if (section < static_cast<int>(EConnectionModelColumns::Size))
		{
			switch (section)
			{
			case static_cast<int>(EConnectionModelColumns::Name):
				return tr("Name");
			case static_cast<int>(EConnectionModelColumns::Path):
				return tr("Path");
			}
		}
		else
		{
			return m_platformNames[section - static_cast<int>(EConnectionModelColumns::Size)];
		}
	}

	return QVariant();
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags CConnectionModel::flags(QModelIndex const& index) const
{
	Qt::ItemFlags flags = QAbstractItemModel::flags(index);

	if (index.isValid() && (index.column() >= static_cast<int>(EConnectionModelColumns::Size)))
	{
		flags |= Qt::ItemIsUserCheckable;
	}

	return flags | Qt::ItemIsDropEnabled;
}

//////////////////////////////////////////////////////////////////////////
bool CConnectionModel::setData(QModelIndex const& index, QVariant const& value, int role)
{
	if ((index.column() >= static_cast<int>(EConnectionModelColumns::Size)) && (role == Qt::CheckStateRole))
	{
		ConnectionPtr const pConnection = m_connectionsCache[index.row()];
		pConnection->EnableForPlatform(index.column() - static_cast<int>(EConnectionModelColumns::Size), value == Qt::Checked);
		QVector<int> roleVector(1, role);
		dataChanged(index, index, roleVector);
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
QModelIndex CConnectionModel::index(int row, int column, QModelIndex const& parent /*= QModelIndex()*/) const
{
	if ((m_pAudioSystem != nullptr) && (m_pControl != nullptr))
	{
		if ((row >= 0) && (column >= 0))
		{
			if (row < m_connectionsCache.size())
			{
				ConnectionPtr const pConnection = m_connectionsCache[row];

				if (pConnection != nullptr)
				{
					IAudioSystemItem const* const pItem = m_pAudioSystem->GetControl(pConnection->GetID());

					if (pItem != nullptr)
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
	if ((m_pAudioSystem != nullptr) && (m_pControl != nullptr))
	{
		std::vector<CID> ids;
		DecodeMimeData(pData, ids);

		for (auto const id : ids)
		{
			IAudioSystemItem const* const pItem = m_pAudioSystem->GetControl(id);

			if (pItem != nullptr)
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

//////////////////////////////////////////////////////////////////////////
QStringList CConnectionModel::mimeTypes() const
{
	QStringList list = QAbstractItemModel::mimeTypes();
	list << CMiddlewareDataModel::ms_szMimeType;
	return list;
}

//////////////////////////////////////////////////////////////////////////
bool CConnectionModel::dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent)
{
	if ((m_pAudioSystem != nullptr) && (m_pControl != nullptr))
	{
		std::vector<CID> ids;
		DecodeMimeData(pData, ids);
		bool wasConnectionAdded = false;

		for (auto const id : ids)
		{
			IAudioSystemItem* const pItem = m_pAudioSystem->GetControl(id);

			if (pItem != nullptr)
			{
				ConnectionPtr pConnection = m_pControl->GetConnection(pItem);

				if (pConnection == nullptr)
				{
					pConnection = m_pAudioSystem->CreateConnectionToControl(m_pControl->GetType(), pItem);

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
				m_connectionsCache.push_back(pConnection);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CConnectionModel::DecodeMimeData(QMimeData const* pData, std::vector<CID>& ids) const
{
	QString const format = CMiddlewareDataModel::ms_szMimeType;

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
} // namespace ACE
