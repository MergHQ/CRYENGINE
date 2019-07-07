// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Core/Polygon.h"

namespace Designer {
class Model;
class ExcludedEdgeManager;
namespace Display
{
void DisplayHighlightedElements(SDisplayContext& dc, MainContext& mc, int nPickFlag, ExcludedEdgeManager* pExcludedEdgeMgr);
void DisplayHighlightedEdges(SDisplayContext& dc, MainContext& mc, ExcludedEdgeManager* pExcludedEdgeMgr);
void DisplayHighlightedVertices(SDisplayContext& dc, MainContext& mc, bool bExcludeVerticesFromSecondInOpenPolygon = false);
void DisplayHighlightedPolygons(SDisplayContext& dc, MainContext& mc);
void DisplayModel(SDisplayContext& dc, Designer::Model* pModel, ExcludedEdgeManager* pExcludedEdgeMgr, ShelfID nShelf = eShelf_Any, const int nLineThickness = 2, const ColorB& lineColor = ColorB(0, 0, 0));
void DisplayPolygons(SDisplayContext& dc, const std::vector<PolygonPtr>& polygons, ExcludedEdgeManager* pExcludedEdgeMgr, const int nLineThickness, const ColorB& lineColor);
void DisplayPolygon(SDisplayContext& dc, PolygonPtr polygon);
void DisplayBottomPolygon(SDisplayContext& dc, PolygonPtr polygon, const ColorB& lineColor = PolygonLineColor);
void DisplaySubdividedMesh(SDisplayContext& dc, Model* pModel);
void DisplayVertexNormals(SDisplayContext& dc, MainContext& mc);
void DisplayPolygonNormals(SDisplayContext& dc, MainContext& mc);
void DisplayTriangulation(SDisplayContext& dc, MainContext& mc);
}
}
