// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "SceneElementCommon.h"

class CSceneElementSkin : public CSceneElementCommon
{
public:
	CSceneElementSkin(CScene* pScene, int id);
	virtual ~CSceneElementSkin() {}

	virtual ESceneElementType GetType() const override;
};