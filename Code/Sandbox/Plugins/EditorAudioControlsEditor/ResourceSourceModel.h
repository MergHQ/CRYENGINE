// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SystemSourceModel.h"

namespace ACE
{
class CResourceSourceModel final : public CSystemSourceModel
{
public:

	CResourceSourceModel() = delete;
	CResourceSourceModel(CResourceSourceModel const&) = delete;
	CResourceSourceModel(CResourceSourceModel&&) = delete;
	CResourceSourceModel& operator=(CResourceSourceModel const&) = delete;
	CResourceSourceModel& operator=(CResourceSourceModel&&) = delete;

	explicit CResourceSourceModel(QObject* const pParent)
		: CSystemSourceModel(pParent)
	{}

	virtual ~CResourceSourceModel() override = default;

	static QVariant GetHeaderData(int const section, Qt::Orientation const orientation, int const role);

protected:

	// CSystemSourceModel
	virtual int             columnCount(QModelIndex const& parent) const override;
	virtual QVariant        data(QModelIndex const& index, int role) const override;
	virtual bool            setData(QModelIndex const& index, QVariant const& value, int role) override;
	virtual QVariant        headerData(int section, Qt::Orientation orientation, int role) const override;
	virtual Qt::ItemFlags   flags(QModelIndex const& index) const override;
	virtual bool            canDropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const override;
	virtual bool            dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) override;
	virtual Qt::DropActions supportedDropActions() const override;
	// ~CSystemSourceModel
};
} // namespace ACE
