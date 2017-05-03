// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAnimation/IAnimEventPlayer.h>
#include <CryExtension/ClassWeaver.h>
#include <CryExtension/CryCreateClassInstance.h>

// Automatically found by the character tool due to implementing IAnimEventPlayerGame and usign CRYREGISTER_CLASS
class AnimEventPlayer_CRYENGINE_SDK : public IAnimEventPlayerGame
{
public:
	CRYINTERFACE_BEGIN()
		CRYINTERFACE_ADD(IAnimEventPlayer)
		CRYINTERFACE_ADD(IAnimEventPlayerGame)
	CRYINTERFACE_END()
	CRYGENERATE_CLASS(AnimEventPlayer_CRYENGINE_SDK, "AnimEventPlayer_CRYENGINE_SDK", 0x7cb241402ca9e311, 0x058e7ce6b77865e4)

	virtual ~AnimEventPlayer_CRYENGINE_SDK() {}

	const SCustomAnimEventType* GetCustomType(int customTypeIndex) const override;
	int GetCustomTypeCount() const override;
	bool Play(ICharacterInstance* character, const AnimEventInstance& event) override;
};
