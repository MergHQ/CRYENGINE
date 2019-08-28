// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../BaseTool.h"
#include "Core/UVIsland.h"

namespace Designer {
namespace UVMapping
{

class FlipTool : public BaseTool
{
public:
	FlipTool(EUVMappingTool tool) : BaseTool(tool) {}

protected:
	void Flip(const Vec2& normal);
};

class FlipHoriTool : public FlipTool
{
public:
	FlipHoriTool(EUVMappingTool tool) : FlipTool(tool) {}
	void Enter() override;
};

class FlipVertTool : public FlipTool
{
public:
	FlipVertTool(EUVMappingTool tool) : FlipTool(tool) {}
	void Enter() override;
};

}
}
