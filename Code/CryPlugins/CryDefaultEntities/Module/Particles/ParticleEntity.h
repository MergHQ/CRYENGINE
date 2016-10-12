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
	virtual void OnResetState() override;
	// ~CNativeEntityBase

protected:
	int m_particleSlot;
};
