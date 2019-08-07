// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryEntitySystem/IEntityComponent.h>
#include <CryAISystem/NavigationSystem/MNMBaseTypes.h>

struct IEntityMarkupShapeComponent : public IEntityComponent
{
	virtual void SetAnnotationFlag(const char* szShapeName, const NavigationAreaFlagID& flagId, bool enableFlag) = 0;
	virtual void ToggleAnnotationFlags(const char* szShapeName, const MNM::AreaAnnotation annotationFlags) = 0;
	virtual void ResetAnotations() = 0;
	
	static void ReflectType(Schematyc::CTypeDesc<IEntityMarkupShapeComponent>& desc)
	{
		desc.SetGUID("47F2A131-2943-4F29-BE13-5F2B79ABF220"_cry_guid);
	}
};