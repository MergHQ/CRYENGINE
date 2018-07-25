// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySandbox/CrySignal.h>

#include <memory>

struct IAssetEditingSession;
class CAsset;

namespace CryParticleEditor 
{

class CEffectAsset;

class CEffectAssetModel
{
public:
	bool                                  OpenAsset(CAsset* pAsset);
	void                                  ClearAsset();
	CEffectAsset*                         GetEffectAsset();
	std::unique_ptr<IAssetEditingSession> CreateEditingSession();

public:
	CCrySignal<void()> signalBeginEffectAssetChange;
	CCrySignal<void()> signalEndEffectAssetChange;

private:
	std::unique_ptr<CEffectAsset> m_pEffectAsset;
};

}
