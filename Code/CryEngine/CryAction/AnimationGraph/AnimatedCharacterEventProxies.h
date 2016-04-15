// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#ifndef __AnimatedCharacterComponents_h__
	#define __AnimatedCharacterComponents_h__

	#include <CryEntitySystem/IComponent.h>

class CAnimatedCharacter;
class CAnimatedCharacterComponent_Base : public IComponent
{
public:
	struct SComponentInitializerAnimChar : public SComponentInitializer
	{
		SComponentInitializerAnimChar(IEntity* piEntity, CAnimatedCharacter* pAnimChar)
			: SComponentInitializer(piEntity)
			, m_pAnimCharacter(pAnimChar)
		{
		}

		CAnimatedCharacter* m_pAnimCharacter;
	};

	virtual void Initialize(const SComponentInitializer& init);

protected:

	typedef CAnimatedCharacterComponent_Base SuperClass;

	explicit CAnimatedCharacterComponent_Base();

	virtual void ProcessEvent(SEntityEvent& event);
	virtual void OnPrePhysicsUpdate(float elapseTime) = 0;

	CAnimatedCharacter* m_pAnimCharacter;
	IEntity*            m_piEntity;
};

class CAnimatedCharacterComponent_PrepareAnimatedCharacterForUpdate : public CAnimatedCharacterComponent_Base
{
public:
	CAnimatedCharacterComponent_PrepareAnimatedCharacterForUpdate();

	ILINE void QueueRotation(const Quat& rotation) { m_queuedRotation = rotation; m_hasQueuedRotation = true; }
	ILINE void ClearQueuedRotation()               { m_hasQueuedRotation = false; }

protected:

	virtual IComponent::ComponentEventPriority GetEventPriority(const int eventID) const;

private:
	Quat m_queuedRotation;
	bool m_hasQueuedRotation;
	virtual void OnPrePhysicsUpdate(float elapsedTime);
};

DECLARE_COMPONENT_POINTERS(CAnimatedCharacterComponent_PrepareAnimatedCharacterForUpdate);

class CAnimatedCharacterComponent_StartAnimProc : public CAnimatedCharacterComponent_Base
{
protected:

	virtual ComponentEventPriority GetEventPriority(const int eventID) const;
	virtual void                   OnPrePhysicsUpdate(float elapsedTime);
};

class CAnimatedCharacterComponent_GenerateMoveRequest : public CAnimatedCharacterComponent_Base
{
protected:

	virtual IComponent::ComponentEventPriority GetEventPriority(const int eventID) const;
	virtual void                               OnPrePhysicsUpdate(float elapsedTime);
};

#endif // __AnimatedCharacterComponents_h__
