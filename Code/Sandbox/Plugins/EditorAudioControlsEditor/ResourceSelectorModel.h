// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SystemControlsModel.h"
#include <ProxyModels/DeepFilterProxyModel.h>

namespace ACE
{
class CLibrary;

namespace ResourceModelUtils
{
QVariant GetHeaderData(int const section, Qt::Orientation const orientation, int const role);
} // namespace ResourceModelUtils

class CResourceSourceModel final : public CSystemSourceModel
{
public:

	explicit CResourceSourceModel(QObject* const pParent)
		: CSystemSourceModel(pParent)
	{}

	CResourceSourceModel() = delete;

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

class CResourceLibraryModel final : public CSystemLibraryModel
{
public:

	explicit CResourceLibraryModel(CLibrary* const pLibrary, QObject* const pParent)
		: CSystemLibraryModel(pLibrary, pParent)
	{}

	CResourceLibraryModel() = delete;

protected:

	// CSystemLibraryModel
	virtual int             columnCount(QModelIndex const& parent) const override;
	virtual QVariant        data(QModelIndex const& index, int role) const override;
	virtual bool            setData(QModelIndex const& index, QVariant const& value, int role) override;
	virtual QVariant        headerData(int section, Qt::Orientation orientation, int role) const override;
	virtual Qt::ItemFlags   flags(QModelIndex const& index) const override;
	virtual bool            canDropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) const override;
	virtual bool            dropMimeData(QMimeData const* pData, Qt::DropAction action, int row, int column, QModelIndex const& parent) override;
	virtual Qt::DropActions supportedDropActions() const override;
	// ~CSystemLibraryModel
};

class CResourceFilterProxyModel final : public QDeepFilterProxyModel
{
public:

	explicit CResourceFilterProxyModel(EAssetType const type, Scope const scope, QObject* const pParent);

	CResourceFilterProxyModel() = delete;

protected:

	// QDeepFilterProxyModel
	virtual bool rowMatchesFilter(int sourceRow, QModelIndex const& sourcePparent) const override;
	// ~QDeepFilterProxyModel

	// QSortFilterProxyModel
	virtual bool lessThan(QModelIndex const& left, QModelIndex const& right) const override;
	// ~QSortFilterProxyModel

private:

	EAssetType const m_type;
	Scope const      m_scope;
};
} // namespace ACE

