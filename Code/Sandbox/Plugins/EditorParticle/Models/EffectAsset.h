// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ParticleGraphModel.h"

class CAsset;

namespace CryParticleEditor 
{

class CEffectAsset
{
public:
	const char* GetName() const;
	pfx2::IParticleEffectPfx2* GetEffect();
	CryParticleEditor::CParticleGraphModel* GetModel();

	void SetEffect(pfx2::IParticleEffectPfx2* pEffect);

	bool MakeNewComponent(const char* szTemplateName);

private:
	std::unique_ptr<CryParticleEditor::CParticleGraphModel> m_pModel;
	_smart_ptr<pfx2::IParticleEffectPfx2>                   m_pEffect;
};

}
