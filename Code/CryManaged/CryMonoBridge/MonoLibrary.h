// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMono/IMonoRuntime.h>
#include <mono/jit/jit.h>

class CMonoLibrary : public IMonoLibrary
{
public:
	CMonoLibrary(MonoAssembly* pAssembly, char* data = NULL);
	~CMonoLibrary();

	// IMonoLibrary
	virtual const char* GetName() { return m_sName; }
	virtual const char* GetPath() { return m_sPath; }
	bool IsInMemory() { return m_pMemory != NULL; }
	virtual bool IsLoaded() { return m_pAssembly != NULL; }
	virtual bool RunMethod(const char* name);
	// ~IMonoLibrary

	void SetName(const char* value) { m_sName = value; }
	void SetPath(const char* value) { m_sPath = value; }
	void SetData(char* data) { m_pMemory = data; }
	void SetDebugData(mono_byte* data) { m_pMemoryDebug = data; }
	MonoAssembly* GetAssembly() { return m_pAssembly; }
	void Close();
	void CloseDebug();

private:
	MonoAssembly*		m_pAssembly;
	char*				m_pMemory;
	mono_byte*			m_pMemoryDebug;
	const char*			m_sName;
	const char*			m_sPath;
};

