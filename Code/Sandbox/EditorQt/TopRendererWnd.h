// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "2DViewport.h"
#include <Util/Image.h>

class CTopRendererWnd : public C2DViewport
{
	DECLARE_DYNCREATE(CTopRendererWnd)
public:
	CTopRendererWnd();

	/** Get type of this viewport.
	 */
	virtual EViewportType GetType() const { return ET_ViewportMap; }
	virtual void          SetType(EViewportType type);

	virtual void          ResetContent();
	virtual void          UpdateContent(int flags);
	void                  InitHeightmapAlignment();

	//! Map viewport position to world space position.
	virtual Vec3 ViewToWorld(CPoint vp, bool* collideWithTerrain = 0, bool onlyTerrain = false, bool bSkipVegetation = false, bool bTestRenderMesh = false);

	void         SerializeDisplayOptions(Serialization::IArchive& ar);

	void         SetShowWater(bool bShow) { m_bShowWater = bShow; }
	bool         GetShowWater() const     { return m_bShowWater; }

protected:
	void ResetSurfaceTexture();
	void UpdateSurfaceTexture(int flags);

	// Draw everything.
	virtual void Draw(CObjectRenderHelper& objRenderHelper);

	void         DrawStaticObjects();

private:
	bool m_bContentsUpdated;

	//CImage* m_pSurfaceTexture;
	int   m_terrainTextureId;

	CSize m_textureSize;

	// Size of heightmap in meters.
	CSize    m_heightmapSize;

	CImageEx m_terrainTexture;

	CImageEx m_vegetationTexture;
	CPoint   m_vegetationTexturePos;
	CSize    m_vegetationTextureSize;
	int      m_vegetationTextureId;
	bool     m_bFirstTerrainUpdate;

public:
	// Display options.
	bool m_bShowHeightmap;
	bool m_bLastShowHeightmapState;
	bool m_bShowStatObjects;
	bool m_bShowWater;
};
