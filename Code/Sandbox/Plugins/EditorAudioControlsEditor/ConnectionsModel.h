// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QAbstractItemModel>
#include <ACETypes.h>

namespace ACE
{
class CAudioControl;
class IAudioSystemEditor;

class CConnectionModel final: public QAbstractItemModel
{
public:

	CConnectionModel();
	virtual ~CConnectionModel() override;

	void Init(CAudioControl* pControl);

	enum class EConnectionModelRoles
	{
		Id = Qt::UserRole + 1,
	};

	enum class EConnectionModelColumns
	{
		Name = 0,
		Path,
		Size,
	};

	// QAbstractTableModel
	virtual int             rowCount(const QModelIndex& parent) const override;
	virtual int             columnCount(const QModelIndex& parent) const override;
	virtual QVariant        data(const QModelIndex& index, int role) const override;
	virtual QVariant        headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	virtual Qt::ItemFlags   flags(const QModelIndex& index) const override;
	virtual QModelIndex     index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	virtual QModelIndex     parent(const QModelIndex& index) const override;
	virtual bool            canDropMimeData(const QMimeData* pData, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
	virtual QStringList     mimeTypes() const override;
	virtual bool            dropMimeData(const QMimeData* pData, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
	virtual Qt::DropActions supportedDropActions() const override;
	virtual bool            setData(const QModelIndex& index, const QVariant& value, int role) override;
	// ~QAbstractTableModel

private:

	void ResetCache();
	void DecodeMimeData(const QMimeData* pData, std::vector<CID>& ids) const;

	CAudioControl*             m_pControl;
	IAudioSystemEditor*        m_pAudioSystem;
	std::vector<ConnectionPtr> m_connectionsCache;
	std::vector<QString>       m_platformNames;
};
} // namespace ACE
