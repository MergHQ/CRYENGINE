// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "RenderViewport.h"
#include <CryRenderer/IRenderer.h>
#include <DragDrop.h>

class CObjectRenderHelper;
class QViewportHeader;

//! LevelEditorViewport is a specialization of RenderViewport for the Level editor
//! In the past RenderViewport was the level editor viewport and also the base class for all other viewports
//! Which caused anti-patterns and a ton of issues. This was started as an attempt to sanitize viewport code
//! By progressively moving all level editor specific code here. See RenderViewport.h for other refactoring notes.
class CLevelEditorViewport : public CRenderViewport
{
public:
	CLevelEditorViewport();
	~CLevelEditorViewport();

	bool  CreateRenderContext(CRY_HWND hWnd, IRenderer::EViewportType viewportType = IRenderer::eViewportType_Default) override;

	void  SetHeaderWidget(QViewportHeader* headerWidget) { m_headerWidget = headerWidget; }

	bool  DragEvent(EDragEvent eventId, QEvent* event, int flags) override;
	void  PopulateMenu(CPopupMenuItem& menu);

	void  SetFOV(float fov);
	void  SetFOVDeg(float fov);
	float GetFOV() const;

	void  SetSelectedCamera();
	bool  IsSelectedCamera() const;

	//! This switches the active camera to the next one in the list of (default, all custom cams).
	void CycleCamera();

	void CenterOnSelection() override;
	void CenterOnAABB(AABB* const aabb) override;

	void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

	Vec3 ViewToWorld(POINT vp, bool* collideWithTerrain = 0, bool onlyTerrain = false, bool bSkipVegetation = false, bool bTestRenderMesh = false) const override;
	Vec3 ViewToWorldNormal(POINT vp, bool onlyTerrain, bool bTestRenderMesh = false) override;
	bool MouseCallback(EMouseEvent event, CPoint& point, int flags) override;
private:

	void OnCameraSpeedChanged() override;
	void OnMenuCreateCameraFromCurrentView();
	void OnMenuSelectCurrentCamera();
	void OnRender(SDisplayContext& context) override;
	void RenderAll(CObjectRenderHelper& displayInfo);
	void RenderSnappingGrid(SDisplayContext& context);
	void AddCameraMenuItems(CPopupMenuItem& menu);
	//Get the drag event and use type to either create a new object or apply an asset
	bool HandleDragEvent(EDragEvent eventId, QEvent* event, int flags);
	//This CDragDropData contains an asset that can be used in the viewport
	bool DropHasAsset(const CDragDropData& dragDropData);
	//If possible applies the asset in dragDropData to an object
	bool ApplyAsset(const CDragDropData& dragDropData, QDropEvent* pDropEvent, EDragEvent eventId);

	float            m_camFOV;
	QViewportHeader* m_headerWidget;
};
