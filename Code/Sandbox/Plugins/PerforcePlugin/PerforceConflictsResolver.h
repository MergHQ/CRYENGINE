// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "IPerforceConflictsResolver.h"

struct IPerforceExecutor;
struct IPerforceOutputParser;

class CPerforceConflictsResolver : public IPerforceConflictsResolver
{
public:
	CPerforceConflictsResolver(IPerforceExecutor* executor, IPerforceOutputParser* parser)
		: m_executor(executor)
		, m_parser(parser)
	{}

	virtual void ResolveMMOurs(const std::vector<string>& filePaths, FileStatusesMap& statusesMap) override;

	virtual void ResolveMMTheir(const std::vector<string>& filePaths, FileStatusesMap& statusesMap) override;

	virtual void ResolveMDOurs(const std::vector<string>& filePaths, FileStatusesMap& statusesMap) override;

	virtual void ResolveMDTheir(const std::vector<string>& filePaths, FileStatusesMap& statusesMap) override;

	virtual void ResolveDMOurs(const std::vector<string>& filePaths, FileStatusesMap& statusesMap) override;

	virtual void ResolveDMTheir(const std::vector<string>& filePaths, FileStatusesMap& statusesMap) override;

	virtual void ResolveAAOurs(const std::vector<string>& filePaths, FileStatusesMap& statusesMap) override;

	virtual void ResolveAATheir(const std::vector<string>& filePaths, FileStatusesMap& statusesMap) override;

	virtual void ResolveDD(const std::vector<string>& filePaths, FileStatusesMap& statusesMap) override;

private:
	IPerforceExecutor*     m_executor;
	IPerforceOutputParser* m_parser;
};
