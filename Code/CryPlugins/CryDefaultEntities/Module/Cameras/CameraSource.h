#pragma once

#include <CryEntitySystem/IEntityComponent.h>

////////////////////////////////////////////////////////
// Sample entity for creating a camera source
////////////////////////////////////////////////////////
class CCameraSource : public IEntityComponent
{
	CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(CCameraSource, "CameraSource", 0x19D1AD13186749A0, 0xA93EA7CA58B1F698);

	virtual ~CCameraSource() {}

	// IEntityComponent
	virtual void Initialize() override;
	// ~IEntityComponent
};
