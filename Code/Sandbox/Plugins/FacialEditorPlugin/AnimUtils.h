// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct ICharacterInstance;

namespace AnimUtils
{
void StartAnimation(ICharacterInstance* pCharacter, const char* pAnimName);
void SetAnimationTime(ICharacterInstance* pCharacter, float fNormalizedTime);
void StopAnimations(ICharacterInstance* pCharacter);
}
