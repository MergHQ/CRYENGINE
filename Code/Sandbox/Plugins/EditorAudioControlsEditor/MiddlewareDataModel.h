// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ProxyModels/AttributeFilterProxyModel.h>

class CItemModelAttribute;

namespace ACE
{
class CImplItem;

class CMiddlewareDataModel final : public QAbstractItemModel
{
public:

	enum class EColumns
	{
		Notification,
		Connected,
		Localized,
		Name,
		Count,
	};

	enum class ERoles
	{
		Id = Qt::UserRole + 1,
		Name,
		ItemType,
		IsPlaceholder,
	};

	CMiddlewareDataModel(QObject* const pParent);
	virtual ~CMiddlewareDataModel() override;

	static CItemModelAttribute* GetAttributeForColumn(EColumns const column);
	static QVariant             GetHeaderData(int const section, Qt::Orientation const orientation, int const role);

	void                        DisconnectSignals();
	void                        Reset();

protected:

	// QAbstractItemModel
	virtual int             rowCount(QModelIndex const& parent) const override;
	virtual int             columnCount(QModelIndex const& parent) const override;
	virtual QVariant        data(QModelIndex const& index, int role) const override;
	virtual QVariant        headerData(int section, Qt::Orientation orientation, int role) const override;
	virtual Qt::ItemFlags   flags(QModelIndex const& index) const override;
	virtual QModelIndex     index(int row, int column, QModelIndex const& parent = QModelIndex()) const override;
	virtual QModelIndex     parent(QModelIndex const& index) const override;
	virtual Qt::DropActions supportedDragActions() const override;
	virtual QStringList     mimeTypes() const override;
	virtual QMimeData*      mimeData(QModelIndexList const& indexes) const override;
	// ~QAbstractItemModel

private:

	void        ConnectSignals();
	CImplItem*  ItemFromIndex(QModelIndex const& index) const;
	QModelIndex IndexFromItem(CImplItem const* const pImplItem) const;
};

class CMiddlewareFilterProxyModel final : public QAttributeFilterProxyModel
{
public:

	CMiddlewareFilterProxyModel(QObject* const pParent);

protected:

	// QAttributeFilterProxyModel
	virtual bool rowMatchesFilter(int sourceRow, QModelIndex const& sourcePparent) const override;
	// ~QAttributeFilterProxyModel

	// QSortFilterProxyModel
	virtual bool lessThan(QModelIndex const& left, QModelIndex const& right) const override;
	// ~QSortFilterProxyModel
};
} // namespace ACE
