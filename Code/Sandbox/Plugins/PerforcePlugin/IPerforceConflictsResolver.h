// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "VersionControl/IVersionControlAdapter.h"

struct IPerforceConflictsResolver
{
	using FileStatusesMap = IVersionControlAdapter::FileStatusesMap;

	virtual ~IPerforceConflictsResolver() {}

	virtual void ResolveMMOurs(const std::vector<string>&, FileStatusesMap&) {}

	virtual void ResolveMMTheir(const std::vector<string>&, FileStatusesMap&) {}

	virtual void ResolveMDOurs(const std::vector<string>&, FileStatusesMap&) {}

	virtual void ResolveMDTheir(const std::vector<string>&, FileStatusesMap&) {}

	virtual void ResolveDMOurs(const std::vector<string>&, FileStatusesMap&) {}

	virtual void ResolveDMTheir(const std::vector<string>&, FileStatusesMap&) {}

	virtual void ResolveAAOurs(const std::vector<string>&, FileStatusesMap&) {}

	virtual void ResolveAATheir(const std::vector<string>&, FileStatusesMap&) {}

	virtual void ResolveDD(const std::vector<string>&, FileStatusesMap&) {}
};
