// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMono/IMonoAssembly.h>
#include <CryMono/IMonoRuntime.h>

#include <mono/jit/jit.h>

class CMonoDomain;
class CMonoClass;
class CMonoObject;

class CMonoLibrary final : public IMonoAssembly
{
public:
	CMonoLibrary(const char* filePath, CMonoDomain* pDomain);
	CMonoLibrary(MonoAssembly* pAssembly, const char* filePath, CMonoDomain* pDomain);
	virtual ~CMonoLibrary();

	// IMonoAssembly
	virtual bool                  IsLoaded() const override { return m_pAssembly != nullptr; }

	virtual const char* GetFilePath() override;

	virtual IMonoDomain* GetDomain() const override;

	virtual IMonoClass* GetClass(const char *nameSpace, const char *className) override;
	virtual std::shared_ptr<IMonoClass> GetTemporaryClass(const char *nameSpace, const char *className) override;

	virtual std::shared_ptr<IMonoException> GetExceptionInternal(const char* nameSpace, const char* exceptionClass, const char* message = "") override;
	// ~IMonoAssembly

	bool Load();
	void Unload();
	void Reload();

	void Serialize(CMonoObject* pSerializer);
	void Deserialize(CMonoObject* pSerializer);

	const char*           GetPath() const { return m_assemblyPath; }
	const char*           GetImageName() const;
	
	std::shared_ptr<CMonoClass> GetClassFromMonoClass(MonoClass* pClass);

	MonoObject* GetManagedObject();

	MonoAssembly* GetAssembly() const { return m_pAssembly; }
	MonoImage* GetImage() const { return m_pImage; }

private:
	MonoAssembly* m_pAssembly;
	MonoImage* m_pImage;

	string        m_assemblyPath;

	CMonoDomain*   m_pDomain;

	std::list<std::shared_ptr<CMonoClass>> m_classes;

	std::vector<char> m_assemblyData;
	std::vector<mono_byte> m_assemblyDebugData;
};