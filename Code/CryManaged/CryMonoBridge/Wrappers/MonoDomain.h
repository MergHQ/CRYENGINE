#pragma once

#include "MonoLibrary.h"

#include <CryMono/IMonoDomain.h>

#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/object.h>

// Wrapped behavior for mono appdomain functionality
class CMonoDomain : public IMonoDomain
{
	struct SLoadedLibrary
	{
		SLoadedLibrary(CMonoLibrary* pLib, const char* path)
			: pLibrary(pLib)
			, filePath(path) {}

		SLoadedLibrary(const char* path)
			: filePath(path) {}

		string filePath;
		std::unique_ptr<CMonoLibrary> pLibrary;
	};

public:
	CMonoDomain();
	~CMonoDomain();

	// IMonoDomain
	virtual bool IsRoot() override { return false; }

	virtual bool Activate(bool bForce = false) override;
	virtual bool IsActive() const override;
	// ~IMonoDomain

	virtual void Release() = 0;

	// TODO: Wrap string
	MonoString* CreateString(const char *text);

	MonoDomain *GetHandle() const { return m_pDomain; }

	MonoAssembly* LoadMonoAssembly(const char* path, FILE* pFile, char** pImageDataOut, mono_byte** pDebugDataOut, bool bRefOnly = false);

	CMonoLibrary* LoadLibrary(const char* path, bool bRefOnly = false);
	CMonoLibrary* GetLibraryFromMonoAssembly(MonoAssembly* pAssembly);
	
protected:
	void Unload();

protected:
	MonoDomain *m_pDomain;
	bool m_bNativeAssembly;

	std::vector<SLoadedLibrary> m_loadedLibraries;

	// Folders in which we have loaded assemblies
	// This is required in order to resolve dependencies reliably
	std::vector<string> m_binaryDirectories;
};