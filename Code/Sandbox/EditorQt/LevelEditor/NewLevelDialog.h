// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "Controls/EditorDialog.h"

#include "LevelAssetType.h"

#include <memory>

class CNewLevelDialog
	: public CEditorDialog
{
public:
	explicit CNewLevelDialog();
	~CNewLevelDialog();

	CLevelType::SLevelCreateParams GetResult() const;

private:
	struct Implementation;
	std::unique_ptr<Implementation> p;
};
