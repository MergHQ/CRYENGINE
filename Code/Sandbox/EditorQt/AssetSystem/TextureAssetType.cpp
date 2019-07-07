// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include <StdAfx.h>
#include "TextureAssetType.h"

#include <Util/Image.h>
#include <Util/ImageUtil.h>
#include <AssetSystem/AssetEditor.h>
#include <AssetSystem/AssetManager.h>
#include <PathUtils.h>
#include <CryCore/ToolsHelpers/ResourceCompilerHelper.h>
#include <QDir>

REGISTER_ASSET_TYPE(CTextureType)

namespace Private_TextureType
{

static bool CallRcWithUserDialog(const string& filename, const string& options = "")
{	
	const CResourceCompilerHelper::ERcExePath path = CResourceCompilerHelper::eRcExePath_editor;
	const auto result = CResourceCompilerHelper::CallResourceCompiler(
	  filename.c_str(),
	  options.c_str(),
	  nullptr,
	  false, // may show window?
	  path,
	  true,  // silent?
	  false); // no user dialog?
	if (result != CResourceCompilerHelper::eRcCallResult_success)
	{
		return false;
	}
	return true;
}

} // namespace Private_TextureType

 // Detail attributes.
CItemModelAttribute CTextureType::s_widthAttribute("Width", &Attributes::s_intAttributeType, CItemModelAttribute::StartHidden);
CItemModelAttribute CTextureType::s_heightAttribute("Height", &Attributes::s_intAttributeType, CItemModelAttribute::StartHidden);
CItemModelAttribute CTextureType::s_mipCountAttribute("Mip count", &Attributes::s_intAttributeType, CItemModelAttribute::StartHidden);
CItemModelAttribute CTextureType::s_parentAssetAttribute("Parent", &Attributes::s_stringAttributeType, CItemModelAttribute::StartHidden);

CryIcon CTextureType::GetIconInternal() const
{
	return CryIcon("icons:common/assets_texture.ico");
}

void CTextureType::GenerateThumbnail(const CAsset* pAsset) const
{
	CRY_ASSERT(0 < pAsset->GetFilesCount());
	if (1 > pAsset->GetFilesCount())
		return;

	const char* textureFileName = pAsset->GetFile(0);
	if (!textureFileName || !*textureFileName)
		return;

	CImageEx image;
	CImageUtil::LoadImage(textureFileName, image);
	if (!image.IsValid())
		return;

	CImageEx preview;
	int width = min(image.GetWidth(), ASSET_THUMBNAIL_SIZE);
	int height = min(image.GetHeight(), ASSET_THUMBNAIL_SIZE);

	preview.Allocate(width, height);
	preview.Fill(0);
	CImageUtil::ScaleToFit(image, preview);
	preview.SwapRedAndBlue();

	const string thumbnailFileName = PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), pAsset->GetThumbnailPath());

	QDir().mkpath(QtUtil::ToQString(PathUtil::GetPathWithoutFilename(thumbnailFileName)));
	QImage thumbnail = QImage((const uchar*)preview.GetData(), width, height, QImage::Format_ARGB32);
	thumbnail.save(QtUtil::ToQString(thumbnailFileName), "PNG");
}

QWidget* CTextureType::CreateBigInfoWidget(const CAsset* pAsset) const
{
	CRY_ASSERT(0 < pAsset->GetFilesCount());
	if (1 > pAsset->GetFilesCount())
		return nullptr;

	const char* textureFileName = pAsset->GetFile(0);
	if (!textureFileName || !*textureFileName)
		return nullptr;

	CImageEx image;
	CImageUtil::LoadImage(textureFileName, image);
	if (!image.IsValid())
		return nullptr;

	image.SwapRedAndBlue();

	QImage image2 = QImage((const uchar*)image.GetData(), image.GetWidth(), image.GetHeight(), QImage::Format_ARGB32);

	auto label = new QLabel();
	label->setPixmap(QPixmap::fromImage(image2));
	return label;
}

CAssetEditor* CTextureType::Edit(CAsset* pAsset) const
{
	// Editing a texture type presents a special case, as it does not return an asset editor.
	// Instead, we present the modal dialog of the RC.  In the future, this should be changed, and
	// a properly integrated editor should exist in the Sandbox.

	using namespace Private_TextureType;

	if (!pAsset->GetSourceFile() || !*pAsset->GetSourceFile())
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Unable to edit %s. No source file.", pAsset->GetName());
		return nullptr;
	}

	CRY_ASSERT(pAsset && pAsset->GetType() && pAsset->GetType()->GetTypeName() == "Texture");

	// A texture can have as a source different file formats than tif, so we check first if 
	// by any chance the source isn't editable asset itself
	CAsset* potentialAsset = CAssetManager::GetInstance()->FindAssetForFile(pAsset->GetSourceFile());
	if (potentialAsset && strcmp(potentialAsset->GetType()->GetTypeName(), "SubstanceInstance") == 0)
	{
		return CAssetEditor::OpenAssetForEdit("Substance Instance Editor", potentialAsset);
	}
	
	if (!GetISystem()->GetIPak()->IsFileExist(pAsset->GetSourceFile()))
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Unable to edit %s. The source file not found: %s.", pAsset->GetName(), pAsset->GetSourceFile());
		return nullptr;
	}

	// A texture asset references a data file of type DDS and a source file of type TIF.
	// We want to call the RC on the TIF, as it stores the user settings.

	const string absTifFilePath = PathUtil::Make(PathUtil::GetGameProjectAssetsPath(), pAsset->GetSourceFile());
	CallRcWithUserDialog(absTifFilePath);

	return nullptr;
}

void CTextureType::AppendContextMenuActions(const std::vector<CAsset*>& assets, CAbstractMenu* pMenu) const
{
	if (assets.size() != 1)
	{
		return;
	}

	const CAsset* const pAsset = assets.front();
	const string parentAssetFilename = pAsset->GetDetailValue("parent");
	if (parentAssetFilename.empty())
	{
		return;
	}

	const string parentAssetPath = PathUtil::Make(pAsset->GetFolder(), parentAssetFilename);
	const CAsset* const pParentAsset = GetIEditor()->GetAssetManager()->FindAssetForFile(parentAssetPath);
	if (!pParentAsset)
	{
		return;
	}

	const QString parentAssetName = QtUtil::ToQString(pParentAsset->GetName());

	CAbstractMenu* const pSubMenu = pMenu->CreateMenu(QString("%1 (%2)").arg(QObject::tr("Parent"), QtUtil::ToQString(pParentAsset->GetType()->GetUiTypeName())));

	QAction* pAction = pSubMenu->CreateAction(QString("%1 '%2'").arg(QObject::tr("Go to"), parentAssetName));
	QObject::connect(pAction, &QAction::triggered, [parentAssetPath]()
	{
		GetIEditor()->ExecuteCommand("asset.show_in_browser '%s'", parentAssetPath.c_str());
	});

	pAction = pSubMenu->CreateAction(QString("%1 '%2'").arg(QObject::tr("Open"), parentAssetName));
	QObject::connect(pAction, &QAction::triggered, [parentAssetPath]()
	{
		GetIEditor()->ExecuteCommand("asset.edit '%s'", parentAssetPath.c_str());
	});
}

std::vector<CItemModelAttribute*> CTextureType::GetDetails() const
{
	return
	{
		&s_widthAttribute,
		&s_heightAttribute,
		&s_mipCountAttribute,
		&s_parentAssetAttribute
	};
}

QVariant CTextureType::GetDetailValue(const CAsset * pAsset, const CItemModelAttribute * pDetail) const
{
	static const std::pair<CItemModelAttribute*, const char*> attributes[]
	{
		{ &s_widthAttribute, "width" },
		{ &s_heightAttribute , "height" },
		{ &s_mipCountAttribute, "mipCount" }
	};

	if (pDetail == &s_parentAssetAttribute)
	{
		return GetVariantFromDetail(PathUtil::GetFileName(pAsset->GetDetailValue("parent")), pDetail);
	}

	for( const auto& attr : attributes)
	{
		if (attr.first == pDetail)
		{
			return GetVariantFromDetail(pAsset->GetDetailValue(attr.second), pDetail);
		}
	}
	return QVariant();
}
