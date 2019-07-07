// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include <EditorFramework/Preferences.h>

struct EDITOR_COMMON_API SSnappingPreferences : public SPreferencePage
{
	enum Mode
	{
		eNone          = 0x0,
		eTerrain       = 0x01,
		eGeometry      = 0x02,
		eSurfaceNormal = 0x04
	};

	SSnappingPreferences()
		: SPreferencePage("Snapping", "General/Snapping")
		, m_gridSize(1)
		, m_angleSnap(5)
		, m_scaleSnap(1)
		, m_gridScale(1)
		, m_gridMajorLine(16)
		, m_markerSize(1.0f)
		, m_markerColor(RGB(0, 200, 200))
		, m_gridSnappingEnabled(true)
		, m_angleSnappingEnabled(true)
		, m_scaleSnappingEnabled(true)
		, m_markerDisplay(false)
		, m_snapModeFlags(0)
		, m_bPivotSnappingEnabled(false)
	{
	}

	virtual bool Serialize(yasli::Archive& ar) override;

	double       SnapLength(const double length) const;
	float        SnapScale(const float length) const;

	//! Calculate snapped vector on a plane that is given by 2 axis, taking in account current snapping mode.
	//! Snapping in third direction will be restricted
	//! \param vecWS - vector to snap in World Space
	//! \param axisX - axis X of a plane
	//! \param axisX - axis Y of a plane
	//! \return Snapped vector
	Vec3 SnapPlane(const Vec3& vecWS, const Vec3& axisX, const Vec3& axisY) const;

	//! Calculate snapped position in 3D, taking in account current snapping mode
	//! \param posWS - position in World Space
	//! \param axisX - rotation axis X
	//! \param axisY - rotation axis Y
	//! \param gridZoom - optional multiplier
	//! \return Snapped position in World Space
	Vec3 Snap3D(const Vec3& posWs, const Vec3& axisX = { 1, 0, 0 }, const Vec3& axisY = { 0, 1, 0 }, float gridZoom = 1.0f) const;

	//! Calculate snapped position of a point in 3D to World Space snapped grid.
	//! Will calculate the snapped position even if "snapping to grid" is disabled
	//! \param posWS - position in World Space to snap
	//! \return Snapped position in World Space
	Vec3   Snap3DWorldSpace(const Vec3& posWS) const;

	double SnapAngle(double angle) const;
	Ang3   SnapAngle(const Ang3& vec) const;

	uint16 GetSnapMode() const;

	//! Terrain snapping
	void EnableSnapToTerrain(bool bEnable);
	bool IsSnapToTerrainEnabled() const;
	//! Surface normal snapping
	void EnableSnapToNormal(bool bEnable);
	bool IsSnapToNormalEnabled() const;
	//! Geometry snapping
	void EnableSnapToGeometry(bool bEnable);
	bool IsSnapToGeometryEnabled() const;
	//! Pivot snapping
	bool IsPivotSnappingEnabled() const;
	void EnablePivotSnapping(bool bEnable);

	ADD_PREFERENCE_PAGE_PROPERTY(double, gridSize, setGridSize)
	ADD_PREFERENCE_PAGE_PROPERTY(double, angleSnap, setAngleSnap)
	ADD_PREFERENCE_PAGE_PROPERTY(float, scaleSnap, setScaleSnap)
	ADD_PREFERENCE_PAGE_PROPERTY(double, gridScale, setGridScale)
	ADD_PREFERENCE_PAGE_PROPERTY(int, gridMajorLine, setGridMajorLine)
	ADD_PREFERENCE_PAGE_PROPERTY(float, markerSize, setMarkerSize);
	ADD_PREFERENCE_PAGE_PROPERTY(COLORREF, markerColor, setMarkerColor);

	ADD_PREFERENCE_PAGE_PROPERTY(bool, gridSnappingEnabled, setGridSnappingEnabled)
	ADD_PREFERENCE_PAGE_PROPERTY(bool, angleSnappingEnabled, setAngleSnappingEnabled)
	ADD_PREFERENCE_PAGE_PROPERTY(bool, scaleSnappingEnabled, setScaleSnappingEnabled)
	ADD_PREFERENCE_PAGE_PROPERTY(bool, markerDisplay, setMarkerDisplay)

	uint16 m_snapModeFlags;
	bool   m_bPivotSnappingEnabled;
};

EDITOR_COMMON_API extern SSnappingPreferences gSnappingPreferences;
