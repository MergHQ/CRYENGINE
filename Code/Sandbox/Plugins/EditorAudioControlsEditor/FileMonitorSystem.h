// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "FileMonitorBase.h"

namespace ACE
{
class CFileMonitorSystem final : public CFileMonitorBase
{
	Q_OBJECT

public:

	CFileMonitorSystem() = delete;
	CFileMonitorSystem(CFileMonitorSystem const&) = delete;
	CFileMonitorSystem(CFileMonitorSystem&&) = delete;
	CFileMonitorSystem& operator=(CFileMonitorSystem const&) = delete;
	CFileMonitorSystem& operator=(CFileMonitorSystem&&) = delete;

	explicit CFileMonitorSystem(int const delay, QObject* const pParent);
	virtual ~CFileMonitorSystem() override = default;

	void Enable();
	void EnableDelayed();

private:

	string        m_monitorFolder;
	QTimer* const m_delayTimer;
};
} // namespace ACE
