// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   ScriptProxy.h
//  Version:     v1.00
//  Created:     18/5/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __ScriptProxy_h__
#define __ScriptProxy_h__
#pragma once

// forward declarations.
class CEntityComponentLuaScript;
class CEntity;
struct IScriptTable;
struct SScriptState;

#include "EntityScript.h"

//////////////////////////////////////////////////////////////////////////
// Description:
//    CScriptProxy object handles all the interaction of the entity with
//    the entity script.
//////////////////////////////////////////////////////////////////////////
class CEntityComponentLuaScript : public IEntityScriptComponent
{
	CRY_ENTITY_COMPONENT_CLASS(CEntityComponentLuaScript,IEntityScriptComponent,"CEntityComponentLuaScript",0x38CF87CCD44B4A1D,0xA16D7EA3C5BDE757);

public:
	void InitScript(CEntityComponentLuaScript* pScript = NULL, SEntitySpawnParams* pSpawnParams = NULL);

	virtual void Initialize(const SComponentInitializer& init) final;

	//////////////////////////////////////////////////////////////////////////
	// IEntityComponent interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void ProcessEvent(SEntityEvent& event) final;
	virtual uint64 GetEventMask() const final;; // Need all events except pre-physics update
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityComponent implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual EEntityProxy GetProxyType() const  final { return ENTITY_PROXY_SCRIPT; }
	virtual void         Release()  final { delete this; };
	virtual void         Update(SEntityUpdateContext& ctx) final;
	virtual void         SerializeXML(XmlNodeRef& entityNode, bool bLoading) final;
	virtual void         GameSerialize(TSerialize ser) final;
	virtual bool         NeedGameSerialize() final;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityScriptComponent implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void          SetScriptUpdateRate(float fUpdateEveryNSeconds) final { m_fScriptUpdateRate = fUpdateEveryNSeconds; };
	virtual IScriptTable* GetScriptTable() final { return m_pThis; };

	virtual void          CallEvent(const char* sEvent) final;
	virtual void          CallEvent(const char* sEvent, float fValue) final;
	virtual void          CallEvent(const char* sEvent, bool bValue) final;
	virtual void          CallEvent(const char* sEvent, const char* sValue) final;
	virtual void          CallEvent(const char* sEvent, EntityId nEntityId) final;
	virtual void          CallEvent(const char* sEvent, const Vec3& vValue) final;

	virtual void                  CallInitEvent(bool bFromReload) final;

	virtual void          SendScriptEvent(int Event, IScriptTable* pParamters, bool* pRet = NULL) final;
	virtual void          SendScriptEvent(int Event, const char* str, bool* pRet = NULL) final;
	virtual void          SendScriptEvent(int Event, int nParam, bool* pRet = NULL) final;

	virtual void          ChangeScript(IEntityScript* pScript, SEntitySpawnParams* params) final;
	//////////////////////////////////////////////////////////////////////////

	virtual void OnCollision(CEntity* pTarget, int matId, const Vec3& pt, const Vec3& n, const Vec3& vel, const Vec3& targetVel, int partId, float mass) final;
	virtual void OnPreparedFromPool() final;

	//////////////////////////////////////////////////////////////////////////
	// State Management public interface.
	//////////////////////////////////////////////////////////////////////////
	virtual bool GotoState(const char* sStateName) final;
	virtual bool GotoStateId(int nState) final { return GotoState(nState); };
	bool         GotoState(int nState);
	virtual bool         IsInState(const char* sStateName) final;
	bool         IsInState(int nState);
	virtual const char*  GetState() final;
	virtual int          GetStateId() final;
	void         RegisterForAreaEvents(bool bEnable);
	bool         IsRegisteredForAreaEvents() const;

	void         SerializeProperties(TSerialize ser);

	virtual void GetMemoryUsage(ICrySizer* pSizer) const final;

private:
	SScriptState*  CurrentState() { return m_pScript->GetState(m_nCurrStateId); }
	void           CreateScriptTable(SEntitySpawnParams* pSpawnParams);
	void           SetEventTargets(XmlNodeRef& eventTargets);
	IScriptSystem* GetIScriptSystem() const { return gEnv->pScriptSystem; }

	void           SerializeTable(TSerialize ser, const char* name);
	bool           HaveTable(const char* name);

private:
	CEntity*       m_pEntity;
	CEntityScript* m_pScript;
	IScriptTable*  m_pThis;

	float          m_fScriptUpdateTimer;
	float          m_fScriptUpdateRate;

	// Cache Tables.
	SmartScriptTable m_hitTable;

	uint32           m_nCurrStateId           : 8;
	uint32           m_bUpdateScript          : 1;
	bool             m_bEnableSoundAreaEvents : 1;
};

#endif // __ScriptProxy_h__
