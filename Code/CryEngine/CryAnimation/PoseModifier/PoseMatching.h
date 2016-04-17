// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAnimation/IAnimationPoseModifier.h>
#include <CryExtension/ClassWeaver.h>

class CPoseMatching :
	public IAnimationPoseMatching
{
public:
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IAnimationPoseModifier)
	CRYINTERFACE_ADD(IAnimationPoseMatching)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CPoseMatching, "AnimationPoseModifier_PoseMatching", 0x18318a272246464e, 0xa4b7adffa51a9508)

	// IAnimationPoseMatching
public:
	virtual void SetAnimations(const uint* pAnimationIds, uint count) override;
	virtual bool GetMatchingAnimation(uint& animationId) const override;

	// IAnimationPoseModifier
public:
	virtual bool Prepare(const SAnimationPoseModifierParams& params) override;
	virtual bool Execute(const SAnimationPoseModifierParams& params) override;
	virtual void Synchronize() override;

	void         GetMemoryUsage(ICrySizer* pSizer) const override
	{
		pSizer->AddObject(this, sizeof(*this));
	}

private:
	uint        m_jointStartIndex;
	const uint* m_pAnimationIds;
	uint        m_animationCount;

	int         m_animationMatchId;
};
