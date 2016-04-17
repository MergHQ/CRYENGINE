// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   MatMan.cpp
//  Version:     v1.00
//  Created:     28/5/2001 by Vladimir Kajalin
//  Compilers:   Visual Studio.NET
//  Description: Material Manager Implementation
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "MatMan.h"
#include "3dEngine.h"
#include "ObjMan.h"
#include <CryRenderer/IRenderer.h>
#include "SurfaceTypeManager.h"
#include <Cry3DEngine/CGF/CGFContent.h>
#include <CrySystem/File/IResourceManager.h>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
IMaterial* CMatMan::GetDefaultMaterial()
{
	return m_pDefaultMtl;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
IMaterial* CMatInfo::GetSafeSubMtl(int nSubMtlSlot)
{
	if (m_subMtls.empty() || !(m_Flags & MTL_FLAG_MULTI_SUBMTL))
	{
		return this; // Not Multi material.
	}
	if (nSubMtlSlot >= 0 && nSubMtlSlot < (int)m_subMtls.size() && m_subMtls[nSubMtlSlot] != NULL)
		return m_subMtls[nSubMtlSlot];
	else
		return GetMatMan()->GetDefaultMaterial();
}

///////////////////////////////////////////////////////////////////////////////
SShaderItem& CMatInfo::GetShaderItem()
{
#if defined(ENABLE_CONSOLE_MTL_VIZ)
	if (ms_useConsoleMat == 1 && m_pConsoleMtl)
	{
		IMaterial* pConsoleMaterial = m_pConsoleMtl.get();
		return static_cast<CMatInfo*>(pConsoleMaterial)->CMatInfo::GetShaderItem();
	}
#endif

	return m_shaderItem;
}

///////////////////////////////////////////////////////////////////////////////
const SShaderItem& CMatInfo::GetShaderItem() const
{
#if defined(ENABLE_CONSOLE_MTL_VIZ)
	if (ms_useConsoleMat == 1 && m_pConsoleMtl)
	{
		IMaterial* pConsoleMaterial = m_pConsoleMtl.get();
		return static_cast<CMatInfo*>(pConsoleMaterial)->CMatInfo::GetShaderItem();
	}
#endif

	return m_shaderItem;
}

//////////////////////////////////////////////////////////////////////////
SShaderItem& CMatInfo::GetShaderItem(int nSubMtlSlot)
{
#if defined(ENABLE_CONSOLE_MTL_VIZ)
	if (ms_useConsoleMat == 1 && m_pConsoleMtl)
	{
		IMaterial* pConsoleMaterial = m_pConsoleMtl.get();
		return static_cast<CMatInfo*>(pConsoleMaterial)->CMatInfo::GetShaderItem(nSubMtlSlot);
	}
#endif

	SShaderItem* pShaderItem = NULL;
	if (m_subMtls.empty() || !(m_Flags & MTL_FLAG_MULTI_SUBMTL))
	{
		pShaderItem = &m_shaderItem; // Not Multi material.
	}
	else if (nSubMtlSlot >= 0 && nSubMtlSlot < (int)m_subMtls.size() && m_subMtls[nSubMtlSlot] != NULL)
	{
		pShaderItem = &(m_subMtls[nSubMtlSlot]->m_shaderItem);
	}
	else
	{
		IMaterial* pDefaultMaterial = GetMatMan()->GetDefaultMaterial();
		pShaderItem = &(static_cast<CMatInfo*>(pDefaultMaterial)->m_shaderItem);
	}

	return *pShaderItem;
}

///////////////////////////////////////////////////////////////////////////////
const SShaderItem& CMatInfo::GetShaderItem(int nSubMtlSlot) const
{
#if defined(ENABLE_CONSOLE_MTL_VIZ)
	if (ms_useConsoleMat == 1 && m_pConsoleMtl)
	{
		IMaterial* pConsoleMaterial = m_pConsoleMtl.get();
		return static_cast<CMatInfo*>(pConsoleMaterial)->CMatInfo::GetShaderItem(nSubMtlSlot);
	}
#endif

	const SShaderItem* pShaderItem = NULL;
	if (m_subMtls.empty() || !(m_Flags & MTL_FLAG_MULTI_SUBMTL))
	{
		pShaderItem = &m_shaderItem; // Not Multi material.
	}
	else if (nSubMtlSlot >= 0 && nSubMtlSlot < (int)m_subMtls.size() && m_subMtls[nSubMtlSlot] != NULL)
	{
		pShaderItem = &(m_subMtls[nSubMtlSlot]->m_shaderItem);
	}
	else
	{
		IMaterial* pDefaultMaterial = GetMatMan()->GetDefaultMaterial();
		pShaderItem = &(static_cast<CMatInfo*>(pDefaultMaterial)->m_shaderItem);
	}

	return *pShaderItem;
}

///////////////////////////////////////////////////////////////////////////////
bool CMatInfo::IsForwardRenderingRequired()
{
	bool bRequireForwardRendering = (m_Flags & MTL_FLAG_REQUIRE_FORWARD_RENDERING) != 0;

	if (!bRequireForwardRendering)
	{
		for (int i = 0; i < (int)m_subMtls.size(); ++i)
		{
			if (m_subMtls[i] != 0 && m_subMtls[i]->m_Flags & MTL_FLAG_REQUIRE_FORWARD_RENDERING)
			{
				bRequireForwardRendering = true;
				break;
			}
		}
	}

	return bRequireForwardRendering;
}

///////////////////////////////////////////////////////////////////////////////
bool CMatInfo::IsNearestCubemapRequired()
{
	bool bRequireNearestCubemap = (m_Flags & MTL_FLAG_REQUIRE_NEAREST_CUBEMAP) != 0;

	if (!bRequireNearestCubemap)
	{
		for (int i = 0; i < (int)m_subMtls.size(); ++i)
		{
			if (m_subMtls[i] != 0 && m_subMtls[i]->m_Flags & MTL_FLAG_REQUIRE_NEAREST_CUBEMAP)
			{
				bRequireNearestCubemap = true;
				break;
			}
		}
	}

	return bRequireNearestCubemap;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
