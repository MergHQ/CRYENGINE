// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/File/IFileChangeMonitor.h>
#include <QTimer>

namespace ACE
{
class CAudioControlsEditorWindow;

class CFileMonitor : public QTimer, public IFileChangeListener
{
	Q_OBJECT

protected:

	CFileMonitor(CAudioControlsEditorWindow* window, int delay);

	virtual ~CFileMonitor() override;

public:

	void Disable();

protected:

	// IFileChangeListener
	virtual void OnFileChange(const char* filename, EChangeType eType) override;
	// ~IFileChangeListener

	virtual void ReloadData() {}

	CAudioControlsEditorWindow* m_window;
	int                         m_delay;
};

class CFileMonitorSystem final : public CFileMonitor
{
	Q_OBJECT

public:

	CFileMonitorSystem(CAudioControlsEditorWindow* window, int delay);

	void Enable();
	void EnableDelayed();

private:

	// CFileMonitor
	virtual void ReloadData() override;
	// ~CFileMonitor

	string const m_assetFolder;
	QTimer*      m_delayTimer;
};

class CFileMonitorMiddleware final : public CFileMonitor
{
	Q_OBJECT

public:

	CFileMonitorMiddleware(CAudioControlsEditorWindow* window, int delay);

	void Enable();

private:

	// CFileMonitor
	virtual void ReloadData() override;
	// ~CFileMonitor

	std::vector<const char*> m_monitorFolders;
};
} // namespace ACE

