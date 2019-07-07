// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/File/IFileChangeMonitor.h>
#include <QTimer>

namespace ACE
{
class CFileMonitorBase : public QTimer, public IFileChangeListener
{
	Q_OBJECT

public:

	CFileMonitorBase() = delete;
	CFileMonitorBase(CFileMonitorBase const&) = delete;
	CFileMonitorBase(CFileMonitorBase&&) = delete;
	CFileMonitorBase& operator=(CFileMonitorBase const&) = delete;
	CFileMonitorBase& operator=(CFileMonitorBase&&) = delete;

	void              Disable();

signals:

	void SignalReloadData();

protected:

	explicit CFileMonitorBase(int const delay, QObject* const pParent);
	virtual ~CFileMonitorBase() override;

	// IFileChangeListener
	virtual void OnFileChange(char const* szFileName, EChangeType type) override;
	// ~IFileChangeListener

	int const m_delay;
};
} // namespace ACE
