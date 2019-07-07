// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "TerrainLayerModel.h"
#include "IEditorImpl.h"

#include "Terrain/TerrainManager.h"
#include "Terrain/TerrainLayerUndoObject.h"

#include <DragDrop.h>
#include <Util/ImageUtil.h>
#include <ProxyModels/ItemModelAttribute.h>

namespace Private_TerrainLayerModel
{

CItemModelAttribute s_LayerAttributes[] =
{
	Attributes::s_nameAttribute,
	CItemModelAttribute("Filter Color", &Attributes::s_stringAttributeType, CItemModelAttribute::StartHidden, false, ""),
	CItemModelAttribute("Min Height",   &Attributes::s_floatAttributeType,  CItemModelAttribute::StartHidden, false, ""),
	CItemModelAttribute("Max Height",   &Attributes::s_floatAttributeType,  CItemModelAttribute::StartHidden, false, ""),
	CItemModelAttribute("Min Angle",    &Attributes::s_floatAttributeType,  CItemModelAttribute::StartHidden, false, ""),
	CItemModelAttribute("Max Angle",    &Attributes::s_floatAttributeType,  CItemModelAttribute::StartHidden, false, ""),
	CItemModelAttribute("Texture",      &Attributes::s_stringAttributeType, CItemModelAttribute::StartHidden, false, ""),
	CItemModelAttribute("Material",     &Attributes::s_stringAttributeType, CItemModelAttribute::StartHidden, false, ""), };

const int s_attributeCount = sizeof(s_LayerAttributes) / sizeof(CItemModelAttribute);

const char* const s_terrainLayerDragMimeFormat = "TerrainLayerDragFormat";
}

CTerrainLayerModel::CTerrainLayerModel(QWidget* parent, CTerrainManager* pTerrainManager)
	: QAbstractItemModel(parent)
	, m_pTerrainManager(pTerrainManager)
{
	CRY_ASSERT(m_pTerrainManager);

	ReloadLayers();

	m_pTerrainManager->signalLayersChanged.Connect(this, &CTerrainLayerModel::ReloadLayers);
}

CTerrainLayerModel::~CTerrainLayerModel()
{
	m_pTerrainManager->signalLayersChanged.DisconnectObject(this);
}

void CTerrainLayerModel::ReloadLayers()
{
	if (0 < rowCount())
	{
		beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
		endRemoveRows();
	}

	const int layerCount = m_pTerrainManager->GetLayerCount();

	m_previews.clear();
	m_previews.reserve(layerCount);

	CImageEx preview;
	preview.Allocate(s_layerPreviewSize, s_layerPreviewSize);

	for (int i = 0; i < layerCount; ++i)
	{
		preview.Fill(128);

		CLayer* pLayer = GetIEditorImpl()->GetTerrainManager()->GetLayer(i);
		CImageEx& previewImage = pLayer->GetTexturePreviewImage();

		if (previewImage.IsValid())
			CImageUtil::ScaleToFit(previewImage, preview);

		preview.SwapRedAndBlue();

		QImage img = QImage((const uchar*)preview.GetData(), s_layerPreviewSize, s_layerPreviewSize, QImage::Format_ARGB32);
		m_previews.push_back(QIcon(QPixmap().fromImage(img)));
	}

	if (0 < layerCount)
	{
		beginInsertRows(QModelIndex(), 0, rowCount() - 1);
		endInsertRows();
	}
}

QVariant CTerrainLayerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && Private_TerrainLayerModel::s_attributeCount > section)
	{
		switch (role)
		{
		case Qt::DisplayRole:
			return Private_TerrainLayerModel::s_LayerAttributes[section].GetName();

		case Attributes::s_attributeMenuPathRole:
			if (section != (int)eLayerInfo_Name)
				return "";
		}
	}

	return QVariant();
}

int CTerrainLayerModel::columnCount(const QModelIndex& parent) const
{
	if (!parent.isValid())
		return Private_TerrainLayerModel::s_attributeCount;

	return 0;
}

int CTerrainLayerModel::rowCount(const QModelIndex& parent) const
{
	if (parent.isValid())
	{
		// Flat structure: no children for an element
		return 0;
	}

	return m_pTerrainManager->GetLayerCount();
}

QVariant CTerrainLayerModel::data(const QModelIndex& index, int role) const
{
	const int row = index.row();
	if (row >= m_pTerrainManager->GetLayerCount())
		return QVariant();

	const int col = index.column();

	switch (role)
	{
	case Qt::EditRole:
	case Qt::DisplayRole:
		if (Private_TerrainLayerModel::s_attributeCount > col)
		{
			CLayer* pLayer = m_pTerrainManager->GetLayer(row);

			switch ((LayerInfo)col)
			{
			case eLayerInfo_Name:
				return QString(pLayer->GetLayerName().GetBuffer());
			case eLayerInfo_MinHeight:
				return pLayer->GetLayerStart();
			case eLayerInfo_MaxHeight:
				return pLayer->GetLayerEnd();
			case eLayerInfo_MinAngle:
				return pLayer->GetLayerMinSlopeAngle();
			case eLayerInfo_MaxAngle:
				return pLayer->GetLayerMaxSlopeAngle();
			case eLayerInfo_Texture:
				return QString(pLayer->GetTextureFilename().c_str());
			case eLayerInfo_Material:
				return QString(PathUtil::GetFileName(pLayer->GetMaterialName()).c_str());
			}
		}
		break;
	case Qt::DecorationRole:
		if (Private_TerrainLayerModel::s_attributeCount > col)
		{
			CLayer* pLayer = m_pTerrainManager->GetLayer(row);
			switch (col)
			{
			case eLayerInfo_Name:
				return m_previews[row];
			case eLayerInfo_FilterColor:
				return QColor::fromRgbF(pLayer->GetLayerFilterColor().r, pLayer->GetLayerFilterColor().g, pLayer->GetLayerFilterColor().b, pLayer->GetLayerFilterColor().a);
			}
		}
		break;

	case Qt::SizeHintRole:
		return QSize(s_layerPreviewSize, s_layerPreviewSize + 2);
	}

	return QVariant();
}

QModelIndex CTerrainLayerModel::parent(const QModelIndex& child) const
{
	// Data is flat: no parent
	return QModelIndex();
}

QModelIndex CTerrainLayerModel::index(int row, int column, const QModelIndex& parent) const
{
	if (row < 0 || row >= m_pTerrainManager->GetLayerCount())
	{
		return QModelIndex();
	}

	return createIndex(row, column);
}

Qt::DropActions CTerrainLayerModel::supportedDragActions() const
{
	return Qt::MoveAction;
}

Qt::DropActions CTerrainLayerModel::supportedDropActions() const
{
	return Qt::MoveAction;
}

Qt::ItemFlags CTerrainLayerModel::flags(const QModelIndex& index) const
{
	Qt::ItemFlags flags = __super::flags(index) | Qt::ItemNeverHasChildren;

	if (!index.isValid())
	{
		return flags | Qt::ItemIsDropEnabled;
	}

	flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
	if (index.column() == (int)eLayerInfo_Name)
	{
		flags |= Qt::ItemIsEditable;
	}

	return flags;
}

QMimeData* CTerrainLayerModel::mimeData(const QModelIndexList& indexes) const
{
	if (indexes.empty())
	{
		return nullptr;
	}

	QByteArray encodedData;
	QDataStream stream(&encodedData, QIODevice::WriteOnly);

	// This is single-selection model. Any index in indexes is from the same row
	const auto& idx = indexes[0];
	stream << idx.row();

	QMimeData* pMimeData = new QMimeData;
	pMimeData->setData(CDragDropData::GetMimeFormatForType(Private_TerrainLayerModel::s_terrainLayerDragMimeFormat), encodedData);
	return pMimeData;
}

bool CTerrainLayerModel::canDropMimeData(const QMimeData* pData, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
	if (!pData->hasFormat(CDragDropData::GetMimeFormatForType(Private_TerrainLayerModel::s_terrainLayerDragMimeFormat)))
	{
		return false;
	}

	return true;
}

QStringList CTerrainLayerModel::mimeTypes() const
{
	QStringList typeList;
	typeList << CDragDropData::GetMimeFormatForType(Private_TerrainLayerModel::s_terrainLayerDragMimeFormat);
	return typeList;
}

bool CTerrainLayerModel::dropMimeData(const QMimeData* pData, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
	if (!canDropMimeData(pData, action, row, column, parent))
		return false;

	QByteArray byteArray = pData->data(CDragDropData::GetMimeFormatForType(Private_TerrainLayerModel::s_terrainLayerDragMimeFormat));
	if (byteArray.length() != 4)
	{
		CRY_ASSERT("Invalid data size");
		return false;
	}

	if (parent.isValid())
	{
		// Drop on a layer: get drop index from parent's row, and add 1 to drop it below the row
		row = parent.row() + 1;
	}

	QDataStream stream(&byteArray, QIODevice::ReadOnly);
	int oldPos = 0;
	stream >> oldPos;

	m_pTerrainManager->MoveLayer(oldPos, row);

	return true;
}

bool CTerrainLayerModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	const int row = index.row();

	if (m_pTerrainManager->GetLayerCount() <= row && 0 <= row && value.isValid())
		return false;

	switch (role)
	{
	case Qt::EditRole:
		CLayer* pLayer = m_pTerrainManager->GetLayer(row);
		QString newName(value.toString());
		QString oldName(pLayer->GetLayerName().GetBuffer());

		if (newName != oldName && !newName.isEmpty())
		{
			CUndo undo("Terrain Layer Renaming");
			GetIEditorImpl()->GetIUndoManager()->RecordUndo(new CTerrainLayersPropsUndoObject);
			pLayer->SetLayerName(newName.toUtf8().constData());
			return true;
		}
	}

	return false;
}
