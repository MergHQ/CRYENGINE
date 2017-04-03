#pragma once

#include "MonoLibrary.h"

#include <CryMono/IMonoDomain.h>

#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/debug-helpers.h>

// Wrapped behavior for mono appdomain functionality
class CMonoDomain : public IMonoDomain
{
public:
	CMonoDomain();
	virtual ~CMonoDomain();

	// IMonoDomain
	virtual bool IsRoot() override { return false; }

	virtual bool Activate(bool bForce = false) override;
	virtual bool IsActive() const override;

	virtual void* GetHandle() const override { return m_pDomain; }
	virtual void* CreateManagedString(const char* str) override { return CreateString(str); }
	// ~IMonoDomain

	MonoString* CreateString(const char* szText);

	CMonoLibrary* LoadLibrary(const char* szPath);
	CMonoLibrary* GetLibraryFromMonoAssembly(MonoAssembly* pAssembly);

protected:
	void Unload();

protected:
	MonoDomain *m_pDomain;
	// Whether or not this domain was created on the native side
	bool m_bNativeDomain;

	std::vector<std::unique_ptr<CMonoLibrary>> m_loadedLibraries;
};