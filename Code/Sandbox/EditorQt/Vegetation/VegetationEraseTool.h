// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "VegetationPaintTool.h"

class CVegetationEraseTool : public CVegetationPaintTool
{
	DECLARE_DYNCREATE(CVegetationEraseTool)
public:
	CVegetationEraseTool();

	virtual string                           GetDisplayName() const override { return "Erase Vegetation"; }
	static QEditToolButtonPanel::SButtonInfo CreateEraseToolButtonInfo();

protected:
	virtual ~CVegetationEraseTool() {}
	virtual void DeleteThis() override { delete this; }
};
