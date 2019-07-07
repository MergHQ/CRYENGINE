// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Terrain/TerrainBrushTool.h"

#include "IEditorImpl.h"
#include "Terrain/Heightmap.h"
#include "Vegetation/VegetationMap.h"

#include <IObjectManager.h>
#include <IUndoManager.h>
#include <EditorFramework/PersonalizationManager.h>
#include <EditorFramework/Preferences.h>
#include <Objects/BaseObject.h>
#include <Objects/DisplayContext.h>
#include <Serialization/Decorators/EditorActionButton.h>
#include <QtUtil.h>
#include <Viewport.h>
#include <Util/ImageUtil.h>

#include <CrySerialization/Decorators/ActionButton.h>
#include <CrySerialization/Decorators/Resources.h>
#include <QBoxLayout>

#define MAX_BRUSH_SIZE     500.0f
#define MAX_BRUSH_HARDNESS 1.0f
#define MAX_TERRAIN_HEIGHT 1000.0f
#define MAX_RISE_HEIGHT    50.0f

Vec3 CBrushTool::s_pointerPos;
Vec3 CBrushTool::s_pointerPosPrev;
bool CBrushTool::s_bSharedBrushMode;
CTerrainBrush CBrushTool::s_sharedBrush;

namespace
{
const char* const g_szFlatternToolPersonalizationModule = "FlatternTool";
const char* const g_szSmoothToolPersonalizationModule = "SmoothTool";
const char* const g_szRiseLowerToolPersonalizationModule = "RiseLowerTool";
const char* const g_szMakeHolesToolPersonalizationModule = "MakeHolesTool";
const char* const g_szFillHolesToolPersonalizationModule = "FillHolesTool";

} // unnamed namespace

struct SBrushPreferences : public SPreferencePage
{
	SBrushPreferences()
		: SPreferencePage("Brush", "Painting/General")
		, m_hardnessSensistivity(0.0001f)
		, m_radiusSensitivity(0.01f)
	{}

	virtual bool Serialize(yasli::Archive& ar) override
	{
		ar(yasli::Range(m_hardnessSensistivity, -4.0f, 4.0f), "hardnessSensistivity", "Hardness Sensitivity");
		ar(yasli::Range(m_radiusSensitivity, -4.0f, 4.0f), "radiusSensitivity", "Radius Sensitivity");

		return true;
	}

	ADD_PREFERENCE_PAGE_PROPERTY(float, hardnessSensistivity, setHardnessSensistivity);
	ADD_PREFERENCE_PAGE_PROPERTY(float, radiusSensitivity, setRadiusSensitivity);
};

class CBrushPreferencesDialog : public CDockableWidget
{
public:
	CBrushPreferencesDialog(QWidget* parent = nullptr)
	{
		QVBoxLayout* pLayout = new QVBoxLayout();
		pLayout->setMargin(0);
		pLayout->setSpacing(0);

		pLayout->addWidget(GetIEditor()->GetPreferences()->GetPageWidget("Painting/General"));
		setLayout(pLayout);
	}

	virtual IViewPaneClass::EDockingDirection GetDockingDirection() const override { return IViewPaneClass::DOCK_FLOAT; }
	virtual const char*                       GetPaneTitle() const override        { return "Brush Preferences"; }
	virtual QRect                             GetPaneRect() override               { return QRect(0, 0, 270, 200); }
};

REGISTER_HIDDEN_VIEWPANE_FACTORY(CBrushPreferencesDialog, "Brush Settings", "Preferences", true)

SBrushPreferences gBrushPreferences;
REGISTER_PREFERENCES_PAGE_PTR(SBrushPreferences, &gBrushPreferences)

//////////////////////////////////////////////////////////////////////////
CTerrainBrush::CTerrainBrush()
	: radiusOutside(2.0f)
	, radiusInside(0.0f) // default 0 is preferred by designers
	, height(1.0f)
	, riseHeight(1.0f)
	, bDisplacement(false)
	, hardness(0.2f)
	, displacementScale(50.0f)
	, bRepositionObjects(false)
	, bRepositionVegetation(true)
{}

CTerrainBrush& CTerrainBrush::operator=(const CTerrainBrush& rhs)
{
	radiusOutside = rhs.radiusOutside;
	radiusInside = rhs.radiusInside;
	height = rhs.height;
	riseHeight = rhs.riseHeight;
	hardness = rhs.hardness;
	bDisplacement = rhs.bDisplacement;
	displacementScale = rhs.displacementScale;
	bRepositionObjects = rhs.bRepositionObjects;
	bRepositionVegetation = rhs.bRepositionVegetation;
	displacementMapName = rhs.displacementMapName;
	//Displacement image will not be copied

	return *this;
}

//////////////////////////////////////////////////////////////////////////
CBrushTool::CBrushTool(const char* const szPersonalizationModuleName)
	: m_szPersonalizationModuleName(szPersonalizationModuleName)
	, m_nPaintingMode(ePaintMode_None)
	, m_bPickHeight(false)
	, m_LMButtonDownPoint(0, 0)
{
	CRY_ASSERT(m_szPersonalizationModuleName && *m_szPersonalizationModuleName);

	CPersonalizationManager* pManager = GetIEditorImpl()->GetPersonalizationManager();
	if (!pManager)
		return;

	pManager->GetProperty(m_szPersonalizationModuleName, "radiusOutside", m_brush.radiusOutside);
	pManager->GetProperty(m_szPersonalizationModuleName, "radiusInside", m_brush.radiusInside);
	pManager->GetProperty(m_szPersonalizationModuleName, "height", m_brush.height);
	pManager->GetProperty(m_szPersonalizationModuleName, "riseHeight", m_brush.riseHeight);
	pManager->GetProperty(m_szPersonalizationModuleName, "hardness", m_brush.hardness);
	pManager->GetProperty(m_szPersonalizationModuleName, "bDisplacement", m_brush.bDisplacement);
	pManager->GetProperty(m_szPersonalizationModuleName, "displacementScale", m_brush.displacementScale);
	QString displacementMapName;
	pManager->GetProperty(m_szPersonalizationModuleName, "displacementMapName", displacementMapName);
	m_brush.displacementMapName = QtUtil::ToString(displacementMapName);
	pManager->GetProperty(m_szPersonalizationModuleName, "bRepositionObjects", m_brush.bRepositionObjects);
	pManager->GetProperty(m_szPersonalizationModuleName, "bRepositionVegetation", m_brush.bRepositionVegetation);
}

CBrushTool::~CBrushTool()
{
	CPersonalizationManager* pManager = GetIEditorImpl()->GetPersonalizationManager();

	if (!pManager)
		return;

	CTerrainBrush& brush = GetBrush();

	pManager->SetProperty(m_szPersonalizationModuleName, "radiusOutside", brush.radiusOutside);
	pManager->SetProperty(m_szPersonalizationModuleName, "radiusInside", brush.radiusInside);
	pManager->SetProperty(m_szPersonalizationModuleName, "height", brush.height);
	pManager->SetProperty(m_szPersonalizationModuleName, "riseHeight", brush.riseHeight);
	pManager->SetProperty(m_szPersonalizationModuleName, "hardness", brush.hardness);
	pManager->SetProperty(m_szPersonalizationModuleName, "bDisplacement", brush.bDisplacement);
	pManager->SetProperty(m_szPersonalizationModuleName, "displacementScale", brush.displacementScale);
	pManager->SetProperty(m_szPersonalizationModuleName, "displacementMapName", brush.displacementMapName.c_str());
	pManager->SetProperty(m_szPersonalizationModuleName, "bRepositionObjects", brush.bRepositionObjects);
	pManager->SetProperty(m_szPersonalizationModuleName, "bRepositionVegetation", brush.bRepositionVegetation);
}

void CBrushTool::SetBrushRadius(float inRadius, float outRadius)
{
	inRadius = clamp_tpl(inRadius, 0.0f, MAX_BRUSH_SIZE);
	outRadius = clamp_tpl(outRadius, 0.0f, MAX_BRUSH_SIZE);

	if (GetBrush().radiusOutside != outRadius)
	{
		GetBrush().radiusInside = std::min(GetBrush().radiusInside, outRadius);
		GetBrush().radiusOutside = outRadius;
		signalPropertiesChanged(this);
	}
	else if (GetBrush().radiusInside != inRadius)
	{
		GetBrush().radiusOutside = std::max(inRadius, GetBrush().radiusOutside);
		GetBrush().radiusInside = inRadius;
		signalPropertiesChanged(this);
	}
}

void CBrushTool::SetRiseHeight(float height)
{
	GetBrush().riseHeight = clamp_tpl(height, -MAX_RISE_HEIGHT, MAX_RISE_HEIGHT);
	signalPropertiesChanged(this);
}

void CBrushTool::SetHeight(float height)
{
	GetBrush().height = clamp_tpl(height, -MAX_TERRAIN_HEIGHT, MAX_TERRAIN_HEIGHT);
	signalPropertiesChanged(this);
}

void CBrushTool::SetShareBrushParams(bool share)
{
	//Exchange brush params between shared per-tool brushes
	if (share == true)
	{
		s_sharedBrush = m_brush;
	}
	else
	{
		m_brush = s_sharedBrush;
	}

	s_bSharedBrushMode = share;
	signalPropertiesChanged(this);
}

CTerrainBrush& CBrushTool::GetBrush()
{
	return s_bSharedBrushMode ? s_sharedBrush : m_brush;
}

bool CBrushTool::MouseCallback(CViewport* view, EMouseEvent event, CPoint& point, int flags)
{
	// If it's a mouse wheel event and Alt is not pressed, then early out
	if (event == eMouseWheel && MK_ALT & ~flags)
		return false;

	if (event == eMouseLDown)
		m_LMButtonDownPoint = point;

	if (m_nPaintingMode == ePaintMode_InProgress)
		m_nPaintingMode = ePaintMode_Ready;

	bool bCollideWithTerrain = true;
	Vec3 pointerPos = view->ViewToWorld(point, &bCollideWithTerrain, true);

	// Initialize it to the first position so we can avoid a huge delta on the first call
	static CPoint s_lastMousePoint = point;

	if (MK_ALT & flags)
	{
		static float oldHardness, oldRadius, oldRadiusInside;
		static bool modifyRadiusSensitivity = true;

		CTerrainBrush& brush = GetBrush();

		if (event == eMouseLDown)
		{
			oldHardness = brush.hardness;
			oldRadius = brush.radiusOutside;
			oldRadiusInside = brush.radiusInside;
		}
		else if (event == eMouseWheel)
		{
			float mouseDelta = static_cast<float>(point.y);

			// Most mouse types work in steps of 15 degrees, in which case the delta value is a multiple of 120.
			// i.e., 120 units * 1/8 = 15 degrees. http://doc.qt.io/qt-5/qwheelevent.html#angleDelta
			mouseDelta /= 120.0f;

			if (modifyRadiusSensitivity)
			{
				float currValue = gBrushPreferences.radiusSensitivity();
				float deltaValue = (fabs(currValue) * 1.1f - currValue) * static_cast<float>(mouseDelta);
				gBrushPreferences.setRadiusSensitivity(currValue + deltaValue);
			}
			else
			{
				float currValue = gBrushPreferences.hardnessSensistivity();
				float deltaValue = (fabs(currValue) * 1.1f - currValue) * static_cast<float>(mouseDelta);
				gBrushPreferences.setHardnessSensistivity(currValue + deltaValue);
			}
		}
		else if (event == eMouseMove && bCollideWithTerrain && (flags & MK_LBUTTON))
		{
			const CPoint delta = s_lastMousePoint - point;
			const CPoint magnitudeFromStart(abs(m_LMButtonDownPoint.x - point.x), abs(m_LMButtonDownPoint.y - point.y));

			if (magnitudeFromStart.x > magnitudeFromStart.y)
			{
				// When calculating the weight for changing radius, make sure to invert the axis as well
				const float finalAmount = static_cast<float>(delta.x) * -gBrushPreferences.radiusSensitivity();

				brush.radiusOutside = min(MAX_BRUSH_SIZE, max(0.0f, brush.radiusOutside + finalAmount));
				brush.radiusInside = min(brush.radiusInside, brush.radiusOutside);

				brush.hardness = oldHardness;
				// Set the flag so the user can modify radius sensitivity
				modifyRadiusSensitivity = true;
			}
			else
			{
				const float finalAmount = static_cast<float>(delta.y) * gBrushPreferences.hardnessSensistivity();

				brush.radiusOutside = oldRadius;
				brush.radiusInside = oldRadiusInside;
				brush.hardness = min(MAX_BRUSH_HARDNESS, max(0.0f, brush.hardness + finalAmount));
				// Set the flag so the user can modify hardness sensitivity
				modifyRadiusSensitivity = false;
			}
			signalPropertiesChanged(this);
		}
	}
	else
	{
		if (event == eMouseLUp)
			m_bPickHeight = false;
		else if ((event == eMouseLDown || (event == eMouseMove && (flags & MK_LBUTTON))) && (MK_CONTROL & flags))
			m_bPickHeight = true;

		s_pointerPos = pointerPos;
		if (m_bPickHeight)
			m_bPickHeight = PickHeight(QPoint(s_pointerPos.x, s_pointerPos.y));

		if (!m_bPickHeight && event == eMouseLDown && bCollideWithTerrain)
		{
			if (!GetIEditorImpl()->GetIUndoManager()->IsUndoRecording())
				GetIEditorImpl()->GetIUndoManager()->Begin();

			// clean the list of affected objects (it is needed only in case if previous eMouseLUp was not called)
			m_objElevations.clear();
			m_vegBox.Reset();

			// reset previous brush position
			s_pointerPosPrev = Vec3(-MAX_TERRAIN_HEIGHT);

			Paint();
			m_nPaintingMode = ePaintMode_InProgress;
		}

		if (m_nPaintingMode == ePaintMode_Ready && event == eMouseMove && (flags & MK_LBUTTON) && bCollideWithTerrain)
		{
			Paint();
			m_nPaintingMode = ePaintMode_InProgress;
		}

		if (m_nPaintingMode == ePaintMode_Ready || event == eMouseLUp)
		{
			ApplyObjectsReposition();

			if (GetIEditorImpl()->GetIUndoManager()->IsUndoRecording())
				GetIEditorImpl()->GetIUndoManager()->Accept("Terrain Modify");
			m_nPaintingMode = ePaintMode_None;

			gEnv->p3DEngine->GetITerrain()->OnTerrainPaintActionComplete();
		}
	}

	s_lastMousePoint = point;

	return true;
}

bool CBrushTool::OnKeyDown(CViewport* view, uint32 key, uint32 nRepCnt, uint32 nFlags)
{
	switch (key)
	{
	case Qt::Key_Escape:
		if (m_bPickHeight)
		{
			m_bPickHeight = false;
			return true;
		}
		break;

	case Qt::Key_Plus:
		SetBrushRadius(GetBrush().radiusInside, GetBrush().radiusOutside + 1.0f);
		return true;

	case Qt::Key_Minus:
		SetBrushRadius(GetBrush().radiusInside, GetBrush().radiusOutside - 1.0f);
		return true;

	case Qt::Key_Asterisk:
		SetHeight(GetBrush().height + 1.0f);
		return true;

	case Qt::Key_Slash:
		SetHeight(GetBrush().height - 1.0f);
		return true;

	case Qt::Key_Shift:
		{
			if (!IsKindOf(RUNTIME_CLASS(CSmoothTool)))
			{
				CSmoothTool* pSmoothTool = new CSmoothTool;
				pSmoothTool->SetPrevToolClass(GetRuntimeClass());
				GetIEditorImpl()->GetLevelEditorSharedState()->SetEditTool(pSmoothTool);
				return true;
			}
		}
		break;
	}
	return false;
}

void CBrushTool::DrawTool(SDisplayContext& dc, bool innerCircle, bool line, bool terrainCircle)
{
	if (dc.view)
	{
		CPoint cursor;
		GetCursorPos(&cursor);
		dc.view->ScreenToClient(&cursor);
		int width, height;
		dc.view->GetDimensions(&width, &height);
		if (cursor.x < 0 || cursor.y < 0 || cursor.x > width || cursor.y > height)
			return;
	}

	if (innerCircle)
	{
		dc.SetColor(0.5f, 1, 0.5f, 1);
		dc.DrawTerrainCircle(s_pointerPos, GetBrush().radiusInside, 0.2f);
	}

	dc.SetColor(0, 1, 0, 1);
	dc.DrawTerrainCircle(s_pointerPos, GetBrush().radiusOutside, 0.2f);

	if (s_pointerPos.z < GetBrush().height)
	{
		if (line)
		{
			Vec3 p = s_pointerPos;
			p.z = GetBrush().height;
			dc.SetColor(1, 1, 0, 1);
			if (terrainCircle)
				dc.DrawTerrainCircle(p, GetBrush().radiusInside, GetBrush().height - s_pointerPos.z);

			dc.DrawLine(s_pointerPos, p);
		}
	}
}

void CBrushTool::ApplyObjectsReposition()
{
	// Make sure objects preserve height.
	if (GetIEditorImpl()->GetVegetationMap() && !m_vegBox.IsReset())
	{
		GetIEditorImpl()->GetVegetationMap()->RepositionArea(m_vegBox);
	}

	for (auto it : m_objElevations)
	{
		Vec3 pos = it.first->GetPos();
		float zCoord = GetIEditorImpl()->GetTerrainElevation(pos.x, pos.y) + it.second;
		it.first->SetPos(Vec3(pos.x, pos.y, zCoord));
	}

	m_objElevations.clear();
	m_vegBox.Reset();
}

void CBrushTool::Paint()
{
	CRY_PROFILE_FUNCTION(PROFILE_EDITOR);

	if (!GetIEditorImpl()->Get3DEngine()->GetITerrain())
		return;

	CHeightmap* pHeightmap = GetIEditorImpl()->GetHeightmap();

	const CTerrainBrush& brush = GetBrush();
	float unitSize = pHeightmap->GetUnitSize();

	int tx = pos_directed_rounding(s_pointerPos.y / unitSize);
	int ty = pos_directed_rounding(s_pointerPos.x / unitSize);

	float fInsideRadius = (brush.radiusInside / unitSize);
	int tsize = (brush.radiusOutside / unitSize);
	if (tsize == 0)
		tsize = 1;
	int tsize2 = tsize * 2;
	int x1 = tx - tsize;
	int y1 = ty - tsize;

	AABB brushBox;
	brushBox.min = s_pointerPos - Vec3(brush.radiusOutside, brush.radiusInside, 0);
	brushBox.max = s_pointerPos + Vec3(brush.radiusOutside, brush.radiusInside, 0);
	brushBox.min.z -= 10000.0f;
	brushBox.max.z += 10000.0f;

	if (brush.bRepositionObjects)
	{
		CBaseObjectsArray areaObjects;
		GetIEditorImpl()->GetObjectManager()->GetObjectsRepeatFast(areaObjects, brushBox);

		// collect only new affected objects
		for (CBaseObject* obj : areaObjects)
		{
			AABB instanceBox;
			obj->GetBoundBox(instanceBox);
			if (!obj->GetGroup() && instanceBox.GetDistance(s_pointerPos) < brush.radiusOutside && m_objElevations.find(obj) == m_objElevations.end())
			{
				Vec3 pos = obj->GetPos();
				m_objElevations[obj] = pos.z - GetIEditorImpl()->GetTerrainElevation(pos.x, pos.y);
			}
		}
	}

	if (brush.bRepositionVegetation)
	{
		m_vegBox.Add(brushBox);
	}

	// calculate brush movement speed
	float brushSpeed = SATURATE((s_pointerPosPrev - s_pointerPos).GetLength2D() / brush.radiusOutside * 2.f);
	s_pointerPosPrev = s_pointerPos;

	PaintTerrain(pHeightmap, tx, ty, tsize, fInsideRadius, brushSpeed);

	pHeightmap->UpdateEngineTerrain(x1, y1, tsize2, tsize2, CHeightmap::ETerrainUpdateType::Elevation);

	GetIEditorImpl()->UpdateViews(eUpdateHeightmap, &brushBox);
}

IMPLEMENT_DYNAMIC(CBrushTool, CEditTool)
IMPLEMENT_DYNAMIC(CTerrainTool, CBrushTool)

//////////////////////////////////////////////////////////////////////////
CFlattenTool::CFlattenTool()
	: CTerrainTool(g_szFlatternToolPersonalizationModule)
{}

void CFlattenTool::Serialize(Serialization::IArchive& ar)
{
	bool brushShared = IsBrushShared();
	if (ar.openBlock("brush", "Brush"))
	{
		float outRadius = GetBrush().radiusOutside;
		float inRadius = GetBrush().radiusInside;

		ar(brushShared, "shareparams", "Share Parameters");
		ar(Serialization::Range(outRadius, 0.0f, MAX_BRUSH_SIZE), "outradius", "Radius (outside)");
		ar(Serialization::Range(inRadius, 0.0f, MAX_BRUSH_SIZE), "inradius", "Radius (inside)");
		ar(Serialization::Range(GetBrush().hardness, 0.0f, MAX_BRUSH_HARDNESS), "hardness", "Hardness");

		if (ar.openBlock("height", "Height Target"))
		{
			ar(Serialization::Range(GetBrush().height, -MAX_TERRAIN_HEIGHT, MAX_TERRAIN_HEIGHT), "height", "^");
			ar(Serialization::ActionButton([this]
			{
				m_bPickHeight = true;
			}, "icons:General/Picker.ico"), "pick", "^");
			ar.doc("Pick height by mouse click on the terrain.");
			ar.closeBlock();
		}

		if (ar.isInput())
			SetBrushRadius(inRadius, outRadius);

		ar.closeBlock();
	}

	if (ar.openBlock("Displacement", "Displacement"))
	{
		ar(GetBrush().bDisplacement, "bDisplacement", "Enable Displacement");
		ar(GetBrush().displacementScale, "displacementScale", "Displacement Scale");
		ar(Serialization::TextureFilename(GetBrush().displacementMapName), "displacementMapName", "Displacement Map");
		ar.closeBlock();

		// load displacement brush image
		GetBrush().displacementMapData.Release();
		CImageUtil::LoadImage(GetBrush().displacementMapName, GetBrush().displacementMapData);
	}

	if (ar.openBlock("reposition", "Reposition"))
	{
		ar(GetBrush().bRepositionObjects, "repoobjects", "Objects");
		ar(GetBrush().bRepositionVegetation, "repoveget", "Vegetation");
		ar.closeBlock();
	}

	if (brushShared != IsBrushShared())
		SetShareBrushParams(brushShared);
}

bool CFlattenTool::PickHeight(QPoint point)
{
	GetBrush().height = GetIEditorImpl()->GetTerrainElevation(point.x(), point.y());
	signalPropertiesChanged(this);
	return true;
}

void CFlattenTool::PaintTerrain(CHeightmap* pHeightmap, int tx, int ty, int tsize, float fInsideRadius, float brushSpeed)
{
	pHeightmap->DrawSpot(tx, ty, tsize, fInsideRadius, GetBrush().height, GetBrush().hardness * brushSpeed,
	                     (GetBrush().bDisplacement && GetBrush().displacementMapData.GetData()) ? &GetBrush().displacementMapData : nullptr, GetBrush().displacementScale);
}

IMPLEMENT_DYNCREATE(CFlattenTool, CTerrainTool)

//////////////////////////////////////////////////////////////////////////
CSmoothTool::CSmoothTool()
	: CTerrainTool(g_szSmoothToolPersonalizationModule)
	, m_pPrevToolClass(nullptr)
{}

void CSmoothTool::Serialize(Serialization::IArchive& ar)
{
	bool brushShared = IsBrushShared();
	if (ar.openBlock("brush", "Brush"))
	{
		ar(brushShared, "shareparams", "Share Parameters");
		ar(Serialization::Range(GetBrush().radiusOutside, 0.0f, MAX_BRUSH_SIZE), "outradius", "Radius (outside)");
		ar(Serialization::Range(GetBrush().hardness, 0.0f, 1.0f), "hardness", "Hardness");
		ar.closeBlock();
	}

	if (ar.openBlock("reposition", "Reposition"))
	{
		ar(GetBrush().bRepositionObjects, "repoobjects", "Objects");
		ar(GetBrush().bRepositionVegetation, "repoveget", "Vegetation");
		ar.closeBlock();
	}

	if (brushShared != IsBrushShared())
		SetShareBrushParams(brushShared);
}

void CSmoothTool::PaintTerrain(CHeightmap* pHeightmap, int tx, int ty, int tsize, float fInsideRadius, float brushSpeed)
{
	// Increase Hardness from shared range[0:1] between all tools to [0:2]
	const float smoothToolHardnessMultiplier = 2.f;
	pHeightmap->SmoothSpot(tx, ty, tsize, GetBrush().height, smoothToolHardnessMultiplier * GetBrush().hardness * brushSpeed);
}

bool CSmoothTool::OnKeyUp(CViewport* view, uint32 key, uint32 nRepCnt, uint32 nFlags)
{
	switch (key)
	{
	case Qt::Key_Shift:
		{
			CSmoothTool* pSmoothTool = (CSmoothTool*)this;
			CRuntimeClass* pPrevToolClass = pSmoothTool->GetPrevToolClass();
			if (pPrevToolClass)
			{
				CEditTool* pTool = (CEditTool*)pPrevToolClass->CreateObject();
				if (pTool)
				{
					GetIEditorImpl()->GetLevelEditorSharedState()->SetEditTool(pTool);
					return true;
				}
			}
		}
		break;
	}

	return __super::OnKeyUp(view, key, nRepCnt, nFlags);
}

IMPLEMENT_DYNCREATE(CSmoothTool, CTerrainTool)

//////////////////////////////////////////////////////////////////////////
CRiseLowerTool::CRiseLowerTool()
	: CTerrainTool(g_szRiseLowerToolPersonalizationModule)
	, m_lowerTool(false)
{}

bool CRiseLowerTool::OnKeyUp(CViewport* view, uint32 key, uint32 nRepCnt, uint32 nFlags)
{
	switch (key)
	{
	case Qt::Key_Control:
		if (m_lowerTool)
		{
			m_lowerTool = false;
			SetRiseHeight(-GetBrush().riseHeight);
		}
		return true;
	}

	return __super::OnKeyUp(view, key, nRepCnt, nFlags);
}

bool CRiseLowerTool::MouseCallback(CViewport* view, EMouseEvent event, CPoint& point, int flags)
{
	if ((flags & MK_CONTROL) && !m_lowerTool)
	{
		m_lowerTool = true;
		SetRiseHeight(-GetBrush().riseHeight);
	}
	else if (!(flags & MK_CONTROL) && m_lowerTool)
	{
		m_lowerTool = false;
		SetRiseHeight(-GetBrush().riseHeight);
	}

	return __super::MouseCallback(view, event, point, flags);
}

bool CRiseLowerTool::OnKeyDown(CViewport* view, uint32 key, uint32 nRepCnt, uint32 nFlags)
{
	switch (key)
	{
	case Qt::Key_Control:
		if (!m_lowerTool)
		{
			m_lowerTool = true;
			SetRiseHeight(-GetBrush().riseHeight);
		}
		return true;

	case Qt::Key_Asterisk:
		SetRiseHeight(GetBrush().riseHeight + 1.0f);
		return true;

	case Qt::Key_Slash:
		SetRiseHeight(GetBrush().riseHeight - 1.0f);
		return true;
	}

	return __super::OnKeyDown(view, key, nRepCnt, nFlags);
}

void CRiseLowerTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
	if (event == eNotify_OnBeginGameMode && m_lowerTool)
	{
		m_lowerTool = false;
		SetRiseHeight(-GetBrush().riseHeight);
	}
}

void CRiseLowerTool::Serialize(Serialization::IArchive& ar)
{
	bool brushShared = IsBrushShared();
	if (ar.openBlock("brush", "Brush"))
	{
		ar(brushShared, "shareparams", "Share Parameters");
		float outRadius = GetBrush().radiusOutside;
		float inRadius = GetBrush().radiusInside;
		ar(Serialization::Range(outRadius, 0.0f, MAX_BRUSH_SIZE), "outradius", "Radius (outside)");
		ar(Serialization::Range(inRadius, 0.0f, MAX_BRUSH_SIZE), "inradius", "Radius (inside)");
		ar(Serialization::Range(GetBrush().hardness, 0.0f, 1.0f), "hardness", "Hardness");

		ar(Serialization::Range(GetBrush().riseHeight, -MAX_RISE_HEIGHT, MAX_RISE_HEIGHT), "height", "Height");

		if (ar.isInput())
			SetBrushRadius(inRadius, outRadius);

		ar.closeBlock();
	}

	if (ar.openBlock("Displacement", "Displacement"))
	{
		ar(GetBrush().bDisplacement, "bDisplacement", "Enable Displacement");
		ar(GetBrush().displacementScale, "displacementScale", "Displacement Scale");
		ar(Serialization::TextureFilename(GetBrush().displacementMapName), "displacementMapName", "Displacement Map");
		ar.closeBlock();

		// load displacement brush image
		GetBrush().displacementMapData.Release();
		CImageUtil::LoadImage(GetBrush().displacementMapName, GetBrush().displacementMapData);
	}

	if (ar.openBlock("reposition", "Reposition"))
	{
		ar(GetBrush().bRepositionObjects, "repoobjects", "Objects");
		ar(GetBrush().bRepositionVegetation, "repoveget", "Vegetation");
		ar.closeBlock();
	}

	if (brushShared != IsBrushShared())
		SetShareBrushParams(brushShared);
}

void CRiseLowerTool::PaintTerrain(CHeightmap* pHeightmap, int tx, int ty, int tsize, float fInsideRadius, float brushSpeed)
{
	pHeightmap->RiseLowerSpot(tx, ty, tsize, fInsideRadius, GetBrush().riseHeight, GetBrush().hardness * brushSpeed,
	                          (GetBrush().bDisplacement && GetBrush().displacementMapData.GetData()) ? &GetBrush().displacementMapData : nullptr, GetBrush().displacementScale);
}

IMPLEMENT_DYNCREATE(CRiseLowerTool, CTerrainTool)

//////////////////////////////////////////////////////////////////////////
CHolesTool::CHolesTool(const char* const szPersonalizationModuleName)
	: CTerrainTool(szPersonalizationModuleName)
{}

void CHolesTool::Serialize(Serialization::IArchive& ar)
{
	bool brushShared = IsBrushShared();
	if (ar.openBlock("brush", "Brush"))
	{
		ar(brushShared, "shareparams", "Share Parameters");
		ar(Serialization::Range(GetBrush().radiusOutside, 0.0f, MAX_BRUSH_SIZE), "outradius", "Radius (outside)");
		ar.closeBlock();
	}

	if (brushShared != IsBrushShared())
		SetShareBrushParams(brushShared);
}

void CHolesTool::Display(SDisplayContext& dc)
{
	if (dc.display2D)
		return;

	CHeightmap* pHeightmap = GetIEditorImpl()->GetHeightmap();
	if (!pHeightmap)
	{
		return;
	}

	float unitSize = pHeightmap->GetUnitSize();

	dc.SetColor(0, 1, 0, 1);
	dc.DrawTerrainCircle(s_pointerPos, GetBrush().radiusOutside, 0.2f);

	Vec2i min;
	Vec2i max;

	CalculateHoleArea(s_pointerPos, GetBrush().radiusOutside, min, max);

	dc.SetColor(m_toolColor);
	dc.DrawTerrainRect(min.y * unitSize, min.x * unitSize, max.y * unitSize + unitSize, max.x * unitSize + unitSize, 0.2f);
}

bool CHolesTool::MouseCallback(CViewport* view, EMouseEvent event, CPoint& point, int flags)
{
	bool bHitTerrain = false;
	Vec3 stPointerPos = view->ViewToWorld(point, &bHitTerrain, true);

	// Initialize it to the first position so we can avoid a huge delta on the first call
	static CPoint s_lastMousePoint = point;

	if (MK_ALT & flags)
	{
		return CTerrainTool::MouseCallback(view, event, point, flags);
	}
	else
	{
		if (bHitTerrain)
		{
			s_pointerPos = stPointerPos;
		}

		if (event == eMouseLDown || (event == eMouseMove && (flags & MK_LBUTTON)))
		{
			if (!GetIEditorImpl()->Get3DEngine()->GetITerrain())
			{
				return false;
			}

			if (!GetIEditorImpl()->GetIUndoManager()->IsUndoRecording())
			{
				GetIEditorImpl()->GetIUndoManager()->Begin();
			}

			CHeightmap* pHeightmap = GetIEditorImpl()->GetHeightmap();
			PaintTerrain(pHeightmap, 0, 0, 0, 0, 0);
		}
		else
		{
			if (GetIEditorImpl()->GetIUndoManager()->IsUndoRecording())
			{
				GetIEditorImpl()->GetIUndoManager()->Accept("Terrain Hole");
			}
		}
	}

	s_lastMousePoint = point;

	return true;
}

bool CHolesTool::CalculateHoleArea(const Vec3& pointer, float radius, Vec2i& min, Vec2i& max)
{
	CHeightmap* pHeightmap = GetIEditorImpl()->GetHeightmap();
	if (!pHeightmap)
	{
		return false;
	}

	float unitSize = pHeightmap->GetUnitSize();

	float fx1 = (pointer.y - radius) / (float)unitSize;
	float fy1 = (pointer.x - radius) / (float)unitSize;
	float fx2 = (pointer.y + radius) / (float)unitSize;
	float fy2 = (pointer.x + radius) / (float)unitSize;

	min.x = MAX(fx1, 0);
	min.y = MAX(fy1, 0);
	max.x = MIN(fx2, pHeightmap->GetWidth() - 1.0f);
	max.y = MIN(fy2, pHeightmap->GetHeight() - 1.0f);

	return true;
}

IMPLEMENT_DYNAMIC(CHolesTool, CTerrainTool)

//////////////////////////////////////////////////////////////////////////
CMakeHolesTool::CMakeHolesTool()
	: CHolesTool(g_szMakeHolesToolPersonalizationModule)
{
	m_toolColor = ColorF(1, 0, 0, 1);
}

void CMakeHolesTool::PaintTerrain(CHeightmap* pHeightmap, int tx, int ty, int tsize, float fInsideRadius, float brushSpeed)
{
	Vec2i min;
	Vec2i max;
	CalculateHoleArea(s_pointerPos, GetBrush().radiusOutside, min, max);

	if (((max.x - min.x) >= 0) && ((max.y - min.y) >= 0))
	{
		pHeightmap->MakeHole(min.x, min.y, max.x - min.x, max.y - min.y, true);
	}
}

IMPLEMENT_DYNCREATE(CMakeHolesTool, CHolesTool)

//////////////////////////////////////////////////////////////////////////
CFillHolesTool::CFillHolesTool()
	: CHolesTool(g_szFillHolesToolPersonalizationModule)
{
	m_toolColor = ColorF(0, 0, 1, 1);
}

void CFillHolesTool::PaintTerrain(CHeightmap* pHeightmap, int tx, int ty, int tsize, float fInsideRadius, float brushSpeed)
{
	Vec2i min;
	Vec2i max;
	CalculateHoleArea(s_pointerPos, GetBrush().radiusOutside, min, max);

	if (((max.x - min.x) >= 0) && ((max.y - min.y) >= 0))
	{
		pHeightmap->MakeHole(min.x, min.y, max.x - min.x, max.y - min.y, false);
	}
}

IMPLEMENT_DYNCREATE(CFillHolesTool, CHolesTool)
