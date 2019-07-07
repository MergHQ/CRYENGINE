// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ProxyModels/DeepFilterProxyModel.h>

namespace ACE
{
class CFolderSelectorFilterModel final : public QDeepFilterProxyModel
{
public:

	CFolderSelectorFilterModel() = delete;
	CFolderSelectorFilterModel(CFolderSelectorFilterModel const&) = delete;
	CFolderSelectorFilterModel(CFolderSelectorFilterModel&&) = delete;
	CFolderSelectorFilterModel& operator=(CFolderSelectorFilterModel const&) = delete;
	CFolderSelectorFilterModel& operator=(CFolderSelectorFilterModel&&) = delete;

	explicit CFolderSelectorFilterModel(QString const& assetpath, QObject* pParent)
		: QDeepFilterProxyModel(QDeepFilterProxyModel::Behavior::AcceptIfChildMatches, pParent)
		, m_assetPath(assetpath)
	{}

	virtual ~CFolderSelectorFilterModel() override = default;

protected:

	// QDeepFilterProxyModel
	virtual bool rowMatchesFilter(int sourceRow, QModelIndex const& sourcePparent) const override;
	// ~QDeepFilterProxyModel

private:

	QString const m_assetPath;
};
} // namespace ACE
