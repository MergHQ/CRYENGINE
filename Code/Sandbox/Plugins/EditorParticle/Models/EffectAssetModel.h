// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ParticleGraphModel.h"
#include <CrySandbox/CrySignal.h>
#include <memory>

struct IAssetEditingSession;
class CAsset;

namespace CryParticleEditor 
{

class CEffectAssetModel
{
public:
	bool                                    OpenAsset(CAsset* pAsset);
	void                                    ClearAsset();
	std::unique_ptr<IAssetEditingSession>   CreateEditingSession();

	const char*                             GetName() const;
	pfx2::IParticleEffect*                  GetEffect() { return m_pEffect; }
	CryParticleEditor::CParticleGraphModel* GetModel() { return m_pModel.get(); }

	void SetEffect(pfx2::IParticleEffect* pEffect);

	bool MakeNewComponent(const char* szTemplateName);

	void SetModified(bool bModified) { if (m_pAsset) m_pAsset->SetModified(bModified); }
		
public:
	CCrySignal<void()> signalBeginEffectAssetChange;
	CCrySignal<void()> signalEndEffectAssetChange;
	CCrySignal<void(int, int)> signalEffectEdited;

private:
	CAsset*                                                 m_pAsset;
	std::unique_ptr<CryParticleEditor::CParticleGraphModel> m_pModel;
	_smart_ptr<pfx2::IParticleEffect>                       m_pEffect;
};

}
