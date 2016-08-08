// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

enum EMonoLogLevel
{
	eMLL_NULL = 0,
	eMLL_Error,
	eMLL_Critical,
	eMLL_Warning,
	eMLL_Message,
	eMLL_Info,
	eMLL_Debug
};

struct ICryEngineBasePlugin;
struct SMonoDomainHandlerLibrary;

struct IMonoLibrary
{
	virtual ~IMonoLibrary() {}

	virtual ICryEngineBasePlugin* Initialize(void* pMonoDomain, void* pDomainHandler = nullptr) = 0;
	virtual bool                  RunMethod(const char* szMethodName) const = 0;
	virtual const char*           GetImageName() const = 0;
	virtual bool                  IsInMemory() const = 0;
	virtual bool                  IsLoaded() const = 0;
};

struct IMonoListener
{
	virtual ~IMonoListener() {}

	//! Forwarded from CrySystem
	//! \param flags One or more flags from ESystemUpdateFlags structure.
	//! \param nPauseMode 0=normal(no pause), 1=menu/pause, 2=cutscene.
	virtual void OnUpdate(int updateFlags, int nPauseMode) = 0;
};

struct IMonoRuntime
{
	virtual ~IMonoRuntime() {}
	virtual void                  Initialize(EMonoLogLevel logLevel) = 0;
	virtual bool                  LoadBinary(const char* szBinaryPath) = 0;
	virtual bool                  UnloadBinary(const char* szBinaryPath) = 0;

	virtual void                  Update(int updateFlags = 0, int nPauseMode = 0) = 0;
	virtual ICryEngineBasePlugin* GetPlugin(const char* szBinaryPath) const = 0;
	virtual bool                  RunMethod(const ICryEngineBasePlugin* pPlugin, const char* szMethodName) const = 0;

	virtual void                  RegisterListener(IMonoListener* pListener) = 0;
	virtual void                  UnregisterListener(IMonoListener* pListener) = 0;
	virtual EMonoLogLevel         GetLogLevel() = 0;
};

// TODO: #CryPlugins: make this an abstract container
struct ICryEngineBasePlugin
{
	bool        Call_Initialize() const                           { return gEnv->pMonoRuntime->RunMethod(this, "Initialize"); }
	bool        Call_OnGameStart() const                          { return gEnv->pMonoRuntime->RunMethod(this, "OnGameStart"); }
	bool        Call_OnGameStop() const                           { return gEnv->pMonoRuntime->RunMethod(this, "OnGameStop"); }
	bool        Call_Shutdown() const                             { return gEnv->pMonoRuntime->RunMethod(this, "Shutdown"); }

	void        SetBinaryDirectory(const char* szBinaryDirectory) { cry_strcpy(m_szBinaryDirectory, szBinaryDirectory); }
	void        SetAssetDirectory(const char* szAssetDirectory)   { cry_strcpy(m_szAssetDirectory, szAssetDirectory); }

	const char* GetBinaryDirectory() const                        { return m_szBinaryDirectory; }
	const char* GetAssetDirectory() const                         { return m_szAssetDirectory; }

private:
	char m_szAssetDirectory[_MAX_PATH];
	char m_szBinaryDirectory[_MAX_PATH];
};

struct IMonoEntityPropertyHandler : IEntityPropertyHandler
{
	virtual ~IMonoEntityPropertyHandler() {}

	virtual IEntityPropertyHandler::SPropertyInfo* GetMonoPropertyInfo(int index) const = 0;
	virtual bool                                   GetPropertyInfo(int index, IEntityPropertyHandler::SPropertyInfo& info) const
	{
		const IEntityPropertyHandler::SPropertyInfo* pProperty = GetMonoPropertyInfo(index);
		if (!pProperty)
		{
			return false;
		}

		info = *pProperty;
		return true;
	}
};
