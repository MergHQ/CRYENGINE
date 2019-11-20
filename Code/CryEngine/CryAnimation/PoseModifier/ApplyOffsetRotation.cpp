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

class CApplyOffsetRotation : public IAnimationPoseModifier, public IAnimationSerializable
{
public:

	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimationPoseModifier)
		CRYINTERFACE_ADD(IAnimationSerializable)
	CRYINTERFACE_END()
	CRYGENERATE_CLASS_GUID(CApplyOffsetRotation, "AnimationPoseModifier_ApplyOffsetRotation", "6a078d00-c194-41e1-b891-9c52d094076a"_cry_guid);

private:

	// IAnimationPoseModifier
	virtual bool Prepare(const SAnimationPoseModifierParams& params) override;
	virtual bool Execute(const SAnimationPoseModifierParams& params) override;
	virtual void Synchronize() override {}
	virtual void GetMemoryUsage(ICrySizer* pSizer) const override {}

	// IAnimationSerializable
	virtual void Serialize(Serialization::IArchive& ar) override;

	void Draw(const QuatT& location, const QuatT& sourceAbsolute, const QuatT& targetAbsolute, const float weight);

private:

	struct SNodeWeightLimitsDesc
	{
		SJointNode node;
		Ang3 minLimit;
		Ang3 maxLimit;
		Vec3 multiplier;
		float weight;
		bool enabled;

		SNodeWeightLimitsDesc()
			: minLimit(Vec3(DEG2RAD(-360.0f)))
			, maxLimit(Vec3(DEG2RAD(360.0f)))
			, multiplier(1)
			, weight(1.0f)
			, enabled(true)
		{
		}

		void Serialize(Serialization::IArchive& ar);
	};

	struct
	{
		std::vector<SNodeWeightLimitsDesc> nodes;
		SJointNode sourceNode;
		SJointNode targetNode;
		SJointNode weightNode;
		float weight = 1.0f;
	}
	m_desc;

	DynArray<uint32> m_jointIndices;
	bool m_bInitialized = false;
	bool m_bDraw = false;
};

CRYREGISTER_CLASS(CApplyOffsetRotation);

} // namespace PoseModifier


namespace PoseModifier
{

bool CApplyOffsetRotation::Prepare(const SAnimationPoseModifierParams& params)
{
	if (!m_bInitialized)
	{
		const IDefaultSkeleton& defaultSkeleton = params.pCharacterInstance->GetIDefaultSkeleton();

		m_jointIndices.clear();
		for (int i = 0; i < m_desc.nodes.size(); i++)
		{
			m_desc.nodes[i].node.ResolveJointIndex(defaultSkeleton);
			m_jointIndices.push_back(m_desc.nodes[i].node.jointId);
		}

		if (m_desc.sourceNode.ResolveJointIndex(defaultSkeleton))
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

bool CApplyOffsetRotation::Execute(const SAnimationPoseModifierParams& params)
{
	CRY_PROFILE_FUNCTION(PROFILE_ANIMATION);

	if (m_bInitialized && params.pPoseData)
	{
		const IDefaultSkeleton& defaultSkeleton = params.pCharacterInstance->GetIDefaultSkeleton();
		IAnimationPoseData* pPoseData = params.pPoseData;

		uint32 numJoints = m_jointIndices.size();

		if (numJoints > 0)
		{
			uint32 numSkelJoints = params.pPoseData->GetJointCount();

			QuatT targetAbs(IDENTITY);
			if (m_desc.targetNode.jointId < numSkelJoints)
				targetAbs = pPoseData->GetJointAbsolute(m_desc.targetNode.jointId);

			QuatT sourceAbs(IDENTITY);
			if (m_desc.sourceNode.jointId < numSkelJoints)
				sourceAbs = pPoseData->GetJointAbsolute(m_desc.sourceNode.jointId);

			Quat targetOffsetRel = !sourceAbs.q * targetAbs.q;

			float weight = m_desc.weight;
			if (m_desc.weightNode.jointId < numSkelJoints)
				weight *= clamp_tpl(pPoseData->GetJointRelative(m_desc.weightNode.jointId).t.x, 0.0f, 1.0f);

			for (uint32 i = 0; i < numJoints; i++)
			{
				uint32 j = m_jointIndices[i];
				if (j != INVALID_JOINT_ID)
				{
					uint32 p = defaultSkeleton.GetJointParentIDByID(j);
					SNodeWeightLimitsDesc nodeDesc = m_desc.nodes[i];
					Ang3 anglesOffset = Ang3(targetOffsetRel);
					for (int a = 0; a < 3; a++)
					{
						anglesOffset[a] *= nodeDesc.multiplier[a] * nodeDesc.weight * weight;
						float limitMax = nodeDesc.maxLimit[a];
						float limitMin = nodeDesc.minLimit[a];
						if (anglesOffset[a] > limitMax)
							anglesOffset[a] = limitMax;
						if (anglesOffset[a] < limitMin)
							anglesOffset[a] = limitMin;
					}

					Quat rotInSourceAbs = !sourceAbs.q * pPoseData->GetJointAbsolute(j).q;
					Quat rotAbs = sourceAbs.q * Quat(anglesOffset) * rotInSourceAbs;
					rotAbs.NormalizeSafe();

					Quat parentAbs = pPoseData->GetJointAbsolute(p).q;
					Quat rotRel = !parentAbs * rotAbs;

					pPoseData->SetJointAbsoluteO(j, rotAbs);
					pPoseData->SetJointRelativeO(j, rotRel);
				}
			}

			// calculate absolute pose from changed joints

			for (uint32 j = 1; j < numSkelJoints; j++)
			{
				if (uint32 p = defaultSkeleton.GetJointParentIDByID(j))
				{
					QuatT poseAbs = pPoseData->GetJointAbsolute(p) * pPoseData->GetJointRelative(j);
					poseAbs.q.NormalizeSafe();
					pPoseData->SetJointAbsolute(j, poseAbs);
				}
			}

			if (m_bDraw)
				Draw((QuatT)params.location, sourceAbs, targetAbs, weight);
		}
	}

	return true;
}

void CApplyOffsetRotation::Serialize(Serialization::IArchive& ar)
{
	using Serialization::Range;

	ar(m_bDraw, "draw", "^Draw");

	ar(m_desc.sourceNode, "sourceNode", "Source Node");
	ar(m_desc.targetNode, "targetNode", "Target Node");
	ar(m_desc.nodes, "nodes", "Driven Nodes");
	ar(m_desc.weightNode, "weightNode", "Weight Node");
	ar(Range(m_desc.weight, 0.0f, 1.0f), "weight", "Weight");

	if (ar.isInput())
	{
		m_desc.weight = clamp_tpl(m_desc.weight, 0.0f, 1.0f);
	}

	if (ar.isEdit())
	{
		if (m_desc.sourceNode.name.empty())
		{
			ar.warning(m_desc.sourceNode.name, "Source Node is not specified.");
		}
		if (m_desc.targetNode.name.empty())
		{
			ar.warning(m_desc.targetNode.name, "Target Node is not specified.");
		}
		if (!m_desc.nodes.size())
		{
			ar.warning(m_desc.nodes, "Node list is empty.");
		}
	}

	if (ar.isInput())
	{
		m_bInitialized = false;
	}
}

void CApplyOffsetRotation::SNodeWeightLimitsDesc::Serialize(Serialization::IArchive& ar)
{
	using Serialization::Range;

	ar(enabled, "enabled", "^");
	ar(node, "node", "^Node");
	ar(Serialization::RadiansAsDeg(minLimit), "minLimit", "Min Limit");
	ar(Serialization::RadiansAsDeg(maxLimit), "maxLimit", "Max Limit");
	ar(multiplier, "multiplier", "Multiplier");
	ar(Range(weight, 0.0f, 1.0f), "weight", "Weight");

	if (ar.isEdit())
	{
		if (node.name.empty())
		{
			ar.warning(node.name, "Node is not specified.");
		}
	}
}

void CApplyOffsetRotation::Draw(const QuatT& location, const QuatT& sourceAbsolute, const QuatT& targetAbsolute, const float weight)
{
	if (gEnv->pRenderer == nullptr)
	{
		return;
	}

	IRenderAuxGeom* pAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
	if (!pAuxGeom)
		return;

	SAuxGeomRenderFlags renderFlags(e_Def3DPublicRenderflags | e_DepthTestOff | e_DrawInFrontOn);
	pAuxGeom->SetRenderFlags(renderFlags);

	OBB obb;
	obb.m33 = Matrix33(IDENTITY);
	obb.c = Vec3(0);
	obb.h = Vec3(0.01f);

	pAuxGeom->DrawOBB(obb, Matrix34(location * sourceAbsolute), false, ColorB(0x80, 0x80, 0xff, 0x85), eBBD_Faceted);
	pAuxGeom->DrawOBB(obb, Matrix34(location * targetAbsolute), false, ColorB(0xff, 0x80, 0x80, 0x85), eBBD_Faceted);
	pAuxGeom->Submit();
}

} // namespace PoseModifier
