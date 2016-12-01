#pragma once

#include "Helpers/DesignerEntityComponent.h"

#include <CrySerialization/STL.h>
#include <CrySerialization/Decorators/ResourceFilePath.h>
#include <CrySerialization/IArchive.h>
#include <CrySerialization/Math.h>
#include <CrySerialization/Decorators/ResourceFilePathImpl.h>

#include <CryRenderer/IRenderer.h>
#include <CryPhysics/physinterface.h>
#include <Cry3DEngine/IRenderNode.h>

////////////////////////////////////////////////////////
// Sample entity for creating a cloud entity
////////////////////////////////////////////////////////
class CCloudEntity final  
	: public CDesignerEntityComponent<>
	, public IEntityPropertyGroup
{
	CRY_ENTITY_COMPONENT_INTERFACE_AND_CLASS(CCloudEntity, "CloudEntity", 0xA22A4310BC074CBC, 0x8E9FDC669F3BF254);

public:
	CCloudEntity();
	virtual ~CCloudEntity() {}

	// CDesignerEntityComponent
	virtual void Initialize() final;

	virtual IEntityPropertyGroup* GetPropertyGroup() final { return this; }

	virtual void OnResetState() final;
	// ~CDesignerEntityComponent

	// IEntityPropertyGroup
	virtual const char* GetLabel() const override { return "Cloud Properties"; }

	virtual void SerializeProperties(Serialization::IArchive& archive) override
	{
		archive(Serialization::ResourceFilePath(m_cloudFile, "XML files|*.xml", "Libs/Clouds"), "CloudFile", "CloudFile");
		archive(m_scale, "Scale", "Scale");

		if (archive.openBlock("Movement", "Movement"))
		{
			archive(m_properties.m_autoMove, "AutoMove", "AutoMove");
			archive(m_properties.m_speed, "Speed", "Speed");
			archive(m_properties.m_spaceLoopBox, "SpaceLoopBox", "SpaceLoopBox");
			archive(m_properties.m_fadeDistance, "FadeDistance", "FadeDistance");

			archive.closeBlock();
		}

		if (archive.isInput())
		{
			OnResetState();
		}
	}
	// ~IEntityPropertyGroup

protected:
	int m_cloudSlot;

	string m_cloudFile;
	float m_scale = 1.f;

	SCloudMovementProperties m_properties;
};
