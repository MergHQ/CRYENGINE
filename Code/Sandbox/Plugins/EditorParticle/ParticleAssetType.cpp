// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include <StdAfx.h>
#include "ParticleAssetType.h"

#include <AssetSystem/AssetEditor.h>
#include <AssetSystem/EditableAsset.h>
#include <AssetSystem/Loader/AssetLoaderHelpers.h>
#include <AssetSystem/MaterialType.h>
#include <FileDialogs/EngineFileDialog.h>
#include <PathUtils.h>

#include <CryParticleSystem/IParticlesPfx2.h>

#include <CrySerialization/IArchive.h>
#include <CrySerialization/IArchiveHost.h>

#include <CryCore/ToolsHelpers/ResourceCompilerHelper.h>

// #TODO Move this somewhere else.
#include <CryCore/ToolsHelpers/ResourceCompilerHelper.inl>
#include <CryCore/ToolsHelpers/SettingsManagerHelpers.inl>
#include <CryCore/ToolsHelpers/EngineSettingsManager.inl>

REGISTER_ASSET_TYPE(CParticlesType)

// Detail attributes.
CItemModelAttribute CParticlesType::s_componentsCountAttribute("Components count", &Attributes::s_intAttributeType, CItemModelAttribute::StartHidden);
CItemModelAttribute CParticlesType::s_featuresCountAttribute("Features count", &Attributes::s_intAttributeType, CItemModelAttribute::StartHidden);

namespace Private_ParticleAssetType
{

static string ShowSaveDialog()
{
	CEngineFileDialog::RunParams runParams;
	runParams.title = QStringLiteral("Save Particle Effects");
	runParams.extensionFilters << CExtensionFilter(QStringLiteral("Particle Effects (pfx)"), "pfx");

	const QString filePath = CEngineFileDialog::RunGameSave(runParams, nullptr);
	return QtUtil::ToString(filePath);
}

static bool MakeNewComponent(pfx2::IParticleEffect* pEffect)
{
	pfx2::IParticleComponent* const pComp = pEffect->AddComponent();
	if (!pComp)
	{
		return false;
	}

	const string templateName = "%ENGINE%/EngineAssets/Particles/Default.pfxp";

	return Serialization::LoadJsonFile(*pComp, templateName);
}

static string CreateAssetMetaData(const string& pfxFilePath)
{
	const string options = "/refresh /overwriteextension=cryasset";
	const CResourceCompilerHelper::ERcExePath path = CResourceCompilerHelper::eRcExePath_editor;
	const auto result = CResourceCompilerHelper::CallResourceCompiler(
	  pfxFilePath.c_str(),
	  options.c_str(),
	  nullptr,
	  false, // may show window?
	  path,
	  true,  // silent?
	  true); // no user dialog?
	if (result != CResourceCompilerHelper::eRcCallResult_success)
	{
		return string();
	}
	return pfxFilePath + ".cryasset";
}

} // namespace Private_ParticleAssetType

CryIcon CParticlesType::GetIconInternal() const
{
	return CryIcon("icons:common/assets_particles.ico");
}

std::vector<CItemModelAttribute*> CParticlesType::GetDetails() const
{
	return
	{
		&s_componentsCountAttribute,
		&s_featuresCountAttribute
	};
}

QVariant CParticlesType::GetDetailValue(const CAsset* pAsset, const CItemModelAttribute* pDetail) const
{
	if (pDetail == &s_componentsCountAttribute)
	{
		return GetVariantFromDetail(pAsset->GetDetailValue("componentsCount"), pDetail);
	}
	else if (pDetail == &s_featuresCountAttribute)
	{
		return GetVariantFromDetail(pAsset->GetDetailValue("featuresCount"), pDetail);
	}
	return QVariant();
}

CAssetEditor* CParticlesType::Edit(CAsset* asset) const
{
	return CAssetEditor::OpenAssetForEdit("Particle Editor", asset);
}

bool CParticlesType::CreateForExistingEffect(const char* szFilePath) const
{
	SParticlesCreateParams params;
	params.bUseExistingEffect = true;
	return Create(szFilePath, &params);
}

bool CParticlesType::OnCreate(INewAsset& asset, const SCreateParams* pCreateParams) const
{
	using namespace Private_ParticleAssetType;

	const string pfxFilePath = PathUtil::RemoveExtension(asset.GetMetadataFile());
	CRY_ASSERT(stricmp(PathUtil::GetExt(pfxFilePath.c_str()), GetFileExtension()) == 0);

	std::shared_ptr<pfx2::IParticleSystem> pParticleSystem = pfx2::GetIParticleSystem();
	if (!pParticleSystem)
	{
		return false;
	}

	const bool bCreatePfxFile = !(pCreateParams && ((SParticlesCreateParams*)pCreateParams)->bUseExistingEffect);

	if (bCreatePfxFile)
	{
		pfx2::PParticleEffect pEffect = pParticleSystem->CreateEffect();
		pParticleSystem->RenameEffect(pEffect, pfxFilePath.c_str());
		MakeNewComponent(pEffect.get());
		pEffect->Update();

		if (!Serialization::SaveJsonFile(pfxFilePath.c_str(), *pEffect))
		{
			return false;
		}
	}

	asset.AddFile(pfxFilePath.c_str());

	return true;
}
