#pragma once

#include "AssetSystem/AssetConverter.h"
#include "SandboxAPI.h"

//! Converts from a SelectionGroup of CBaseObject to a prefab asset
class SANDBOX_API CObjectToPrefabAssetConverter : public CAssetConverter
{
	DECLARE_ASSET_CONVERTER_DESC(CObjectToPrefabAssetConverter)

public:

	virtual bool   CanConvert(const QMimeData& data) override;
	virtual void   Convert(const QMimeData& data, const SAssetConverterConversionInfo& info) override;
	virtual string ConversionInfo(const QMimeData& data) override;

private:
	virtual bool HasObjects(const QMimeData& data);
};
