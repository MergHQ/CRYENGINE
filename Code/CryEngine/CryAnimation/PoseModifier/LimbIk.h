// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAnimation/IAnimationPoseModifier.h>
#include <CryExtension/ClassWeaver.h>

class CLimbIk :
	public IAnimationPoseModifier
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IAnimationPoseModifier)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CLimbIk, "AnimationPoseModifier_LimbIk", 0x3b00bbad5b9c4fa4, 0x97e9b720fcbc8839)

private:
	struct Setup
	{
		LimbIKDefinitionHandle setup;
		Vec3                   targetPositionLocal;
	};

private:
	Setup  m_setupsBuffer[2][16];

	Setup* m_pSetups;
	uint32 m_setupCount;

	Setup* m_pSetupsExecute;
	uint32 m_setupCountExecute;

public:
	void AddSetup(LimbIKDefinitionHandle setup, const Vec3& targetPositionLocal);

	// IAnimationPoseModifier
public:
	virtual bool Prepare(const SAnimationPoseModifierParams& params) override;
	virtual bool Execute(const SAnimationPoseModifierParams& params) override;
	virtual void Synchronize() override;

	void         GetMemoryUsage(ICrySizer* pSizer) const override
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};
