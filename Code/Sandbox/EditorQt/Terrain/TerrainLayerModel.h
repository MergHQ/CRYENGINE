// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <QAbstractItemModel>

class CTerrainManager;

class CTerrainLayerModel : public QAbstractItemModel
{
	enum LayerInfo
	{
		eLayerInfo_Name,
		eLayerInfo_FilterColor,
		eLayerInfo_MinHeight,
		eLayerInfo_MaxHeight,
		eLayerInfo_MinAngle,
		eLayerInfo_MaxAngle,
		eLayerInfo_Texture,
		eLayerInfo_Material
	};

public:
	CTerrainLayerModel(QWidget* parent, CTerrainManager* pTerrainManager);
	~CTerrainLayerModel();

	static const int s_layerPreviewSize = 32;

private:
	//QAbstractItemModel
	virtual QVariant        headerData(int section, Qt::Orientation orientation, int role) const override;
	virtual int             columnCount(const QModelIndex& parent = QModelIndex()) const override;
	virtual int             rowCount(const QModelIndex& parent = QModelIndex()) const override;
	virtual QVariant        data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	virtual QModelIndex     parent(const QModelIndex& child) const override;
	virtual QModelIndex     index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

	virtual Qt::ItemFlags   flags(const QModelIndex& index) const override;
	virtual bool            setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
	virtual QMimeData*      mimeData(const QModelIndexList& indexes) const override;
	virtual bool            canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
	virtual QStringList     mimeTypes() const override;
	virtual bool            dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

	virtual Qt::DropActions supportedDragActions() const override;
	virtual Qt::DropActions supportedDropActions() const override;

	void                    ReloadLayers();

	CTerrainManager* m_pTerrainManager;
	QList<QIcon>     m_previews;
};
