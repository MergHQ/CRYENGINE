// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include <CryEntitySystem/IEntityComponent.h>

class CAIEntityComponent final : public IEntityComponent
{
public:
	CAIEntityComponent(const CWeakRef<CAIObject>& reference)
		: m_objectReference(reference) {}
	virtual ~CAIEntityComponent();

	// IEntityComponent
	virtual void Initialize() override;

	virtual void ProcessEvent(const SEntityEvent& event) override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	// ~IEntityComponent

	static void ReflectType(Schematyc::CTypeDesc<CAIEntityComponent>& desc)
	{
		desc.SetGUID("{435E4CAE-2A4D-453C-BAAB-F3006E329DA7}"_cry_guid);
		desc.SetComponentFlags({ IEntityComponent::EFlags::NoSave, IEntityComponent::EFlags::HiddenFromUser });
	}

	tAIObjectID GetAIObjectID() const { return m_objectReference.GetObjectID(); }

protected:
	CWeakRef<CAIObject> m_objectReference;
};