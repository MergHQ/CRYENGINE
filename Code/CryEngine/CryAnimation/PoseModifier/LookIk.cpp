// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include <CryAnimation/ICryAnimation.h>
#include <CryAnimation/IAnimationPoseModifier.h>
#include <CryExtension/ClassWeaver.h>
#include <CryRenderer/IRenderAuxGeom.h>
#include <CrySerialization/Forward.h>
#include <CrySerialization/Math.h>
#include <CrySerialization/Decorators/Range.h>

#include "PoseModifierDesc.h"
#include "PoseModifierHelper.h"


namespace PoseModifier
{

enum class EWeightJointAxisToUse
{
	eWeightJointAxisToUse_X = 0,
	eWeightJointAxisToUse_Y = 1,
	eWeightJointAxisToUse_Z = 2,
};

SERIALIZATION_ENUM_BEGIN(EWeightJointAxisToUse, "Weight Joint Translation Axis")
	SERIALIZATION_ENUM(EWeightJointAxisToUse::eWeightJointAxisToUse_X, "EWeightJointAxisToUse_X", "X-Axis")
	SERIALIZATION_ENUM(EWeightJointAxisToUse::eWeightJointAxisToUse_Y, "EWeightJointAxisToUse_Y", "Y-Axis")
	SERIALIZATION_ENUM(EWeightJointAxisToUse::eWeightJointAxisToUse_Z, "EWeightJointAxisToUse_Z", "Z-Axis")
SERIALIZATION_ENUM_END()

enum class EWeightToConsiderFullBlend
{
	eWeightToConsiderFullBlend_0 = 0,
	eWeightToConsiderFullBlend_100 = 1,
};

SERIALIZATION_ENUM_BEGIN(EWeightToConsiderFullBlend, "Weight To Consider Full Blend On The Axis")
	SERIALIZATION_ENUM(EWeightToConsiderFullBlend::eWeightToConsiderFullBlend_0, "EWeightToConsiderFullBlend_0", "0")
	SERIALIZATION_ENUM(EWeightToConsiderFullBlend::eWeightToConsiderFullBlend_100, "EWeightToConsiderFullBlend_100", "100")
SERIALIZATION_ENUM_END()

class CLookIK : public IAnimationPoseModifier, public IAnimationSerializable
{
private:

	struct SRotationLimitsDesc
	{
		Ang3 minLimit = Ang3{ -gf_PI };
		Ang3 maxLimit = Ang3{ gf_PI };

		void Serialize(Serialization::IArchive& ar);
	};

	struct SJointLimitedConstraintAimDesc
	{
		SJointNode jointToAimNode;
		SJointNode aimTargetJointNode;
		SJointNode upJointNode;
		SJointNode weightNode;
		EWeightJointAxisToUse weightAxis = EWeightJointAxisToUse::eWeightJointAxisToUse_X;
		EWeightToConsiderFullBlend weightForFullBlend = EWeightToConsiderFullBlend::eWeightToConsiderFullBlend_0;
		float weight = 1.0f;
		Vec3 aimVector = { 1.0f, 0.0f, 0.0f };
		Vec3 upVector = { 0.0f, 1.0f, 0.0f };
		SRotationLimitsDesc rotationLimits;
		Quat frame = { IDENTITY };
		bool relativeVectors = false;

		void Serialize(Serialization::IArchive& ar);
		void InitRotationFrame();
	};

public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimationPoseModifier)
		CRYINTERFACE_ADD(IAnimationSerializable)
		CRYINTERFACE_END()
	CRYGENERATE_CLASS_GUID(CLookIK, "AnimationPoseModifier_LookIK", "6613e10e-503a-4c5a-9ade-b4eacd00dd54"_cry_guid);

private:

	// IAnimationPoseModifier
	virtual bool Prepare(const SAnimationPoseModifierParams& params) override;
	virtual bool Execute(const SAnimationPoseModifierParams& params) override;
	virtual void Synchronize() override { }
	virtual void GetMemoryUsage(ICrySizer* pSizer) const override { pSizer->Add(*this); }

	// IAnimationSerializable
	virtual void Serialize(Serialization::IArchive& ar) override;

	static float GetEffectiveWeight(const IAnimationPoseData* const pPoseData, const float descriptionWeight, const uint32 weightJointId, const EWeightJointAxisToUse weightAxis, const EWeightToConsiderFullBlend weightToConsiderFullBlend);
	static void CalculateRequiredAbsoluteJointOrientation(const IDefaultSkeleton& defaultSkeleton, IAnimationPoseData* const pPoseData, const TJointId jointToAimId, const TJointId aimTargetJointId, const TJointId upJointId, const Quat& orientationFrame, const float weight, const SRotationLimitsDesc& rotationLimits, Quat& outJointToAimRequiredOrientationAbsolute);
	bool PrepareLookIKSection(const IDefaultSkeleton& defaultSkeleton, SJointLimitedConstraintAimDesc& lookIKSection);
	void Draw(const Vec3& origin, const Vec3& target, const Vec3& up, const float weight);

private:

	struct
	{
		std::vector<SJointLimitedConstraintAimDesc> lookIKSections;
		float weight = 1.0f;
	}
	m_desc;

	bool m_bInitialized = false;
	bool m_bDraw = false;
	bool m_bEnabled = true;
};

CRYREGISTER_CLASS(CLookIK)

} // namespace PoseModifier


namespace PoseModifier
{

bool CLookIK::Prepare(const SAnimationPoseModifierParams & params)
{
	if (m_bEnabled && !m_bInitialized)
	{
		const IDefaultSkeleton& defaultSkeleton = params.pCharacterInstance->GetIDefaultSkeleton();

		m_bInitialized = true;
		for (int i = 0; i < m_desc.lookIKSections.size() && m_bInitialized; ++i)
		{
			m_bInitialized = m_bInitialized && PrepareLookIKSection(defaultSkeleton, m_desc.lookIKSections[i]);
		}
	}

	return m_bInitialized;
}

bool CLookIK::Execute(const SAnimationPoseModifierParams& params)
{
	CRY_PROFILE_FUNCTION(PROFILE_ANIMATION);

	bool bSuccess = false;
	if (const ICVar* const pLookIKEnableCVar = gEnv->pConsole->GetCVar("ca_UseLookIK"))
	{
		if (pLookIKEnableCVar->GetIVal() != 0)
		{
			if (m_bEnabled && m_bInitialized)
			{
				if (IAnimationPoseData* const pPoseData = params.pPoseData)
				{
					const IDefaultSkeleton& defaultSkeleton = params.pCharacterInstance->GetIDefaultSkeleton();

					for (int i = 0; i < m_desc.lookIKSections.size(); ++i)
					{
						SJointLimitedConstraintAimDesc& lookIkSection = m_desc.lookIKSections[i];

						const float weight = m_desc.weight * GetEffectiveWeight(pPoseData, lookIkSection.weight, lookIkSection.weightNode.jointId, lookIkSection.weightAxis, lookIkSection.weightForFullBlend);

						if (weight > FLT_EPSILON)
						{
							QuatT jointToAimAbsolute = pPoseData->GetJointAbsolute(lookIkSection.jointToAimNode.jointId);
							Quat& jointToAimAbsoluteO = jointToAimAbsolute.q;

							if (lookIkSection.relativeVectors)
							{
								QuatT aimTargetJointAbsolute = defaultSkeleton.GetDefaultAbsJointByID(lookIkSection.aimTargetJointNode.jointId);
								QuatT upJointAbsolute = pPoseData->GetJointAbsolute(lookIkSection.upJointNode.jointId);

								//construct default aim & up vectors
								const Vec3 aimChainBaseToAimTargetRequiredAbsolute = (aimTargetJointAbsolute.t - jointToAimAbsolute.t).GetNormalized();
								const Vec3 aimChainBaseUpAbsolute = (upJointAbsolute.t - jointToAimAbsolute.t).GetNormalized();
								const Quat invertedJointToAimAbsoluteO = jointToAimAbsoluteO.GetInverted();

								lookIkSection.aimVector = invertedJointToAimAbsoluteO * aimChainBaseToAimTargetRequiredAbsolute;
								lookIkSection.upVector = invertedJointToAimAbsoluteO * aimChainBaseUpAbsolute;

								lookIkSection.InitRotationFrame();
							}

							CalculateRequiredAbsoluteJointOrientation(defaultSkeleton, pPoseData,
								lookIkSection.jointToAimNode.jointId, lookIkSection.aimTargetJointNode.jointId, lookIkSection.upJointNode.jointId, lookIkSection.frame,
								weight, lookIkSection.rotationLimits, jointToAimAbsoluteO);

							//push changes to the pose
							pPoseData->SetJointAbsolute(lookIkSection.jointToAimNode.jointId, jointToAimAbsolute);

							//propagate the absolute transformation changes down the skeleton chain
							PoseModifierHelper::ComputeJointChildrenAbsolute(PoseModifierHelper::GetDefaultSkeleton(params), static_cast<Skeleton::CPoseData&>(*params.pPoseData), lookIkSection.jointToAimNode.jointId);

							bSuccess = true;

							if (m_bDraw)
							{
								const QuatT& jointToAimAbsolute = pPoseData->GetJointAbsolute(lookIkSection.jointToAimNode.jointId);
								const QuatT& aimTargetAbsolute = pPoseData->GetJointAbsolute(lookIkSection.aimTargetJointNode.jointId);
								const QuatT& upJointAbsolute = pPoseData->GetJointAbsolute(lookIkSection.upJointNode.jointId);

								const Vec3 jointToAimAbsoluteP = jointToAimAbsolute.t;
								const Vec3 aimTargetAbsoluteP = aimTargetAbsolute.t;
								const Vec3 upJointAbsoluteP = upJointAbsolute.t;

								Draw(params.location * jointToAimAbsoluteP, params.location * aimTargetAbsoluteP, params.location * upJointAbsoluteP, 1.0f);
							}
						}
					}
				}
			}
		}
	}

	return bSuccess;
}

void CLookIK::Serialize(Serialization::IArchive & ar)
{
	ar(m_bDraw, "Draw", "^Draw");
	ar(Serialization::Range(m_desc.weight, 0.0f, 1.0f), "weight", "Weight");
	ar(m_desc.lookIKSections, "lookIKSections", "Look IK Sections");

	if (ar.isInput())
	{
		m_bInitialized = false;
	}
}

void CLookIK::SRotationLimitsDesc::Serialize(Serialization::IArchive& ar)
{
	ar(Serialization::RadiansAsDeg(minLimit), "minLimit", "Min Limit");
	ar(Serialization::RadiansAsDeg(maxLimit), "maxLimit", "Max Limit");
}

void CLookIK::SJointLimitedConstraintAimDesc::Serialize(Serialization::IArchive& ar)
{
	ar(jointToAimNode, "jointToAimNode", "Joint To Aim");
	ar(aimTargetJointNode, "aimTargetJointNode", "Aim Target Joint");
	ar(upJointNode, "upJointNode", "Up Joint");
	ar(weightNode, "weightNode", "Weight Node");
	ar(weightAxis, "weightAxis", "Weight Bone Translation Axis");
	ar(weightForFullBlend, "weightForFullBlend", "Weight To Consider Full Blend On The Axis");
	ar(Serialization::Range(weight, 0.0f, 1.0f), "weight", "Weight");
	ar(relativeVectors, "relativeVectors", "Aim & Up vectors are relative rather than absolute");
	ar(aimVector, "aimVector", "Aim Vector");
	ar(upVector, "upVector", "Up Vector");
	ar(rotationLimits, "rotationLimits", "Rotation Limits");

	if (ar.isInput())
	{
		InitRotationFrame();
	}
}

void CLookIK::SJointLimitedConstraintAimDesc::InitRotationFrame()
{
	const Vec3 normalizedAimVector = aimVector.GetNormalizedSafe(Vec3(1.0f, 0.0f, 0.0f));
	Vec3 normalizedRightVector = upVector.GetNormalizedSafe(Vec3(0.0f, 1.0f, 0.0f));
	if (fabsf(normalizedAimVector*normalizedRightVector) > 0.9999f)
	{
		normalizedRightVector = normalizedAimVector.GetOrthogonal();
	}
	Vec3 normalizedUpVector = normalizedAimVector.Cross(normalizedRightVector).normalize();
	normalizedRightVector = normalizedUpVector.Cross(normalizedAimVector).normalize();
	frame = Quat(Matrix33(normalizedAimVector, normalizedRightVector, normalizedUpVector));
}

float CLookIK::GetEffectiveWeight(const IAnimationPoseData* const pPoseData, const float descriptionWeight, const uint32 weightJointId, const EWeightJointAxisToUse weightAxis, const EWeightToConsiderFullBlend weightToConsiderFullBlend)
{
	float effectiveWeight = descriptionWeight;

	if (effectiveWeight > FLT_EPSILON)
	{
		if (weightJointId != INVALID_JOINT_ID && pPoseData != nullptr)
		{
			const Vec3 weightBoneTranslationVector = pPoseData->GetJointAbsoluteP(weightJointId);
			uint8 weightAxisIndex = 0;

			switch (weightAxis)
			{
			case EWeightJointAxisToUse::eWeightJointAxisToUse_X:
				weightAxisIndex = 0;
				break;

			case EWeightJointAxisToUse::eWeightJointAxisToUse_Y:
				weightAxisIndex = 1;
				break;

			case EWeightJointAxisToUse::eWeightJointAxisToUse_Z:
				weightAxisIndex = 2;
				break;

			default:
				break;
			}

			float boneWeight = clamp_tpl(abs(weightBoneTranslationVector[weightAxisIndex]), 0.0f, 1.0f);
			if (weightToConsiderFullBlend == EWeightToConsiderFullBlend::eWeightToConsiderFullBlend_0)
			{
				boneWeight = 1.0f - boneWeight;
			}
			effectiveWeight *= boneWeight;
		}
	}

	return effectiveWeight;
}

void CLookIK::CalculateRequiredAbsoluteJointOrientation(const IDefaultSkeleton& defaultSkeleton, IAnimationPoseData* const pPoseData, const TJointId jointToAimId, const TJointId aimTargetJointId, const TJointId upJointId, const Quat& orientationFrame, const float weight, const SRotationLimitsDesc& rotationLimits, Quat& outJointToAimRequiredOrientationAbsolute)
{
	const Quat& jointToAimAbsoluteO = pPoseData->GetJointAbsoluteO(jointToAimId);
	const Quat& parentJointAbsoluteO = pPoseData->GetJointAbsoluteO(defaultSkeleton.GetJointParentIDByID(jointToAimId));

	const Vec3 jointToAimPos = pPoseData->GetJointAbsoluteP(jointToAimId);
	const Vec3 aimTargetPos = pPoseData->GetJointAbsoluteP(aimTargetJointId);
	const Vec3 upJointPos = pPoseData->GetJointAbsoluteP(upJointId);

	const Vec3 xVector = (aimTargetPos - jointToAimPos).GetNormalizedSafe();
	Vec3 yVector = (upJointPos - jointToAimPos).GetNormalizedSafe();
	if (fabsf(xVector.Dot(yVector)) > 0.9999f)
		yVector = yVector.GetOrthogonal();
	const Vec3 zVector = xVector.Cross(yVector).normalize();
	yVector = zVector.Cross(xVector).normalize();
	const Quat aimOrientation = Quat(Matrix33(xVector, yVector, zVector));
	outJointToAimRequiredOrientationAbsolute = aimOrientation * !orientationFrame;

	Quat jointToAimRequiredOrientationRelative = outJointToAimRequiredOrientationAbsolute * !parentJointAbsoluteO;

	Ang3 rotations = Ang3(jointToAimRequiredOrientationRelative);

	for (size_t axisIndex = 0; axisIndex < 3; ++axisIndex)
	{
		rotations[axisIndex] = clamp_tpl(rotations[axisIndex], rotationLimits.minLimit[axisIndex], rotationLimits.maxLimit[axisIndex]);
	}

	jointToAimRequiredOrientationRelative.SetRotationXYZ(rotations);

	outJointToAimRequiredOrientationAbsolute = jointToAimRequiredOrientationRelative * parentJointAbsoluteO;
	outJointToAimRequiredOrientationAbsolute = Quat::CreateSlerp(jointToAimAbsoluteO, outJointToAimRequiredOrientationAbsolute, weight);
}

bool CLookIK::PrepareLookIKSection(const IDefaultSkeleton& defaultSkeleton, SJointLimitedConstraintAimDesc& lookIKSection)
{
	bool bSuccess = false;
	if (lookIKSection.jointToAimNode.ResolveJointIndex(defaultSkeleton))
	{
		if (lookIKSection.aimTargetJointNode.ResolveJointIndex(defaultSkeleton))
		{
			if (lookIKSection.upJointNode.ResolveJointIndex(defaultSkeleton))
			{
				lookIKSection.weightNode.ResolveJointIndex(defaultSkeleton);
				bSuccess = true;
			}
		}
	}

	return bSuccess;
}

void CLookIK::Draw(const Vec3& origin, const Vec3& target, const Vec3& up, const float weight)
{
	if (gEnv->pRenderer == nullptr)
	{
		return;
	}

	if (IRenderAuxGeom* pAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom())
	{
		const uint8 alpha = static_cast<uint8>(0x80 * weight);

		SAuxGeomRenderFlags flags = pAuxGeom->GetRenderFlags();
		flags.SetDepthTestFlag(e_DepthTestOff);
		flags.SetAlphaBlendMode(e_AlphaBlended);
		pAuxGeom->SetRenderFlags(flags);

		pAuxGeom->DrawSphere(origin, 0.01f, ColorB(0xff, 0xff, 0xff, alpha));

		pAuxGeom->DrawSphere(target, 0.01f, ColorB(0xff, 0x80, 0x80, alpha));
		IRenderAuxText::DrawLabelF(target, 1.0f, "Target");

		pAuxGeom->DrawSphere(up, 0.01f, ColorB(0x80, 0xff, 0x80, alpha));
		IRenderAuxText::DrawLabelF(up, 1.0f, "Up");

		pAuxGeom->DrawLine(
			origin, ColorB(0xff, 0x00, 0x00, alpha),
			target, ColorB(0xff, 0x80, 0x80, alpha));

		pAuxGeom->DrawLine(
			origin, ColorB(0x00, 0xff, 0x00, alpha),
			up, ColorB(0x80, 0xff, 0x80, alpha));
	}
}

} // namespace PoseModifier

