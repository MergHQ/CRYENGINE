#pragma once

#include "Helpers/NativeEntityBase.h"

////////////////////////////////////////////////////////
// Sample entity for creating a cloud entity
////////////////////////////////////////////////////////
class CCloudEntity : public CNativeEntityBase
{
public:
	// Indices of the properties, registered in the Register function
	enum EProperties
	{
		eProperty_CloudFile = 0,
		eProperty_Scale,

		ePropertyGroup_MovementBegin,
		eProperty_AutoMove,
		eProperty_Speed,
		eProperty_SpaceLoopBox,
		eProperty_FadeDistance,
		ePropertyGroup_MovementEnd,

		eNumProperties
	};

public:
	CCloudEntity();
	virtual ~CCloudEntity() {}

	// CNativeEntityBase
	virtual bool Init(IGameObject* pGameObject) override;

	virtual void ProcessEvent(SEntityEvent& event) override;
	// ~CNativeEntityBase

protected:
	// Called on entity spawn, or when the state of the entity changes in Editor
	void Reset();

protected:
	int m_cloudSlot;
};
