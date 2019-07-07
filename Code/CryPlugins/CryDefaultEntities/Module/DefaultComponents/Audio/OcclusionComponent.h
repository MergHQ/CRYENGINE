// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioSystem.h>
#include <CrySerialization/Forward.h>
#include <CrySchematyc/Reflection/TypeDesc.h>
#include <CrySchematyc/Env/IEnvRegistrar.h>
#include <CryEntitySystem/IEntityComponent.h>
#include <CryString/CryString.h>

class CPlugin_CryDefaultEntities;

namespace Cry
{
namespace Audio
{
namespace DefaultComponents
{
class COcclusionComponent final : public IEntityComponent
{
protected:

	friend CPlugin_CryDefaultEntities;
	static void Register(Schematyc::CEnvRegistrationScope& componentScope);

	// IEntityComponent
	virtual void                    Initialize() override;
	virtual void                    OnShutDown() override;
	virtual Cry::Entity::EventFlags GetEventMask() const override;
	virtual void                    ProcessEvent(SEntityEvent const& event) override;
	// ~IEntityComponent

public:

	CryAudio::EOcclusionType m_occlusionType = CryAudio::EOcclusionType::None;
	float                    m_occlusionRayOffset;

public:

	COcclusionComponent() = default;

	static void ReflectType(Schematyc::CTypeDesc<COcclusionComponent>& desc);
	void        SetOcclusionType(CryAudio::EOcclusionType const occlusionType);
	void        SetOcclusionRayOffset(float const offset);

private:

	IEntityAudioComponent* m_pIEntityAudioComponent = nullptr;
};
} // namespace DefaultComponents
} // namespace Audio
} // namespace Cry
