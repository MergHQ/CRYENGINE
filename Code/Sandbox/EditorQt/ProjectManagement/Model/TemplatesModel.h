// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ProjectManagement/Model/SubIconContainer.h"
#include "ProjectManagement/Utils.h"
#include <QSortFilterProxyModel>

struct STemplateDescription;

class CTemplateManager;

class CTemplatesModel : public QAbstractItemModel
{
public:
	enum Column
	{
		eColumn_Name,
		eColumn_Language,
	};

	CTemplatesModel(QObject* pParent, const CTemplateManager& templateManager);

	const STemplateDescription* GetTemplateDescr(const QModelIndex& index) const;

private:
	// QAbstractItemModel
	virtual int         columnCount(const QModelIndex& parent) const override;
	virtual QVariant    headerData(int section, Qt::Orientation orientation, int role) const override;
	virtual int         rowCount(const QModelIndex& parent) const override;
	virtual QVariant    data(const QModelIndex& index, int role) const override;
	virtual QModelIndex index(int row, int column, const QModelIndex& parent) const override;
	virtual QModelIndex parent(const QModelIndex& child) const override;

	const CTemplateManager& m_templateManager;
	const CSubIconContainer m_subicons;
};

// Sort + Search via several columns
class CTemplatesSortProxyModel : public QSortFilterProxyModel
{
public:
	explicit CTemplatesSortProxyModel(QObject* pParent);

private:
	virtual bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
};
