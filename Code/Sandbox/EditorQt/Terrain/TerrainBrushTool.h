// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "LevelEditor/Tools/EditTool.h"
#include <Util/Image.h>

struct CTerrainBrush
{
	CTerrainBrush();
	CTerrainBrush& operator=(const CTerrainBrush& rhs);  // Will not copy displacementMapData

	//! Outside [main] radius of brush.
	float    radiusOutside;
	//! Inside Radius of brush.
	float    radiusInside;
	//! Height where to paint.
	float    height;
	//! Height that will be raised/lowered
	float    riseHeight;
	//! How hard this brush.
	float    hardness;
	//! Is this brush have displacement map.
	bool     bDisplacement;
	//! Scale of applied displacement.
	float    displacementScale;
	//! True if objects should be repositioned on modified terrain.
	bool     bRepositionObjects;
	//! True if vegetation should be repositioned on modified terrain.
	bool     bRepositionVegetation;
	//! Displacement map name
	string   displacementMapName;
	//! Displacement map data
	CImageEx displacementMapData;
};

class CBrushTool : public CEditTool
{
	DECLARE_DYNAMIC(CBrushTool)
public:
	enum EPaintMode
	{
		ePaintMode_None = 0,
		ePaintMode_Ready,
		ePaintMode_InProgress,
	};

	explicit CBrushTool(const char* const szPersonalizationModuleName);
	virtual ~CBrushTool() override;

	virtual bool MouseCallback(CViewport* view, EMouseEvent event, CPoint& point, int flags) override;
	virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;
	virtual void DeleteThis() { delete this; }

protected:
	void           SetBrushRadius(float inner, float outer);
	void           SetRiseHeight(float height);
	void           SetHeight(float height);
	virtual bool   PickHeight(QPoint point)                                                                               { return false; }
	void           DrawTool(SDisplayContext& dc, bool innerCircle, bool line, bool terrainCircle);
	virtual void   PaintTerrain(CHeightmap* pHeightmap, int tx, int ty, int tsize, float fInsideRadius, float brushSpeed) {}

	bool           IsBrushShared()                                                                                        { return s_bSharedBrushMode; }
	void           SetShareBrushParams(bool share);
	CTerrainBrush& GetBrush();

private:
	void Paint();
	void ApplyObjectsReposition();

protected:
	static Vec3                   s_pointerPos;
	static Vec3                   s_pointerPosPrev;
	bool                          m_bPickHeight;
	std::map<CBaseObject*, float> m_objElevations;
	AABB                          m_vegBox = AABB::RESET;

private:
	static bool          s_bSharedBrushMode;
	static CTerrainBrush s_sharedBrush;

	CTerrainBrush        m_brush;
	CPoint               m_LMButtonDownPoint;
	EPaintMode           m_nPaintingMode;
	const char* const    m_szPersonalizationModuleName;
};

class CTerrainTool : public CBrushTool
{
	DECLARE_DYNAMIC(CTerrainTool)
public:
	explicit CTerrainTool(const char* const szPersonalizationModuleName) : CBrushTool(szPersonalizationModuleName) {}

	virtual void DeleteThis() { delete this; }
};

class CFlattenTool : public CTerrainTool
{
	DECLARE_DYNCREATE(CFlattenTool)
public:
	CFlattenTool();

	virtual string GetDisplayName() const override { return "Flatten Terrain"; }
	virtual void   Serialize(Serialization::IArchive& ar) override;
	virtual void   Display(SDisplayContext& dc)    { DrawTool(dc, true, true, true); }
	virtual void   PaintTerrain(CHeightmap* pHeightmap, int tx, int ty, int tsize, float fInsideRadius, float brushSpeed) override;
	virtual bool   PickHeight(QPoint point) override;
};

class CSmoothTool : public CTerrainTool
{
	DECLARE_DYNCREATE(CSmoothTool)
public:
	CSmoothTool();

	virtual bool   OnKeyUp(CViewport* view, uint32 key, uint32 nRepCnt, uint32 nFlags) override;
	virtual string GetDisplayName() const override { return "Smooth Terrain"; }
	virtual void   Serialize(Serialization::IArchive& ar) override;
	virtual void   Display(SDisplayContext& dc)    { DrawTool(dc, false, false, false); }
	virtual void   PaintTerrain(CHeightmap* pHeightmap, int tx, int ty, int tsize, float fInsideRadius, float brushSpeed) override;

	void           SetPrevToolClass(CRuntimeClass* pPrevToolClass) { m_pPrevToolClass = pPrevToolClass; }
	CRuntimeClass* GetPrevToolClass() const                        { return m_pPrevToolClass; }

private:
	CRuntimeClass* m_pPrevToolClass;
};

class CRiseLowerTool : public CTerrainTool, public IAutoEditorNotifyListener
{
	DECLARE_DYNCREATE(CRiseLowerTool)
public:
	CRiseLowerTool();

	virtual bool   MouseCallback(CViewport* view, EMouseEvent event, CPoint& point, int flags) override;
	virtual bool   OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags) override;
	virtual bool   OnKeyUp(CViewport* view, uint32 key, uint32 nRepCnt, uint32 nFlags) override;
	virtual string GetDisplayName() const override { return "Raise/Lower Terrain"; }
	virtual void   Serialize(Serialization::IArchive& ar) override;
	virtual void   Display(SDisplayContext& dc)    { DrawTool(dc, true, false, false); }
	virtual void   PaintTerrain(CHeightmap* pHeightmap, int tx, int ty, int tsize, float fInsideRadius, float brushSpeed) override;
	virtual void   OnEditorNotifyEvent(EEditorNotifyEvent event) override;

private:
	bool m_lowerTool;
};

class CHolesTool : public CTerrainTool
{
	DECLARE_DYNAMIC(CHolesTool)
public:
	explicit CHolesTool(const char* const szPersonalizationModuleName);

	virtual string GetDisplayName() const override { return "Terrain Holes"; }
	virtual void   Serialize(Serialization::IArchive& ar) override;
	virtual void   Display(SDisplayContext& dc) override;
	virtual bool   MouseCallback(CViewport* view, EMouseEvent event, CPoint& point, int flags) override;

protected:
	bool CalculateHoleArea(const Vec3& pointer, float radius, Vec2i& min, Vec2i& max);

	ColorF m_toolColor;
};

class CMakeHolesTool : public CHolesTool
{
	DECLARE_DYNCREATE(CMakeHolesTool)
public:
	CMakeHolesTool();

	virtual string GetDisplayName() const override { return "Make Terrain Holes"; }
	virtual void   PaintTerrain(CHeightmap* pHeightmap, int tx, int ty, int tsize, float fInsideRadius, float brushSpeed) override;
};

class CFillHolesTool : public CHolesTool
{
	DECLARE_DYNCREATE(CFillHolesTool)
public:
	CFillHolesTool();

	virtual string GetDisplayName() const override { return "Fill Terrain Holes"; }
	virtual void   PaintTerrain(CHeightmap* pHeightmap, int tx, int ty, int tsize, float fInsideRadius, float brushSpeed) override;
};
