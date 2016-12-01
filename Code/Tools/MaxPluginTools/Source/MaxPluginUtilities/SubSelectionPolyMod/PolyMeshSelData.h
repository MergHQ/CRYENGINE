#pragma once

// Todo: Prune list of includes
#include "iparamm2.h"
#include "MeshDLib.h"
#include "namesel.h"
#include "nsclip.h"
#include "istdplug.h"
#include "iColorMan.h"
#include "MaxIcon.h"
#include "IActionItemOverrideManager.h"
#include "EditSoftSelectionMode.h"
#include <ICUIMouseConfigManager.h>

#include "mesh.h"
#include "mnmesh.h"
#include "polyobj.h"

class SubObjectType;

// Named selection set levels:
#define NS_VERTEX 0
#define NS_EDGE 1
#define NS_FACE 2
static int namedSetLevel[] = { NS_VERTEX, NS_VERTEX, NS_EDGE, NS_EDGE, NS_FACE, NS_FACE };
static int namedClipLevel[] = { CLIP_VERT, CLIP_VERT, CLIP_EDGE, CLIP_EDGE, CLIP_FACE, CLIP_FACE };

class PolyMeshSelData : public LocalModData, public IMeshSelectData 
{
private:

	Interval vdValid;	// validity interval of vertex distances from selection.
	BitArray *mpNewSelection;
	MNMeshSelectionConverter mSelConv;

public:
	// LocalModData
	void* GetInterface(ULONG id) { if (id == I_MESHSELECTDATA) return(IMeshSelectData*)this; else return LocalModData::GetInterface(id); }

	// Selection sets
	BitArray vertSel;
	BitArray faceSel;
	BitArray edgeSel;

	// Lists of named selection sets
	GenericNamedSelSetList vselSet;
	GenericNamedSelSetList fselSet;
	GenericNamedSelSetList eselSet;

	BOOL held;
	MNMesh *mesh;

	PolyMeshSelData(MNMesh* mesh);
	PolyMeshSelData() : held(false), mesh(NULL), mpNewSelection(NULL) { vdValid.SetEmpty(); }
	~PolyMeshSelData() { FreeCache(); if (mpNewSelection) delete mpNewSelection; }
	LocalModData *Clone();
	MNMesh *GetMesh() { return mesh; }
	void SetCache(MNMesh &mesh);
	void FreeCache();
	void SynchBitArrays();

	void SelVertByFace();
	void SelVertByEdge();
	void SelFaceByVert();
	void SelFaceByEdge();
	void SelElementByVert();
	void SelElementByEdge();
	void SelBorderByVert();
	void SelBorderByFace();
	void SelEdgeByVert();
	void SelEdgeByFace();

	// russom - 1/20/04 - FID 1495
	void GrowSelection(IMeshSelect *imod, SubObjectType* level);
	void ShrinkSelection(IMeshSelect *imod, SubObjectType* level);
	void SelectEdgeRing(IMeshSelect *imod);
	void SelectEdgeLoop(IMeshSelect *imod);

	//laurent r - 3/05 
	void SelectEdgeLoopShift(IMeshSelect *imod, const int in_loop_shift, const bool in_remove, const bool in_add);
	void SelectEdgeRingShift(IMeshSelect *imod, const int in_ring_shift, const bool in_remove, const bool in_add);

	// From IMeshSelectData:
	BitArray GetVertSel() { return vertSel; }
	BitArray GetFaceSel() { return faceSel; }
	BitArray GetEdgeSel() { return edgeSel; }

	void SetVertSel(BitArray &set, IMeshSelect *imod, TimeValue t);
	void SetFaceSel(BitArray &set, IMeshSelect *imod, TimeValue t);
	void SetEdgeSel(BitArray &set, IMeshSelect *imod, TimeValue t);

	GenericNamedSelSetList & GetNamedVertSelList() { return vselSet; }
	GenericNamedSelSetList & GetNamedEdgeSelList() { return eselSet; }
	GenericNamedSelSetList & GetNamedFaceSelList() { return fselSet; }
	GenericNamedSelSetList & GetNamedSel(int nsl) 
	{
		if (nsl == NS_VERTEX) return vselSet;
		if (nsl == NS_EDGE) return eselSet;
		return fselSet;
	}

	// New selection methods:
	void SetupNewSelection(int selLevel);
	BitArray *GetNewSelection() { return mpNewSelection; }
	void TranslateNewSelection(int selLevelFrom, int selLevelTo);
	void ApplyNewSelection(int selLevel, bool keepOld = false, bool invert = false, bool select = true);
	void SetConverterFlag(DWORD flag, bool value) { mSelConv.SetFlag(flag, value); }

	void InvalidateVDistances() { vdValid.SetEmpty(); }
	void GetWeightedVertSel(int nv, float *sel, TimeValue t, PolyObject *pobj,
		float falloff, float pinch, float bubble, int edgeDist, bool ignoreBackfacing,
		Interval & eDistValid, bool lockedSoftsel);
};