// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "PerceptionComponentHelpers.h"

#include <CryAISystem/IAuditionMap.h>

namespace Perception
{

namespace ComponentHelpers
{

SERIALIZATION_ENUM_BEGIN(EStimulusObstructionHandling, "Stimulus Obstruction Handling")
	SERIALIZATION_ENUM(EStimulusObstructionHandling::IgnoreAllObstructions, "IgnoreAllObstructions", "Ignore All Obstructions")
	SERIALIZATION_ENUM(EStimulusObstructionHandling::RayCastWithLinearFallOff, "RayCastWithLinearFallOff", "Raycast with Linear Falloff")
SERIALIZATION_ENUM_END()

SERIALIZATION_ENUM_BEGIN_NESTED(SLocation, EType, "Location Type")
	SERIALIZATION_ENUM(SLocation::EType::Bone, "Bone", "Bone")
	SERIALIZATION_ENUM(SLocation::EType::Pivot, "Pivot", "Pivot")
SERIALIZATION_ENUM_END()

SERIALIZATION_ENUM_BEGIN(VisionMapTypes, "Vision Map Types")
	SERIALIZATION_ENUM(VisionMapTypes::General, "General", "General")
	SERIALIZATION_ENUM(VisionMapTypes::AliveAgent, "AliveAgent", "Alive Agent")
	SERIALIZATION_ENUM(VisionMapTypes::DeadAgent, "DeadAgent", "Dead Agent")
	SERIALIZATION_ENUM(VisionMapTypes::Player, "Player", "Player")
SERIALIZATION_ENUM_END()

//////////////////////////////////////////////////////////////////////////
void SLocation::GetPositionAndOrientation(IEntity* pEntity, Vec3* pOutPosition, Vec3* pOutOrientation) const
{
	if (type == EType::Bone)
	{
		if (GetTransformFromBone(pEntity, pOutPosition, pOutOrientation))
			return;
	}
	GetTransformFromPivot(pEntity, pOutPosition, pOutOrientation);
}

bool SLocation::GetTransformFromPivot(IEntity* pEntity, Vec3* pOutPosition, Vec3* pOutOrientation) const
{
	if (pOutPosition)
	{
		*pOutPosition = offset.IsZero() ? pEntity->GetWorldPos() : pEntity->GetWorldTM().TransformPoint(offset);
	}
	if (pOutOrientation)
	{
		*pOutOrientation = pEntity->GetForwardDir();
	}
	return true;
}

bool SLocation::GetTransformFromBone(IEntity* pEntity, Vec3* pOutPosition, Vec3* pOutOrientation) const
{
	const ICharacterInstance* pCharacter = pEntity->GetCharacter(0);     //TODO: how to get correct slot?
	if (!pCharacter)
	{
		SCHEMATYC_EDITOR_WARNING("Cannot retrieve bone '%s' because character instance is not available!", boneName.c_str());
		return false;
	}

	const int32 boneId = pCharacter->GetIDefaultSkeleton().GetJointIDByName(boneName);
	if (boneId < 0)
	{
		SCHEMATYC_EDITOR_WARNING("Cannot retrieve bone '%s' because it could not be found on the default skeleton!", boneName.c_str());
		return false;
	}

	const ISkeletonPose* pSkeletonPose = pCharacter->GetISkeletonPose();
	if (!pSkeletonPose)
	{
		SCHEMATYC_EDITOR_WARNING("Cannot retrieve bone '%s' because there is no skeleton pose for the character!", boneName.c_str());
		return false;
	}

	const QuatT localJointQuatT = pSkeletonPose->GetAbsJointByID(boneId);
	const Matrix34 localJointTransform(localJointQuatT);

	//TODO: get orthogonal matrix in the case transform matrix is scaled?
	const Matrix34& worldTransform = pEntity->GetWorldTM();
	const Matrix34 worldJointTransform = worldTransform * localJointTransform;

	if (pOutPosition != nullptr)
	{
		*pOutPosition = offset.IsZero() ? worldJointTransform.GetTranslation() : worldJointTransform.TransformPoint(offset);
	}
	if (pOutOrientation != nullptr)
	{
		*pOutOrientation = worldJointTransform.GetColumn1();
	}
	return true;
}

} //! ComponentHelpers
} //! Perception
