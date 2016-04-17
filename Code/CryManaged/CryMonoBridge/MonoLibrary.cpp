// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "MonoLibrary.h"
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>
#include <mono/metadata/mono-debug.h>

CMonoLibrary::CMonoLibrary(MonoAssembly* pAssembly, char* data) :
	m_pAssembly(pAssembly),
	m_pMemory(data),
	m_pMemoryDebug(NULL)
{
	MonoImage* pImage = mono_assembly_get_image(pAssembly);
	m_sName = mono_image_get_name(pImage);
	m_sPath = mono_image_get_filename(pImage);
}

CMonoLibrary::~CMonoLibrary()
{
}

bool CMonoLibrary::RunMethod(const char* name)
{
	if(!m_pAssembly)
	{
		gEnv->pLog->LogError("[Mono][RunMethod] Assembly '%s' not loaded!", m_sName);
		return false;
	}

	MonoImage* pImg = mono_assembly_get_image(m_pAssembly);
	if(!pImg)
	{
		gEnv->pLog->LogError("[Mono][RunMethod] Invalid Assembly-Image '%s'!", m_sName);
		return false;
	}

	MonoMethodDesc* pMethodDesc = mono_method_desc_new(name, true);
	MonoMethod* pMethod = mono_method_desc_search_in_image(pMethodDesc, pImg);
	if(!pMethod)
	{
		gEnv->pLog->LogError("[Mono][RunMethod] Could not find method '%s' in assembly'%s'!", name, m_sName);
		return false;
	}

	mono_runtime_invoke(pMethod, NULL, NULL, NULL);
	return true;
}

void CMonoLibrary::Close()
{
	if(m_pMemory)
	{
		delete[] m_pMemory;
		m_pMemory = NULL;
	}
}

void CMonoLibrary::CloseDebug()
{
	if(m_pMemoryDebug && m_pAssembly)
	{
		mono_debug_close_image(mono_assembly_get_image(m_pAssembly));
		delete[] m_pMemoryDebug;
		m_pMemoryDebug = NULL;
	}
}