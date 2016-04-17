// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   SubstitutionProxy.h
//  Version:     v1.00
//  Created:     7/6/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __SubstitutionProxy_h__
#define __SubstitutionProxy_h__
#pragma once

//////////////////////////////////////////////////////////////////////////
// Description:
//    Implements base substitution proxy class for entity.
//////////////////////////////////////////////////////////////////////////
struct CSubstitutionProxy : IEntitySubstitutionProxy
{
public:
	CSubstitutionProxy() { m_pSubstitute = 0; }
	~CSubstitutionProxy() { if (m_pSubstitute) m_pSubstitute->ReleaseNode(); };

	//////////////////////////////////////////////////////////////////////////
	// IEntityProxy interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void ProcessEvent(SEntityEvent& event) {}
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityProxy interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual EEntityProxy GetType()                                           { return ENTITY_PROXY_SUBSTITUTION; }
	virtual void         Release()                                           { delete this; }
	virtual void         Done();
	virtual void         Update(SEntityUpdateContext& ctx)                   {}
	virtual bool         Init(IEntity* pEntity, SEntitySpawnParams& params)  { return true; }
	virtual void         Reload(IEntity* pEntity, SEntitySpawnParams& params);
	virtual void         SerializeXML(XmlNodeRef& entityNode, bool bLoading) {}
	virtual void         Serialize(TSerialize ser);
	virtual bool         NeedSerialize();
	virtual bool         GetSignature(TSerialize signature);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntitySubstitutionProxy interface.
	//////////////////////////////////////////////////////////////////////////
	virtual void         SetSubstitute(IRenderNode* pSubstitute);
	virtual IRenderNode* GetSubstitute() { return m_pSubstitute; }
	//////////////////////////////////////////////////////////////////////////

	virtual void GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
protected:
	IRenderNode* m_pSubstitute;
};

#endif // __SubstitutionProxy_h__
