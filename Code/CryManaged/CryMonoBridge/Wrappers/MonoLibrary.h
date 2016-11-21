// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMono/IMonoAssembly.h>
#include <CryMono/IMonoRuntime.h>

#include <mono/jit/jit.h>

class CMonoDomain;
class CMonoClass;

class CMonoLibrary : public IMonoAssembly
{
public:
	CMonoLibrary(MonoAssembly* pAssembly, const char* filePath, CMonoDomain* pDomain, char* data = nullptr);
	virtual ~CMonoLibrary();

	// IMonoAssembly
	virtual bool                  IsLoaded() const override { return m_pAssembly != nullptr; }

	virtual const char* GetFilePath() override;

	virtual IMonoDomain* GetDomain() const override;

	virtual IMonoClass* GetClass(const char *nameSpace, const char *className) override;
	virtual std::shared_ptr<IMonoClass> GetTemporaryClass(const char *nameSpace, const char *className) override;

	virtual std::shared_ptr<IMonoException> GetExceptionInternal(const char* nameSpace, const char* exceptionClass, const char* message = "") override;
	// ~IMonoAssembly

	void Unload();
	void Reload();

	void Serialize();
	void Deserialize();

	const char*           GetImageName() const;
	bool                  IsInMemory() const { return m_pMemory != nullptr; }
	
	std::shared_ptr<CMonoClass> GetClassFromMonoClass(MonoClass* pClass);

	MonoObject* GetManagedObject();

	void          SetData(char* data)           { m_pMemory = data; }
	void          SetDebugData(mono_byte* data) { m_pMemoryDebug = data; }

	MonoAssembly* GetAssembly() const { return m_pAssembly; }
	MonoImage* GetImage() const;

private:
	MonoAssembly* m_pAssembly;
	string        m_assemblyPath;

	CMonoDomain*   m_pDomain;

	char*         m_pMemory;
	mono_byte*    m_pMemoryDebug;

	std::list<std::shared_ptr<CMonoClass>> m_classes;
};