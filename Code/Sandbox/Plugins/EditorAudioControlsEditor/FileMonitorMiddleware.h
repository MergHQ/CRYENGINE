// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "FileMonitorBase.h"

namespace ACE
{
class CFileMonitorMiddleware final : public CFileMonitorBase
{
	Q_OBJECT

public:

	CFileMonitorMiddleware() = delete;
	CFileMonitorMiddleware(CFileMonitorMiddleware const&) = delete;
	CFileMonitorMiddleware(CFileMonitorMiddleware&&) = delete;
	CFileMonitorMiddleware& operator=(CFileMonitorMiddleware const&) = delete;
	CFileMonitorMiddleware& operator=(CFileMonitorMiddleware&&) = delete;

	explicit CFileMonitorMiddleware(int const delay, QObject* const pParent);
	virtual ~CFileMonitorMiddleware() override = default;

	void Enable();

private:

	std::vector<char const*> m_monitorFolders;
};
} // namespace ACE
