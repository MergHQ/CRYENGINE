// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ProxyModels/AttributeFilterProxyModel.h>

namespace ACE
{
class CSystemFilterProxyModel final : public QAttributeFilterProxyModel
{
public:

	CSystemFilterProxyModel() = delete;
	CSystemFilterProxyModel(CSystemFilterProxyModel const&) = delete;
	CSystemFilterProxyModel(CSystemFilterProxyModel&&) = delete;
	CSystemFilterProxyModel& operator=(CSystemFilterProxyModel const&) = delete;
	CSystemFilterProxyModel& operator=(CSystemFilterProxyModel&&) = delete;

	CSystemFilterProxyModel(QObject* pParent)
		: QAttributeFilterProxyModel(QDeepFilterProxyModel::Behavior::AcceptIfChildMatches, pParent)
	{}

	virtual ~CSystemFilterProxyModel() override = default;

protected:

	// QAttributeFilterProxyModel
	virtual bool rowMatchesFilter(int sourceRow, QModelIndex const& sourcePparent) const override;
	// ~QAttributeFilterProxyModel

	// QSortFilterProxyModel
	virtual bool lessThan(QModelIndex const& left, QModelIndex const& right) const override;
	// ~QSortFilterProxyModel
};
} // namespace ACE
