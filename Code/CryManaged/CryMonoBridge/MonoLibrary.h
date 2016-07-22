// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMono/IMonoRuntime.h>
#include <mono/jit/jit.h>

class CMonoLibrary : public IMonoLibrary
{
public:
	CMonoLibrary(MonoAssembly* pAssembly, char* data = nullptr);
	virtual ~CMonoLibrary();

	// IMonoLibrary
	virtual ICryEnginePlugin* Initialize(void* pMonoDomain);
	virtual bool              RunMethod(const char* szMethodName) const;
	virtual const char*       GetImageName() const;
	virtual bool              IsInMemory() const { return m_pMemory != nullptr; }
	virtual bool              IsLoaded() const   { return m_pAssembly != nullptr; }
	// ~IMonoLibrary

	void          Close();
	void          CloseDebug();

	void          SetData(char* data)           { m_pMemory = data; }
	void          SetDebugData(mono_byte* data) { m_pMemoryDebug = data; }

	const char*   TryGetPlugin(MonoDomain* pDomain) const;
	MonoAssembly* GetAssembly() const { return m_pAssembly; }

private:
	MonoAssembly* m_pAssembly;
	char*         m_pMemory;
	mono_byte*    m_pMemoryDebug;
	MonoClass*    m_pMonoClass;
	MonoObject*   m_pMonoObject;
	uint32        m_gcHandle;
};
