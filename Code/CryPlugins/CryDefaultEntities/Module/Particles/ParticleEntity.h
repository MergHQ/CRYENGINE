#pragma once

#include "Helpers/NativeEntityBase.h"

////////////////////////////////////////////////////////
// Sample entity for creating a particle entity
////////////////////////////////////////////////////////
class CDefaultParticleEntity : public CNativeEntityBase
{
public:
	// Indices of the properties, registered in the Register function
	enum EProperties
	{
		eProperty_Active = 0,
		eProperty_EffectName,

		eNumProperties
	};

public:
	CDefaultParticleEntity();
	virtual ~CDefaultParticleEntity() {}

	// CNativeEntityBase
	virtual void ProcessEvent(SEntityEvent& event) override;
	// ~CNativeEntityBase

protected:
	// Called on entity spawn, or when the state of the entity changes in Editor
	void Reset();

protected:
	int m_particleSlot;
};
