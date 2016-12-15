#pragma once

// Interface for CrySystem and Editor to initialize game projects
struct IProjectManager
{
	virtual ~IProjectManager() {}

	virtual const char* GetCurrentProjectName() = 0;

	//! \return Path without trailing separator.
	virtual const char* GetCurrentProjectDirectoryAbsolute() = 0;

	virtual const char* GetCurrentAssetDirectoryRelative() = 0;
	virtual const char* GetCurrentAssetDirectoryAbsolute() = 0;

	virtual const char* GetCurrentBinaryDirectoryAbsolute() = 0;
};