#pragma once

#include "AbnormalsModParamBlock.h"
#include "SubSelectionPolyMod/SubSelectionPolyMod.h"
#include "AbnormalsModLocalData.h"
#include "istdplug.h"
#include <map>
#include "mtl.h"

#define ABNORMALS_MOD_CLASS_NAME "Abnormals"
#define ABNORMALS_MOD_CATEGORY "Crytek"
#define ABNORMALS_MOD_CLASS_ID Class_ID(0x37180919, 0x208e2e07)
#define ID_ABNORMALS_MOD_CHANNEL_DATA 0x396e330e

#define ABNORMALS_NUM_GROUPS 4

#define DEFAULT_GROUP1_COLOR COLVAL_8BIT(103, 158, 33)
#define DEFAULT_GROUP2_COLOR COLVAL_8BIT(249, 183, 0)
#define DEFAULT_GROUP3_COLOR COLVAL_8BIT(234, 80, 28)
#define DEFAULT_GROUP4_COLOR COLVAL_8BIT(0, 133, 191)

#define GROUP_1_VALUE 0.1f
#define GROUP_2_VALUE 0.2f
#define GROUP_3_VALUE 0.3f
#define GROUP_4_VALUE 1.0f // Clear value is 1, because that's the default value assigned to vertex color, illumination, and opacity channels.

#define COLOR_VALUE_TOLERANCE 0.05f

#define CLEAR_GROUP_INDEX ABNORMALS_NUM_GROUPS - 1

class AbnormalsModChannelsDlgProc;
class AbnormalsModEditGroupsDlgProc;
class AbnormalsModAutoChamferDlgProc;
class AbnormalModActionCallback;

class AbnormalsMod : public SubSelectionPolyMod
{
public:
	AbnormalsMod();
	virtual ~AbnormalsMod();

	AbnormalsModLocalData* data;

	IParamBlock2 *pblock2;
	DWORD selectionLevel;
	IObjParam* ip;
	AbnormalsModChannelsDlgProc* generalGroupsDlgProc;
	AbnormalsModEditGroupsDlgProc* editGroupsDlgProc;
	AbnormalsModAutoChamferDlgProc* autoChamferDlgProc;
	AbnormalModActionCallback* abnormalsActionCallback;

	// Cache for rendering overlay
	MaxSDK::Array<Material> overlayMaterials;
	Material selectionMaterial;

	struct DebugNormal
	{
		DebugNormal() {}

		DebugNormal(Point3 aStart, Point3 aEnd)
		{
			start = aStart;
			end = aEnd;
		}
		~DebugNormal() {}
		Point3 start;
		Point3 end;
	};

	MaxSDK::Array<DebugNormal> debugNormals[ABNORMALS_NUM_GROUPS];
	
	struct OverlayTri
	{
	public:
		Point3 pos[3];
		Point3 norms[3];

		OverlayTri() {}

		OverlayTri(Point3 aPos1, Point3 aPos2, Point3 aPos3, Point3 aNorm1, Point3 aNorm2, Point3 aNorm3)
		{
			pos[0] = aPos1;
			pos[1] = aPos2;
			pos[2] = aPos3;
			norms[0] = aNorm1;
			norms[1] = aNorm2;
			norms[2] = aNorm3;
		}
		~OverlayTri() {}
	};

	// One array for each group, plus one for selection. Selection is stored in last.
	MaxSDK::Array<OverlayTri> overlayTris[ABNORMALS_NUM_GROUPS + 1]; 

	MaxSDK::Array<Point3> overlayEdges;
	MaxSDK::Array<Point3> overlaySelectedEdges;

	// Instance variables for param block values.

	// General
	BOOL showNormals;
	float showNormalSize;
	float overlayOpacity;
	BOOL clearSmoothing;

	BOOL autoChamfer;
	enum AutoChamferSource	{ HardEdges = 0, SelectedEdges = 1 };
	AutoChamferSource chamferSource;
	enum AutoChamferMethod { Standard = 0, Quad = 1 };
	AutoChamferMethod chamferMethod;
	float chamferRadius;
	int chamferSegments;

	BOOL autoLoadEnabled;
	int loadFromChannel;
	BOOL loadLegacyColors;
	BOOL autoSaveEnabled;
	int saveToChannel;

	bool AssignGroupToSelectedFaces(int group, bool registerUndo);
	void SelectGroup(int group, TimeValue t);
	static void GenerateChamferNormals(MNMesh* mesh, MaxSDK::Array<UVVert> groupColors, int channel);
	
	void ApplyGroupsToMesh(MNMesh* mesh, int channel);
	void ConvertFromLegacyColors(MNMesh* mesh, int channel);
	int GetAutoLoadChannel(ObjectState *os);
	static int SelectionToMapChannel(int channelType, int uvChannel);
	static int MapChannelToSelection(int channel, int& outUVChannel);

	static UVVert GetGroupValue(int group, bool legacyColors = false);

	void UpdateMemberVarsFromParamBlock(TimeValue t, ObjectState *os);
	void UpdateControls(HWND hWnd, eAbnormalsModRollout::Type rollout);

	virtual void NotifyPostCollapse(INode *node, Object *obj, IDerivedObject *derObj, int index) override;
	void UpdateDisplayCache(MNMesh* mesh);

private:
	bool showingGroupColors;

	// ---- From SubSelectionPolyMod ----
public:
	virtual PolyMeshSelData* NewLocalModData(MNMesh* mesh) override
	{
		if (mesh == NULL)
			return new AbnormalsModLocalData();
		else
			return new AbnormalsModLocalData(mesh);
	}
	virtual void ActivateSubobjSel(int level, XFormModes &modes) override;
	virtual void LoadLocalDataChunk(ILoad *iload, PolyMeshSelData *d) override;
	virtual int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags, ModContext *mc) override;

	// ---- From Modifier ----
public:
	virtual ChannelMask ChannelsUsed() override { return OBJ_CHANNELS; } // We use all channels as input
	virtual ChannelMask ChannelsChanged() override { return SubSelectionPolyMod::ChannelsChanged() | TOPO_CHANNEL | GEOM_CHANNEL | VERTCOLOR_CHANNEL | TEXMAP_CHANNEL; } // Channels we may modify

	virtual BOOL DependOnTopology(ModContext &mc) override;

	virtual void ModifyObject(TimeValue t, ModContext &mc, ObjectState *os, INode *node) override;
	virtual IOResult SaveLocalData(ISave *isave, LocalModData *ld) override;	

	// ---- From RefTargetHandle ----
public:
	virtual RefTargetHandle Clone(RemapDir& remap) override;

	// ---- From ReferenceMaker
public:
	virtual RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate) override;
	virtual RefTargetHandle GetReference(int i) override { return (RefTargetHandle)pblock2; }
	virtual int NumRefs() override  { return 1; }

	// ---- From BaseObject ----
public:
	virtual const TCHAR* GetObjectName() override { return GetString(IDS_ABNORMALSMOD); }
	virtual CreateMouseCallBack* GetCreateMouseCallBack() override { return NULL; }

	// ---- From Animateable ----
public:
	virtual void DeleteThis() override { delete this; }
	Class_ID ClassID() override { return ABNORMALS_MOD_CLASS_ID; }
	SClass_ID SuperClassID() override { return OSM_CLASS_ID; }
	void GetClassName(TSTR& s) override { s = GetString(IDS_ABNORMALSMOD); }
	virtual void BeginEditParams (IObjParam *ip, ULONG flags, Animatable *prev) override;
	virtual void EndEditParams (IObjParam *ip, ULONG flags, Animatable *next) override;	
	virtual void SetReference(int i, RefTargetHandle rtarg) override
	{
		pblock2 = (IParamBlock2*)rtarg;
		Modifier::SetReference(i, rtarg);
	}

	// ---- From BaseObject ----
public:
	int	NumParamBlocks() override { return 1; }	// return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int i) override { return pblock2; } // return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) override
	{
		return (pblock2->ID() == id) ? pblock2 : NULL; // return id'd ParamBlock
	}
};