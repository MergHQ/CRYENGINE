#include "stdafx.h"
#include "PolyMeshSelData.h"
#include "SubSelectionPolyMod.h"
#include "SelectRestore.h"

LocalModData *PolyMeshSelData::Clone()
{
	PolyMeshSelData *d = new PolyMeshSelData;
	d->vertSel = vertSel;
	d->faceSel = faceSel;
	d->edgeSel = edgeSel;
	d->vselSet = vselSet;
	d->eselSet = eselSet;
	d->fselSet = fselSet;
	return d;
}

PolyMeshSelData::PolyMeshSelData(MNMesh* mesh) : mpNewSelection(NULL), held(false), mesh(NULL)
{
	if (mesh != NULL)
	{
		mesh->getVertexSel(vertSel);
		mesh->getFaceSel(faceSel);
		mesh->getEdgeSel(edgeSel);
	}
	vdValid = NEVER;
}

void PolyMeshSelData::SynchBitArrays()
{
	if (!mesh) return;
	if (vertSel.GetSize() != mesh->VNum()) vertSel.SetSize(mesh->VNum(), TRUE);
	if (faceSel.GetSize() != mesh->FNum()) faceSel.SetSize(mesh->FNum(), TRUE);
	if (edgeSel.GetSize() != mesh->ENum()) edgeSel.SetSize(mesh->ENum(), TRUE);
}

void PolyMeshSelData::SetCache(MNMesh &mesh)
{
	if (this->mesh) delete this->mesh;
	this->mesh = new MNMesh(mesh);
	SynchBitArrays();
}

void PolyMeshSelData::FreeCache()
{
	if (mesh) delete mesh;
	mesh = NULL;
}

void PolyMeshSelData::SetVertSel(BitArray &set, IMeshSelect *imod, TimeValue t)
{
	SubSelectionPolyMod *mod = (SubSelectionPolyMod *)imod;
	if (theHold.Holding()) theHold.Put(new SelectRestore(mod, this, SubSelectionPolyMod::SEL_VERTEX));
	vertSel = set;
	if (mesh) mesh->VertexSelect(set);
	InvalidateVDistances();
}

void PolyMeshSelData::SetFaceSel(BitArray &set, IMeshSelect *imod, TimeValue t)
{
	SubSelectionPolyMod *mod = (SubSelectionPolyMod *)imod;
	if (theHold.Holding()) theHold.Put(new SelectRestore(mod, this, SubSelectionPolyMod::SEL_FACE));
	faceSel = set;
	if (mesh) mesh->FaceSelect(set);
	InvalidateVDistances();
}

void PolyMeshSelData::SetEdgeSel(BitArray &set, IMeshSelect *imod, TimeValue t)
{
	SubSelectionPolyMod *mod = (SubSelectionPolyMod *)imod;
	if (theHold.Holding()) theHold.Put(new SelectRestore(mod, this, SubSelectionPolyMod::SEL_EDGE));
	edgeSel = set;
	if (mesh) mesh->EdgeSelect(set);
	InvalidateVDistances();
}

void PolyMeshSelData::SelVertByFace()
{
	DbgAssert(mesh);
	if (!mesh) return;
	for (int i = 0; i < mesh->FNum(); i++)
	{
		if (!faceSel[i]) continue;
		MNFace *mf = mesh->F(i);
		if (mf->GetFlag(MN_DEAD)) continue;
		for (int j = 0; j < mf->deg; j++) vertSel.Set(mf->vtx[j]);
	}
}

void PolyMeshSelData::SelVertByEdge()
{
	DbgAssert(mesh);
	if (!mesh) return;
	if (!mesh->GetFlag(MN_MESH_FILLED_IN)) return;
	for (int i = 0; i < mesh->ENum(); i++) 
	{
		if (!edgeSel[i]) continue;
		MNEdge *me = mesh->E(i);
		if (me->GetFlag(MN_DEAD)) continue;
		vertSel.Set(me->v1);
		vertSel.Set(me->v2);
	}
}

void PolyMeshSelData::SelFaceByVert() 
{
	DbgAssert(mesh);
	if (!mesh) return;
	for (int i = 0; i < mesh->FNum(); i++) 
	{
		MNFace * mf = mesh->F(i);
		int j;
		for (j = 0; j < mf->deg; j++) if (vertSel[mf->vtx[j]]) break;
		if (j < mf->deg) faceSel.Set(i);
	}
}

void PolyMeshSelData::SelFaceByEdge() 
{
	DbgAssert(mesh);
	if (!mesh) return;
	if (!mesh->GetFlag(MN_MESH_FILLED_IN)) return;
	for (int i = 0; i < mesh->FNum(); i++) 
	{
		MNFace * mf = mesh->F(i);
		int j;
		for (j = 0; j < mf->deg; j++) if (edgeSel[mf->edg[j]]) break;
		if (j < mf->deg) faceSel.Set(i);
	}
}

void PolyMeshSelData::SelElementByVert()
{
	DbgAssert(mesh);
	if (!mesh) return;
	BitArray nfsel;
	nfsel.SetSize(mesh->FNum());
	for (int i = 0; i < mesh->FNum(); i++)
	{
		MNFace * mf = mesh->F(i);
		int j;
		for (j = 0; j < mf->deg; j++) if (vertSel[mf->vtx[j]]) break;
		if (j == mf->deg) continue;
		mesh->ElementFromFace(i, nfsel);
	}
	faceSel |= nfsel;
}

void PolyMeshSelData::SelElementByEdge() 
{
	DbgAssert(mesh);
	if (!mesh) return;
	if (!mesh->GetFlag(MN_MESH_FILLED_IN)) return;
	BitArray nfsel;
	nfsel.SetSize(mesh->FNum());
	for (int i = 0; i < mesh->FNum(); i++)
	{
		MNFace * mf = mesh->F(i);
		for (int j = 0; j < mf->deg; j++) 
		{
			if (edgeSel[mf->edg[j]]) {
				mesh->ElementFromFace(i, nfsel);
				break;
			}
		}
	}
	faceSel |= nfsel;
}

void PolyMeshSelData::SelEdgeByVert() 
{
	DbgAssert(mesh);
	if (!mesh) return;
	if (!mesh->GetFlag(MN_MESH_FILLED_IN)) return;
	for (int i = 0; i < mesh->ENum(); i++) 
	{
		MNEdge *me = mesh->E(i);
		if (me->GetFlag(MN_DEAD)) continue;
		if (vertSel[me->v1] || vertSel[me->v2]) edgeSel.Set(i);
	}
}

void PolyMeshSelData::SelEdgeByFace() 
{
	DbgAssert(mesh);
	if (!mesh) return;
	if (!mesh->GetFlag(MN_MESH_FILLED_IN)) return;
	for (int i = 0; i < mesh->FNum(); i++) 
	{
		if (!faceSel[i]) continue;
		MNFace *mf = mesh->F(i);
		if (mf->GetFlag(MN_DEAD)) continue;
		for (int j = 0; j < mf->deg; j++) edgeSel.Set(mf->edg[j]);
	}
}

void PolyMeshSelData::SelBorderByVert() 
{
	DbgAssert(mesh);
	if (!mesh) return;
	if (!mesh->GetFlag(MN_MESH_FILLED_IN)) return;
	BitArray nesel;
	nesel.SetSize(mesh->ENum());
	for (int i = 0; i < mesh->ENum(); i++) 
	{
		MNEdge *me = mesh->E(i);
		if (me->GetFlag(MN_DEAD)) continue;
		if (me->f2 > -1) continue;
		if (vertSel[me->v1] || vertSel[me->v2]) mesh->BorderFromEdge(i, nesel);
	}
	edgeSel |= nesel;
}

void PolyMeshSelData::SelBorderByFace()
{
	DbgAssert(mesh);
	if (!mesh) return;
	if (!mesh->GetFlag(MN_MESH_FILLED_IN)) return;
	BitArray nesel;
	nesel.SetSize(mesh->ENum());
	for (int i = 0; i < mesh->FNum(); i++) 
	{
		MNFace *mf = mesh->F(i);
		if (!faceSel[i]) continue;
		if (mf->GetFlag(MN_DEAD)) continue;
		for (int j = 0; j < mf->deg; j++) mesh->BorderFromEdge(mf->edg[j], nesel);
	}
	edgeSel |= nesel;
}

// russom - 1/20/04 - FID 1495
void PolyMeshSelData::GrowSelection(IMeshSelect *imod, SubObjectType* level)
{
	DbgAssert(mesh);
	if (!mesh) return;

	BitArray newSel;
	int mnSelLevel = mesh->selLevel;
	if (mnSelLevel == MNM_SL_CURRENT) mnSelLevel = level->meshLevel;
	DbgAssert(mesh->GetFlag(MN_MESH_FILLED_IN));
	if (!mesh->GetFlag(MN_MESH_FILLED_IN)) return;

	SynchBitArrays();

	switch (level->levelEnum) 
	{
	case SubSelectionPolyMod::SEL_VERTEX:
		mesh->ClearEFlags(MN_USER);
		mesh->PropegateComponentFlags(MNM_SL_EDGE, MN_USER, mnSelLevel, MN_SEL);
		newSel.SetSize(mesh->numv);
		for (int i = 0; i < mesh->nume; i++)
		{
			if (mesh->e[i].GetFlag(MN_USER))
			{
				newSel.Set(mesh->e[i].v1);
				newSel.Set(mesh->e[i].v2);
			}
		}
		SetVertSel(newSel, imod, TimeValue(0));
		break;
	case SubSelectionPolyMod::SEL_EDGE:
		mesh->ClearVFlags(MN_USER);
		mesh->PropegateComponentFlags(MNM_SL_VERTEX, MN_USER, mnSelLevel, MN_SEL);
		newSel.SetSize(mesh->nume);
		for (int i = 0; i < mesh->nume; i++) 
		{
			if (mesh->v[mesh->e[i].v1].GetFlag(MN_USER) || mesh->v[mesh->e[i].v2].GetFlag(MN_USER))
				newSel.Set(i);
		}
		SetEdgeSel(newSel, imod, TimeValue(0));
		break;
	case SubSelectionPolyMod::SEL_FACE:
		mesh->ClearVFlags(MN_USER);
		mesh->PropegateComponentFlags(MNM_SL_VERTEX, MN_USER, mnSelLevel, MN_SEL);
		newSel.SetSize(mesh->numf);
		for (int i = 0; i < mesh->numf; i++)
		{
			int j;
			for (j = 0; j < mesh->f[i].deg; j++) 
			{
				if (mesh->v[mesh->f[i].vtx[j]].GetFlag(MN_USER)) break;
			}
			if (j < mesh->f[i].deg) newSel.Set(i);
		}
		SetFaceSel(newSel, imod, TimeValue(0));
		break;
	}
}

void PolyMeshSelData::ShrinkSelection(IMeshSelect *imod, SubObjectType* level)
{
	DbgAssert(mesh);
	if (!mesh) return;

	BitArray newSel;
	int mnSelLevel = mesh->selLevel;
	if (mnSelLevel == MNM_SL_CURRENT) mnSelLevel = level->meshLevel;
	DbgAssert(mesh->GetFlag(MN_MESH_FILLED_IN));
	if (!mesh->GetFlag(MN_MESH_FILLED_IN)) return;

	SynchBitArrays();

	switch (level->levelEnum)
	{
	case SubSelectionPolyMod::SEL_VERTEX:
		// Find the edges between two selected vertices.
		mesh->ClearEFlags(MN_USER);
		mesh->PropegateComponentFlags(MNM_SL_EDGE, MN_USER, mnSelLevel, MN_SEL, true);
		if (vertSel.GetSize() != mesh->numv) mesh->getVertexSel(vertSel);
		newSel = vertSel;
		// De-select all the vertices touching edges to unselected vertices:
		for (int i = 0; i < mesh->nume; i++)
		{
			if (!mesh->e[i].GetFlag(MN_USER))
			{
				newSel.Clear(mesh->e[i].v1);
				newSel.Clear(mesh->e[i].v2);
			}
		}
		SetVertSel(newSel, imod, TimeValue(0));
		break;
	case SubSelectionPolyMod::SEL_EDGE:
		// Find all vertices used by only selected edges:
		mesh->ClearVFlags(MN_USER);
		mesh->PropegateComponentFlags(MNM_SL_VERTEX, MN_USER, mnSelLevel, MN_SEL, true);
		if (edgeSel.GetSize() != mesh->nume) mesh->getEdgeSel(edgeSel);
		newSel = edgeSel;
		for (int i = 0; i < mesh->nume; i++) 
		{
			// Deselect edges with at least one vertex touching a nonselected edge:
			if (!mesh->v[mesh->e[i].v1].GetFlag(MN_USER) || !mesh->v[mesh->e[i].v2].GetFlag(MN_USER))
				newSel.Clear(i);
		}
		SetEdgeSel(newSel, imod, TimeValue(0));
		break;
	case SubSelectionPolyMod::SEL_FACE:
		// Find all vertices used by only selected faces:
		mesh->ClearVFlags(MN_USER);
		mesh->PropegateComponentFlags(MNM_SL_VERTEX, MN_USER, mnSelLevel, MN_SEL, true);
		if (faceSel.GetSize() != mesh->numf) mesh->getFaceSel(faceSel);
		newSel = faceSel;
		for (int i = 0; i < mesh->numf; i++) 
		{
			int j;
			for (j = 0; j < mesh->f[i].deg; j++) 
			{
				if (!mesh->v[mesh->f[i].vtx[j]].GetFlag(MN_USER)) break;
			}
			// Deselect faces with at least one vertex touching a nonselected face:
			if (j < mesh->f[i].deg) newSel.Clear(i);
		}
		SetFaceSel(newSel, imod, TimeValue(0));
		break;
	}
}

void PolyMeshSelData::SelectEdgeRing(IMeshSelect *imod) 
{
	DbgAssert(mesh);
	if (!mesh) return;
	BitArray l_currentSel = GetEdgeSel();
	mesh->SelectEdgeRing(l_currentSel);
	SetEdgeSel(l_currentSel, imod, TimeValue(0));
}

void PolyMeshSelData::SelectEdgeLoop(IMeshSelect *imod) 
{
	DbgAssert(mesh);
	if (!mesh) return;
	BitArray l_currentSel = GetEdgeSel();
	mesh->SelectEdgeLoop(l_currentSel);
	SetEdgeSel(l_currentSel, imod, TimeValue(0));
}

void PolyMeshSelData::SelectEdgeLoopShift(IMeshSelect *imod, const int in_loop_shift, const bool in_remove, const bool in_add)
{
	DbgAssert(mesh);
	if (!mesh) return;

	BitArray l_newSel;
	l_newSel.SetSize(mesh->nume);
	l_newSel.ClearAll();


	BitArray l_currentSel = GetEdgeSel();

	IMNMeshUtilities8* l_meshToBridge = static_cast<IMNMeshUtilities8*>(mesh->GetInterface(IMNMESHUTILITIES8_INTERFACE_ID));

	if (l_meshToBridge)
	{
		if (in_loop_shift < 0)
			l_meshToBridge->SelectEdgeLoopShift(-1, l_newSel);
		else if (in_loop_shift > 0)
			l_meshToBridge->SelectEdgeLoopShift(1, l_newSel);

		// update local value 
		if (in_add)
		{
			// add the current selection to the shifted selection
			l_newSel |= l_currentSel;
		}
		else if (in_remove)
		{
			l_newSel &= l_currentSel;
		}

		if (!l_newSel.IsEmpty())
		{
			SubSelectionPolyMod *mod = (SubSelectionPolyMod *)imod;
			mod->ClearSelection(SubSelectionPolyMod::SEL_EDGE);
			// update the mesh selection with a new bitarray
			SetEdgeSel(l_newSel, imod, TimeValue(0));
		}

	}


}

void PolyMeshSelData::SelectEdgeRingShift(IMeshSelect *imod, const int in_ring_shift, const bool in_remove, const bool in_add)
{
	DbgAssert(mesh);
	if (!mesh)
		return;

	BitArray l_currentSel = GetEdgeSel();
	BitArray l_newSel;

	l_newSel.SetSize(mesh->nume);
	l_newSel.ClearAll();

	IMNMeshUtilities8* l_meshToBridge = static_cast<IMNMeshUtilities8*>(mesh->GetInterface(IMNMESHUTILITIES8_INTERFACE_ID));
	if (l_meshToBridge)
	{
		if (in_ring_shift <= 0)
			l_meshToBridge->SelectEdgeRingShift(-1, l_newSel);
		else
			l_meshToBridge->SelectEdgeRingShift(1, l_newSel);

		if (in_add)
		{
			// add the current selection to the shifted selection
			l_newSel |= l_currentSel;
		}
		else if (in_remove)
		{
			l_newSel &= l_currentSel;
		}

		if (!l_newSel.IsEmpty())
		{
			SubSelectionPolyMod *mod = (SubSelectionPolyMod *)imod;
			mod->ClearSelection(SubSelectionPolyMod::SEL_EDGE);
			// update the mesh selection with a new bitarray
			SetEdgeSel(l_newSel, imod, TimeValue(0));
		}
	}
}

void PolyMeshSelData::SetupNewSelection(int selLevel) 
{
	DbgAssert(mesh);
	if (!mesh) return;
	if (!mpNewSelection) mpNewSelection = new BitArray;
	switch (selLevel) 
	{
	case SubSelectionPolyMod::SEL_VERTEX:
		mpNewSelection->SetSize(mesh->numv);
		break;
	case SubSelectionPolyMod::SEL_EDGE:
	case SubSelectionPolyMod::SEL_BORDER:
		mpNewSelection->SetSize(mesh->nume);
		break;
	case SubSelectionPolyMod::SEL_FACE:
	case SubSelectionPolyMod::SEL_ELEMENT:
		mpNewSelection->SetSize(mesh->numf);
		break;
	}
}

void PolyMeshSelData::TranslateNewSelection(int selLevelFrom, int selLevelTo) 
{
	if (!mpNewSelection) 
		return;
	if (!mesh) 
	{
		DbgAssert(0);
		return;
	}

	BitArray intermediateSelection;
	BitArray toSelection;
	switch (selLevelFrom) 
	{
	case SubSelectionPolyMod::SEL_VERTEX:
		switch (selLevelTo) 
		{
		case SubSelectionPolyMod::SEL_EDGE:
			mSelConv.VertexToEdge(*mesh, *mpNewSelection, toSelection);
			break;
		case SubSelectionPolyMod::SEL_BORDER:
			mSelConv.VertexToEdge(*mesh, *mpNewSelection, intermediateSelection);
			mSelConv.EdgeToBorder(*mesh, intermediateSelection, toSelection);
			break;
		case SubSelectionPolyMod::SEL_FACE:
			mSelConv.VertexToFace(*mesh, *mpNewSelection, toSelection);
			break;
		case SubSelectionPolyMod::SEL_ELEMENT:
			mSelConv.VertexToFace(*mesh, *mpNewSelection, intermediateSelection);
			mSelConv.FaceToElement(*mesh, intermediateSelection, toSelection);
			break;
		}
		break;

	case SubSelectionPolyMod::SEL_EDGE:
		if (selLevelTo == SubSelectionPolyMod::SEL_BORDER) 
		{
			mSelConv.EdgeToBorder(*mesh, *mpNewSelection, toSelection);
		}
		break;

	case SubSelectionPolyMod::SEL_FACE:
		if (selLevelTo == SubSelectionPolyMod::SEL_ELEMENT) 
		{
			mSelConv.FaceToElement(*mesh, *mpNewSelection, toSelection);
		}
		break;
	}

	if (toSelection.GetSize() == 0) return;
	*mpNewSelection = toSelection;
}

void PolyMeshSelData::ApplyNewSelection(int selLevel, bool keepOld, bool invert, bool select)
{
	if (!mpNewSelection) 
		return;
	if (!mesh) {
		DbgAssert(0);
		return;
	}

	int properSize = 0;
	BitArray properSelection;
	switch (selLevel) {
	case SubSelectionPolyMod::SEL_VERTEX:
		properSize = mesh->numv;
		break;
	case SubSelectionPolyMod::SEL_EDGE:
	case SubSelectionPolyMod::SEL_BORDER:
		properSize = mesh->nume;
		break;
	case SubSelectionPolyMod::SEL_FACE:
	case SubSelectionPolyMod::SEL_ELEMENT:
		properSize = mesh->numf;
		break;
	}
	if (mpNewSelection->GetSize() != properSize) {
		mpNewSelection->SetSize(properSize);
		DbgAssert(0);
	}

	if (keepOld) 
	{
		switch (selLevel) 
		{
		case SubSelectionPolyMod::SEL_VERTEX:
			properSelection = vertSel;
			break;
		case SubSelectionPolyMod::SEL_EDGE:
		case SubSelectionPolyMod::SEL_BORDER:
			properSelection = edgeSel;
			break;
		case SubSelectionPolyMod::SEL_FACE:
		case SubSelectionPolyMod::SEL_ELEMENT:
			properSelection = faceSel;
			break;
		}
		if (!properSize) return;

		if (properSelection.GetSize() != properSize) 
		{
			properSelection.SetSize(properSize);
			DbgAssert(0);
		}

		if (invert) 
		{
			// Bits in result should be set if set in exactly one of current, incoming selections
			properSelection ^= (*mpNewSelection);
		}
		else 
		{
			if (select) 
			{
				// Result set if set in either of current, incoming:
				properSelection |= (*mpNewSelection);
			}
			else 
			{
				// Result set if in current, and _not_ in incoming:
				properSelection &= ~(*mpNewSelection);
			}
		}
		switch (selLevel) 
		{
		case SubSelectionPolyMod::SEL_VERTEX:
			vertSel = properSelection;
			break;
		case SubSelectionPolyMod::SEL_EDGE:
		case SubSelectionPolyMod::SEL_BORDER:
			edgeSel = properSelection;
			break;
		case SubSelectionPolyMod::SEL_FACE:
		case SubSelectionPolyMod::SEL_ELEMENT:
			faceSel = properSelection;
			break;
		}
	}
	else 
	{
		switch (selLevel) 
		{
		case SubSelectionPolyMod::SEL_VERTEX:
			vertSel = *mpNewSelection;
			break;
		case SubSelectionPolyMod::SEL_EDGE:
		case SubSelectionPolyMod::SEL_BORDER:
			edgeSel = *mpNewSelection;
			break;
		case SubSelectionPolyMod::SEL_FACE:
		case SubSelectionPolyMod::SEL_ELEMENT:
			faceSel = *mpNewSelection;
			break;
		}
	}

	delete mpNewSelection;
	mpNewSelection = NULL;
}