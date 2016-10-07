#pragma once

#include "Helpers/NativeEntityBase.h"

////////////////////////////////////////////////////////
// Physicalized entity of any model that moves without being logically interacted with
////////////////////////////////////////////////////////
class CRigidBody : public CNativeEntityBase
{
public:
	enum EProperties
	{
		eProperty_Model = 0,
		eProperty_Mass,

		eNumProperties,
	};

public:
	virtual ~CRigidBody() {}

	// ISimpleExtension
	virtual bool Init(IGameObject* pGameObject) override;
	virtual void ProcessEvent(SEntityEvent& event) override;
	// ~ISimpleExtension

protected:
	void Reset();
};
