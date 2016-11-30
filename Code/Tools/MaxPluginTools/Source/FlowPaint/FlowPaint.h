#pragma once

#include "IPainterInterface.h"
#include "TangentSpaceCalculator/TangentInfo.h"

#define PAINTDEFORMTEST_CLASS_ID Class_ID(0xb1e2328, 0x1c825299)
#define PBLOCK_REF 0

class PaintDefromModData;

class PainterNodeList
{
public:
	INode* node;
	PaintDefromModData* pmd;
	Matrix3 tmToLocalSpace;
};

//Need to sub class off of IPainterCanvasInterface_V5 so we get access to all the methods

class FlowPaint : public Modifier, public IPainterCanvasInterface_V5
{
	// Cached for fast display
	MaxSDK::Array<Point3> displayVertPositions;
	MaxSDK::Array<Point3> displayFlowVectors;
	MaxSDK::Array<Color> displayVertColors;

	bool tangentInfoDirty;
	TangentInfo tangentInfo;

	float displayLength;

	Point3 lastStrokePoint;
	bool lastStrokePointValid;

	Tab<ObjectState> objstates;

public:

	void FreeObjStates()
	{
		for (int i = 0; i < objstates.Count(); i++)
			if (objstates[i].obj)
				objstates[i].obj->DeleteMe();
		objstates.Resize(0);
		painterNodeList.Resize(0);
	}

	// Parameter block 
	IParamBlock2 *pblock; //ref 0
	static IObjParam *ip; //Access to the interface

	virtual int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags, ModContext* mc);

	// From Animatable
	const TCHAR *GetObjectName() { return GetString(IDS_CLASS_NAME); }

	//From Modifier
	ChannelMask ChannelsUsed() { return GEOM_CHANNEL | TOPO_CHANNEL | VERTCOLOR_CHANNEL; }
	//TODO: Add the channels that the modifier actually modifies
	ChannelMask ChannelsChanged() { return GEOM_CHANNEL | VERTCOLOR_CHANNEL; }
	Class_ID InputType() { return polyObjectClassID; } // Only operate on poly objects

	void ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node);
	Interval LocalValidity(TimeValue t);

	// From BaseObject
	BOOL ChangeTopology() { return FALSE; }

	CreateMouseCallBack* GetCreateMouseCallBack() { return NULL; }

	void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev);
	void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next);

	Interval GetValidity(TimeValue t);
	//From Animatable

	Class_ID ClassID() { return PAINTDEFORMTEST_CLASS_ID; }
	SClass_ID SuperClassID() { return OSM_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_CLASS_NAME); }

	RefTargetHandle Clone(RemapDir &remap);
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
	PartID& partID, RefMessage message, BOOL propagate);

	int NumSubs() { return 1; }
	TSTR SubAnimName(int i) { return GetString(IDS_PARAMS); }
	Animatable* SubAnim(int i) { return pblock; }

	int NumRefs() { return 1; }
	RefTargetHandle GetReference(int i) { return pblock; }
private:
	virtual void SetReference(int i, RefTargetHandle rtarg) { pblock = (IParamBlock2*)rtarg; }
public:

	int	NumParamBlocks() { return 1; } // return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int i) { return pblock; } // return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void DeleteThis() { delete this; }

	IOResult SaveLocalData(ISave *isave, LocalModData *pld);
	IOResult LoadLocalData(ILoad *iload, LocalModData **pld);

	//Constructor/Destructor

	FlowPaint();
	~FlowPaint();

	void* GetInterface(ULONG id);

	// These are the IPainterCanvasInterface_V5 you must instatiate
	// This is called when the user tart a pen stroke
	BOOL  StartStroke();

	BOOL DoStroke(
		BOOL hit,
		IPoint2 mousePos,
		Point3 worldPoint, Point3 worldNormal,
		Point3 localPoint, Point3 localNormal,
		Point3 bary, int index,
		BOOL shift, BOOL ctrl, BOOL alt,
		float radius, float str,
		float pressure, INode *node,
		BOOL mirrorOn,
		Point3 worldMirrorPoint, Point3 worldMirrorNormal,
		Point3 localMirrorPoint, Point3 localMirrorNormal);

	//This is called as the user strokes across the mesh or screen with the mouse down
	BOOL  PaintStroke(
		BOOL hit,
		IPoint2 mousePos,
		Point3 worldPoint, Point3 worldNormal,
		Point3 localPoint, Point3 localNormal,
		Point3 bary, int index,
		BOOL shift, BOOL ctrl, BOOL alt,
		float radius, float str,
		float pressure, INode *node,
		BOOL mirrorOn,
		Point3 worldMirrorPoint, Point3 worldMirrorNormal,
		Point3 localMirrorPoint, Point3 localMirrorNormal
	);

	// This is called as the user ends a strokes when the users has it set to always update
	BOOL  EndStroke();

	// This is called as the user ends a strokes when the users has it set to update on mouse up only
	// the canvas gets a list of all points, normals etc instead of one at a time
	// int ct - the number of elements in the following arrays
	//  <...> see paintstroke() these are identical except they are arrays of values
	BOOL  EndStroke(int ct, BOOL *hit, IPoint2 *mousePos,
		Point3 *worldPoint, Point3 *worldNormal,
		Point3 *localPoint, Point3 *localNormal,
		Point3 *bary, int *index,
		BOOL *shift, BOOL *ctrl, BOOL *alt,
		float *radius, float *str,
		float *pressure, INode **node,
		BOOL mirrorOn,
		Point3 *worldMirrorPoint, Point3 *worldMirrorNormal,
		Point3 *localMirrorPoint, Point3 *localMirrorNormal);

	// This is called as the user cancels a stroke by right clicking
	BOOL  CancelStroke();

	//This is called when the painter want to end a paint session for some reason.
	BOOL  SystemEndPaintSession();
	void PainterDisplay(TimeValue t, ViewExp *vpt, int flags) {}

	void InitUI(HWND hWnd);
	void DestroyUI(HWND hWnd);

	//Some helper functions to handle painter UI
	void Paint();
	void PaintOptions();

private:
	PaintDefromModData *GetPMD(INode *pNode);
	ICustButton *iPaintButton;
	IPainterInterface_V7 *pPainter;
	Tab<PainterNodeList> painterNodeList;
	int lagRate, lagCount;
};