#pragma once

#include "Helpers/NativeEntityBase.h"

class CGeomEntity : public CNativeEntityBase
{
public:
	enum EProperties
	{
		eProperty_Model = 0,
		eProperty_PhysicalizationType,
		eProperty_Mass,

		eNumProperties,
	};

	enum EOutputPorts
	{
		eOutputPort_OnCollision = 0,
	};

	enum EPhysicalizationType
	{
		ePhysicalizationType_None = 0,
		ePhysicalizationType_Static,
		ePhysicalizationType_Rigid
	};

public:
	virtual ~CGeomEntity() {}

	// ISimpleExtension
	virtual void PostInit(IGameObject *pGameObject) final;

	virtual void HandleEvent(const SGameObjectEvent &event) final;

	virtual void OnResetState() final;
	// ~ISimpleExtension

protected:
	IPhysicalEntity* m_pPhysEnt;
};
