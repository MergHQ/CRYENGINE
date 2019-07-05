#include "StdAfx.h"
#include "ObjectToPrefabAssetConverter.h"
#include "LevelModel.h"
#include "Objects/SelectionGroup.h"
#include "Prefabs/PrefabManager.h"

#include <AssetSystem/AssetManager.h>
#include <AssetSystem/Browser/AssetBrowser.h>

REGISTER_ASSET_CONVERTER(CObjectToPrefabAssetConverter)

bool CObjectToPrefabAssetConverter::CanConvert(const QMimeData& data)
{
	return HasObjects(data);
}

void CObjectToPrefabAssetConverter::Convert(const QMimeData& data, const SAssetConverterConversionInfo& info)
{
	std::vector<CBaseObject*> objects;
	std::vector<CObjectLayer*> layers;
	LevelModelsUtil::ProcessDragDropData(&data, objects, layers);
	string folderPath = info.folderPath;

	if (objects.size() > 0)
	{
		/*
		   QueryNewAsset creates a fake asset and then loops over it until the asset creation op is completed.
		   Inside the loop it calls the app processEvents function to avoid freezing the whole engine.
		   This is generally fine, but it is a problem for drag & drop, since the operation keeps going until the dropEvent is completed (with either accept or ignore).
		   This means that the drag popup will be visible if you move the mouse all around the editor, which we absolutely don't want.
		   Currently the best solution is to just finish the drop function and defer asset creation to the following frame
		 */
		QTimer::singleShot(1, [objects, info]()
		{
			CSelectionGroup group;
			for (CBaseObject* object : objects)
			{
			  group.AddObject(object);
			}

			const CAssetType* const pAssetType = GetIEditor()->GetAssetManager()->FindAssetType("Prefab");
			if (!pAssetType)
			{
			  return;
			}

			//we need to select the actual folder where the conversion is going to take place
			info.pOwnerBrowser->SelectAsset(info.folderPath);

			SPrefabCreateParams createParam(&group);
			info.pOwnerBrowser->QueryNewAsset(*pAssetType, &createParam);
		});
	}
}

string CObjectToPrefabAssetConverter::ConversionInfo(const QMimeData& data)
{
	std::vector<CBaseObject*> objects;
	std::vector<CObjectLayer*> layers;
	LevelModelsUtil::ProcessDragDropData(&data, objects, layers);

	string conversionInfo = "";

	if (objects.size() > 0)
	{
		conversionInfo.Format("Creating a Prefab from %d Objects", objects.size());
	}

	return conversionInfo;
}

bool CObjectToPrefabAssetConverter::HasObjects(const QMimeData& data)
{
	std::vector<CBaseObject*> objects;
	std::vector<CObjectLayer*> layers;
	LevelModelsUtil::ProcessDragDropData(&data, objects, layers);

	if (objects.size() > 0)
	{
		return true;
	}

	return false;
}
