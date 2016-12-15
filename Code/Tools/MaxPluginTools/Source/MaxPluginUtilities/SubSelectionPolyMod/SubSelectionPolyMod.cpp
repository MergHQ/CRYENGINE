#include "stdafx.h"

#include "SubSelectionPolyMod.h"
#include "PolyMeshSelData.h"
#include "SelectRestore.h"

#include "stdafx.h"
#include "objmode.h"
#include <ICUIMouseConfigManager.h>
#include "ifnpub.h"
#include "gfx.h"
#include "SubObjAxisCallback.h"
#include <map>

//selection box , fixed sized , no matter where the object is 
#define DEF_PICKBOX_SIZE	4

static SubObjectType vertSelection(_T("Vertex"), 1, MNM_SL_VERTEX, SUBHIT_MNVERTS, MNDISP_VERTTICKS | MNDISP_SELVERTS, SubSelectionPolyMod::SEL_VERTEX);
static SubObjectType edgeSelection(_T("Edge"), 2, MNM_SL_EDGE, SUBHIT_MNEDGES, MNDISP_SELEDGES, SubSelectionPolyMod::SEL_EDGE);
static SubObjectType borderSelection(_T("Border"), 9, MNM_SL_EDGE, SUBHIT_MNEDGES | SUBHIT_OPENONLY, MNDISP_SELEDGES, SubSelectionPolyMod::SEL_BORDER);
static SubObjectType faceSelection(_T("Face"), 4, MNM_SL_FACE, SUBHIT_MNFACES, MNDISP_SELFACES, SubSelectionPolyMod::SEL_FACE);
static SubObjectType elementSelection(_T("Element"), 5, MNM_SL_FACE, SUBHIT_MNFACES, MNDISP_SELFACES, SubSelectionPolyMod::SEL_ELEMENT);

static std::map<int, SubObjectType*> CreateSubObjectTypeMap()
{
	std::map<int, SubObjectType*> map;
	map[SubSelectionPolyMod::SEL_VERTEX] = &vertSelection;	
	map[SubSelectionPolyMod::SEL_EDGE] = &edgeSelection;	
	map[SubSelectionPolyMod::SEL_BORDER] = &borderSelection;	
	map[SubSelectionPolyMod::SEL_FACE] = &faceSelection;	
	map[SubSelectionPolyMod::SEL_ELEMENT] = &elementSelection;
	return map;
}

const std::map<int, SubObjectType*> subObjectTypes = CreateSubObjectTypeMap();
static SelectModBoxCMode* selectMode = NULL;
static IObjParam* ip = NULL;
static SubSelectionPolyMod* editMod = NULL;

SubSelectionPolyMod::SubSelectionPolyMod(int SubSelectionTypes) : 
	enabledSubSelectionTypes(SubSelectionTypes)
{
	currentSubSelectionType = SEL_OBJECT;
}

SubObjectType* SubSelectionPolyMod::GetSubSelection(int type)
{
	return subObjectTypes.at(type);
}

SubObjectType* SubSelectionPolyMod::GetCurrentSubSelection()
{
	if (subObjectTypes.count(currentSubSelectionType) == 0)
		return NULL;
	else
		return subObjectTypes.at(currentSubSelectionType);
}

SubSelectionPolyMod::SelectionTypeFlag SubSelectionPolyMod::GetTypeFlagFromLevel(int level)
{
	if (level == 0)
		return SEL_OBJECT; // Level 0 is always object selection.

	// Loop through the enum and count how many flags are enabled. When we reach the desired index, return it.
	int count = 0;
	for (int i = 0; i < SelectionjTypeQty; i++)
	{
		if (enabledSubSelectionTypes & 1 << i)
			count++;

		if (count == level)
			return SelectionTypeFlag(1 << i);
	}
	return SEL_OBJECT; // Means an invalid level was specified.
}

void SubSelectionPolyMod::SelectOpenEdges(int selType)
{
	if (!ip) return;
	bool localHold = false;
	if (!theHold.Holding()) 
	{
		localHold = true;
		theHold.Begin();
	}
	ModContextList list;
	INodeTab nodes;
	ip->GetModContexts(list, nodes);
	PolyMeshSelData* d = NULL;
	for (int i = 0; i<list.Count(); i++) 
	{
		d = (PolyMeshSelData*)list[i]->localData;
		if (!d) continue;
		if (!d->mesh) continue;

		if (!d->held) theHold.Put(new SelectRestore(this, d));
		d->SynchBitArrays();

		for (int j = 0; j<d->mesh->ENum(); j++) {
			MNEdge *me = d->mesh->E(j);
			if (me->GetFlag(MN_DEAD)) continue;
			if (me->f2 < 0) {
				if (selType < 0) continue;
				me->SetFlag(MN_SEL);
				d->edgeSel.Set(j);
			}
			else {
				if (selType > 0) continue;
				me->ClearFlag(MN_SEL);
				d->edgeSel.Clear(j);
			}
		}
	}
	nodes.DisposeTemporary();
	if (localHold) theHold.Accept(_T("Select Open Edges"));
	LocalDataChanged();
	ip->RedrawViews(ip->GetTime());
}

int SubSelectionPolyMod::HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext* mc) {

	if (!vpt || !vpt->IsAlive())
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	Interval valid;
	int savedLimits, res = 0;
	GraphicsWindow *gw = vpt->getGW();
	HitRegion hr;

	bool selByVert = false;
	bool ignoreBackfaces = false;

	// Setup GW
	MakeHitRegion(hr, type, crossing, DEF_PICKBOX_SIZE, p);
	gw->setHitRegion(&hr);
	Matrix3 mat = inode->GetObjectTM(t);
	gw->setTransform(mat);
	gw->setRndLimits(((savedLimits = gw->getRndLimits()) | GW_PICK) & ~GW_ILLUM);
	if (ignoreBackfaces) 
		gw->setRndLimits(gw->getRndLimits() | GW_BACKCULL);
	else 
		gw->setRndLimits(gw->getRndLimits() & ~GW_BACKCULL);
	gw->clearHitCode();

	SubObjHitList hitList;

	if (!mc->localData || GetCurrentSubSelection() == NULL)
		return 0;

	PolyMeshSelData *pData = (PolyMeshSelData *)mc->localData;
	//pData->SetConverterFlag (MNM_SELCONV_IGNORE_BACK, ignoreBackfaces?true:false);
	pData->SetConverterFlag(MNM_SELCONV_REQUIRE_ALL, (crossing || (type == HITTYPE_POINT)) ? false : true);

	MNMesh *pMesh = pData->GetMesh();
	if (!pMesh) 
		return 0;
	if ((GetCurrentSubSelectionType() > SEL_VERTEX) && selByVert) 
		res = pMesh->SubObjectHitTest(gw, gw->getMaterial(), &hr, flags | GetSubSelection(SEL_VERTEX)->hitLevel | SUBHIT_MNUSECURRENTSEL, hitList);
	else
	{
		Material* mat = gw->getMaterial();
		pMesh->SubObjectHitTest(gw, mat, &hr, flags | GetCurrentSubSelection()->hitLevel, hitList);
		//res = pMesh->SubObjectHitTest(gw, gw->getMaterial(), &hr, flags | GetCurrentSubSelection()->hitLevel, hitList);
	}
	
#if MAX_API_NUM <= 46 // 3ds max 2016
	MeshSubHitRec* rec = NULL;
	rec = hitList.First();
	while (rec) {
		vpt->LogHit(inode, mc, rec->dist, rec->index, NULL);
		rec = rec->Next();
	}
#else
	for (MeshSubHitRec rec : hitList)
	{
		vpt->LogHit(inode, mc, rec.dist, rec.index, NULL);
	}
#endif
	

	gw->setRndLimits(savedLimits);
	return res;
}

int SubSelectionPolyMod::Display(TimeValue t, INode* inode, ViewExp *vpt, int flags, ModContext *mc) 
{

	if (!vpt || !vpt->IsAlive())
	{
		// why are we here
		DbgAssert(!_T("Invalid viewport!"));
		return FALSE;
	}

	if (!ip || !ip->GetShowEndResult()) 
		return 0;
	if (GetCurrentSubSelectionType() == SEL_OBJECT)
		return 0;
	if (!mc->localData) 
		return 0;

	PolyMeshSelData *modData = (PolyMeshSelData *)mc->localData;
	MNMesh *mesh = modData->GetMesh();
	if (!mesh) 
		return 0;

	// Set up GW
	GraphicsWindow *gw = vpt->getGW();
	Matrix3 tm = inode->GetObjectTM(t);
	int savedLimits;
	gw->setRndLimits((savedLimits = gw->getRndLimits()) & ~GW_ILLUM);
	gw->setTransform(tm);

	// We need to draw a "gizmo" version of the mesh:
	Point3 colSel = GetSubSelColor();
	Point3 colTicks = GetUIColor(COLOR_VERT_TICKS);
	Point3 colGiz = GetUIColor(COLOR_GIZMOS);
	Point3 colGizSel = GetUIColor(COLOR_SEL_GIZMOS);
	gw->setColor(LINE_COLOR, colGiz);

	Point3 rp[3];
	int i;

#ifdef MESH_CAGE_BACKFACE_CULLING
	if (savedLimits & GW_BACKCULL) mesh->UpdateBackfacing(gw, true);
#endif

	int es[3];
	gw->startSegments();
	for (i = 0; i<mesh->nume; i++) 
	{
		if (mesh->e[i].GetFlag(MN_DEAD | MN_HIDDEN)) continue;
		if (!mesh->e[i].GetFlag(MN_EDGE_INVIS)) 
		{
			es[0] = GW_EDGE_VIS;
		}
		else 
		{
			if (GetCurrentSubSelectionType() < SEL_EDGE) continue;
			if (GetCurrentSubSelectionType() > SEL_FACE) continue;
			es[0] = GW_EDGE_INVIS;
		}
#ifdef MESH_CAGE_BACKFACE_CULLING
		if ((savedLimits & GW_BACKCULL) && mesh->e[i].GetFlag(MN_BACKFACING)) continue;
#endif
		if ((GetCurrentSubSelectionType() == SEL_EDGE) || (GetCurrentSubSelectionType() == SEL_BORDER))
		{
			if (modData->GetEdgeSel()[i]) gw->setColor(LINE_COLOR, colGizSel);
			else gw->setColor(LINE_COLOR, colGiz);
		}
		if ((GetCurrentSubSelectionType() == SEL_FACE) || (GetCurrentSubSelectionType() == SEL_ELEMENT))
		{
			if (modData->GetFaceSel()[mesh->e[i].f1] || ((mesh->e[i].f2>-1) && modData->GetFaceSel()[mesh->e[i].f2]))
				gw->setColor(LINE_COLOR, colGizSel);
			else gw->setColor(LINE_COLOR, colGiz);
		}
		rp[0] = mesh->v[mesh->e[i].v1].p;
		rp[1] = mesh->v[mesh->e[i].v2].p;
		//		gw->polyline (2, rp, NULL, NULL, FALSE, es);
		if (es[0] == GW_EDGE_VIS)
			gw->segment(rp, 1);
		else gw->segment(rp, 0);
	}
	gw->endSegments();

	if (GetCurrentSubSelectionType() == SEL_VERTEX)
	{
		float *ourvw = NULL;
		int affectRegion = FALSE;
		//if (pblock) pblock->GetValue(ps_use_softsel, t, affectRegion, FOREVER);
		if (affectRegion) ourvw = mesh->getVSelectionWeights();
		gw->startMarkers();
		for (i = 0; i<mesh->numv; i++) 
		{
			if (mesh->v[i].GetFlag(MN_DEAD | MN_HIDDEN)) continue;

#ifdef MESH_CAGE_BACKFACE_CULLING
			if ((savedLimits & GW_BACKCULL) && !getDisplayBackFaceVertices()) 
			{
				if (mesh->v[i].GetFlag(MN_BACKFACING)) continue;
			}
#endif

			if (modData->GetVertSel()[i]) gw->setColor(LINE_COLOR, colSel);
			else
			{
				if (ourvw) gw->setColor(LINE_COLOR, SoftSelectionColor(ourvw[i]));
				else gw->setColor(LINE_COLOR, colTicks);
			}

			if (getUseVertexDots()) gw->marker(&(mesh->v[i].p), VERTEX_DOT_MARKER(getVertexDotType()));
			else gw->marker(&(mesh->v[i].p), PLUS_SIGN_MRKR);
		}
		gw->endMarkers();
	}
	gw->setRndLimits(savedLimits);
	return 0;
}

void SubSelectionPolyMod::GetWorldBoundBox(TimeValue t, INode* inode, ViewExp * /*vpt*/, Box3& box, ModContext *mc) 
{
	if (!ip || !ip->GetShowEndResult() || !mc->localData) 
		return;
	if (!GetCurrentSubSelectionType()) 
		return;
	PolyMeshSelData *modData = (PolyMeshSelData *)mc->localData;
	MNMesh *mesh = modData->GetMesh();
	if (!mesh) return;
	Matrix3 tm = inode->GetObjectTM(t);
	box = mesh->getBoundingBox(&tm);
}

void SubSelectionPolyMod::GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node, ModContext *mc)
{
	if (!mc->localData)
		return;
	if (GetCurrentSubSelectionType() == SEL_OBJECT)
		return;	// shouldn't happen.
	PolyMeshSelData *modData = (PolyMeshSelData *)mc->localData;
	MNMesh *mesh = modData->GetMesh();
	if (!mesh)
		return;
	Matrix3 tm = node->GetObjectTM(t);

	// For Mesh Select, we merely return the center of the bounding box of the current selection.
	BitArray sel = mesh->VertexTempSel();
	if (!sel.NumberSet())
		return;
	Box3 box;
	for (int i = 0; i < mesh->numv; i++)
	{
		if (sel[i])
			box += mesh->v[i].p * tm;
	}
	cb->Center(box.Center(), 0);
}

void SubSelectionPolyMod::GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue t, INode *node, ModContext *mc)
{
	if (!mc->localData) 
		return;
	if (GetCurrentSubSelectionType() == SEL_OBJECT) 
		return;	// shouldn't happen.
	PolyMeshSelData *modData = (PolyMeshSelData *)mc->localData;
	MNMesh *mesh = modData->GetMesh();
	if (!mesh) return;
	Matrix3 tm = node->GetObjectTM(t);

	// For Mesh Select, we merely return the center of the bounding box of the current selection.
	BitArray sel = mesh->VertexTempSel();
	if (!sel.NumberSet()) 
		return;
	Box3 box;
	for (int i = 0; i<mesh->numv; i++) if (sel[i]) box += mesh->v[i].p * tm;
	Matrix3 ctm(1);
	ctm.SetTrans(box.Center());
	cb->TM(ctm, 0);
}

int SubSelectionPolyMod::NumSubObjTypes()
{
	// Loop through the enum and count how many flags are enabled.
	int count = 0;
	for (int i = 0; i < SelectionjTypeQty; i++)
	{
		if (enabledSubSelectionTypes & 1 << i)
			count++;
	}
	return count;
}

ISubObjType* SubSelectionPolyMod::GetSubObjType(int i)
{
	SelectionTypeFlag flag = GetTypeFlagFromLevel(i + 1);
	if (subObjectTypes.count(flag))
		return subObjectTypes.at(flag);
	else return NULL;
}

void SubSelectionPolyMod::ActivateSubobjSel(int level, XFormModes &modes)
{
	if (level <= 0) // Object
		currentSubSelectionType = SEL_OBJECT;
	else // Set to flag value
	{
		SubObjectType* soType = (SubObjectType*)GetSubObjType(level - 1);
		if (soType)
			currentSubSelectionType = soType->levelEnum;
		else 
			currentSubSelectionType = SEL_OBJECT;
	}

	if (level != SEL_OBJECT)
	{
		// This tells max which selection modes are available. In this case, only selection is enabled and everything else (move, rotate, scale) is disabled.
		modes = XFormModes(NULL, NULL, NULL, NULL, NULL, selectMode);
	}

	NotifyDependents(FOREVER, PART_SELECT | PART_DISPLAY | PART_SUBSEL_TYPE, REFMSG_CHANGE);
}

void SubSelectionPolyMod::SelectSubComponent(HitRecord *firstHit, BOOL selected, BOOL all, BOOL invert) 
{
	if (currentSubSelectionType == SEL_OBJECT) return;	// shouldn't happen.
	if (!ip) return;

	PolyMeshSelData *pData = NULL;
	HitRecord* hr = NULL;

	ip->ClearCurNamedSelSet();

	int selByVert = 0;

	// Prepare a bitarray representing the particular hits we got:
	if (selByVert) 
	{
		for (hr = firstHit; hr != NULL; hr = hr->Next())
		{
			pData = (PolyMeshSelData*)hr->modContext->localData;
			if (!pData->GetNewSelection()) 
			{
				pData->SetupNewSelection(SEL_VERTEX);
				pData->SynchBitArrays();
			}
			pData->GetNewSelection()->Set(hr->hitInfo, true);
			if (!all) break;
		}
	}
	else 
	{
		for (hr = firstHit; hr != NULL; hr = hr->Next()) 
		{
			pData = (PolyMeshSelData*)hr->modContext->localData;
			if (!pData->GetNewSelection()) 
			{
				pData->SetupNewSelection(GetCurrentSubSelectionType());
				pData->SynchBitArrays();
			}
			pData->GetNewSelection()->Set(hr->hitInfo, true);
			if (!all) break;
		}
	}

	// in Maya selection mode, shift is for add/substract selection, so disable loop selection
	bool inMayaSelectionMode = MaxSDK::CUI::GetIMouseConfigManager()->GetMayaSelection();

	// Now that the hit records are translated into bitarrays, apply them:
	for (hr = firstHit; hr != NULL; hr = hr->Next()) 
	{
		pData = (PolyMeshSelData*)hr->modContext->localData;
		if (!pData->GetNewSelection()) 
		{
			if (!all) break;
			continue;
		}

		// Translate the hits into the correct selection level:
		if (selByVert && (GetCurrentSubSelectionType()>SEL_VERTEX)) 
		{
			pData->TranslateNewSelection(SEL_VERTEX, GetCurrentSubSelectionType());
		}
		else 
		{
			if (GetCurrentSubSelectionType() == SEL_BORDER) pData->TranslateNewSelection(SEL_EDGE, SEL_BORDER);
			if (GetCurrentSubSelectionType() == SEL_ELEMENT) pData->TranslateNewSelection(SEL_FACE, SEL_ELEMENT);
		}

		if (!inMayaSelectionMode && (GetKeyState(VK_SHIFT) & 0x8000)) //Select loop or ring if Shift key is pressed
		{
			MNMesh* mm = pData->mesh;
			BitArray* newSel = pData->GetNewSelection();
			BitArray currentSel;
			bool foundLoopRing = false;
			IMNMeshUtilities13* l_mesh13 = static_cast<IMNMeshUtilities13*>(mm->GetInterface(IMNMESHUTILITIES13_INTERFACE_ID));
			if (l_mesh13)
			{
				if (GetCurrentSubSelectionType() == SEL_VERTEX)
				{
					mm->getVertexSel(currentSel);
					foundLoopRing = l_mesh13->FindLoopVertex(*newSel, currentSel);
				}
				else if (GetCurrentSubSelectionType() == SEL_EDGE)
				{
					mm->getEdgeSel(currentSel);
					foundLoopRing = l_mesh13->FindLoopOrRingEdge(*newSel, currentSel);
				}
				else if (GetCurrentSubSelectionType() == SEL_FACE)
				{
					mm->getFaceSel(currentSel);
					foundLoopRing = l_mesh13->FindLoopFace(*newSel, currentSel);
				}
				if (!foundLoopRing) (*newSel).ClearAll(); //If no loop or ring is found, do not add anything to selection
			}
		}

		// Create the undo object:
		if (theHold.Holding()) theHold.Put(new SelectRestore(this, pData));

		// Apply the new selection.
		pData->ApplyNewSelection(GetCurrentSubSelectionType(), true, invert ? true : false, selected ? true : false);
		if (!all) break;
	}

	LocalDataChanged();
}

void SubSelectionPolyMod::ClearSelection(int selLevel) 
{
	SelectionTypeFlag selectionType = GetTypeFlagFromLevel(selLevel);

	if (!ip) 
		return;
	bool inMayaSelectionMode = MaxSDK::CUI::GetIMouseConfigManager()->GetMayaSelection();
	if (!inMayaSelectionMode && (GetKeyState(VK_SHIFT) & 0x8000)) 
		return; //Do not clear selection when attempting to select loop or ring
	ModContextList list;
	INodeTab nodes;
	ip->GetModContexts(list, nodes);
	PolyMeshSelData* d = NULL;
	for (int i = 0; i<list.Count(); i++) 
	{
		d = (PolyMeshSelData*)list[i]->localData;
		if (!d) continue;

		// Check if we have anything selected first:
		switch (selectionType)
		{
		case SEL_VERTEX:
			if (!d->vertSel.NumberSet()) continue;
			else break;
		case SEL_FACE:
		case SEL_ELEMENT:
			if (!d->faceSel.NumberSet()) continue;
			else break;
		case SEL_EDGE:
		case SEL_BORDER:
			if (!d->edgeSel.NumberSet()) continue;
			else break;
		}

		if (theHold.Holding() && !d->held) theHold.Put(new SelectRestore(this, d));
		d->SynchBitArrays();

		switch (selectionType)
		{
		case SEL_VERTEX:
			d->vertSel.ClearAll();
			break;
		case SEL_FACE:
		case SEL_ELEMENT:
			d->faceSel.ClearAll();
			break;
		case SEL_BORDER:
		case SEL_EDGE:
			d->edgeSel.ClearAll();
			break;
		}
	}
	nodes.DisposeTemporary();
	LocalDataChanged();
}

void SubSelectionPolyMod::SelectAll(int selLevel) 
{
	if (selLevel == SEL_BORDER) 
	{
		SelectOpenEdges(); 
		return; 
	}

	SelectionTypeFlag selectionType = GetTypeFlagFromLevel(selLevel);

	if (!ip) return;
	ModContextList list;
	INodeTab nodes;
	ip->GetModContexts(list, nodes);
	PolyMeshSelData* d = NULL;
	for (int i = 0; i<list.Count(); i++) {
		d = (PolyMeshSelData*)list[i]->localData;
		if (!d) 
			continue;
		if (theHold.Holding() && !d->held) 
			theHold.Put(new SelectRestore(this, d));

		d->SynchBitArrays();

		switch (selectionType)
		{
		case SEL_VERTEX:
			d->vertSel.SetAll();
			break;
		case SEL_FACE:
		case SEL_ELEMENT:
			d->faceSel.SetAll();
			break;
		case SEL_EDGE:
			d->edgeSel.SetAll();
			break;
		}
	}
	nodes.DisposeTemporary();
	LocalDataChanged();
}

void SubSelectionPolyMod::InvertSelection(int selLevel) 
{
	SelectionTypeFlag selectionType = GetTypeFlagFromLevel(selLevel);

	ModContextList list;
	INodeTab nodes;
	if (!ip)
		return;
	ip->GetModContexts(list, nodes);
	PolyMeshSelData* d = NULL;
	for (int i = 0; i<list.Count(); i++) 
	{
		d = (PolyMeshSelData*)list[i]->localData;
		if (!d) continue;
		if (theHold.Holding() && !d->held) theHold.Put(new SelectRestore(this, d));
		d->SynchBitArrays();

		switch (selectionType)
		{
		case SEL_VERTEX:
			d->vertSel = ~d->vertSel;
			break;
		case SEL_FACE:
		case SEL_ELEMENT:
			d->faceSel = ~d->faceSel;
			break;
		case SEL_EDGE:
		case SEL_BORDER:
			d->edgeSel = ~d->edgeSel;
			break;
		}
	}
	if (selLevel == SEL_BORDER) { SelectOpenEdges(-1); }	// indicates deselect only
	nodes.DisposeTemporary();
	LocalDataChanged();
}

void SubSelectionPolyMod::BeginEditParams(IObjParam *aIp, ULONG flags, Animatable *prev)
{
	ip = aIp;
	editMod = this;
	// Create selection mode
	selectMode = new SelectModBoxCMode(this, ip);

	TimeValue t = ip->GetTime();
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_BEGIN_EDIT);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_MOD_DISPLAY_ON);
	SetAFlag(A_MOD_BEING_EDITED);
}

void SubSelectionPolyMod::EndEditParams(IObjParam *aIp, ULONG flags, Animatable *next)
{
	// Destroy selection mode
	ip->DeleteMode(selectMode);
	if (selectMode)
		delete selectMode;
	selectMode = NULL;

	editMod = NULL;
	ip = NULL;	

	TimeValue t = aIp->GetTime();
	// aszabo|feb.05.02 This flag must be cleared before sending the REFMSG_END_EDIT
	ClearAFlag(A_MOD_BEING_EDITED);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_END_EDIT);
	NotifyDependents(Interval(t, t), PART_ALL, REFMSG_MOD_DISPLAY_OFF);
}

#define SELLEVEL_CHUNKID		0x0100
#define FLAGS_CHUNKID			0x0120
#define VERTSEL_CHUNKID			0x0200
#define FACESEL_CHUNKID			0x0210
#define EDGESEL_CHUNKID			0x0220

IOResult SubSelectionPolyMod::Save(ISave *isave)
{
	IOResult res;
	ULONG nb;
	Modifier::Save(isave);

	isave->BeginChunk(SELLEVEL_CHUNKID);
	res = isave->Write(&currentSubSelectionType, sizeof(currentSubSelectionType), &nb);
	isave->EndChunk();

	return res;
}

IOResult SubSelectionPolyMod::Load(ILoad *iload)
{
	IOResult res;
	ULONG nb;
	DWORD flags = 0;

	Modifier::Load(iload);

	while (IO_OK == (res = iload->OpenChunk())) 
	{
		switch (iload->CurChunkID())
		{
		case SELLEVEL_CHUNKID:
			iload->Read(&currentSubSelectionType, sizeof(currentSubSelectionType), &nb);
			break;
		}
		iload->CloseChunk();

		if (res != IO_OK)
			return res;
	}
	return IO_OK;
}

IOResult SubSelectionPolyMod::SaveLocalData(ISave *isave, LocalModData *ld)
{
	PolyMeshSelData *d = (PolyMeshSelData*)ld;

	isave->BeginChunk(VERTSEL_CHUNKID);
	d->vertSel.Save(isave);
	isave->EndChunk();

	isave->BeginChunk(FACESEL_CHUNKID);
	d->faceSel.Save(isave);
	isave->EndChunk();

	isave->BeginChunk(EDGESEL_CHUNKID);
	d->edgeSel.Save(isave);
	isave->EndChunk();

	return IO_OK;
}

IOResult SubSelectionPolyMod::LoadLocalData(ILoad *iload, LocalModData **pld)
{
	PolyMeshSelData *d = NewLocalModData();
	*pld = d;
	IOResult res;
	while (IO_OK == (res = iload->OpenChunk())) 
	{
		LoadLocalDataChunk(iload, d);

		iload->CloseChunk();

		if (res != IO_OK)
			return res;
	}
	return IO_OK;
}

void SubSelectionPolyMod::LoadLocalDataChunk(ILoad *iload, PolyMeshSelData *d)
{
	switch (iload->CurChunkID())
	{
	case VERTSEL_CHUNKID:
		d->vertSel.Load(iload);
		break;
	case FACESEL_CHUNKID:
		d->faceSel.Load(iload);
		break;
	case EDGESEL_CHUNKID:
		d->edgeSel.Load(iload);
		break;
	}
}

void SubSelectionPolyMod::ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node)
{
	if (!os->obj->IsSubClassOf(polyObjectClassID)) 
		return;

	PolyObject *pobj = (PolyObject*)os->obj;
	MNMesh& mesh = pobj->GetMesh();

	PolyMeshSelData *d = (PolyMeshSelData*)mc.localData;
	if (!d)
		mc.localData = d = NewLocalModData(&mesh);

	BitArray vertSel = d->vertSel;
	BitArray faceSel = d->faceSel;
	BitArray edgeSel = d->edgeSel;
	vertSel.SetSize(mesh.VNum(), TRUE);
	faceSel.SetSize(mesh.FNum(), TRUE);
	edgeSel.SetSize(mesh.ENum(), TRUE);
	mesh.VertexSelect(vertSel);
	mesh.FaceSelect(faceSel);
	mesh.EdgeSelect(edgeSel);

	if (GetCurrentSubSelection() != NULL)
		mesh.selLevel = GetCurrentSubSelection()->meshLevel;
	else
		mesh.selLevel = SEL_OBJECT;

	// Update the cache used for display, hit-testing, painting:
	if (!d->GetMesh())
		d->SetCache(mesh);
	else 
		*(d->GetMesh()) = MNMesh(mesh);

	// Set display flags - but be sure to turn off vertex display in stack result if
	// "Show End Result" is turned on - we want the user to just see the Poly Select
	// level vertices (from the Display method).
	DWORD dispFlagMask = MNDISP_VERTTICKS | MNDISP_SELVERTS | MNDISP_SELFACES | MNDISP_SELEDGES | MNDISP_NORMALS | MNDISP_SMOOTH_SUBSEL | MNDISP_BEEN_DISP | MNDISP_DIAGONALS | MNDISP_HIDE_SUBDIVISION_INTERIORS;
	mesh.dispFlags &= ~dispFlagMask;

	if ((GetCurrentSubSelectionType() > SEL_VERTEX) || !ip || !ip->GetShowEndResult())
	{
		if (GetCurrentSubSelection() != NULL)
			mesh.SetDispFlag(GetCurrentSubSelection()->levelDisplayFlags);
	}

}

DWORD SubSelectionPolyMod::GetSelLevel()
{
	switch (currentSubSelectionType)
	{
	case SEL_VERTEX:
		return IMESHSEL_VERTEX;
	case SEL_EDGE:
	case SEL_BORDER:
		return IMESHSEL_EDGE;
	case SEL_FACE:
	case SEL_ELEMENT:
		return IMESHSEL_FACE;
	default:
		return IMESHSEL_OBJECT;
	}
}

void SubSelectionPolyMod::SetSelLevel(DWORD level)
{
	int selLevel = 0;

	switch (level) 
	{
	case IMESHSEL_OBJECT:
		selLevel = 0;
		break;
	case IMESHSEL_VERTEX:
		selLevel = 1;
		break;
	case IMESHSEL_EDGE:
		// Don't change if we're already in an edge level:
		if (GetSelLevel() == IMESHSEL_EDGE)
			break;
		selLevel = 2;
		break;
	case IMESHSEL_FACE:
		// Don't change if we're already in a face level:
		if (GetSelLevel() == IMESHSEL_FACE) 
			break;
		selLevel = 4;
		break;
	}
	if (ip && editMod == this) 
		ip->SetSubObjectLevel(selLevel);
}

void SubSelectionPolyMod::LocalDataChanged()
{
	NotifyDependents(FOREVER, PART_SELECT, REFMSG_CHANGE);
}