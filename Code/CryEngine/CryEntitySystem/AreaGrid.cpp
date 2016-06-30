// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "AreaGrid.h"
#include "Area.h"
#include <CryRenderer/IRenderAuxGeom.h>

const uint32 CAreaGrid::GRID_CELL_SIZE = 4;
const float CAreaGrid::GRID_CELL_SIZE_R = 1.0f / CAreaGrid::GRID_CELL_SIZE;

//////////////////////////////////////////////////////////////////////////
CAreaGrid::CAreaGrid()
	: m_pEntitySystem(nullptr)
	, m_pbitFieldX(nullptr)
	, m_pbitFieldY(nullptr)
	, m_pAreaBounds(nullptr)
	, m_pAreas(nullptr)
	, m_bitFieldSizeU32(0)
	, m_maxNumAreas(0)
	, m_numCells(0)
{
}

//////////////////////////////////////////////////////////////////////////
CAreaGrid::~CAreaGrid()
{
	Reset();
}

//////////////////////////////////////////////////////////////////////////
TAreaPointers const& CAreaGrid::GetAreas(uint32 const x, uint32 const y)
{
	// Must be empty, don't clear here for performance reasons!
	CRY_ASSERT(m_areasTmp.empty());
	CRY_ASSERT(x < m_numCells && y < m_numCells);

	uint32 const* const pBitsLHS = m_pbitFieldX + (m_bitFieldSizeU32 * x);
	uint32 const* const pBitsRHS = m_pbitFieldY + (m_bitFieldSizeU32 * y);

	for (uint32 i = 0, offset = 0; i < m_bitFieldSizeU32; ++i, offset += 32)
	{
		uint32 currentBitField = pBitsLHS[i] & pBitsRHS[i];

		for (uint32 j = 0; currentBitField != 0; ++j, currentBitField >>= 1)
		{
			if (currentBitField & 1)
			{
				m_areasTmp.push_back(m_pAreas->at(offset + j));
			}
		}
	}

	return m_areasTmp;
}

//////////////////////////////////////////////////////////////////////////
bool CAreaGrid::GetAreaIndex(CArea const* const pArea, size_t& outIndex)
{
	bool bSuccess = false;

	if (m_pAreas != nullptr)
	{
		TAreaPointers::const_iterator Iter(m_pAreas->begin());
		TAreaPointers::const_iterator const IterEnd(m_pAreas->end());

		for (; Iter != IterEnd; ++Iter)
		{
			if ((*Iter) == pArea)
			{
				outIndex = std::distance(m_pAreas->begin(), Iter);
				CRY_ASSERT(outIndex < m_pAreas->size());
				bSuccess = true;

				break;
			}
		}
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
void CAreaGrid::AddAreaBit(const Vec2i& start, const Vec2i& end, uint32 areaIndex)
{
	CRY_ASSERT(start.x >= 0 && start.y >= 0 && end.x < (int)m_numCells && end.y < (int)m_numCells);
	CRY_ASSERT(start.x <= end.x && start.y <= end.y);

	uint32* pBits;
	const uint32 bucketBit = 1 << (areaIndex & 31);
	const uint32 bucketIndex = areaIndex >> 5;

	// -- X ---
	pBits = m_pbitFieldX + (m_bitFieldSizeU32 * start.x) + bucketIndex;
	for (int i = start.x; i <= end.x; i++, pBits += m_bitFieldSizeU32)
		*pBits |= bucketBit;

	// -- Y ---
	pBits = m_pbitFieldY + (m_bitFieldSizeU32 * start.y) + bucketIndex;
	for (int i = start.y; i <= end.y; i++, pBits += m_bitFieldSizeU32)
		*pBits |= bucketBit;

	m_pAreaBounds[areaIndex][0] = start;
	m_pAreaBounds[areaIndex][1] = end;
}

//////////////////////////////////////////////////////////////////////////
void CAreaGrid::RemoveAreaBit(uint32 areaIndex)
{
	Vec2i& start = m_pAreaBounds[areaIndex][0];
	Vec2i& end = m_pAreaBounds[areaIndex][1];

	if (start == Vec2i(-1, -1) && end == Vec2i(-1, -1))
	{
		return; // Hasn't been added yet
	}

	CRY_ASSERT(start.x >= 0 && start.y >= 0 && end.x < (int)m_numCells && end.y < (int)m_numCells);
	CRY_ASSERT(start.x <= end.x && start.y <= end.y);

	uint32* pBits;
	const uint32 bucketBit = ~(1 << (areaIndex & 31));
	const uint32 bucketIndex = areaIndex >> 5;

	// -- X ---
	pBits = m_pbitFieldX + (m_bitFieldSizeU32 * start.x) + bucketIndex;
	for (int i = start.x; i <= end.x; i++, pBits += m_bitFieldSizeU32)
		*pBits &= bucketBit;

	// -- Y ---
	pBits = m_pbitFieldY + (m_bitFieldSizeU32 * start.y) + bucketIndex;
	for (int i = start.y; i <= end.y; i++, pBits += m_bitFieldSizeU32)
		*pBits &= bucketBit;

	start = Vec2i(-1, -1);
	end = Vec2i(-1, -1);
}

//////////////////////////////////////////////////////////////////////////
void CAreaGrid::ClearAllBits()
{
	memset(m_pbitFieldX, 0, m_bitFieldSizeU32 * m_numCells * sizeof(m_pbitFieldX[0]));
	memset(m_pbitFieldY, 0, m_bitFieldSizeU32 * m_numCells * sizeof(m_pbitFieldY[0]));
	memset(m_pAreaBounds, -1, sizeof(m_pAreaBounds[0]) * m_maxNumAreas);
}

//////////////////////////////////////////////////////////////////////////
bool CAreaGrid::ResetArea(CArea* pArea)
{
	bool bSuccess = false;

	if (m_pAreas != nullptr)
	{
		FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

		uint32 index = 0;
		TAreaPointers::const_iterator iter(m_pAreas->begin());
		TAreaPointers::const_iterator const iterEnd(m_pAreas->end());
		uint32 const numAreas = std::min<uint32>((uint32)m_maxNumAreas, (uint32)m_pAreas->size());

		for (; iter != iterEnd && index < numAreas; ++iter, ++index)
		{
			if (*iter == pArea)
			{
				RemoveAreaBit(index);
				AddArea(pArea, index);
				bSuccess = true;
			}
		}
	}

	return bSuccess;
}

//////////////////////////////////////////////////////////////////////////
void CAreaGrid::AddArea(CArea* pArea, uint32 areaIndex)
{
	// Calculate a loose bounding box (ie one that covers all the region we will have to check this
	// shape for, including fade area).
	Vec2 vBBCentre(0.0f, 0.0f);
	Vec2 vBBExtent(0.0f, 0.0f);

	switch (pArea ? pArea->GetAreaType() : EEntityAreaType(-1))
	{
	case ENTITY_AREA_TYPE_SPHERE:
		{
			Vec3 sphereCenter(ZERO);
			float sphereRadius(0.0f);
			if (pArea)
				pArea->GetSphere(sphereCenter, sphereRadius);
			vBBCentre = Vec2(sphereCenter.x, sphereCenter.y);
			float const greatestFadeDistance = pArea->GetGreatestFadeDistance();
			vBBExtent = Vec2(sphereRadius + greatestFadeDistance, sphereRadius + greatestFadeDistance);
		}
		break;
	case ENTITY_AREA_TYPE_SHAPE:
		{
			Vec2 shapeMin, shapeMax;
			pArea->GetBBox(shapeMin, shapeMax);
			vBBCentre = (shapeMax + shapeMin) * 0.5f;
			float const greatestFadeDistance = pArea->GetGreatestFadeDistance();
			vBBExtent = (shapeMax - shapeMin) * 0.5f + Vec2(greatestFadeDistance, greatestFadeDistance);
		}
		break;
	case ENTITY_AREA_TYPE_BOX:
		{
			Vec3 boxMin, boxMax;
			pArea->GetBox(boxMin, boxMax);
			Matrix34 tm;
			pArea->GetMatrix(tm);
			vBBCentre = Vec2(tm.GetTranslation());
			vBBExtent = Matrix33(tm).GetFabs() * (Vec2(boxMax - boxMin) * 0.5f);
			float const greatestFadeDistance = pArea->GetGreatestFadeDistance();
			vBBExtent += Vec2(greatestFadeDistance, greatestFadeDistance);
		}
		break;
	case ENTITY_AREA_TYPE_SOLID:
		{
			AABB boundbox;
			pArea->GetSolidBoundBox(boundbox);
			Matrix34 tm;
			pArea->GetMatrix(tm);
			static const float sqrt2 = 1.42f;
			float const greatestFadeDistance = pArea->GetGreatestFadeDistance();
			Vec3 const boxMinWorld(tm.TransformPoint(boundbox.min));
			Vec3 const boxMaxWorld(tm.TransformPoint(boundbox.max));
			vBBExtent = Vec2(abs(boxMaxWorld.x - boxMinWorld.x), abs(boxMaxWorld.y - boxMinWorld.y)) * sqrt2 * 0.5f;
			vBBExtent += Vec2(greatestFadeDistance, greatestFadeDistance);
			vBBCentre = Vec2(boxMinWorld.x + boxMaxWorld.x, boxMinWorld.y + boxMaxWorld.y) * 0.5f;
		}
		break;
	}

	//Covert BB pos into grid coords
	Vec2i start((int)((vBBCentre.x - vBBExtent.x) * GRID_CELL_SIZE_R), (int)((vBBCentre.y - vBBExtent.y) * GRID_CELL_SIZE_R));
	Vec2i end((int)((vBBCentre.x + vBBExtent.x) * GRID_CELL_SIZE_R), (int)((vBBCentre.y + vBBExtent.y) * GRID_CELL_SIZE_R));

	if ((end.x | end.y) < 0 || start.x >= (int)m_numCells || start.y > (int)m_numCells)
		return;

	start.x = std::max<int>(start.x, 0);
	start.y = std::max<int>(start.y, 0);
	end.x = std::min<int>(end.x, (int)m_numCells - 1);
	end.y = std::min<int>(end.y, (int)m_numCells - 1);

	AddAreaBit(start, end, areaIndex);

#ifndef _RELEASE
	//query bb extents to see if they are correctly added to the grid
	//Debug_CheckBB(vBBCentre, vBBExtent, pArea);
#endif
}

//////////////////////////////////////////////////////////////////////////
void CAreaGrid::Compile(CEntitySystem* pEntitySystem, TAreaPointers const& areas)
{
	FUNCTION_PROFILER(GetISystem(), PROFILE_ENTITY);

	uint32 oldNumCells = m_numCells;
	uint32 terrainSize = gEnv->p3DEngine->GetTerrainSize();
	uint32 numCells = terrainSize / GRID_CELL_SIZE;
	numCells = std::max<uint32>(numCells, 2048);

	// No point creating an area grid if there are no areas.
	if (areas.empty() || numCells == 0)
	{
		Reset();
		return;
	}

	uint32 const numAreas = static_cast<uint32>(areas.size());
	uint32 bitFieldSizeU32 = ((numAreas + 31) >> 5);

	if (numCells != oldNumCells || numAreas > m_maxNumAreas || m_bitFieldSizeU32 != bitFieldSizeU32)
	{
		// Full reset and reallocate bit-fields
		Reset();
		m_numCells = numCells;

		m_bitFieldSizeU32 = bitFieldSizeU32;
		m_maxNumAreas = numAreas;

		CRY_ASSERT(m_pbitFieldX == nullptr && m_areasTmp.empty());

		m_pbitFieldX = new uint32[m_bitFieldSizeU32 * m_numCells];
		m_pbitFieldY = new uint32[m_bitFieldSizeU32 * m_numCells];

		m_areasTmp.reserve(m_maxNumAreas);

		m_pAreaBounds = new Vec2i[m_maxNumAreas][2];
	}

	m_pAreas = &areas;
	ClearAllBits();

	m_pEntitySystem = pEntitySystem;
	uint32 areaIndex = 0;

	for (auto const pArea : areas)
	{
		AddArea(pArea, areaIndex++);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAreaGrid::Reset()
{
	SAFE_DELETE_ARRAY(m_pbitFieldX);
	SAFE_DELETE_ARRAY(m_pbitFieldY);
	SAFE_DELETE_ARRAY(m_pAreaBounds);
	stl::free_container(m_areasTmp);
	m_pAreas = nullptr;
	m_bitFieldSizeU32 = 0;
	m_maxNumAreas = 0;
	m_numCells = 0;
}

//////////////////////////////////////////////////////////////////////////
TAreaPointers const& CAreaGrid::GetAreas(Vec3 const& position)
{
	// Clear this once before the call to GetAreas!
	ClearTmpAreas();

	if (m_pbitFieldX != nullptr)
	{
		uint32 const gridX = static_cast<uint32>(position.x * GRID_CELL_SIZE_R);
		uint32 const gridY = static_cast<uint32>(position.y * GRID_CELL_SIZE_R);

		if (gridX < m_numCells && gridY < m_numCells)
		{
			return GetAreas(gridX, gridY);
		}
		else
		{
			//printf("Trying to index outside of grid\n");
		}
	}

	return m_areasTmp;
}

//////////////////////////////////////////////////////////////////////////
void CAreaGrid::Draw()
{
#ifndef _RELEASE
	if (GetNumAreas() == 0)
		return;

	// Clear this once before the call to GetAreas!
	ClearTmpAreas();

	I3DEngine* p3DEngine = gEnv->p3DEngine;
	IRenderAuxGeom* pRC = gEnv->pRenderer->GetIRenderAuxGeom();
	pRC->SetRenderFlags(e_Def3DPublicRenderflags);

	ColorF const colorsArray[] = {
		ColorF(1.0f, 0.0f, 0.0f, 1.0f),
		ColorF(0.0f, 1.0f, 0.0f, 1.0f),
		ColorF(0.0f, 0.0f, 1.0f, 1.0f),
		ColorF(1.0f, 1.0f, 0.0f, 1.0f),
		ColorF(1.0f, 0.0f, 1.0f, 1.0f),
		ColorF(0.0f, 1.0f, 1.0f, 1.0f),
		ColorF(1.0f, 1.0f, 1.0f, 1.0f),
	};

	for (uint32 gridX = 0; gridX < m_numCells; gridX++)
	{
		for (uint32 gridY = 0; gridY < m_numCells; gridY++)
		{
			TAreaPointers const& rAreas = GetAreas(gridX, gridY);

			if (!rAreas.empty())
			{
				Vec3 const vGridCentre = Vec3((float(gridX) + 0.5f) * GRID_CELL_SIZE, (float(gridY) + 0.5f) * GRID_CELL_SIZE, 0.0f);

				ColorF colour(0.0f, 0.0f, 0.0f, 0.0f);
				float divisor = 0.0f;
				TAreaPointers::const_iterator Iter(rAreas.begin());
				TAreaPointers::const_iterator const IterEnd(rAreas.end());

				for (; Iter != IterEnd; ++Iter)
				{
					// Pick a random color from the array.
					size_t nAreaIndex = 0;

					if (GetAreaIndex(*Iter, nAreaIndex))
					{
						ColorF const areaColour = colorsArray[nAreaIndex % (sizeof(colorsArray) / sizeof(ColorF))];
						colour += areaColour;
						++divisor;
					}
					else
					{
						// Areas must be known!
						CRY_ASSERT(false);

						if (divisor == 0.0f)
						{
							divisor = 0.1f;
						}
					}
				}

				// Immediately clear the array to prevent it from re-entering this loop and messing up drawing of the grid.
				ClearTmpAreas();

				// "divisor" won't be 0!
				colour /= divisor;

				ColorB const colourB = colour;

				int const gridCellSize = GRID_CELL_SIZE;

				Vec3 points[] = {
					vGridCentre + Vec3(-gridCellSize * 0.5f, -gridCellSize * 0.5f, 0.0f),
					vGridCentre + Vec3(+gridCellSize * 0.5f, -gridCellSize * 0.5f, 0.0f),
					vGridCentre + Vec3(+gridCellSize * 0.5f, +gridCellSize * 0.5f, 0.0f),
					vGridCentre + Vec3(-gridCellSize * 0.5f, +gridCellSize * 0.5f, 0.0f)
				};

				enum {NUM_POINTS = CRY_ARRAY_COUNT(points)};

				for (int i = 0; i < NUM_POINTS; ++i)
				{
					points[i].z = p3DEngine->GetTerrainElevation(points[i].x, points[i].y);
				}

				pRC->DrawTriangle(points[0], colourB, points[1], colourB, points[2], colourB);
				pRC->DrawTriangle(points[2], colourB, points[3], colourB, points[0], colourB);
			}
		}
	}

	float const yPos = 300.0f;
	float const fColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	gEnv->pRenderer->Draw2dLabel(30, yPos, 1.35f, fColor, false, "Area Grid Mem Use: num cells: %d, memAlloced: %.2fk",
	                             m_numCells, (4 * m_bitFieldSizeU32 * m_numCells * 2) / 1024.f);
#endif // _RELEASE
}

#ifndef _RELEASE
//////////////////////////////////////////////////////////////////////////
void CAreaGrid::Debug_CheckBB(Vec2 const& vBBCentre, Vec2 const& vBBExtent, CArea const* const pArea)
{
	uint32 nAreas = 0;

	Vec2 minPos(vBBCentre.x - vBBExtent.x, vBBCentre.y - vBBExtent.y);
	TAreaPointers const& rAreasAtMinPos = GetAreas(minPos);
	bool bFoundMin = false;

	TAreaPointers::const_iterator IterMin(rAreasAtMinPos.begin());
	TAreaPointers::const_iterator const IterMinEnd(rAreasAtMinPos.end());

	for (; IterMin != IterMinEnd; ++IterMin)
	{
		if (*IterMin == pArea)
		{
			bFoundMin = true;
			break;
		}
	}

	Vec2 maxPos(vBBCentre.x + vBBExtent.x, vBBCentre.y + vBBExtent.y);
	TAreaPointers const& rAreasAtMaxPos = GetAreas(maxPos);
	bool bFoundMax = false;

	TAreaPointers::const_iterator IterMax(rAreasAtMaxPos.begin());
	TAreaPointers::const_iterator const IterMaxEnd(rAreasAtMaxPos.end());

	for (; IterMax != IterMaxEnd; ++IterMax)
	{
		if (*IterMax == pArea)
		{
			bFoundMax = true;
			break;
		}
	}

	//caveat: this may fire if the area was clamped
	if ((!bFoundMin || !bFoundMax) /*&& !bClamped*/)
	{
		CryLogAlways("Error: AABB extent was not found in grid\n");
	}
}
#endif
