#pragma once

#include "resource.h"

#include "maxapi.h"
#include "bitarray.h"
#include "GraphicsConstants.h"

class MNMesh;
class TriObject;
class PolyObject;

extern HINSTANCE hInstance;
TCHAR* GetString(int id);

#define NUM_ELEMENTS(array) (sizeof(array) / sizeof(array[0]))
#define MN_EDITPOLY_OP_SELECT (DWORD)(MN_USER<<1)
#define COLVAL_8BIT(R,G,B) Color((float)(R) / 255.0f, (float)(G) / 255.0f, (float)(B) / 255.0f)

Point3 DegToRad(const Point3& deg);
float DegToRad(const float& deg);

Point3 ComponentwiseMin(const Point3& a, const Point3& b);
Point3 ComponentwiseMax(const Point3& a, const Point3& b);

INode* FindNodeFromRefMaker(ReferenceMaker *rm);
INode* FindNodeFromRefTarget(ReferenceTarget *rt);
TriObject* GetTriObjectFromNode(INode *node, TimeValue t, int &deleteIt);
PolyObject* GetPolyObjectFromNode(INode *node, TimeValue t, int &deleteIt);

Color GetFaceColor(int num, int mapChannel, MNMesh* mesh);
UVVert GetMapVert(int face, int vertex, int mapChannel, MNMesh* mesh);
void SetMapVert(UVVert faceColor, int face, int vertex, int mapChannel, MNMesh* mesh);
bool colorAboutEqual(const Color& c1, const Color& c2, float tolerance);
BitArray GetFacesOfColor(MNMesh* mesh, const Color& faceColor, int channel, float colorTolerance);
void InitializeMap(int channel, MNMesh* mesh, bool andClear);
bool CopyMap(MNMesh*, int sourceMapIndex, int destinationMapIndex);
// Copies normals from the source to the destination mesh. Meshes MUST have the same topology. Will ignore faces flagged as MN_DEAD
void CopyNormals(MNMesh& sourceMesh, MNMesh& targetMesh);
// Deletes the faces specified faces from the bitarray. Does NOT collapse structs. This must be called manually if you wish to do so.
bool DeleteFaces(MNMesh& mesh, BitArray facesToDelete);

Box3 TransformBox(Box3 box,const Matrix3 matrix);

BitArray GetHardEdges(MNMesh* mesh);
float LongestEdge(MNMesh* mesh, DWORD face, bool ignoreTriangles);
bool PerformMakeSmoothEdges(MNMesh & mesh, DWORD flags);

// Macro for registering class descriptions
// Use this as a replacement for LibNumberClasses() and LibClassDesc(int)
#define DEFINE_PLUGIN_CLASS_DESCRIPTIONS(...)\
static ClassDesc* classDescriptions[] = {\
	__VA_ARGS__\
};\
__declspec(dllexport) int LibNumberClasses()\
{\
	return NUM_ELEMENTS(classDescriptions);\
}\
__declspec(dllexport) ClassDesc* LibClassDesc(int i)\
{\
	return classDescriptions[i];\
}