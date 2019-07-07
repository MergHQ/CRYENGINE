// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "AssetSystem/AssetType.h"

class CGeometryCacheType : public CAssetType
{
public:
	DECLARE_ASSET_TYPE_DESC(CGeometryCacheType)

	virtual const char* GetTypeName() const override { return GetTypeNameStatic(); }
	virtual const char*   GetUiTypeName() const override     { return QT_TR_NOOP("Geometry Cache"); }
	virtual QColor        GetThumbnailColor() const override { return QColor(210, 75, 64); }
	virtual const char*   GetFileExtension() const override  { return "cax"; }
	virtual bool          IsImported() const override        { return true; }
	virtual bool          CanBeCopied() const                { return true; }
	virtual bool          CanBeEdited() const override       { return true; }
	virtual CAssetEditor* Edit(CAsset* pAsset) const override;

	static const char*    GetTypeNameStatic() { return "GeometryCache"; }

private:
	virtual CryIcon GetIconInternal() const override;
};
