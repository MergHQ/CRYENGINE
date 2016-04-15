// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   Description: Signature generated to describe the blueprint of proxies
        and their internal status that comprise an Entity generated
        from the given Class

   -------------------------------------------------------------------------
   History:
   - 15:03:2010: Created by Kevin Kirst

*************************************************************************/

#include "stdafx.h"
#include "EntityPoolSignature.h"
#include "Entity.h"

//////////////////////////////////////////////////////////////////////////
CEntityPoolSignature::CEntityPoolSignature()
	: m_bGenerated(false)
	, m_pSignatureSerializer(NULL)
{
	ISystem* pSystem = GetISystem();
	assert(pSystem);

	m_Signature = pSystem->CreateXmlNode("EntityPoolSignature");
	m_pSignatureSerializer = pSystem->GetXmlUtils()->CreateXmlSerializer();
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPoolSignature::CalculateFromEntity(CEntity* pEntity)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

	assert(pEntity);

	if (!m_bGenerated && pEntity)
	{
		m_bGenerated = true;

		ISerialize* pWriter = m_pSignatureSerializer->GetWriter(m_Signature);
		TSerialize signatureWriter(pWriter);

		m_bGenerated &= pEntity->GetSignature(signatureWriter);

		if (!m_bGenerated)
		{
			m_Signature->removeAllChilds();
			m_Signature->removeAllAttributes();
		}
	}

	return m_bGenerated;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPoolSignature::CompareSignature(const CEntityPoolSignature& otherSignature) const
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

	bool bResult = false;

	if (m_bGenerated)
	{
		// Perform quick test first
		//if (m_uTestMask == otherSignature.m_uTestMask)
		{
			// Perform in-depth test using serialization objects
			bResult = CompareNodes(m_Signature, otherSignature.m_Signature);
		}
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPoolSignature::CompareNodes(const XmlNodeRef& a, const XmlNodeRef& b, bool bRecursive)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

	assert(a);
	assert(b);

	bool bResult = (a && b && a->isTag(b->getTag()));

	// Check value
	bResult &= (bResult && 0 == strcmp(a->getContent(), b->getContent()));

	// Check attributes
	bResult &= (bResult && CompareNodeAttributes(a, b));

	// Check children if recursive
	if (bResult && bRecursive)
	{
		const int childCount_a = a->getChildCount();
		const int childCount_b = b->getChildCount();
		bResult &= (childCount_a == childCount_b);

		if (bResult)
		{
			for (int child = 0; bResult && child < childCount_a; ++child)
			{
				XmlNodeRef child_a = a->getChild(child);
				XmlNodeRef child_b = b->getChild(child);
				if (child_a && child_b)
				{
					bResult &= CompareNodes(child_a, child_b, true);
				}
			}
		}
	}

	return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CEntityPoolSignature::CompareNodeAttributes(const XmlNodeRef& a, const XmlNodeRef& b)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

	assert(a);
	assert(b);

	const int attrCount_a = a->getNumAttributes();
	const int attrCount_b = b->getNumAttributes();
	bool bResult = (attrCount_a == attrCount_b);

	if (bResult)
	{
		const char* attrKey_a, * attrValue_a;
		const char* attrKey_b, * attrValue_b;

		for (int attr = 0; bResult && attr < attrCount_a; ++attr)
		{
			const bool bHasForA = a->getAttributeByIndex(attr, &attrKey_a, &attrValue_a);
			const bool bHasForB = b->getAttributeByIndex(attr, &attrKey_b, &attrValue_b);
			if ((bHasForA && !bHasForB) || (!bHasForA && bHasForB))
				bResult = false;
			else if (bHasForA && (0 != strcmp(attrKey_a, attrKey_b) || 0 != strcmp(attrValue_a, attrValue_b)))
				bResult = false;
		}
	}

	return bResult;
}
