// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"

#include <CryAnimation/ICryAnimation.h>
#include <CryAnimation/IAnimationPoseModifier.h>
#include <CryExtension/ClassWeaver.h>
#include <CrySerialization/Forward.h>
#include <CrySerialization/Decorators/Range.h>

#include "PoseModifierDesc.h"


namespace PoseModifier
{

enum class ECopyTransformSpace
{
	Local,
	World
};

SERIALIZATION_ENUM_BEGIN(ECopyTransformSpace, "Space")
	SERIALIZATION_ENUM(ECopyTransformSpace::Local, "local", "Local")
	SERIALIZATION_ENUM(ECopyTransformSpace::World, "world", "World")
SERIALIZATION_ENUM_END()

class CCopyTransform : public IAnimationPoseModifier, public IAnimationSerializable
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimationPoseModifier)
		CRYINTERFACE_ADD(IAnimationSerializable)
	CRYINTERFACE_END()
	CRYGENERATE_CLASS_GUID(CCopyTransform, "AnimationPoseModifier_CopyTransform", "6a078d00-c194-41e1-b891-9c52d092045a"_cry_guid);

private:

	// IAnimationPoseModifier
	virtual bool Prepare(const SAnimationPoseModifierParams& params) override;
	virtual bool Execute(const SAnimationPoseModifierParams& params) override;
	virtual void Synchronize() override {}
	virtual void GetMemoryUsage(ICrySizer* pSizer) const override {}

	// IAnimationSerializable
	virtual void Serialize(Serialization::IArchive& ar) override;

private:

	struct
	{
		SJointNode drivenNode;
		SJointNode targetNode;
		bool translation = true;
		bool rotation = true;
		ECopyTransformSpace space = ECopyTransformSpace::Local;
		SJointNode weightNode;
		float weight = 1.0f;
	}
	m_desc;

	bool m_bInitialized = false;
};

CRYREGISTER_CLASS(CCopyTransform)

} // namespace PoseModifier


namespace PoseModifier
{

bool CCopyTransform::Prepare(const SAnimationPoseModifierParams& params)
{
	if (!m_bInitialized)
	{
		const IDefaultSkeleton& defaultSkeleton = params.pCharacterInstance->GetIDefaultSkeleton();

		if (m_desc.drivenNode.ResolveJointIndex(defaultSkeleton))
		{
			if (m_desc.targetNode.ResolveJointIndex(defaultSkeleton))
			{
				m_desc.weightNode.ResolveJointIndex(defaultSkeleton);
				m_bInitialized = true;
			}
		}
	}

	return m_bInitialized;
}

bool CCopyTransform::Execute(const SAnimationPoseModifierParams& params)
{
	CRY_PROFILE_FUNCTION(PROFILE_ANIMATION);

	if (m_bInitialized && params.pPoseData)
	{
		const IDefaultSkeleton& defaultSkeleton = params.pCharacterInstance->GetIDefaultSkeleton();
		IAnimationPoseData* pPoseData = params.pPoseData;

		uint32 numSkelJoints = params.pPoseData->GetJointCount();

		QuatT targetAbs(IDENTITY);
		QuatT sourceAbs(IDENTITY);
		if (m_desc.drivenNode.jointId < numSkelJoints)
			sourceAbs = pPoseData->GetJointAbsolute(m_desc.drivenNode.jointId);

		switch (m_desc.space)
		{
		case ECopyTransformSpace::Local: targetAbs = sourceAbs * pPoseData->GetJointRelative(m_desc.targetNode.jointId); break;
		case ECopyTransformSpace::World: targetAbs = pPoseData->GetJointAbsolute(m_desc.targetNode.jointId); break;
		}

		if (!m_desc.rotation)
			targetAbs.q = sourceAbs.q;
		if (!m_desc.translation)
			targetAbs.t = sourceAbs.t;

		float weight = m_desc.weight;
		if (m_desc.weightNode.jointId < numSkelJoints)
			weight *= clamp_tpl(pPoseData->GetJointRelative(m_desc.weightNode.jointId).t.x, 0.0f, 1.0f);

		targetAbs = QuatT::CreateNLerp(sourceAbs, targetAbs, weight);

		uint32 p = defaultSkeleton.GetJointParentIDByID(m_desc.drivenNode.jointId);
		QuatT targetRel = targetAbs;
		if (p != INVALID_JOINT_ID)
			targetRel = pPoseData->GetJointAbsolute(p).GetInverted() * targetAbs;

		pPoseData->SetJointRelative(m_desc.drivenNode.jointId, targetRel);
		pPoseData->SetJointAbsolute(m_desc.drivenNode.jointId, targetAbs);

		// update children absolute
		for (uint32 i = m_desc.drivenNode.jointId + 1; i < numSkelJoints; i++)
		{
			const uint32 parentJointIdx = defaultSkeleton.GetJointParentIDByID(i);
			pPoseData->SetJointAbsolute(i, pPoseData->GetJointAbsolute(parentJointIdx) * pPoseData->GetJointRelative(i));
		}
	}

	return true;
}

void CCopyTransform::Serialize(Serialization::IArchive& ar)
{
	using Serialization::Range;

	ar(m_desc.drivenNode, "drivenNode", "Driven Node");
	ar(m_desc.targetNode, "targetNode", "Target Node");
	ar(m_desc.translation, "translation", "Translation");
	ar(m_desc.rotation, "rotation", "Rotation");
	ar(m_desc.space, "space", "Space");
	ar(m_desc.weightNode, "weightNode", "Weight Node");
	ar(Range(m_desc.weight, 0.0f, 1.0f), "weight", "Weight");

	if (ar.isInput())
	{
		m_desc.weight = clamp_tpl(m_desc.weight, 0.0f, 1.0f);
	}

	if (ar.isValidation())
	{
		if (m_desc.drivenNode.name.empty())
		{
			ar.warning(m_desc.drivenNode.name, "Driven Node is not specified.");
		}
	}

	if (ar.isInput())
	{
		m_bInitialized = false;
	}
}

} // namespace PoseModifier
