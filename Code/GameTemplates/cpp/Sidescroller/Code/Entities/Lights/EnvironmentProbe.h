#pragma once

#include "Entities/Helpers/NativeEntityBase.h"

class CGameEntityNodeFactory;

////////////////////////////////////////////////////////
// Sample entity for creating an environment probe
////////////////////////////////////////////////////////
class CEnvironmentProbeEntity
	: public CGameObjectExtensionHelper<CEnvironmentProbeEntity, CNativeEntityBase>
{
public:
	// Indices of the properties, registered in the Register function
	enum EProperties
	{
		eProperty_Active = 0,

		eProperty_BoxSizeX,
		eProperty_BoxSizeY,
		eProperty_BoxSizeZ,

		ePropertyGroup_ProjectionBegin,
		eProperty_BoxProject,

		eProperty_fBoxWidth,
		eProperty_fBoxHeight,
		eProperty_fBoxLength,

		ePropertyGroup_ProjectionEnd,

		ePropertyGroup_OptionsBegin,
		eProperty_IgnoreVisAreas,
		eProperty_SortPriority,
		eProperty_AttenuationFalloffMax,
		ePropertyGroup_OptionsEnd,

		ePropertyGroup_AdvancedBegin,
		eProperty_Cubemap,
		ePropertyGroup_AdvancedEnd,

		eNumProperties
	};

public:
	CEnvironmentProbeEntity();
	virtual ~CEnvironmentProbeEntity() {}

	// CNativeEntityBase
	virtual void ProcessEvent(SEntityEvent& event) override;
	// ~CNativeEntityBase

protected:
	// Called on entity spawn, or when the state of the entity changes in Editor
	void Reset();

	void GetCubemapTextures(const char *path, ITexture **pSpecular, ITexture **pDiffuse);

	// Specifies the entity geometry slot in which the light is loaded, -1 if not currently loaded
	// We default to using slot 1 for this light sample, in case the user would want to put geometry into slot 0.
	int m_lightSlot;

	// Light parameters, updated in the Reset function
	CDLight m_light;
};

