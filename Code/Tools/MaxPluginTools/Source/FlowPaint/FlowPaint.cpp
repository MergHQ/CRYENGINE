#include "stdafx.h"

#include "FlowPaint.h"
#include "FlowPaintClassDesc.h"
#include "FlowPaintModData.h"
#include "FlowPaintRestore.h"
#include "FlowPaintParamBlock.h"
#include "FlowPaintDlgProc.h"
#include "FlowPaintParamBlock.h"
#include "TangentSpaceCalculator/TangentInfo.h"

IObjParam *FlowPaint::ip = NULL;

#define FLIP_FLOW_X
///#define FLIP_FLOW_Y

Point2 GetFlowVector(int face, int vertex, MNMesh* mesh)
{
	UVVert vertColor = GetMapVert(face, vertex, 0, mesh);
	Point2 flowVec = Point2(vertColor.x, vertColor.y) * 2.0f + -1.0f;
#ifdef FLIP_FLOW_X
	flowVec.x = -flowVec.x;
#endif
#ifdef FLIP_FLOW_Y
	flowVec.y = -flowVec.y;
#endif
	return flowVec;
}

void SetFlowVector(Point2 flowVec, int face, int vertex, MNMesh* mesh)
{
#ifdef FLIP_FLOW_X
	flowVec.x = -flowVec.x;
#endif
#ifdef FLIP_FLOW_Y
	flowVec.y = -flowVec.y;
#endif
	UVVert vertColor = GetMapVert(face, vertex, 0, mesh);
	Point2 transformedFlowVector = (flowVec + 1) / 2;
	vertColor.x = transformedFlowVector.x;
	vertColor.y = transformedFlowVector.y;

	SetMapVert(vertColor, face, vertex, 0, mesh);
}

class MyEnumProc : public DependentEnumProc
{
public:
	INodeTab Nodes;
	int count;

	virtual int proc(ReferenceMaker *rmaker)
	{
		if (rmaker->SuperClassID() == BASENODE_CLASS_ID)
		{
			Nodes.Append(1, (INode **)&rmaker);
			count++;
			return DEP_ENUM_SKIP;
		}
		return DEP_ENUM_CONTINUE;
	}
};

//--- FlowPaint -------------------------------------------------------
FlowPaint::FlowPaint() : Modifier()
	, tangentInfo()
	, tangentInfoDirty(true)
{
	pblock = NULL;
	FlowPaintTestClassDesc::GetInstance()->MakeAutoParamBlocks(this);
}

FlowPaint::~FlowPaint()
{
	FreeObjStates();
}

/*===========================================================================*\
 |	The validity of the parameters.  First a test for editing is performed
 |  then Start at FOREVER, and intersect with the validity of each item
\*===========================================================================*/
Interval FlowPaint::LocalValidity(TimeValue t)
{
	// if being edited, return NEVER forces a cache to be built 
	// after previous modifier.
	if (TestAFlag(A_MOD_BEING_EDITED))
		return NEVER;
	//TODO: Return the validity interval of the modifier
	return NEVER;
}

int FlowPaint::Display(TimeValue t, INode* inode, ViewExp* vpt, int flags, ModContext* mc)
{
	PaintDefromModData* modData = (PaintDefromModData*)mc->localData;

	if (!modData)
		return 1;

	GraphicsWindow *gw = vpt->getGW();
	ViewExp13* viewExp13 = (ViewExp13*)vpt;
	Matrix3 tm = inode->GetObjectTM(t);
	int savedLimits = gw->getRndLimits();

	gw->setRndLimits(savedLimits & ~GW_ILLUM);
	gw->setTransform(tm);

	gw->startSegments();
	
	for (int i = 0; i < painterNodeList.Count(); i++)
	{
		Matrix3 tm = painterNodeList[i].tmToLocalSpace;

		int cleanupTriObject;
		PolyObject* triObject = GetPolyObjectFromNode(painterNodeList[i].node, t, cleanupTriObject);
		MNMesh& mesh = triObject->GetMesh();
		MNMap* colorMap = mesh.M(0);

		if (modData->offsetList.Count() != colorMap->numv)
			continue;

		//gw->setColor(LINE_COLOR, Point3(0, 0, 1));
		//	Matrix3 tm = painterNodeList[i].tmToLocalSpace;
		//	for (int j = 0; j < displayVertPositions.length(); j++)
		//	{
		//		Point3 segment[2];
		//		segment[0] = displayVertPositions[j];
		//		segment[1] = displayVertPositions[j] + tangentInfo.normals[j];
		//		gw->segment(segment, 1);
		//	}
		//gw->setColor(LINE_COLOR, Point3(1, 0, 0));
		//	Matrix3 tm = painterNodeList[i].tmToLocalSpace;
		//	for (int j = 0; j < displayVertPositions.length(); j++)
		//	{
		//		Point3 segment[2];
		//		segment[0] = displayVertPositions[j];
		//		segment[1] = displayVertPositions[j] + tangentInfo.tangents[j];
		//		gw->segment(segment, 1);
		//	}
		//gw->setColor(LINE_COLOR, Point3(0, 1, 0));
		//	Matrix3 tm = painterNodeList[i].tmToLocalSpace;
		//	for (int j = 0; j < displayVertPositions.length(); j++)
		//	{
		//		Point3 segment[2];
		//		segment[0] = displayVertPositions[j];
		//		segment[1] = displayVertPositions[j] + tangentInfo.binormals[j];
		//		gw->segment(segment, 1);
		//	}

		gw->setColor(LINE_COLOR, Point3(1, 0, 0));
		
		for (int f = 0; f < colorMap->numf; f++)
		{
			MNMapFace& face = colorMap->f[f];
			for (int v = 0; v < face.deg; v++)
			{
				const Point3 vertPosition = mesh.v[mesh.f[f].vtx[v]].p;
				const Point2 flowVect = GetFlowVector(f, v, &mesh);
				const Point3 localFlowVect = tangentInfo.FromTangentSpace(Point3(flowVect.x, flowVect.y, 0.0f), f, v);

				Point3 segment[2];
				segment[0] = vertPosition;
				segment[1] = vertPosition + localFlowVect * displayLength;
				gw->segment(segment, 1);
			}
		}

		//gw->setColor(LINE_COLOR, Point3(0, 1, 0));
		//for (int f = 0; f < colorMap->numf; f++)
		//{
		//	MNMapFace& face = colorMap->f[f];
		//	for (int v = 0; v < face.deg; v++)
		//	{
		//		const Point3 vertPosition = mesh.v[mesh.f[f].vtx[v]].p;
		//		//const Point2 flowVect = GetFlowVector(f, v, &mesh);
		//		//const Point3 localFlowVect = tangentInfo.FromTangentSpace(Point3(flowVect.x, flowVect.y, 0.0f), f, v);
		//		const Point3 localFlowVect = modData->offsetList[face.tv[v]];

		//		Point3 segment[2];
		//		segment[0] = vertPosition;
		//		segment[1] = vertPosition + localFlowVect;
		//		gw->segment(segment, 1);
		//	}
		//}

		if (cleanupTriObject)
			triObject->DeleteMe();
	}

	gw->endSegments();

	// Reset render limits
	gw->setRndLimits(savedLimits);
	// Purposely don't call super, we don't want orange edges drawn when show end result is enabled.
	//return SubSelectionPolyMod::Display(t, inode, vpt, flags, mc);
	return 0;
}

void FlowPaint::ModifyObject(TimeValue t, ModContext &mc, ObjectState* os, INode *node)
{
	if (!os->obj->IsSubClassOf(polyObjectClassID))
		return;

	Interval ivalid = FOREVER;
	pblock->GetValue(eFlowPaintParam::show_flow_length, t, displayLength, ivalid);

	PolyObject *pobj = (PolyObject*)os->obj;
	MNMesh& mesh = pobj->GetMesh();
	

	int v = mesh.numv;
	PaintDefromModData *pmd = NULL;
	if (mc.localData == NULL)
	{
		pmd = new PaintDefromModData();
		mc.localData = (LocalModData*)pmd;
	}
	else
		pmd = (PaintDefromModData *)mc.localData;

	if (tangentInfoDirty)
	{
		tangentInfo.Update(&mesh);
		tangentInfoDirty = false;
	}

	// Make sure there is a color map there.
	InitializeMap(0, &mesh, false);
	
	// If mesh indexes have changed, clear offset list.
	MNMap* colorMap = mesh.M(0);

	if (!colorMap->CheckAllData(0, mesh.numf, mesh.f))
	{
		int error = 1;
	}

	if (pmd->offsetList.Count() != colorMap->numv)
	{
		pmd->offsetList.SetCount(colorMap->numv);
		tangentInfo.Update(&mesh);

		// Load existing flow vectors saved in map color channel
		for (int f = 0; f < colorMap->numf; f++)
		{
			MNMapFace& face = colorMap->f[f];

			for (int v = 0; v < face.deg; v++)
			{
				Point2 flowVec = GetFlowVector(f, v, &mesh);
				int texVert = face.tv[v];
				pmd->offsetList[texVert] = tangentInfo.FromTangentSpace(Point3(flowVec.x, flowVec.y, 0.0f), f, v);
			}
		}
	}
	
	// Apply offset list to mesh
	for (int f = 0; f < colorMap->numf; f++)
	{
		MNMapFace& face = colorMap->f[f];

		for (int v = 0; v < face.deg; v++)
		{
			const Point3 localSpaceFlowVector = pmd->offsetList[face.tv[v]];
			const Point3 tangentSpaceFlowVector = tangentInfo.ToTangentSpace(localSpaceFlowVector, f, v);
			const Point2 flowVector = Point2(tangentSpaceFlowVector.x, tangentSpaceFlowVector.y);
			SetFlowVector(flowVector, f, v, &mesh);
		}
	}
	mesh.InvalidateHardwareMesh();

	//// Caches for debug displaying
	//displayVertPositions.setLengthUsed(mesh.numv);
	//displayFlowVectors.setLengthUsed(mesh.numv);
	//displayVertColors.setLengthUsed(mesh.numv);
	//for (int i = 0; i < mesh.numv; i++)
	//{
	//	displayVertPositions[i] = mesh.v[i].p;
	//	displayFlowVectors[i] = GetLocalSpaceFlowVector(i, tangentInfo) * displayLength;
	//	Color vertCol = mesh.MV(0, i);
	//	displayVertColors[i] = 1 - vertCol;
	//}

	os->obj->UpdateValidity(GEOM_CHAN_NUM, FOREVER);
}

void FlowPaint::BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev)
{
	tangentInfoDirty = true;
	this->ip = ip;
	FlowPaintTestClassDesc::GetInstance()->BeginEditParams(ip, this, flags, prev);
	flowPaintParamBlockDesc.SetUserDlgProc(new PaintDeformTestDlgProc(this));

	pPainter = NULL;

	ReferenceTarget *painterRef = (ReferenceTarget*)GetCOREInterface()->CreateInstance(REF_TARGET_CLASS_ID, PAINTERINTERFACE_CLASS_ID);

	//Set it to the correct version
	if (painterRef)
	{
		pPainter = (IPainterInterface_V7 *)painterRef->GetInterface(PAINTERINTERFACE_V7);
	}

	TimeValue t = ip->GetTime();

	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_BEGIN_EDIT);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_MOD_DISPLAY_ON);
	SetAFlag(A_MOD_BEING_EDITED);
}

void FlowPaint::EndEditParams(IObjParam *ip, ULONG flags, Animatable *next)
{
	FlowPaintTestClassDesc::GetInstance()->EndEditParams(ip, this, flags, next);
	this->ip = NULL;
	// NOTE: This flag must be cleared before sending the REFMSG_END_EDIT
	ClearAFlag(A_MOD_BEING_EDITED);

	TimeValue t = ip->GetTime();

	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_END_EDIT);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);
}

Interval FlowPaint::GetValidity(TimeValue t)
{
	Interval valid = FOREVER;
	//TODO: Return the validity interval of the modifier
	return valid;
}

RefTargetHandle FlowPaint::Clone(RemapDir& remap)
{
	FlowPaint* newmod = new FlowPaint();
	newmod->ReplaceReference(PBLOCK_REF, remap.CloneRef(pblock));
	BaseClone(this, newmod, remap);
	return(newmod);
}

//From ReferenceMaker 
RefResult FlowPaint::NotifyRefChanged(
	const Interval& changeInt, RefTargetHandle hTarget,
	PartID& partID, RefMessage message, BOOL propagate)
{
	//TODO: Add code to handle the various reference changed messages
	return REF_SUCCEED;
}

//just some function to handle UI creatation and destruction
void FlowPaint::InitUI(HWND hWnd)
{
	iPaintButton = GetICustButton(GetDlgItem(hWnd, IDC_PAINT));
	iPaintButton->SetType(CBT_CHECK);
	iPaintButton->SetHighlightColor(GREEN_WASH);
}
void FlowPaint::DestroyUI(HWND hWnd)
{
	ReleaseICustButton(iPaintButton);
	iPaintButton = NULL;
	if (pPainter)
	{
		if (pPainter->InPaintMode())
			pPainter->EndPaintSession();
	}
}

PaintDefromModData *FlowPaint::GetPMD(INode *pNode)
{
	ModContext *mc = NULL;

	Object* pObj = pNode->GetObjectRef();

	if (!pObj) return NULL;


	while (pObj->SuperClassID() == GEN_DERIVOB_CLASS_ID && mc == NULL)
	{
		IDerivedObject* pDerObj = (IDerivedObject *)(pObj);

		int Idx = 0;

		while (Idx < pDerObj->NumModifiers())
		{
			// Get the modifier. 
			Modifier* mod = pDerObj->GetModifier(Idx);


			if (mod->ClassID() == PAINTDEFORMTEST_CLASS_ID)
			{
				// is this the correct Physique Modifier based on index?
				FlowPaint *pmod = (FlowPaint*)mod;
				if (pmod == this)
				{
					mc = pDerObj->GetModContext(Idx);
					break;
				}
			}

			Idx++;
		}

		pObj = pDerObj->GetObjRef();
	}

	if (!mc) return NULL;

	if (!mc->localData) return NULL;

	PaintDefromModData *bmd = (PaintDefromModData *)mc->localData;

	return bmd;
}

//Some helper functions to handle painter UI
void FlowPaint::Paint()
{
	if (pPainter) //need to check this since some one could have taken the painterinterface plugin out
	{
		if (!pPainter->InPaintMode())
		{
			pPainter->InitializeCallback(this); //initialize the callback
			//load up our nodes
			//gather up all our nodes and local data and store em off
			MyEnumProc dep;
			DoEnumDependents(&dep);
			Tab<INode *> nodes;
			painterNodeList.ZeroCount();
			TimeValue t = GetCOREInterface()->GetTime();
			for (int i = 0; i < dep.Nodes.Count(); i++)
			{
				PaintDefromModData *pmd = GetPMD(dep.Nodes[i]);

				if (pmd)
				{
					nodes.Append(1, &dep.Nodes[i]);
					PainterNodeList temp;
					temp.node = dep.Nodes[i];
					temp.pmd = pmd;
					temp.tmToLocalSpace = Inverse(dep.Nodes[i]->GetObjectTM(GetCOREInterface()->GetTime()));
					painterNodeList.Append(1, &temp);
					ObjectState sos;
					
					TriObject* triObj = CreateNewTriObject();
					BOOL deleteIt = FALSE;
					TriObject* nobj = GetTriObjectFromNode(dep.Nodes[i], t, deleteIt);
					if (nobj)
					{
						triObj->mesh.CopyBasics(nobj->mesh);
						triObj->SetChannelLocks(ALL_CHANNELS);
						if (deleteIt)
							nobj->DeleteMe();
					}
					ObjectState os(triObj);
					objstates.Append(1, &os);
				}
			}

			//we use the point gather so we need to tell the system to turn it on
			pPainter->SetEnablePointGather(TRUE);
			pPainter->SetBuildNormalData(TRUE);

			//this sends all our dependant nodes to the painter
			pPainter->InitializeNodesByObjState(0, nodes, objstates);

			BOOL updateMesh = FALSE;
			for (int i = 0; i < nodes.Count(); i++)
			{

				ObjectState os = nodes[i]->EvalWorldState(GetCOREInterface()->GetTime());

				int ct = os.obj->NumPoints();
				//is our local vertex count does not match or the curremt object count we have some
				//sort of topo change above us so we need to load a custom point list
				//We really need add normals here
				//so right now this does not work with patches, nurbs or things that have different  topos at the top
				//top of the stack
				if (os.obj->NumPoints() != painterNodeList[i].pmd->offsetList.Count())
				{
					Tab<Point3> pointList;
					pointList.SetCount(ct);
					Matrix3 tm = nodes[i]->GetObjectTM(GetCOREInterface()->GetTime());
					for (int j = 0; j < ct; j++)
					{
						pointList[j] = os.obj->GetPoint(j)*tm;
					}
					pPainter->LoadCustomPointGather(ct, pointList.Addr(0), nodes[i]);
					updateMesh = TRUE;
				}
			}
			//reinitialize our nodes if we had a custom list
			if (updateMesh)
				pPainter->UpdateMeshesByObjState(TRUE, objstates);

			pPainter->StartPaintSession(); //start the paint session
			iPaintButton->SetCheck(TRUE);
		}
		else //we are currently in a paint mode so turn it off
		{
			pPainter->EndPaintSession(); //end the paint session
			iPaintButton->SetCheck(FALSE);
		}
	}
}

void FlowPaint::PaintOptions()
{
	if (pPainter) //need to check this since some one could have taken the painterinterface plugin out
	{
		pPainter->BringUpOptions();
	}

}

#define POINTCOUNT_CHUNK 0x1000
#define POINT_CHUNK 0x1010

IOResult FlowPaint::SaveLocalData(ISave *isave, LocalModData *pld)
{
	ULONG nb;
	PaintDefromModData *pmd = (PaintDefromModData*)pld;

	int ct = pmd->offsetList.Count();

	isave->BeginChunk(POINTCOUNT_CHUNK);
	isave->Write(&ct, sizeof(ct), &nb);
	isave->EndChunk();

	isave->BeginChunk(POINT_CHUNK);
	isave->Write(pmd->offsetList.Addr(0), sizeof(Point3)*ct, &nb);
	isave->EndChunk();
	return IO_OK;
}

IOResult FlowPaint::LoadLocalData(ILoad *iload, LocalModData **pld)
{
	IOResult res;
	ULONG nb;

	PaintDefromModData *pmd = new PaintDefromModData();
	*pld = pmd;

	int ct = 0;

	while (IO_OK == (res = iload->OpenChunk()))
	{
		switch (iload->CurChunkID())
		{
		case POINTCOUNT_CHUNK:
		{
			iload->Read(&ct, sizeof(ct), &nb);
			pmd->offsetList.SetCount(ct);
			break;
		}
		case POINT_CHUNK:
		{
			iload->Read(pmd->offsetList.Addr(0), sizeof(Point3)*ct, &nb);
			break;
		}
		}
		iload->CloseChunk();
		if (res != IO_OK) return res;
	}

	return IO_OK;
}

void* FlowPaint::GetInterface(ULONG id)
{
	switch (id)
	{
	case PAINTERCANVASINTERFACE_V5: return (IPainterCanvasInterface_V5 *) this;
		break;
	default: return Modifier::GetInterface(id);
		break;
	}
}

BOOL FlowPaint::StartStroke()
{
	//start holding
	theHold.Begin();
	for (int i = 0; i < painterNodeList.Count(); i++)
	{
		theHold.Put(new PointRestore(this, painterNodeList[i].pmd));
	}
	//put our hold records down
	lagRate = pPainter->GetLagRate();
	lagCount = 1;

	lastStrokePointValid = false;

	return TRUE;
}

BOOL FlowPaint::DoStroke(
	BOOL hit,
	IPoint2 mousePos,
	Point3 worldPoint,
	Point3 worldNormal,
	Point3 localPoint,
	Point3 localNormal,
	Point3 bary,
	int index,
	BOOL shift,
	BOOL ctrl,
	BOOL alt,
	float radius,
	float str,
	float pressure,
	INode* node,
	BOOL mirrorOn,
	Point3 worldMirrorPoint,
	Point3 worldMirrorNormal,
	Point3 localMirrorPoint,
	Point3 localMirrorNormal)
{
	ICurve* curve = pPainter->GetFalloffGraph();

	for (int i = 0; i < painterNodeList.Count(); i++)
	{
		radius /= 2; // Passed in radius seems to actually be diameter.
		
		if (lastStrokePointValid)
		{
			int cleanupTriObject;
			PolyObject* triObject = GetPolyObjectFromNode(painterNodeList[i].node, 0, cleanupTriObject);
			MNMesh& mesh = triObject->GetMesh();
			MNMap* colorMap = mesh.M(0);

			Matrix3 tm = painterNodeList[i].tmToLocalSpace;
			Point3 brushDirection = VectorTransform(tm, worldPoint - lastStrokePoint);
			Point3 localBrushPosition = tm * worldPoint;

			PaintDefromModData* modData = painterNodeList[i].pmd;

			Tab<float> strengths;
			strengths.SetCount(modData->offsetList.Count());

			// Get strengths
			for (int f = 0; f < colorMap->numf; f++)
			{
				MNMapFace& mapFace = colorMap->f[f];
				MNFace& meshFace = mesh.f[f];

				for (int v = 0; v < mapFace.deg; v++)
				{
					const Point3 vertPosition = mesh.v[meshFace.vtx[v]].p;
					const float pointDistance = (vertPosition - localBrushPosition).Length();
					const int offsetIndex = mapFace.tv[v];

					if (pointDistance < radius)
					{
						const float strength = curve->GetValue(0, pointDistance / radius) * str;
						strengths[offsetIndex] = strength;
					}
					else
					{
						strengths[offsetIndex] = 0;
					}
				}
			}

			// Apply
			for (int s = 0; s < strengths.Count(); s++)
			{
				modData->offsetList[s] += brushDirection * strengths[s];
				modData->offsetList[s] = modData->offsetList[s].Normalize();
			}


			/*for (int j = 0; j < count; j++)
			{
				const float pointDistance = (*points - localBrushPosition).Length();

				if (pointDistance < radius)
				{
					const float strength = curve->GetValue(0, pointDistance / radius) * str;

					if (strength > 0.0f)
					{
						painterNodeList[i].pmd->offsetList[j] += brushDirection * strength;
						painterNodeList[i].pmd->offsetList[j] = painterNodeList[i].pmd->offsetList[j].Normalize();
					}
				}
				strengths++;
				points++;
			}*/
			if (cleanupTriObject)
				triObject->DeleteMe();
		}
	}
	lastStrokePointValid = true;
	lastStrokePoint = worldPoint;

	return TRUE;
}

//This is called as the user strokes across the mesh or screen with the mouse down
BOOL FlowPaint::PaintStroke(BOOL hit, IPoint2 mousePos, Point3 worldPoint, Point3 worldNormal, Point3 localPoint, Point3 localNormal, Point3 bary, int index, BOOL shift, BOOL ctrl, BOOL alt, float radius, float str, float pressure, INode *node, BOOL mirrorOn, Point3 worldMirrorPoint, Point3 worldMirrorNormal, Point3 localMirrorPoint, Point3 localMirrorNormal)
{
	//theHold.Restore();

	DoStroke(hit, mousePos, worldPoint, worldNormal, localPoint, localNormal, bary, index, shift, ctrl, alt, radius, str, pressure, node, mirrorOn, worldMirrorPoint, worldMirrorNormal, localMirrorPoint, localMirrorNormal);

	if ((lagRate == 0) || (lagCount%lagRate) == 0)
		NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);

	lagCount++;
	return TRUE;
}

// This is called as the user ends a strokes when the users has it set to always update
BOOL FlowPaint::EndStroke()
{
	//accept hold
	NotifyDependents(FOREVER, PART_GEOM, REFMSG_CHANGE);
	theHold.Accept(GetString(IDS_PW_PAINT));
	TimeValue t = GetCOREInterface()->GetTime();
	for (int i = 0; i < painterNodeList.Count(); i++)
	{
		if (objstates[i].obj)
		{
			TriObject* triObj = static_cast<TriObject*>(objstates[i].obj);
			BOOL deleteIt = FALSE;
			TriObject* nobj = GetTriObjectFromNode(painterNodeList[i].node, t, deleteIt);
			if (nobj)
			{
				triObj->mesh.CopyBasics(nobj->mesh);
				triObj->SetChannelLocks(ALL_CHANNELS);
				if (deleteIt)
					nobj->DeleteMe();
			}
		}
	}

	pPainter->UpdateMeshesByObjState(TRUE, objstates);
	
	return TRUE;
}
// This is called as the user ends a strokes when the users has it set to update on mouse up only
// the canvas gets a list of all points, normals etc instead of one at a time
//		int ct - the number of elements in the following arrays
//  <...> see paintstroke() these are identical except they are arrays of values
BOOL FlowPaint::EndStroke(int ct, BOOL *hit, IPoint2 *mousePos, Point3 *worldPoint, Point3 *worldNormal, Point3 *localPoint, Point3 *localNormal, Point3 *bary, int *index, BOOL *shift, BOOL *ctrl, BOOL *alt, float *radius, float *str, float *pressure, INode **node, BOOL mirrorOn, Point3 *worldMirrorPoint, Point3 *worldMirrorNormal, Point3 *localMirrorPoint, Point3 *localMirrorNormal)
{
	// Not currently supported.
	return FALSE;
	for (int i = 0; i < ct; i++)
	{
		DoStroke(hit[i],
			mousePos[i],
			worldPoint[i],
			worldNormal[i],
			localPoint[i],
			localNormal[i],
			bary[i],
			index[i],
			shift[i],
			ctrl[i],
			alt[i],
			radius[i],
			str[i],
			pressure[i],
			node[i],
			mirrorOn,
			worldMirrorPoint[i],
			worldMirrorNormal[i],
			localMirrorPoint[i],
			localMirrorNormal[i]);
	}

	return EndStroke();
}

// This is called as the user cancels a stroke by right clicking
BOOL FlowPaint::CancelStroke()
{
	//cancel hold
	theHold.Cancel();
	return TRUE;
}

//This is called when the painter want to end a paint session for some reason.
BOOL FlowPaint::SystemEndPaintSession()
{
	return TRUE;
}
