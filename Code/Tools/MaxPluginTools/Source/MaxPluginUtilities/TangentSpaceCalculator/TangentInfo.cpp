#include "stdafx.h"

#include "TangentInfo.h"

#include <functional>


int TangentInfo::getNumFaces(const SMikkTSpaceContext* pContext)
{
	MNMesh* mesh = ((TangentInfo*)pContext->m_pUserData)->mesh;
	return mesh->numf;
}


int TangentInfo::getNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace)
{
	MNMesh* mesh = ((TangentInfo*)pContext->m_pUserData)->mesh;
	return mesh->f[iFace].deg;
}

void TangentInfo::getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert)
{
	MNMesh* mesh = ((TangentInfo*)pContext->m_pUserData)->mesh;
	Point3 pos = mesh->v[mesh->f[iFace].vtx[iVert]].p;
	fvPosOut[0] = pos.x;
	fvPosOut[1] = pos.y;
	fvPosOut[2] = pos.z;
}

void TangentInfo::getNormal(const SMikkTSpaceContext* pContext, float fvNormOut[], const int iFace, const int iVert)
{
	TangentInfo* info = ((TangentInfo*)pContext->m_pUserData);
	Point3 normal = info->faces[iFace].normals[iVert];
	fvNormOut[0] = normal.x;
	fvNormOut[1] = normal.y;
	fvNormOut[2] = normal.z;
}

void TangentInfo::getTexCoord(const SMikkTSpaceContext* pContext, float fvTexcOut[], const int iFace, const int iVert)
{
	MNMesh* mesh = ((TangentInfo*)pContext->m_pUserData)->mesh;
	MNMap* m = mesh->M(1);
	UVVert uv = m->v[m->f[iFace].tv[iVert]];
	fvTexcOut[0] = uv.x;
	fvTexcOut[1] = uv.y;
}

void TangentInfo::setTSpace(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fvBiTangent[], const float fMagS, const float fMagT, const tbool bIsOrientationPreserving, const int iFace, const int iVert)
{
	TangentInfo* info = ((TangentInfo*)pContext->m_pUserData);
	info->faces[iFace].tangents[iVert] = Point3(fvTangent[0], fvTangent[1], fvTangent[2]);
	info->faces[iFace].binormals[iVert] = Point3(fvBiTangent[0], fvBiTangent[1], fvBiTangent[2]);
}

TangentInfo::TangentInfo()
	: faces()
{
}

void TangentInfo::Update(MNMesh* fromMesh)
{
	mesh = fromMesh;

	faces.setLengthUsed(mesh->numf);

	mesh->buildNormals();
	for (int f = 0; f < fromMesh->numf; f++)
	{
		MNFace& face = fromMesh->f[f];

		faces[f].tangents.setLengthUsed(face.deg);
		faces[f].binormals.setLengthUsed(face.deg);
		faces[f].normals.setLengthUsed(face.deg);

		for (int v = 0; v < face.deg; v++)
		{
			faces[f].normals[v] = mesh->GetVertexNormal(face.vtx[v]);
		}
	}

	SMikkTSpaceInterface mikkTSpaceInterface;
	mikkTSpaceInterface.m_getNumFaces = &getNumFaces;
	mikkTSpaceInterface.m_getNumVerticesOfFace = &getNumVerticesOfFace;
	mikkTSpaceInterface.m_getPosition = &getPosition;
	mikkTSpaceInterface.m_getNormal = &getNormal;
	mikkTSpaceInterface.m_getTexCoord = &getTexCoord;
	mikkTSpaceInterface.m_setTSpace = &setTSpace;
	mikkTSpaceInterface.m_setTSpaceBasic = NULL;

	SMikkTSpaceContext mikkTSpaceContext;
	mikkTSpaceContext.m_pInterface = &mikkTSpaceInterface;
	mikkTSpaceContext.m_pUserData = this;

	genTangSpaceDefault(&mikkTSpaceContext);
}

Matrix3 TangentInfo::GetTangentspaceTransform(int face, int vert) const
{
	return Matrix3(
		faces[face].tangents[vert],
		faces[face].binormals[vert],
		faces[face].normals[vert],
		Point3(0, 0, 0)
	);
}

Point3 TangentInfo::ToTangentSpace(Point3 vec, int face, int vert) const
{
	Matrix3 matrix = GetTangentspaceTransform(face, vert);
	matrix.Invert();
	return matrix.VectorTransform(vec);
}

Point3 TangentInfo::FromTangentSpace(Point3 vec, int face, int vert) const
{
	Matrix3 matrix = GetTangentspaceTransform(face, vert);
	return matrix.VectorTransform(vec);
}
