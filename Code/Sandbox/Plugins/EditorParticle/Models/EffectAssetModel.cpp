// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EffectAssetModel.h"
#include "EffectAsset.h"

#include <AssetSystem/Asset.h>

#include <CryParticleSystem/IParticlesPfx2.h>

namespace CryParticleEditor 
{

class CSession final : public IAssetEditingSession
{
public:
	CSession(pfx2::IParticleEffectPfx2* pEffect)
		:m_pEffect(pEffect)
	{
	}

	virtual const char* GetEditorName() const override
	{
		return "Particle Editor";
	}

	virtual bool OnSaveAsset(IEditableAsset& asset) override
	{
		if (!m_pEffect)
		{
			return false;
		}

		const string basePath = PathUtil::RemoveExtension(PathUtil::RemoveExtension(asset.GetMetadataFile()));
		const string pfxFilePath = PathUtil::ReplaceExtension(basePath, "pfx"); // Relative to asset directory.
		
		if (!GetISystem()->GetIPak()->MakeDir(asset.GetFolder(), true))
		{
			return false;
		}

		if (!Serialization::SaveJsonFile(pfxFilePath.c_str(), *m_pEffect))
		{
			return false;
		}

		// Get effect dependency
		{
			std::vector<SAssetDependencyInfo> dependencies;
			for (size_t component = 0, componentsCount = m_pEffect->GetNumComponents(); component < componentsCount; ++component)
			{
				const pfx2::IParticleComponent* pComponent = m_pEffect->GetComponent(component);
				for (size_t feature = 0, featuresCount = pComponent->GetNumFeatures(); feature < featuresCount; ++feature)
				{
					const pfx2::IParticleFeature* pFeature = pComponent->GetFeature(feature);
					for (size_t resource = 0, resourcesCount = pFeature->GetNumResources(); resource < resourcesCount; ++resource)
					{
						const char* szResourceFilename = pFeature->GetResourceName(resource);
						auto it = std::find_if(dependencies.begin(), dependencies.end(), [szResourceFilename](const auto& x)
						{
							return x.path.CompareNoCase(szResourceFilename);
						});

						if (it == dependencies.end())
						{
							dependencies.emplace_back(szResourceFilename, 1);
						}
						else // increment instance count
						{
							++(it->usageCount);
						}
					}
				}
			}
			asset.SetDependencies(dependencies);
		}

		asset.SetFiles({ pfxFilePath });
		return true;
	}


	virtual void DiscardChanges(IEditableAsset& asset) override
	{
		if (!m_pEffect)
		{
			return;
		}

		const string pfxFilePath = PathUtil::RemoveExtension(asset.GetMetadataFile());
		Serialization::LoadJsonFile(*m_pEffect, pfxFilePath.c_str());
	}

	static CEffectAsset* CreateEffectAsset(CAsset* pAsset)
	{
		pfx2::PParticleEffect pEffect = GetEffectFromAsset(pAsset);
		if (!pEffect)
		{
			const char* const szPfxFilePath = pAsset->GetFile(0);

			pEffect = GetParticleSystem()->FindEffect(szPfxFilePath);
			if (!pEffect)
			{
				pEffect = GetParticleSystem()->FindEffect(szPfxFilePath, true);
				if (!pEffect)
				{
					return nullptr;
				}
			}
			else
			{
				// Reload effect from file every time it is opened, since it might be that the effect has changed
				// in memory. Opening means reading the current state from disk.
				Serialization::LoadJsonFile(*pEffect, szPfxFilePath);
			}
		}

		CEffectAsset* const pEffectAsset = new CEffectAsset();
		pEffectAsset->SetEffect(pEffect);
		pEffectAsset->GetModel()->signalChanged.Connect([pAsset]()
		{
			pAsset->SetModified(true);
		});
		return pEffectAsset;
	}

private:
	static pfx2::IParticleEffectPfx2* GetEffectFromAsset(CAsset* pAsset)
	{
		IAssetEditingSession* const pSession = pAsset->GetEditingSession();
		if (!pSession || strcmp(pSession->GetEditorName(), "Particle Editor") != 0)
		{
			return nullptr;
		}
		return static_cast<CSession*>(pSession)->GetEffect();
	}

	pfx2::IParticleEffectPfx2* GetEffect() 
	{
		return m_pEffect.get();
	}

private:
	_smart_ptr<pfx2::IParticleEffectPfx2> m_pEffect;

};


bool CEffectAssetModel::OpenAsset(CAsset* pAsset)
{
	CRY_ASSERT(pAsset);

	if (!pAsset->GetFilesCount())
	{
		return false;
	}

	signalBeginEffectAssetChange();
	m_pEffectAsset.reset(CSession::CreateEffectAsset(pAsset));
	signalEndEffectAssetChange();
	return true;
}

void CEffectAssetModel::ClearAsset()
{
	signalBeginEffectAssetChange();
	m_pEffectAsset.reset();
	signalEndEffectAssetChange();
}

CEffectAsset* CEffectAssetModel::GetEffectAsset()
{
	return m_pEffectAsset.get();
}

std::unique_ptr<IAssetEditingSession> CEffectAssetModel::CreateEditingSession()
{
	return std::make_unique<CSession>(m_pEffectAsset->GetEffect());
}

}
