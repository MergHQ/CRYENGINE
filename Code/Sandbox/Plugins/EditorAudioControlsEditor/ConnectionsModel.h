// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QAbstractItemModel>
#include <SharedData.h>

class CItemModelAttribute;

namespace ACE
{
class CControl;

class CConnectionsModel final : public QAbstractItemModel
{
	Q_OBJECT

public:

	enum class EColumns
	{
		Notification,
		Name,
		Path,
		Count,
	};

	enum class ERoles
	{
		Id = Qt::UserRole + 1,
		Name,
	};

	explicit CConnectionsModel(QObject* const pParent);
	virtual ~CConnectionsModel() override;

	CConnectionsModel() = delete;

	void                        Init(CControl* const pControl);
	void                        DisconnectSignals();

	static CItemModelAttribute* GetAttributeForColumn(EColumns const column);
	static QVariant             GetHeaderData(int const section, Qt::Orientation const orientation, int const role);

	// QAbstractItemModel
	virtual int             rowCount(QModelIndex const& parent) const override;
	virtual int             columnCount(QModelIndex const& parent) const override;
	virtual QVariant        data(QModelIndex const& index, int role) const override;
	virtual QVariant        headerData(int section, Qt::Orientation orientation, int role) const override;
	virtual Qt::ItemFlags   flags(QModelIndex const& index) const override;
	virtual QModelIndex     index(int row, int column, QModelIndex const& parent = QModelIndex()) const override;
	virtual QModelIndex     parent(QModelIndex const& index) const override;
	virtual bool            canDropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const override;
	virtual QStringList     mimeTypes() const override;
	virtual bool            dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) override;
	virtual Qt::DropActions supportedDropActions() const override;
	virtual bool            setData(QModelIndex const& index, QVariant const& value, int role) override;
	// ~QAbstractItemModel

signals:

	void SignalConnectionAdded(ControlId const id);

private:

	using ImplItemIds = std::vector<ControlId>;

	void ConnectSignals();
	void ResetCache();
	void ResetModelAndCache();
	void DecodeMimeData(QMimeData const* pData, ImplItemIds& ids) const;

	CControl*                  m_pControl;
	std::vector<ConnectionPtr> m_connectionsCache;
};
} // namespace ACE

