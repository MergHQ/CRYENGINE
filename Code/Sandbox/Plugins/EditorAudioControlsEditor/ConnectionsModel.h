// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/SharedData.h"
#include <QAbstractItemModel>

namespace ACE
{
struct IConnection;
class CControl;

class CConnectionsModel final : public QAbstractItemModel
{
	Q_OBJECT

public:

	CConnectionsModel() = delete;
	CConnectionsModel(CConnectionsModel const&) = delete;
	CConnectionsModel(CConnectionsModel&&) = delete;
	CConnectionsModel& operator=(CConnectionsModel const&) = delete;
	CConnectionsModel& operator=(CConnectionsModel&&) = delete;

	enum class EColumns : CryAudio::EnumFlagsType
	{
		Notification,
		Name,
		Path,
		Count, };

	explicit CConnectionsModel(QObject* const pParent);
	virtual ~CConnectionsModel() override;

	void Init(CControl* const pControl);
	void DisconnectSignals();

	// QAbstractItemModel
	virtual int             rowCount(QModelIndex const& parent) const override;
	virtual int             columnCount(QModelIndex const& parent) const override;
	virtual QVariant        data(QModelIndex const& index, int role) const override;
	virtual bool            setData(QModelIndex const& index, QVariant const& value, int role) override;
	virtual QVariant        headerData(int section, Qt::Orientation orientation, int role) const override;
	virtual Qt::ItemFlags   flags(QModelIndex const& index) const override;
	virtual QModelIndex     index(int row, int column, QModelIndex const& parent = QModelIndex()) const override;
	virtual QModelIndex     parent(QModelIndex const& index) const override;
	virtual bool            canDropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const override;
	virtual bool            dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) override;
	virtual Qt::DropActions supportedDropActions() const override;
	virtual QStringList     mimeTypes() const override;
	// ~QAbstractItemModel

signals:

	void SignalConnectionAdded(ControlId const id);

private:

	void ConnectSignals();
	void ResetCache();
	void ResetModelAndCache();

	CControl*  m_pControl;
	ControlIds m_connectionIdCache;
};
} // namespace ACE
