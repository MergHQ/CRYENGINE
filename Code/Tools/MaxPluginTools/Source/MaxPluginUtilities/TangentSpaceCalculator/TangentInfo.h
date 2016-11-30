#pragma once

#include "mikktspace.h"

class TangentInfo
{
private:
	MNMesh* mesh;

	static int getNumFaces(const SMikkTSpaceContext* pContext);
	static int getNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace);
	static void getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert);
	static void getNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert);
	static void getTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert);
	static void setTSpace(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fvBiTangent[], const float fMagS, const float fMagT, const tbool bIsOrientationPreserving, const int iFace, const int iVert);

public:

	struct TInfoFace
	{
		MaxSDK::Array<Point3> tangents;
		MaxSDK::Array<Point3> binormals;
		MaxSDK::Array<Point3> normals;
	};

	MaxSDK::Array<TInfoFace> faces;

	TangentInfo();

	void Update(MNMesh* fromMesh);
	MNMesh* GetMesh() const { return mesh; }
	Matrix3 GetTangentspaceTransform(int face, int vert) const;
	Point3 ToTangentSpace(Point3 vec, int face, int vert) const;
	Point3 FromTangentSpace(Point3 vec, int face, int vert) const;
};