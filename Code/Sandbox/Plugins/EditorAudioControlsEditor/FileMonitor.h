// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/File/IFileChangeMonitor.h>
#include <QTimer>

namespace ACE
{
class CSystemAssetsManager;

class CFileMonitor : public QTimer, public IFileChangeListener
{
	Q_OBJECT

public:

	void Disable();

signals:

	void SignalReloadData();

protected:

	CFileMonitor(int const delay, CSystemAssetsManager const& assetsManager, QObject* const pParent);
	virtual ~CFileMonitor() override;

	// IFileChangeListener
	virtual void OnFileChange(char const* filename, EChangeType eType) override;
	// ~IFileChangeListener

	int const                   m_delay;
	CSystemAssetsManager const& m_assetsManager;
};

class CFileMonitorSystem final : public CFileMonitor
{
	Q_OBJECT

public:

	CFileMonitorSystem(int const delay, CSystemAssetsManager const& assetsManager, QObject* const pParent);

	void Enable();
	void EnableDelayed();

private:

	string const  m_monitorFolder;
	QTimer* const m_delayTimer;
};

class CFileMonitorMiddleware final : public CFileMonitor
{
	Q_OBJECT

public:

	CFileMonitorMiddleware(int const delay, CSystemAssetsManager const& assetsManager, QObject* const pParent);

	void Enable();

private:

	std::vector<char const*> m_monitorFolders;
};
} // namespace ACE

