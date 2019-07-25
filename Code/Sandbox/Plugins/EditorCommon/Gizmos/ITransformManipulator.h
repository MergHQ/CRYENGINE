// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"
#include "LevelEditor/LevelEditorSharedState.h"
#include <CrySandbox/CrySignal.h>

struct SDragData
{
	SDragData(int flags, const Vec2i& viewportPos, const Vec3& accumulateDelta = { 0.0f, 0.0f, 0.0f }, const Vec3& frameDelta = { 0.0f, 0.0f, 0.0f })
		: flags(flags)
		, viewportPos(viewportPos) 
		, accumulateDelta(accumulateDelta)
		, frameDelta(frameDelta)
	{}

	int   flags;           // MK_* flags
	Vec2i viewportPos;
	Vec3  accumulateDelta; // Accumulated delta since the beginning of the dragging operation
	Vec3  frameDelta;      // Change since last move
};

struct EDITOR_COMMON_API ITransformManipulator
{
	virtual ~ITransformManipulator() {}
	virtual Matrix34 GetTransform() const = 0;
	virtual void     SetCustomTransform(bool on, const Matrix34& m) = 0;
	// invalidate. Manipulator will request transform from owner during next draw
	virtual void     Invalidate() = 0;

	CCrySignal<void (IDisplayViewport*, ITransformManipulator*, const Vec2i&, int)>     signalBeginDrag;
	CCrySignal<void (IDisplayViewport*, ITransformManipulator*, const SDragData& data)> signalDragging;
	CCrySignal<void (IDisplayViewport*, ITransformManipulator*)> signalEndDrag;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ITransform is an interface for classes that own a transform manipulator. Basically it includes callbacks so that transform manipulators
// can get the position and orientation of its components when it is tagged for an update
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct EDITOR_COMMON_API ITransformManipulatorOwner
{
	virtual ~ITransformManipulatorOwner() {}
	// called at update time if reference coordinate system is  local or parent. Return true if there's a valid matrix else
	// world orientation (identity matrix) is used instead
	virtual bool GetManipulatorMatrix(Matrix34& tm) { return false; }
	// called at update time when the transform manipulator is tagged as invalid
	virtual void GetManipulatorPosition(Vec3& position) = 0;
	// called during display and hit test of manipulator. This way owners can deactivate their manipulator under some circumstances
	virtual bool IsManipulatorVisible() { return true; }
};
