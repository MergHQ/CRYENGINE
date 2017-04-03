#include "StdAfx.h"
#include "RootMonoDomain.h"

#include <mono/jit/jit.h>

CRootMonoDomain::CRootMonoDomain()
{
	m_pDomain = nullptr;
}

void CRootMonoDomain::Initialize()
{
	m_pDomain = mono_jit_init_version("CryEngine", "v4.0.30319");
	m_bNativeDomain = true;
}

CMonoLibrary* CRootMonoDomain::GetNetCoreLibrary()
{
	auto* pMonoCorlib = mono_get_corlib();
	
	return GetLibraryFromMonoAssembly(mono_image_get_assembly(pMonoCorlib));
}