#pragma once
#include "istdplug.h"

#include "PolyMeshSelData.h"

class SubObjectType;

// A simple abstract modifier class that supports subselection of verts, edges, faces, and elements. Which are supported can specified in the constructor.
class SubSelectionPolyMod : public Modifier, public IMeshSelect
{
public:
	enum SelectionTypeFlag
	{
		SEL_OBJECT = 0,
		SEL_VERTEX = 1 << 0,
		SEL_EDGE = 1 << 1,
		SEL_BORDER = 1 << 2,
		SEL_FACE = 1 << 3,
		SEL_ELEMENT = 1 << 4,
		SelectionjTypeQty = 5
	};

public:
	SubSelectionPolyMod(int SubSelectionTypes);

private:
	const int enabledSubSelectionTypes; // Flags determine which subobject types are supported. This must be set at construction time.
	int currentSubSelectionType; // Currently enabled selection flag.

public:
	SubObjectType* GetSubSelection(int type);
	SubObjectType* GetCurrentSubSelection();
	SelectionTypeFlag GetTypeFlagFromLevel(int level);
	int GetCurrentSubSelectionType() { return currentSubSelectionType; }

	void SelectOpenEdges(int selType = 0);

protected:
	virtual PolyMeshSelData* NewLocalModData(MNMesh* mesh = NULL)
	{
		if (mesh == NULL)
			return new PolyMeshSelData();
		else
			return new PolyMeshSelData(mesh);
	}

	// ---- From Animatable ----
	void* GetInterface(ULONG id) 
	{ 
		if (id == I_MESHSELECT) 
			return (IMeshSelect *)this; 
		else 
			return Modifier::GetInterface(id); 
	}

	// ---- From Modifier ----
public:
	virtual Class_ID InputType() override { return polyObjectClassID; } // Only operate on poly objects
	virtual ChannelMask ChannelsChanged() override { return SELECT_CHANNEL; }

	virtual int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt, ModContext* mc) override;
	virtual int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags, ModContext *mc) override;
	virtual void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp *vpt, Box3& box, ModContext *mc) override;
	virtual void GetSubObjectCenters(SubObjAxisCallback *cb, TimeValue t, INode *node, ModContext *mc) override;
	virtual void GetSubObjectTMs(SubObjAxisCallback *cb, TimeValue t, INode *node, ModContext *mc) override;

	virtual int NumSubObjTypes() override;
	virtual ISubObjType* GetSubObjType(int i) override;

	virtual void ActivateSubobjSel(int level, XFormModes &modes) override;
	virtual void SelectSubComponent(HitRecord *firstHit, BOOL selected, BOOL all, BOOL invert)override;
	virtual void ClearSelection(int selLevel) override;
	virtual void SelectAll(int selLevel) override;
	virtual void InvertSelection(int selLevel) override;

	virtual void ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node) override;
	virtual void BeginEditParams(IObjParam *ip, ULONG flags, Animatable *prev) override;
	virtual void EndEditParams(IObjParam *ip, ULONG flags, Animatable *next) override;

	virtual IOResult Save(ISave *isave) override;
	virtual IOResult Load(ILoad *iload) override;
	virtual IOResult SaveLocalData(ISave *isave, LocalModData *ld) override;
	virtual IOResult LoadLocalData(ILoad *iload, LocalModData **pld) override;
	virtual void LoadLocalDataChunk(ILoad *iload, PolyMeshSelData *pld);

	// ----- From IMeshSelect
public:
	virtual DWORD GetSelLevel() override;
	virtual void SetSelLevel(DWORD level) override;
	virtual void LocalDataChanged() override;
};

class SubObjectType : public GenSubObjType
{
public:
	SubObjectType(MSTR name, int idx, PMeshSelLevel MeshLevel, int HitLevel, int LevelDisplayFlags, int LevelEnum) : GenSubObjType (name, _T(""), idx),
		meshLevel(MeshLevel), hitLevel(HitLevel), levelDisplayFlags(LevelDisplayFlags), levelEnum(LevelEnum) {}

	const PMeshSelLevel meshLevel;
	const int hitLevel;
	const int levelDisplayFlags;
	const int levelEnum;
};