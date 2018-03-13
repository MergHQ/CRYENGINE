// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/File/IFileChangeMonitor.h>
#include <QTimer>

namespace ACE
{
class CFileMonitor : public QTimer, public IFileChangeListener
{
	Q_OBJECT

public:

	void Disable();

signals:

	void SignalReloadData();

protected:

	explicit CFileMonitor(int const delay, QObject* const pParent);
	virtual ~CFileMonitor() override;

	CFileMonitor() = delete;

	// IFileChangeListener
	virtual void OnFileChange(char const* filename, EChangeType eType) override;
	// ~IFileChangeListener

	int const m_delay;
};

class CFileMonitorSystem final : public CFileMonitor
{
	Q_OBJECT

public:

	explicit CFileMonitorSystem(int const delay, QObject* const pParent);

	CFileMonitorSystem() = delete;

	void Enable();
	void EnableDelayed();

private:

	string        m_monitorFolder;
	QTimer* const m_delayTimer;
};

class CFileMonitorMiddleware final : public CFileMonitor
{
	Q_OBJECT

public:

	explicit CFileMonitorMiddleware(int const delay, QObject* const pParent);

	CFileMonitorMiddleware() = delete;

	void Enable();

private:

	std::vector<char const*> m_monitorFolders;
};
} // namespace ACE
