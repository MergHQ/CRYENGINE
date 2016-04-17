// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   Description: Signature generated to describe the blueprint of proxies
        and their internal status that comprise an Entity generated
        from the given class

   -------------------------------------------------------------------------
   History:
   - 15:03:2010: Created by Kevin Kirst

*************************************************************************/

#ifndef __ENTITYPOOLSIGNATURE_H__
#define __ENTITYPOOLSIGNATURE_H__

#include <CryEntitySystem/IEntityProxy.h>
#include <CryNetwork/ISerialize.h>

class CEntityPoolSignature
{
public:
	CEntityPoolSignature();

	operator bool() const { return m_bGenerated; }
	bool operator==(const CEntityPoolSignature& otherSignature) const { return CompareSignature(otherSignature); }
	bool operator!=(const CEntityPoolSignature& otherSignature) const { return !(*this == otherSignature); }

	//! Returns if this signature has been generated yet
	bool IsGenerated() const { return m_bGenerated; }

	//! Calculate the signature using an instantiated Entity container as a reference
	bool CalculateFromEntity(CEntity* pEntity);

	//! Compare two signatures to see if they match up
	bool CompareSignature(const CEntityPoolSignature& otherSignature) const;

private:
	//! Helpers for signature comparing
	static bool CompareNodes(const XmlNodeRef& a, const XmlNodeRef& b, bool bRecursive = true);
	static bool CompareNodeAttributes(const XmlNodeRef& a, const XmlNodeRef& b);

private:
	enum { SIGNATURE_MAX_SIZE = ENTITY_PROXY_LAST };
	bool m_bGenerated;

	// Serialized signature for in-depth checking
	_smart_ptr<IXmlSerializer> m_pSignatureSerializer;
	XmlNodeRef                 m_Signature;
};

#endif //__ENTITYPOOLSIGNATURE_H__
