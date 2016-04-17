// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   IComponent.h
//  Version:     v1.00
//  Created:     28/04/2012 : Stephen M. North
//  Compilers:   Visual Studio.NET 2003
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __IComponent_h__
	#define __IComponent_h__

struct SEntityEvent;
struct IEntity;
class IComponent;

	#include <CryCore/CryFlags.h>

	#define DECLARE_COMPONENT_POINTERS(name) \
	  DECLARE_SHARED_POINTERS(name)

typedef unsigned int EntityId;
DECLARE_COMPONENT_POINTERS(IComponent)

struct IComponentEventDistributer
{
	enum
	{
		EVENT_ALL = 0xffffffff
	};

	virtual void RegisterEvent(const EntityId entityID, IComponentPtr pComponent, const int eventID, const int flags) = 0;
	virtual ~IComponentEventDistributer() {}
};

class IComponent : public std::enable_shared_from_this<IComponent>
{
public:
	enum
	{
		EComponentFlags_IsRegistered     = BIT(0),
		EComponentFlags_Enable           = BIT(1),
		EComponentFlags_Disable          = BIT(2),
		EComponentFlags_LazyRegistration = BIT(3),
	};
	typedef int             ComponentEventPriority;
	typedef CCryFlags<uint> ComponentFlags;

	struct SComponentInitializer
	{
		SComponentInitializer(IEntity* pEntity) : m_pEntity(pEntity) {}
		IEntity* m_pEntity;
	};

	IComponent() : m_pComponentEventDistributer(NULL), m_componentEntityId(0) {}
	virtual ~IComponent() {}

	// <interfuscator:shuffle>
	//! By overriding this function proxy will be able to handle events sent from the host Entity.
	//! \param event Event structure, contains event id and parameters.
	virtual void                   ProcessEvent(SEntityEvent& event) = 0;
	virtual ComponentEventPriority GetEventPriority(const int eventID) const = 0;
	virtual void                   Initialize(const SComponentInitializer& init) {}
	// </interfuscator:shuffle>

	ComponentFlags&       GetFlags()       { return m_flags; }
	const ComponentFlags& GetFlags() const { return m_flags; }

	void                  SetDistributer(IComponentEventDistributer* piEntityEventDistributer, const EntityId entityID)
	{
		m_pComponentEventDistributer = piEntityEventDistributer;
		m_componentEntityId = entityID;
	}

protected:

	void RegisterEvent(const int eventID, const int flags)
	{
		CRY_ASSERT(m_pComponentEventDistributer);
		if (m_pComponentEventDistributer)
		{
			m_pComponentEventDistributer->RegisterEvent(m_componentEntityId, shared_from_this(), eventID, flags);
		}
	}

private:
	IComponentEventDistributer* m_pComponentEventDistributer;
	EntityId                    m_componentEntityId;
	ComponentFlags              m_flags;
};

#endif // __IComponent_h__
