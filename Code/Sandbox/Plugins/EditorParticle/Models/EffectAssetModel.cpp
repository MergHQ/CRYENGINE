// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EffectAssetModel.h"

#include <AssetSystem/Asset.h>

#include <CryParticleSystem/IParticlesPfx2.h>
#include <CrySerialization/IArchiveHost.h>

namespace CryParticleEditor 
{

class CSession final : public IAssetEditingSession
{
public:
	CSession(pfx2::IParticleEffect* pEffect)
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

		// Get effect dependency and collect details information.
		SetDetailsAndDependencies(asset);

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

	virtual bool OnCopyAsset(INewAsset& asset) override
	{
		if (!m_pEffect)
		{
			return false;
		}

		_smart_ptr<pfx2::IParticleEffect> pEffect = DuplicateEffect(PathUtil::RemoveExtension(asset.GetMetadataFile()), m_pEffect);

		CSession session(pEffect);
		return session.OnSaveAsset(asset);
	}

	static pfx2::IParticleEffect* GetEffectFromAsset(CAsset* pAsset)
	{
		IAssetEditingSession* const pSession = pAsset->GetEditingSession();
		if (!pSession || strcmp(pSession->GetEditorName(), "Particle Editor") != 0)
		{
			return nullptr;
		}
		return static_cast<CSession*>(pSession)->GetEffect();
	}

private:
	pfx2::IParticleEffect* GetEffect() 
	{
		return m_pEffect.get();
	}

	void SetDetailsAndDependencies(IEditableAsset &asset) const
	{
		const size_t componentsCount = m_pEffect->GetNumComponents();
		size_t totalFeaturesCount = 0;
		std::vector<SAssetDependencyInfo> dependencies;
		for (size_t component = 0; component < componentsCount; ++component)
		{
			const pfx2::IParticleComponent* pComponent = m_pEffect->GetComponent(component);
			const size_t featuresCount = pComponent->GetNumFeatures();
			totalFeaturesCount += featuresCount;
			for (size_t feature = 0; feature < featuresCount; ++feature)
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

		std::vector<std::pair<string, string>> details =
		{
			{ "componentsCount", string().Format("%d", componentsCount) },
			{ "featuresCount",   string().Format("%d", totalFeaturesCount) },
		};
		asset.SetDetails(details);
	}

	static _smart_ptr<pfx2::IParticleEffect> DuplicateEffect(const char* szNewEffectName, pfx2::IParticleEffect* pOriginal)
	{
		pfx2::IParticleSystem* const pParticleSystem = GetParticleSystem();
		_smart_ptr<pfx2::IParticleEffect> pEffect = pParticleSystem->CreateEffect();
		pParticleSystem->RenameEffect(pEffect, szNewEffectName);
		DynArray<char> buffer;
		Serialization::SaveJsonBuffer(buffer, *pOriginal);
		Serialization::LoadJsonBuffer(*pEffect, buffer.begin(), buffer.size());
		return pEffect;
	}

private:
	_smart_ptr<pfx2::IParticleEffect> m_pEffect;
};

bool CEffectAssetModel::OpenAsset(CAsset* pAsset)
{
	CRY_ASSERT(pAsset);

	if (!pAsset->GetFilesCount())
	{
		return false;
	}

	pfx2::PParticleEffect pEffect = CSession::GetEffectFromAsset(pAsset);
	if (!pEffect)
	{
		const char* const szPfxFilePath = pAsset->GetFile(0);
		pEffect = GetParticleSystem()->FindEffect(szPfxFilePath, true);
		if (!pEffect)
			return false;
	}

	signalBeginEffectAssetChange();
	m_pAsset = pAsset;
	SetEffect(pEffect);
	signalEndEffectAssetChange();

	m_pModel->signalChanged.Connect([this]()
	{
		SetModified(true);
	});

	return true;
}

void CEffectAssetModel::ClearAsset()
{
	signalBeginEffectAssetChange();
	m_pAsset = nullptr;
	SetEffect(nullptr);
	signalEndEffectAssetChange();
}

std::unique_ptr<IAssetEditingSession> CEffectAssetModel::CreateEditingSession()
{
	return std::make_unique<CSession>(m_pEffect);
}

const char* CEffectAssetModel::GetName() const
{
	return m_pEffect ? m_pEffect->GetName() : "";
}

void CEffectAssetModel::SetEffect(pfx2::IParticleEffect* pEffect)
{
	m_pEffect = pEffect;
	m_pModel = pEffect ? stl::make_unique<CParticleGraphModel>(*pEffect) : nullptr;
}

bool CEffectAssetModel::MakeNewComponent(const char* szTemplateName)
{
	if (m_pModel)
	{
		return m_pModel->CreateNode(szTemplateName, QPointF()) != nullptr;
	}
	return false;
}


}
