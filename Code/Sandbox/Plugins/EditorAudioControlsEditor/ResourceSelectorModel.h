// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SystemControlsModel.h"
#include <ProxyModels/DeepFilterProxyModel.h>

namespace ACE
{
class CSystemLibrary;
class CSystemAssetsManager;

namespace ResourceModelUtils
{
QVariant GetHeaderData(int const section, Qt::Orientation const orientation, int const role);
} // namespace ResourceModelUtils

class CResourceSourceModel final : public CSystemSourceModel
{
public:

	CResourceSourceModel(CSystemAssetsManager* const pAssetsManager, QObject* const pParent)
		: CSystemSourceModel(pAssetsManager, pParent)
	{}

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

	CResourceLibraryModel(CSystemAssetsManager* const pAssetsManager, CSystemLibrary* const pLibrary, QObject* const pParent)
		: CSystemLibraryModel(pAssetsManager, pLibrary, pParent)
	{}

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

	CResourceFilterProxyModel(ESystemItemType const type, Scope const scope, QObject* const pParent);

protected:

	// QDeepFilterProxyModel
	virtual bool rowMatchesFilter(int sourceRow, QModelIndex const& sourcePparent) const override;
	// ~QDeepFilterProxyModel

	// QSortFilterProxyModel
	virtual bool lessThan(QModelIndex const& left, QModelIndex const& right) const override;
	// ~QSortFilterProxyModel

private:

	ESystemItemType const m_type;
	Scope const           m_scope;
};
} // namespace ACE
