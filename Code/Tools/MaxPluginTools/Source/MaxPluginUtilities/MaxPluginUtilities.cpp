#include "stdafx.h"
#include "MaxPluginUtilities.h"
#include "mnmesh.h"

HINSTANCE hInstance;

TCHAR* GetString(int id)
{
	static TCHAR buf[256];

	if (hInstance)
		return LoadString(hInstance, id, buf, _countof(buf)) ? buf : NULL;
	return NULL;
}

Point3 DegToRad(const Point3& deg) 
{ 
	return deg * (pi / 180.0f); 
}

float DegToRad(const float& deg) 
{ 
	return deg * (pi / 180.0f);
}

Point3 ComponentwiseMin(const Point3& a, const Point3& b)
{
	Point3 retVal;

	for (int i = 0; i < 3; i++)
		retVal[i] = min(a[i], b[i]);

	return retVal;
}

Point3 ComponentwiseMax(const Point3& a, const Point3& b)
{
	Point3 retVal;

	for (int i = 0; i < 3; i++)
		retVal[i] = max(a[i], b[i]);

	return retVal;
}

INode* FindNodeFromRefMaker(ReferenceMaker *rm)
{
	if (rm->SuperClassID() == BASENODE_CLASS_ID)
		return (INode *)rm;
	else
		return rm->IsRefTarget() ? FindNodeFromRefTarget((ReferenceTarget *)rm) : NULL;
}

INode* FindNodeFromRefTarget(ReferenceTarget *rt)
{
	DependentIterator di(rt);
	ReferenceMaker *rm = NULL;
	INode *nd = NULL;
	while ((rm = di.Next()) != NULL)
	{
		nd = FindNodeFromRefMaker(rm);
		if (nd) return nd;
	}
	return NULL;
}

TriObject* GetTriObjectFromNode(INode *node, TimeValue t, int &deleteIt)
{
	deleteIt = FALSE;
	Object *obj = node->EvalWorldState(t).obj;
	if (obj->CanConvertToType(Class_ID(TRIOBJ_CLASS_ID, 0)))
	{
		TriObject *tri = (TriObject *)obj->ConvertToType(0,
			Class_ID(TRIOBJ_CLASS_ID, 0));
		// Note that the TriObject should only be deleted
		// if the pointer to it is not equal to the object
		// pointer that called ConvertToType()
		if (obj != tri) 
			deleteIt = TRUE;

		return tri;
	}
	else
	{
		return NULL;
	}
}

PolyObject* GetPolyObjectFromNode(INode *node, TimeValue t, int &deleteIt)
{
	deleteIt = FALSE;
	Object *obj = node->EvalWorldState(t).obj;
	if (obj->CanConvertToType(Class_ID(POLYOBJ_CLASS_ID, 0)))
	{
		PolyObject *poly = (PolyObject *)obj->ConvertToType(0,
			Class_ID(POLYOBJ_CLASS_ID, 0));
		// Note that the PolyObject should only be deleted
		// if the pointer to it is not equal to the object
		// pointer that called ConvertToType()
		if (obj != poly)
			deleteIt = TRUE;

		return poly;
	}
	else
	{
		return NULL;
	}
}

Color GetFaceColor(int num, int mapChannel, MNMesh* mesh)
{
	const MNMapFace *mapFaces = mesh->M(mapChannel)->f;
	const UVVert *colorVerts = mesh->M(mapChannel)->v;
	return colorVerts[mapFaces[num].tv[0]]; // Just return first vert color, assume all are the same.

	//// Old slow (and safe) logic.
	//static Color white(1, 1, 1), black(0, 0, 0);

	//if (mapChannel >= mesh->MNum())
	//	return white;

	//BOOL init = FALSE;
	//Color returnColor = white;

	//const MNMapFace *mapFaces = mesh->M(mapChannel)->f;
	//const UVVert *colorVerts = mesh->M(mapChannel)->v;

	//if (mapFaces == NULL || colorVerts == NULL)
	//	return Color(1, 1, 1);

	//int *mappingVerts = mapFaces[num].tv;
	//for (int faceVert = 0; faceVert < mapFaces[num].deg; faceVert++)
	//{
	//	return colorVerts[mappingVerts[faceVert]]; // Just return first vert color, assume all are the same.
	//}
	//return Color(1, 1, 1); // Shouldn't happen.
}

UVVert GetMapVert(int face, int vertex, int mapChannel, MNMesh*mesh)
{
	const MNMapFace *mapFaces = mesh->M(mapChannel)->f;
	const UVVert *colorVerts = mesh->M(mapChannel)->v;
	return colorVerts[mapFaces[face].tv[vertex]];
}

void SetMapVert(UVVert faceColor, int face, int vertex, int mapChannel, MNMesh* mesh)
{
	const MNMapFace *mapFaces = mesh->M(mapChannel)->f;
	UVVert *colorVerts = mesh->M(mapChannel)->v;
	colorVerts[mapFaces[face].tv[vertex]] = faceColor;
}

bool colorAboutEqual(const Color& c1, const Color& c2, float tolerance)
{
	//// Old slow logic:
	//const float halfTolerance = tolerance / 2.0f;

	//// Loop for each channel (r,g,b)
	//for (int i = 0; i < 3; i++)
	//{
	//	if (c1[i] > c2[i] + halfTolerance || c1[i] < c2[i] - halfTolerance)
	//		return false;
	//}
	//return true;

	// Optimized function, works without any branching. Sorry for the crazyness.
	bool returnValue = true;
	tolerance /= 2;
	// Also unrolled the loop.
	returnValue = returnValue && int(tolerance / abs((c1[0] - c2[0])));
	returnValue = returnValue && int(tolerance / abs((c1[1] - c2[1])));
	returnValue = returnValue && int(tolerance / abs((c1[2] - c2[2])));
	return returnValue;

}

BitArray GetFacesOfColor(MNMesh* mesh, const Color& faceColor, int channel, float colorTolerance)
{
	if (mesh->numm <= channel)
		return BitArray(mesh->numf);

	MNMap* map = mesh->M(channel);

	if (map == NULL || map->numf == 0)
		return BitArray(mesh->numf);

	BitArray facesOfColor = BitArray(map->numf);

	for (int f = 0; f < map->numf; f++)
	{
		Color vertColor = GetFaceColor(f, channel, mesh);

		if (colorAboutEqual(vertColor, faceColor, colorTolerance))
			facesOfColor.Set(f);
	}

	return facesOfColor;
}

void InitializeMap(int channel, MNMesh* mesh, bool andClear)
{
	if (channel >= mesh->MNum())
		mesh->SetMapNum(channel + 1); // destination channel doesn't exist yet, create it.

	MNMap* map = mesh->M(channel);	

	bool clear = andClear;

	if (map->GetFlag(MN_DEAD))
		clear = true;

	if (clear)
	{
		map->ClearFlag(MN_DEAD);
		map->setNumFaces(mesh->FNum());
		map->setNumVerts(mesh->VNum());

		for (int i = 0; i < mesh->numv; i++)
			map->v[i] = UVVert(1, 1, 1);
		for (int i = 0; i < mesh->numf; i++)
			map->f[i] = mesh->f[i];
	}
}

bool CopyMap(MNMesh* mesh, int sourceMapIndex, int destinationMapIndex)
{
	if (sourceMapIndex >= mesh->MNum())
		return false; // Source map is invalid; does not exist.

	MNMap* sourceMap = mesh->M(sourceMapIndex);

	if (sourceMap->GetFlag(MN_DEAD))
		return false; // Source map is invalid; marked as dead.

	if (destinationMapIndex >= mesh->MNum())
	{		
		mesh->SetMapNum(destinationMapIndex + 1); // destination channel doesn't exist yet, create it.
		sourceMap = mesh->M(sourceMapIndex); // Becomes invalid after SetMapNum, so call again.
	}

	MNMap* destinationMap = mesh->M(destinationMapIndex);
	destinationMap->ClearFlag(MN_DEAD);
	destinationMap->setNumFaces(sourceMap->FNum());
	destinationMap->setNumVerts(sourceMap->VNum());

	for (int v = 0; v < sourceMap->VNum(); v++)
		destinationMap->v[v] = sourceMap->V(v);
	for (int f = 0; f < sourceMap->FNum(); f++)
		destinationMap->f[f] = sourceMap->f[f];

	return true;
}

// Copies normals from the source to the destination mesh. Meshes MUST have the same topology. Will ignore faces flagged as MN_DEAD
void CopyNormals(MNMesh& sourceMesh, MNMesh& targetMesh)
{
	MNNormalSpec* sourceNormalSpec = sourceMesh.GetSpecifiedNormals();
	MNNormalSpec* targetNormalSpec = targetMesh.GetSpecifiedNormals();

	// Ensure normals are specified on target mesh.
	sourceMesh.SpecifyNormals();
	targetMesh.SpecifyNormals();

	// For each face in the source mesh
	for (DWORD sf = 0; sf < sourceMesh.numf; sf++)
	{
		Point3 sourceFaceCenter;
		sourceMesh.ComputeCenter(sf, sourceFaceCenter);

		// If face in the source mesh is not dead, copy it to the desitination mesh
		if (!sourceMesh.f[sf].GetFlag(MN_DEAD))
		{
			for (int corner = 0; corner < sourceMesh.F(sf)->deg; corner++)
			{
				Point3 sourceNormal = sourceNormalSpec->GetNormal(sf, corner);
				DWORD targetNormalID = targetNormalSpec->Face(sf).GetNormalID(corner);
				Point3& targetNormalRef = targetNormalSpec->GetNormalArray()[targetNormalID];
				targetNormalRef = sourceNormal;
			}
		}
	}
}

// Deletes the faces specified faces from the bitarray. Does NOT collapse structs. This must be called manually if you wish to do so.
bool DeleteFaces(MNMesh& mesh, BitArray facesToDelete)
{
	mesh.ClearFFlags(MN_EDITPOLY_OP_SELECT);

	for (int f = 0; f < facesToDelete.GetSize(); f++)
	{
		if (facesToDelete[f] == true)
		{
			mesh.F(f)->SetFlag(MN_DEAD, facesToDelete[f] ? true : false);
		}
	}

	mesh.DeleteFlaggedFaces(MN_DEAD);
	return true;
}

Box3 TransformBox(Box3 box, Matrix3 matrix)
{
	Point3 points[8] = {
		box.Center() + box.Width() * Point3(0.5f, 0.5f, -0.5f),
		box.Center() + box.Width() * Point3(0.5f, 0.5f, 0.5f),
		box.Center() + box.Width() * Point3(0.5f, -0.5f, -0.5f),
		box.Center() + box.Width() * Point3(0.5f, -0.5f, 0.5f),
		box.Center() + box.Width() * Point3(-0.5f, 0.5f, -0.5f),
		box.Center() + box.Width() * Point3(-0.5f, 0.5f, 0.5f),
		box.Center() + box.Width() * Point3(-0.5f, -0.5f, -0.5f),
		box.Center() + box.Width() * Point3(-0.5f, -0.5f, 0.5f),
	};
	matrix.TransformPoints(points, 8);
	box = Box3(points[0], points[0]);
	box.IncludePoints(points, 8);
	return box;
}

BitArray GetHardEdges(MNMesh* mesh)
{
	BitArray hardEdges = BitArray(mesh->nume);

	for (int i = 0; i < mesh->nume; ++i)
	{
		MNEdge &theEdge = mesh->e[i];
		if (theEdge.GetFlag(MN_DEAD)) continue;
		// Single-face edges aren't hard
		if (theEdge.f2 < 0) continue;
		MNFace &face1 = mesh->f[theEdge.f1];
		MNFace &face2 = mesh->f[theEdge.f2];
		if (!(face1.smGroup & face2.smGroup))
			hardEdges.Set(i);
	}

	return hardEdges;
}

float LongestEdge(MNMesh* mesh, DWORD f, bool ignoreTriangles)
{
	MNFace& Face = mesh->f[f];

	if (ignoreTriangles && Face.TriNum() <= 1)
		return false;

	float longest = 0;

	for (DWORD e = 0; e < Face.deg; e++)
	{
		MNEdge& Edge = mesh->e[Face.edg[e]];
		Point3 v1 = mesh->v[Edge.v1].p;
		Point3 v2 = mesh->v[Edge.v2].p;

		const float edgeLength = (v1 - v2).LengthSquared(); // Use length squared for now, since at this point all we care about is which is longer, and squared length is much faster to find.

		if (edgeLength > longest || longest == 0)
			longest = edgeLength;
	}

	// Now square root the length.
	longest = Sqrt(longest);

	// Is a long face if the longest edge is more than twice the length of the shortest.
	return longest;
}


// Note: This code is copied from "Editpolyops.cpp" in the max 2016 SDK sample. Check there for updates!
bool PerformMakeSmoothEdges(MNMesh & mesh, DWORD flags)
{
#if MAX_RELEASE >= 18000
	BitArray esel;
	mesh.getEdgesByFlag(esel, flags);
	// If nothing selected, punt
	if (esel.NumberSet() == 0) return false;

	bool altered = false;
	bool failed = false;	// Goes to true if we fail to get an available smoothing group for any edge

							// Go through the edges, and if you have one that needs to be made smooth, handle it
	for (int i = 0; i < mesh.nume; ++i) {
		MNEdge &theEdge = mesh.e[i];
		// Ignore dead, unselected or one-faced edges
		if (theEdge.GetFlag(MN_DEAD) || (esel[i] == 0) || (theEdge.f2 < 0)) continue;
		// Grab the "f1" side face
		MNFace &theAFace = mesh.f[theEdge.f1];
		// This face shouldn't be dead, but just in case...
		if (theAFace.GetFlag(MN_DEAD)) { DbgAssert(0); continue; }
		DWORD aSmooth = theAFace.smGroup;
		// Grab the "f2" side face
		MNFace &theBFace = mesh.f[theEdge.f2];
		// This face shouldn't be dead, but just in case...
		if (theBFace.GetFlag(MN_DEAD)) { DbgAssert(0); continue; }
		DWORD bSmooth = theBFace.smGroup;
		// If they already share any smooth groups, move on to the next one
		if ((aSmooth & bSmooth) != 0) continue;
		// No smoothing groups in common -- We can't just turn on smoothing to match the next face arbitrarily
		// because that could cause unintended smoothing to other adjacent faces.
		// Need to set one that isn't used by any faces adjacent to either face
		// Make up a BitArray with a bit set for each face that we'll be dealing with
		BitArray adjacentFaces;
		adjacentFaces.SetSize(mesh.numf);
		adjacentFaces.ClearAll();
		// And an IntTab to hold the list of adjacent face indexes
		IntTab adjacentFaceIndexes;
		// get faces adjacent to face A
		mesh.FaceFindAdjacentFaces(theEdge.f1, adjacentFaces, adjacentFaceIndexes, true);
		// get faces adjacent to face B (adds to list)
		mesh.FaceFindAdjacentFaces(theEdge.f2, adjacentFaces, adjacentFaceIndexes, true);
		DWORD smooth = mesh.FindAvailableSmoothingGroupFromFaceList(adjacentFaceIndexes);
		if (smooth) {
			// Smooth group is available! Apply it to both faces and move on to the next one
			theAFace.smGroup |= smooth;
			theBFace.smGroup |= smooth;
			altered = true;
			continue;	// All done
		}
		// Well, no available smoothing groups were found, so now we need to optimize the smoothing groups in those involved faces, and hope
		// that we can free up one for use in this edge smoothing

		if (mesh.OptimizeSmoothingGroups(adjacentFaces)) {
			// try again to get a smoothing group
			DWORD smooth = mesh.FindAvailableSmoothingGroupFromFaceList(adjacentFaceIndexes);
			if (smooth) {
				// Smooth group is available! Apply it to both faces and move on to the next one
				theAFace.smGroup |= smooth;
				theBFace.smGroup |= smooth;
				altered = true;
				continue;	// All done
			}
		}
		// This is a rare situation; no available smoothing groups despite optimizing neighboring faces.
		// Repeat, this time optimizing the entire object
		adjacentFaces.SetAll();

		if (mesh.OptimizeSmoothingGroups(adjacentFaces)) {
			// Try one last time to get a smoothing group
			DWORD smooth = mesh.FindAvailableSmoothingGroupFromFaceList(adjacentFaceIndexes);
			if (smooth) {
				// Smooth group is available! Apply it to both faces and move on to the next one
				theAFace.smGroup |= smooth;
				theBFace.smGroup |= smooth;
				altered = true;
				continue;	// All done
			}
			failed = true;	// Couldn't do everything -- Should we display a message?
		}
		else
			failed = true;	// Couldn't optimize -- Should we display a message?
	}

	//if (failed)
	//	GetCOREInterface()->DisplayTempPrompt(GetString(IDS_SMOOTH_EDGE_FAILURE_MSG), 4000);

	return altered;
#else
	return false; // Not supported in this version of max.
#endif
}
