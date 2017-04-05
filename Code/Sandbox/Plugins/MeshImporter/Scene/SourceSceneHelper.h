// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <memory>

namespace FbxTool
{
struct SNode;
class CScene;
};

class CScene;
class CSceneElementSourceNode;

CSceneElementSourceNode* CreateSceneFromSourceScene(CScene& scene, FbxTool::CScene& sourceScene);

CSceneElementSourceNode* FindSceneElementOfNode(CScene& scene, const FbxTool::SNode* pFbxNode);
