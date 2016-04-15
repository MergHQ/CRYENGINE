// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAnimation/IAnimationPoseModifier.h>
#include <CryExtension/ClassWeaver.h>

struct SFeetData
{
	QuatT  m_WorldEndEffector;
	uint32 m_IsEndEffector;
	void   GetMemoryUsage(ICrySizer* pSizer) const
	{
		pSizer->AddObject(this, sizeof(*this));
	}
};

class CFeetPoseStore :
	public IAnimationPoseModifier
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IAnimationPoseModifier)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CFeetPoseStore, "AnimationPoseModifier_FeetPoseStore", 0x4095cfb096b5494f, 0x864d3c007b71d31d)

	// IAnimationPoseModifier
public:
	virtual bool Prepare(const SAnimationPoseModifierParams& params) override { return true; }
	virtual bool Execute(const SAnimationPoseModifierParams& params) override;
	virtual void Synchronize() override                                       {}

	void         GetMemoryUsage(ICrySizer* pSizer) const override
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_pFeetData);
	}
public://private:
	SFeetData* m_pFeetData;
};

class CFeetPoseRestore :
	public IAnimationPoseModifier
{
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IAnimationPoseModifier)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CFeetPoseRestore, "AnimationPoseModifier_FeetPoseRestore", 0x90662f0ed05a4bf4, 0x8fb69924b5da2872)

	// IAnimationPoseModifier
public:
	virtual bool Prepare(const SAnimationPoseModifierParams& params) override { return true; }
	virtual bool Execute(const SAnimationPoseModifierParams& params) override;
	virtual void Synchronize() override                                       {}

	void         GetMemoryUsage(ICrySizer* pSizer) const override
	{
		pSizer->AddObject(this, sizeof(*this));
		pSizer->AddObject(m_pFeetData);
	}
public://private:
	SFeetData* m_pFeetData;
};

class CFeetLock
{
private:
	IAnimationPoseModifierPtr m_store;
	IAnimationPoseModifierPtr m_restore;

public:
	CFeetLock();

public:
	void                    Reset()   {}
	IAnimationPoseModifier* Store()   { return m_store.get(); }
	IAnimationPoseModifier* Restore() { return m_restore.get(); }

private:
	SFeetData m_FeetData[MAX_FEET_AMOUNT];
};
