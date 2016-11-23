#pragma once

#include "Helpers/DesignerEntityComponent.h"

#include <CrySerialization/Decorators/Resources.h>

////////////////////////////////////////////////////////
// Sample entity for creating a particle entity
////////////////////////////////////////////////////////
class CDefaultParticleEntity final
	: public CDesignerEntityComponent
	, public IEntityPropertyGroup
{
	CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(CDefaultParticleEntity, "ParticleEntity", 0x31B3EAD4C34442F7, 0xB794B33746D4232B);

	CDefaultParticleEntity();
	virtual ~CDefaultParticleEntity() {}

public:
	// CDesignerEntityComponent
	virtual IEntityPropertyGroup* GetPropertyGroup() final { return this; }

	virtual void OnResetState() override;
	// ~CDesignerEntityComponent

	// IEntityPropertyGroup
	virtual const char* GetLabel() const override { return "ParticleEffect Properties"; }

	virtual void SerializeProperties(Serialization::IArchive& archive) override
	{
		archive(m_bActive, "Active", "Active");
		archive(Serialization::ParticleName(m_particleEffectPath), "ParticleEffect", "ParticleEffect");

		if (archive.isInput())
		{
			OnResetState();
		}
	}
	// ~IEntityPropertyGroup

protected:
	int m_particleSlot;

	bool m_bActive = true;
	string m_particleEffectPath;
};
