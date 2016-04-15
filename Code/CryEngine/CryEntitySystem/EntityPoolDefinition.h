// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
   -------------------------------------------------------------------------
   $Id$
   $DateTime$
   Description: Pool definition to associate an id with settings

   -------------------------------------------------------------------------
   History:
   - 07:04:2010: Created by Kevin Kirst

*************************************************************************/

#ifndef __ENTITYPOOLDEFINITION_H__
#define __ENTITYPOOLDEFINITION_H__

#include "EntityPoolCommon.h"

class CEntityPoolDefinition
{
public:
	CEntityPoolDefinition(TEntityPoolDefinitionId id = INVALID_ENTITY_POOL_DEFINITION);
	void                    GetMemoryStatistics(ICrySizer* pSizer) const;

	void                    OnLevelLoadStart();

	TEntityPoolDefinitionId GetId() const               { return m_Id; }
	bool                    HasAI() const               { return m_bHasAI; }
	bool                    IsDefaultBookmarked() const { return m_bDefaultBookmarked; }
	bool                    IsForcedBookmarked() const  { return m_bForcedBookmarked; }
	uint32                  GetMaxSize() const          { return m_uMaxSize; }
	uint32                  GetDesiredPoolCount() const { return m_uCount; }
	const string& GetName() const             { return m_sName; }
	const string& GetDefaultClass() const     { return m_sDefaultClass; }

	bool          ContainsEntityClass(IEntityClass* pClass) const;

	bool          LoadFromXml(CEntitySystem* pEntitySystem, XmlNodeRef pDefinitionNode);
	bool          ParseLevelXml(XmlNodeRef pLevelNode);

#ifdef ENTITY_POOL_SIGNATURE_TESTING
	bool TestEntityClassSignature(CEntitySystem* pEntitySystem, IEntityClass* pClass, const class CEntityPoolSignature& poolSignature) const;
#endif //ENTITY_POOL_SIGNATURE_TESTING

#ifdef ENTITY_POOL_DEBUGGING
	void DebugDraw(IRenderer* pRenderer, float& fColumnX, float& fColumnY) const;
#endif //ENTITY_POOL_DEBUGGING

private:
	TEntityPoolDefinitionId m_Id;

	bool                    m_bHasAI;
	bool                    m_bDefaultBookmarked;
	bool                    m_bForcedBookmarked;
	uint32                  m_uMaxSize;
	uint32                  m_uCount;
	string                  m_sName;
	string                  m_sDefaultClass;

	typedef std::vector<IEntityClass*> TContainClasses;
	TContainClasses m_ContainClasses;

#ifdef ENTITY_POOL_SIGNATURE_TESTING
	typedef std::map<IEntityClass*, bool> TSignatureTests;
	mutable TSignatureTests m_SignatureTests;
#endif //ENTITY_POOL_SIGNATURE_TESTING
};

#endif //__ENTITYPOOLDEFINITION_H__
