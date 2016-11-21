// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAnimation/ICryAnimation.h>
#include <CryExtension/ClassWeaver.h>
#include <CrySerialization/Forward.h>
#include <CryAnimation/IAnimationPoseModifier.h>

struct IAnimationPoseData;

class CPoseModifierStack :
	public IAnimationPoseModifier
{
public:
	CRYINTERFACE_BEGIN()
	CRYINTERFACE_ADD(IAnimationPoseModifier)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CPoseModifierStack, "AnimationPoseModifier_PoseModifierStack", 0xaf9efa2dfec04de4, 0xa1663950bde6a3c6)

public:
	void Clear() { m_modifiers.clear(); }
	bool Push(IAnimationPoseModifierPtr instance);

	// IAnimationPoseModifier
public:
	virtual bool Prepare(const SAnimationPoseModifierParams& params) override;
	virtual bool Execute(const SAnimationPoseModifierParams& params) override;
	virtual void Synchronize() override;

	void         GetMemoryUsage(ICrySizer* pSizer) const override {}

private:
	std::vector<IAnimationPoseModifierPtr> m_modifiers;
};

DECLARE_SHARED_POINTERS(CPoseModifierStack);

//

class CPoseModifierSetup :
	public IAnimationPoseModifierSetup
{
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimationPoseModifierSetup)
	CRYINTERFACE_END()

	CRYGENERATE_CLASS(CPoseModifierSetup, "PoseModifierSetup", 0x18b8cca76db947cc, 0x84dd1f003e97cbee)

private:
	struct Entry
	{
		Entry() : enabled(true) {}

		IAnimationPoseModifierPtr instance;
		bool                      enabled;

		void                      Serialize(Serialization::IArchive& ar);
	};

public:
	bool                  Create(CPoseModifierSetup& setup);
	CPoseModifierStackPtr GetPoseModifierStack() { return m_pPoseModifierStack; }

private:
	bool CreateStack();

	// IAnimationSerializable
public:
	virtual void Serialize(Serialization::IArchive& ar) override;

	// IAnimationPoseModifierSetup
public:
	virtual IAnimationPoseModifier* GetEntry(int index) override;
	virtual int GetEntryCount() override;

private:
	std::vector<Entry>    m_modifiers;
	CPoseModifierStackPtr m_pPoseModifierStack;
};

DECLARE_SHARED_POINTERS(CPoseModifierSetup);
