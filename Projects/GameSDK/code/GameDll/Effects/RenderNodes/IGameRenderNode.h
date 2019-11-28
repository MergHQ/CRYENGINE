// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef _IGAME_RENDERNODE_
#define _IGAME_RENDERNODE_

#pragma once

#include <CryPhysics/IPhysics.h> // Required by IRenderer
#include <CryRenderer/IRenderer.h> // required by IRenderNode and RenderElement.
#include "Effects/RenderElements/GameRenderElement.h"
#include "Effects/GameEffectsSystem.h"
#include <Cry3DEngine/IRenderNode.h>
#include <CryCore/smartptr.h>

// Forward declares
struct IGameRenderNodeParams;

//==================================================================================================
// Name: IGameRenderNode
// Desc: Base interface for all game render nodes
// Author: James Chilvers
//==================================================================================================
struct IGameRenderNode : public IRenderNode, public _i_reference_target_t
{
	virtual ~IGameRenderNode() {}

	virtual bool InitialiseGameRenderNode() = 0;
	virtual void ReleaseGameRenderNode() = 0;

	virtual void SetParams(const IGameRenderNodeParams* pParams = NULL) = 0;

};//------------------------------------------------------------------------------------------------

//==================================================================================================
// Name: IGameRenderNodeParams
// Desc: Game render node params
// Author: James Chilvers
//==================================================================================================
struct IGameRenderNodeParams
{
	virtual ~IGameRenderNodeParams() {}
};//------------------------------------------------------------------------------------------------

#endif // _IGAME_RENDERNODE_
