#pragma once

#include "GeomDecalParamBlock.h"

#include <xtcobject.h>
#include "Graphics/IObjectDisplay2.h"
#include "ref.h"

#define GEOM_DECAL_CLASS_NAME "GeomDecal"
#define GEOM_DECAL_CATEGORY "Crytek"
#define GEOM_DECAL_CLASS_ID Class_ID(0x431b432b, 0x6980c907)

namespace eSpacingType
{
	enum Type
	{
		bounds = 0,
		pivot = 1
	};
}

namespace eCopyType
{
	enum Type
	{
		distance = 0,
		count = 1
	};
}

class GeomDecal : public SimpleObject2
{
public:
	GeomDecal(BOOL loading);
	virtual ~GeomDecal();

	static IObjParam *ip; //Access to the interface

	// Instance variables for param block values.
	Point3 projectionSize;	
	float angleCutoff;
	MaxSDK::Array<INode*> objectPool;
	MaxSDK::Array<BOOL> objectsNeedCleanup;
	Point3 uvTiling;
	Point3 uvOffset;
	float meshPush;
	int meshMatID;

	void UpdateObjectPool(TimeValue t);
	void ClearObjectPool();
	void UpdateMemberVarsFromParamBlock(TimeValue t);
	void UpdateControls(HWND hWnd, eGeomDecalRollout::Type rollout);
	Box3 GetProjectionCage();
	Matrix3 DecalProjectionMatrix();
	int DrawAndHit(TimeValue t, INode* inode, ViewExp *vpt);	

	// ---- From BaseObject ----
	virtual unsigned long GetObjectDisplayRequirement() const override
	{
		// Re-enable the Display() function.
		unsigned long prev = SimpleObject2::GetObjectDisplayRequirement();
		return prev | MaxSDK::Graphics::ObjectDisplayRequireLegacyDisplayMode;
	}
	virtual BOOL IsWorldSpaceObject() override { return TRUE; }
	virtual void WSStateInvalidate() override {	MeshInvalid(); }

	virtual const TCHAR* GetObjectName() override { return GetString(IDS_GEOM_DECAL); }
	virtual int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) override;
	virtual int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) override;

	// ---- From GeomObject ----
	virtual int IsInstanceDependent() { return 1; }

	// ---- From SimpleObject ----
	virtual void BuildMesh(TimeValue t) override;
	virtual void InvalidateUI() override;
	virtual CreateMouseCallBack* GetCreateMouseCallBack() override;
	virtual BOOL OKtoDisplay(TimeValue t) override { return TRUE; }
	virtual void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box) override;
	virtual void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box) override;
	virtual void GetDeformBBox(TimeValue t, Box3& box, Matrix3 *tm, BOOL useSel) override;

	// ---- From RefTargetHandle ----
	virtual RefTargetHandle Clone(RemapDir& remap) override;

	// ---- From Animateable ----
	Class_ID ClassID() { return GEOM_DECAL_CLASS_ID; }
	SClass_ID SuperClassID() { return GEOMOBJECT_CLASS_ID; }
	void GetClassName(TSTR& s) { s = GetString(IDS_GEOM_DECAL); }
	virtual void BeginEditParams (IObjParam *ip, ULONG flags, Animatable *prev) override;
	virtual void EndEditParams (IObjParam *ip, ULONG flags, Animatable *next) override;	

	// ---- From BaseObject ----
	int	NumParamBlocks() override { return 1; }
	IParamBlock2* GetParamBlock(int i) override { return pblock2; }
	IParamBlock2* GetParamBlockByID(BlockID id) override { return (pblock2->ID() == id) ? pblock2 : NULL; } 
};

#define CLASS_ID_GeomDecalXTC Class_ID(0x13648d5d, 0xa45a4d9)

class GeomDecalXTC : public XTCObject
{
public:
	GeomDecalXTC() { }
	~GeomDecalXTC() { }
	virtual Class_ID ExtensionID() override { return CLASS_ID_GeomDecalXTC; }
	ChannelMask DependsOn() { return TOPO_CHANNEL | TEXMAP_CHANNEL | VERTCOLOR_CHANNEL; }
	ChannelMask ChannelsChanged() { return TOPO_CHANNEL | TEXMAP_CHANNEL | VERTCOLOR_CHANNEL; }
	void PreChanChangedNotify(TimeValue t, ModContext &mc, ObjectState *os, INode *node, Modifier *mod, bool bEndOfPipeline);
	void PostChanChangedNotify(TimeValue t, ModContext &mc, ObjectState *os, INode *node, Modifier *mod, bool bEndOfPipeline);
	virtual XTCObject *Clone() override { return new GeomDecalXTC(); }
	virtual void DeleteThis() override { delete this; }
	virtual BOOL SuspendObjectDisplay() override { return false; }
};