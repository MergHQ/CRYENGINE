#include "stdafx.h"
#include "GeomDecal.h"

#include "GeomDecalClassDesc.h"
#include "GeomDecalCreateCallback.h"
#include "GeomDecalParamBlock.h"
#include "GeomDecalDlgProcs.h"
#include "TriBox.h"

#include <vector>
#include "gfx.h"

IObjParam* GeomDecal::ip = NULL;

GeomDecal::GeomDecal(BOOL loading) : SimpleObject2()
{
	GeomDecalClassDesc::GetInstance()->MakeAutoParamBlocks(this);
}

GeomDecal::~GeomDecal()
{
	ClearObjectPool();
}

void GeomDecal::UpdateObjectPool(TimeValue t)
{
	ClearObjectPool();

	const int nodeCount = pblock2->Count(eGeomDecalParam::node_pool);
	for (int i = 0; i < nodeCount; i++)
	{
		INode* node;
		pblock2->GetValue(eGeomDecalParam::node_pool, t, node, ivalid, i);

		if (node != NULL)
		{
			objectPool.append(node);
		}
	}
}

void GeomDecal::ClearObjectPool()
{
	objectPool.removeAll();
}

void GeomDecal::UpdateMemberVarsFromParamBlock(TimeValue t)
{
	ivalid = FOREVER; // Start with valid forever, then widdle it down as we access pblock values.
	ivalid.SetInstant(t);
	// General	
	pblock2->GetValue(eGeomDecalParam::projection_size, t, projectionSize, ivalid);
	pblock2->GetValue(eGeomDecalParam::projection_anglecutoff, t, angleCutoff, ivalid);

	UpdateObjectPool(t);

	pblock2->GetValue(eGeomDecalParam::uv_tiling, t, uvTiling, ivalid);
	pblock2->GetValue(eGeomDecalParam::uv_offset, t, uvOffset, ivalid);

	pblock2->GetValue(eGeomDecalParam::mesh_push, t, meshPush, ivalid);
	pblock2->GetValue(eGeomDecalParam::mesh_matid, t, meshMatID, ivalid);
}

void GeomDecal::UpdateControls(HWND hWnd, eGeomDecalRollout::Type rollout)
{
	switch (rollout)
	{
	case eGeomDecalRollout::general:
	{
		break;
	}
	default:
		break;
	}
}

Box3 GeomDecal::GetProjectionCage()
{
	return Box3(Point3(-projectionSize.x / 2.0f, -projectionSize.y / 2.0f, -projectionSize.z), Point3(projectionSize.x / 2.0f, projectionSize.y / 2.0f, 0.0f));
}

Matrix3 GeomDecal::DecalProjectionMatrix()
{
	Matrix3 scaleMatrix = Matrix3();
	scaleMatrix.SetScale(projectionSize);
	// TODO: Transform world local space, then scale.

	return scaleMatrix;
}

int GeomDecal::DrawAndHit(TimeValue t, INode* inode, ViewExp* vpt)
{
	GraphicsWindow *gw = vpt->getGW();

	if (inode->Selected())
	{
		gw->setColor(LINE_COLOR, GetUIColor(COLOR_SEL_GIZMOS));
	}
	else if (inode->IsFrozen() && vpt->IsWire())
	{
		gw->setColor(LINE_COLOR, GetUIColor(COLOR_FREEZE));
	}
	else if (vpt->IsWire())
	{
		gw->setColor(LINE_COLOR, GetUIColor(COLOR_GIZMOS));
	}
	else
	{
		return 0;
	}

	Point3 pp[3];

	vpt->getGW()->setTransform(inode->GetObjectTM(t));
	
	// Draw cage showing projection bounds.
	Box3 projectionCage = GetProjectionCage();

	Point3 topPoints[4];
	topPoints[0] = Point3(projectionCage.pmax.x, projectionCage.pmax.y, projectionCage.pmax.z);
	topPoints[3] = Point3(projectionCage.pmax.x, projectionCage.pmin.y, projectionCage.pmax.z);
	topPoints[1] = Point3(projectionCage.pmin.x, projectionCage.pmax.y, projectionCage.pmax.z);
	topPoints[2] = Point3(projectionCage.pmin.x, projectionCage.pmin.y, projectionCage.pmax.z);
	gw->polyline(4, topPoints, NULL, TRUE, NULL);

	Point3 bottomPoints[4];
	bottomPoints[0] = Point3(projectionCage.pmax.x, projectionCage.pmax.y, projectionCage.pmin.z);
	bottomPoints[1] = Point3(projectionCage.pmin.x, projectionCage.pmax.y, projectionCage.pmin.z);
	bottomPoints[2] = Point3(projectionCage.pmin.x, projectionCage.pmin.y, projectionCage.pmin.z);
	bottomPoints[3] = Point3(projectionCage.pmax.x, projectionCage.pmin.y, projectionCage.pmin.z);

	gw->polyline(4, bottomPoints, NULL, TRUE, NULL);

	pp[0] = topPoints[0];
	pp[1] = bottomPoints[0];
	gw->polyline(2, pp, NULL, FALSE, NULL);
	pp[0] = topPoints[1];
	pp[1] = bottomPoints[1];
	gw->polyline(2, pp, NULL, FALSE, NULL);
	pp[0] = topPoints[2];
	pp[1] = bottomPoints[2];
	gw->polyline(2, pp, NULL, FALSE, NULL);
	pp[0] = topPoints[3];
	pp[1] = bottomPoints[3];
	gw->polyline(2, pp, NULL, FALSE, NULL);

	// Draw arrow
	const float arrowSize = 0.1f * ((projectionCage.Width().x + projectionCage.Width().y) / 2);
	pp[0] = projectionCage.Center() * Point3(1.0f, 1.0f, 0.0f);
	pp[1] = pp[0] + Point3(0.0f, 0.0f, -arrowSize * 2);
	gw->polyline(2, pp, NULL, FALSE, NULL);
	pp[0] = pp[1] + Point3(arrowSize, arrowSize, arrowSize);
	pp[2] = pp[1] + Point3(-arrowSize, -arrowSize, arrowSize);
	gw->polyline(3, pp, NULL, FALSE, NULL);
	pp[0] = pp[1] + Point3(-arrowSize, arrowSize, arrowSize);
	pp[2] = pp[1] + Point3(arrowSize, -arrowSize, arrowSize);
	gw->polyline(3, pp, NULL, FALSE, NULL);

	return 1;
}

int GeomDecal::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags)
{
	return DrawAndHit(t, inode, vpt);
}

int GeomDecal::HitTest(TimeValue t, INode * inode, int type, int crossing, int flags, IPoint2 * p, ViewExp * vpt)
{
	Matrix3 tm(1);
	HitRegion hitRegion;
	DWORD savedLimits;
	Point3 pt(0, 0, 0);

	GraphicsWindow *gw = vpt->getGW();
	Material *mtl = gw->getMaterial();

	tm = inode->GetObjectTM(t);
	MakeHitRegion(hitRegion, type, crossing, 4, p);

	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK)&~GW_ILLUM);
	gw->setHitRegion(&hitRegion);
	gw->clearHitCode();

	DrawAndHit(t, inode, vpt);

	gw->setRndLimits(savedLimits);

	return gw->checkHitCode();
}

// The main function where we actually do the creation of the mesh.
void GeomDecal::BuildMesh(TimeValue t)
{
	// Reset current mesh
	mesh = Mesh();

	UpdateMemberVarsFromParamBlock(t);

	if (objectPool.length() <= 0)
		return; // Nothing to project onto

	// Find our own transform matrix.
	INode* selfNode = FindNodeFromRefTarget(this);
	//INode* selfNode = GetWorldSpaceObjectNode();

	Matrix3 ourTransform(1);

	if (selfNode != nullptr)
		ourTransform = Inverse(selfNode->GetObjectTM(t));

	Box3 projectionCage = GetProjectionCage();

	for (int i = 0; i < objectPool.length(); i++)
	{
		INode* node = objectPool[i];
		ObjectState os = node->EvalWorldState(t);

		if (os.obj->SuperClassID() != GEOMOBJECT_CLASS_ID)
			continue;

		GeomObject* obj = (GeomObject*)os.obj;
		TriObject* triObject = (TriObject *)obj->ConvertToType(t, Class_ID(TRIOBJ_CLASS_ID, 0));
		Mesh* currentMesh = &triObject->mesh;

		// Get the node's tm
		Matrix3 objectTM = node->GetObjectTM(t);
		Matrix3 toLocalSpace = objectTM * ourTransform;

		// Create a duplicate of the current mesh, transform it, slice it, then combine it with the existing mesh.
		Mesh tempMesh = Mesh();
		Mesh newMesh = Mesh(*currentMesh);

		// Transform verts
		for (int v = 0; v < newMesh.numVerts; v++)
			newMesh.verts[v] = newMesh.verts[v] * toLocalSpace;

		Point3 scale(toLocalSpace.GetColumn3(0).Length(), toLocalSpace.GetColumn3(1).Length(), toLocalSpace.GetColumn3(2).Length());
		Matrix3 scaleMatrix = Matrix3();
		scaleMatrix.SetScale(scale);

		// If we have been mirrored, flip normals.
		if (scale.x < 0 ^ scale.y < 0 ^ scale.z < 0)
		{
			for (int f = 0; f < newMesh.numFaces; f++)
			{
				newMesh.FlipNormal(f);
			}
		}

		Matrix3 rotationScaleMatrix = toLocalSpace;
		rotationScaleMatrix.SetRow(3, Point3(0, 0, 0));

		// Transform normals.
		const MeshNormalSpec* normalSpec = newMesh.GetSpecifiedNormals();
		if (normalSpec != NULL)
		{
			Point3* normals = normalSpec->GetNormalArray();

			for (int n = 0; n < normalSpec->GetNumNormals(); n++)
				normals[n] = (normals[n] * rotationScaleMatrix).Normalize();
		}

		// Slice mesh
		Point3 n = Point3(1, 0, 0);
		Point3 p = -(n * (projectionCage.Width() / 2)) + projectionCage.Center();
		SliceMesh(newMesh, n, DotProd(n, p), true, true);
		n = Point3(-1, 0, 0);
		p = -(n * (projectionCage.Width() / 2)) + projectionCage.Center();
		SliceMesh(newMesh, n, DotProd(n, p), true, true);
		n = Point3(0, 1, 0);
		p = -(n * (projectionCage.Width() / 2)) + projectionCage.Center();
		SliceMesh(newMesh, n, DotProd(n, p), true, true);
		n = Point3(0, -1, 0);
		p = -(n * (projectionCage.Width() / 2)) + projectionCage.Center();
		SliceMesh(newMesh, n, DotProd(n, p), true, true);
		n = Point3(0, 0, 1);
		p = -(n * (projectionCage.Width() / 2)) + projectionCage.Center();
		SliceMesh(newMesh, n, DotProd(n, p), true, true);
		n = Point3(0, 0, -1);
		p = -(n * (projectionCage.Width() / 2)) + projectionCage.Center();
		SliceMesh(newMesh, n, DotProd(n, p), true, true);


		// Discard any faces that do not intersect the projection cage, or beyond the angle cutoff
		BitArray facesToDiscard(newMesh.numFaces);
		const float dotCutoff = cos(angleCutoff * pi / 180.0f);
		newMesh.buildNormals();
		for (int f = 0; f < newMesh.numFaces; f++)
		{
			if (DotProd(newMesh.getFaceNormal(f), Point3(0, 0, 1)) < dotCutoff)
			{
				facesToDiscard.Set(f);
				continue;
			}

			Point3 tri[3];
			tri[0] = newMesh.getVert(newMesh.faces[f].getVert(0));
			tri[1] = newMesh.getVert(newMesh.faces[f].getVert(1));
			tri[2] = newMesh.getVert(newMesh.faces[f].getVert(2));

			if (!triBoxIntersect(projectionCage, tri))
				facesToDiscard.Set(f);
		}

		newMesh.DeleteFaceSet(facesToDiscard);

		// Update UV coords.
		newMesh.MakeMapPlanar(1);
		MeshMap& uvMap = newMesh.Map(1);
		for (int v = 0; v < uvMap.getNumVerts(); v++)
		{
			uvMap.tv[v] = (uvMap.tv[v] / projectionCage.Width() + 0.5f) * Point3(uvTiling) + uvOffset;
		}

		// Apply push
		// TODO: Better push
		newMesh.buildNormals();
		std::vector<Point3> averageFacenormals = std::vector<Point3>(newMesh.numVerts, Point3(0, 0, 0));
		for (int f = 0; f < newMesh.numFaces; f++)
		{
			Face face = newMesh.faces[f];
			for (int v = 0; v < 3; v++)
			{
				averageFacenormals[face.getVert(v)] += newMesh.getFaceNormal(f).Normalize();
			}
		}

		for (int v = 0; v < newMesh.numVerts; v++)
		{
			newMesh.verts[v] = newMesh.verts[v] + averageFacenormals[v].Normalize() * meshPush;
		}

		// Set Material ID
		for (int f = 0; f < newMesh.numFaces; f++)
		{
			newMesh.faces[f].setMatID(meshMatID - 1);
		}

		// Combine into our mesh.
		CombineMeshes(tempMesh, mesh, newMesh);
		mesh = tempMesh;
	}

	// Important to recompute the bounds of the new mesh.
	mesh.buildBoundingBox();

	AddXTCObject(new GeomDecalXTC());
}

void GeomDecal::InvalidateUI()
{
	GeomDecalParamBlockDesc.InvalidateUI();
}

CreateMouseCallBack* GeomDecal::GetCreateMouseCallBack()
{
	static GeomDecalCreateCallBack createCallback;
	createCallback.SetObj(this);
	return &createCallback;
}

void GeomDecal::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
{
	box = TransformBox(GetProjectionCage(), inode->GetObjectTM(t));
}

void GeomDecal::GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
{
	box = GetProjectionCage();
}

void GeomDecal::GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel)
{
	box = TransformBox(GetProjectionCage(), *tm);
}

RefTargetHandle GeomDecal::Clone(RemapDir& remap)
{
	GeomDecal* newob = new GeomDecal(FALSE);
	newob->ReplaceReference(0, remap.CloneRef(pblock2));
	newob->ivalid.SetEmpty();
	BaseClone(this, newob, remap);
	return(newob);
}

void GeomDecal::BeginEditParams(IObjParam * ip, ULONG flags, Animatable * prev)
{
	this->ip = ip;

	SimpleObject2::BeginEditParams(ip, flags, prev);

	GeomDecalClassDesc::GetInstance()->BeginEditParams(ip, this, flags, prev);

	GeomDecalParamBlockDesc.SetUserDlgProc(eGeomDecalRollout::general, new GeomDecalGeneralDlgProc(this));
}

void GeomDecal::EndEditParams(IObjParam * ip, ULONG flags, Animatable * next)
{
	SimpleObject2::EndEditParams(ip, flags, next);

	GeomDecalClassDesc::GetInstance()->EndEditParams(ip, this, flags, next);

	ip->ClearPickMode();
	this->ip = NULL;
}

void GeomDecalXTC::PreChanChangedNotify(TimeValue t, ModContext & mc, ObjectState * os, INode * node, Modifier * mod, bool bEndOfPipeline)
{
	int i = 0;
	return;
}

void GeomDecalXTC::PostChanChangedNotify(TimeValue t, ModContext & mc, ObjectState * os, INode * node, Modifier * mod, bool bEndOfPipeline)
{
	int i = 5;
	return;
}