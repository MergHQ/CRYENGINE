#pragma once

#include <CrySystem/IProjectManager.h>

class CProjectManager : public IProjectManager
{
public:
	CProjectManager();
	virtual ~CProjectManager() {}

	// IProjectManager
	virtual const char* GetCurrentProjectName() override;

	virtual const char* GetCurrentProjectDirectoryAbsolute() override;

	virtual const char* GetCurrentAssetDirectoryRelative() override;
	virtual const char* GetCurrentAssetDirectoryAbsolute() override;

	virtual const char* GetCurrentBinaryDirectoryAbsolute() override;
	// ~IProjectManager

protected:
	void LoadConfiguration();

protected:
	bool m_bEnabled;

	string m_currentProjectDirectory;
	string m_currentAssetDirectory;
	string m_currentBinaryDirectory;
};