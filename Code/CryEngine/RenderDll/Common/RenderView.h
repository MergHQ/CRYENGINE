// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryThreading/CryThreadSafeRendererContainer.h>
#include <CryThreading/CryThreadSafeWorkerContainer.h>
#include <CryMath/Range.h>

#include "RenderPipeline.h"
#include "RenderItemDrawer.h"

#include "LockFreeAddVector.h"

class CSceneRenderPass;
class CPermanentRenderObject;
struct SGraphicsPipelinePassContext;

// This class encapsulate all renderbale information to render a camera view.
// It stores list of render items added by 3D engine
class CRenderView : public IRenderView
{
public:
	typedef TRange<int> ItemsRange;

	enum EViewType
	{
		eViewType_Default,
		eViewType_Recursive,
		eViewType_Shadow,
	};

	enum eShadowFrustumRenderType
	{
		// NOTE: DO NOT reorder, shadow gen depends on values
		eShadowFrustumRenderType_SunCached   = 0,
		eShadowFrustumRenderType_HeightmapAO = 1,
		eShadowFrustumRenderType_SunDynamic  = 2,
		eShadowFrustumRenderType_LocalLight  = 3,
		eShadowFrustumRenderType_Custom      = 4,

		eShadowFrustumRenderType_Count,
		eShadowFrustumRenderType_First = eShadowFrustumRenderType_SunCached
	};

	//typedef CThreadSafeWorkerContainer<SRendItem> RenderItems;
	typedef lockfree_add_vector<SRendItem>       RenderItems;
	typedef std::vector<SShadowFrustumToRender>  ShadowFrustums;
	typedef std::vector<SShadowFrustumToRender*> ShadowFrustumsPtr;
	typedef lockfree_add_vector<CRenderObject*>  ModifiedObjects;

public:
	//////////////////////////////////////////////////////////////////////////
	// IRenderView
	//////////////////////////////////////////////////////////////////////////
	virtual void   SetFrameId(uint64 frameId) override;
	virtual uint64 GetFrameId() const final                        { return m_frameId; };

	virtual void   SetSkipRenderingFlags(uint32 nFlags) override   { m_nSkipRenderingFlags = nFlags; }
	virtual uint32 GetSkipRenderingFlags() const final             { return m_nSkipRenderingFlags; };

	virtual void   SetShaderRenderingFlags(uint32 nFlags) override { m_nShaderRenderingFlags = nFlags; }
	virtual uint32 GetShaderRenderingFlags() const final           { return m_nShaderRenderingFlags; };

	// Begin/End writing items to the view from 3d engine traversal.
	virtual void         SwitchUsageMode(EUsageMode mode) override;

	virtual CryJobState* GetWriteMutex() override { return &m_jobstate_Write; };
	virtual void         AddPermanentObject(CRenderObject* pObject, const SRenderingPassInfo& passInfo) final;
	//////////////////////////////////////////////////////////////////////////

public:
	CRenderView(const char* name, EViewType type, CRenderView* pParentView = nullptr, ShadowMapFrustum* pShadowFrustumOwner = nullptr);
	~CRenderView();

	EViewType    GetType() const                         { return m_viewType;  }

	void         SetParentView(CRenderView* pParentView) { m_pParentView = pParentView; };
	CRenderView* GetParentView() const                   { return m_pParentView;  };

	RenderItems& GetRenderItems(int nRenderList);
	uint32       GetBatchFlags(int nRenderList) const;

	void         AddRenderItem(CRendElementBase* pElem, CRenderObject* RESTRICT_POINTER pObj, const SShaderItem& shaderItem, uint32 nList, uint32 nBatchFlags,
	                           SRendItemSorter sorter, bool bShadowPass, bool bForceOpaqueForward);

	void       AddPermanentObjectInline(CPermanentRenderObject* pObject, SRendItemSorter sorter, int shadowFrustumSide);

	ItemsRange GetItemsRange(ERenderListID renderList);

	// Find render item index in the sorted list, after when object flag changes.
	int        FindRenderListSplit(ERenderListID list, uint32 objFlag);

	void       PrepareForRendering();

	void       PrepareForWriting();

	bool       IsRecursive() const     { return m_viewType == eViewType_Recursive; }
	bool       IsShadowGenView() const { return m_viewType == eViewType_Shadow; }
	EUsageMode GetUsageMode() const    { return m_usageMode; }
	//////////////////////////////////////////////////////////////////////////
	// Shadows related
	void AddShadowFrustumToRender(const SShadowFrustumToRender& frustum);
	void AddCustomShadowFrustum(const ShadowMapFrustumPtr& pFrustum, const ShadowMapFrustumPtr& pOriginalFrustum);

	// Get render view used for given frustum.
	CRenderView*                         GetShadowsView(ShadowMapFrustum* pFrustum);

	ItemsRange                           GetShadowItemsRange(ShadowMapFrustum* pFrustum, int nFrustumSide);
	RenderItems&                         GetShadowItems(ShadowMapFrustum* pFrustum, int nFrustumSide);
	std::vector<SShadowFrustumToRender>& GetFrustumsToRender() { return m_shadows.m_renderFrustums; };

	ShadowFrustumsPtr& GetShadowFrustumsForLight(int lightId);
	ShadowFrustumsPtr& GetShadowFrustumsByType(eShadowFrustumRenderType type);

	// Can start executing post write jobs on shadow views
	void PostWriteShadowViews();
	void PrepareShadowViews(); // Sync all outstanding shadow preparation jobs
	void SetShadowFrustumOwner(ShadowMapFrustum* pOwner) { m_shadows.m_pShadowFrustumOwner = pOwner; }

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Dynamic Lights
	//////////////////////////////////////////////////////////////////////////
	virtual void          AddDynamicLight(SRenderLight& light) final;
	virtual int           GetDynamicLightsCount() const final;
	virtual SRenderLight& GetDynamicLight(int nLightId) final;

	virtual void          AddLight(eDeferredLightType lightType, SRenderLight& light) final;
	virtual int           GetLightsCount(eDeferredLightType lightType) const final;
	virtual SRenderLight& GetLight(eDeferredLightType lightType, int nLightId) final;
	RenderLightsArray&    GetLightsArray(eDeferredLightType lightType);
	SRenderLight*         AddLightAtIndex(eDeferredLightType lightType, const SRenderLight& light, int index = -1);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Clip Volumes
	//////////////////////////////////////////////////////////////////////////
	uint8                                   AddClipVolume(const IClipVolume* pClipVolume) final;
	void                                    SetClipVolumeBlendInfo(const IClipVolume* pClipVolume, int blendInfoCount, IClipVolume** blendVolumes, Plane* blendPlanes) final;
	const std::vector<SDeferredClipVolume>& GetClipVolumes() const { return m_clipVolumes; }
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Polygons.
	CREClientPoly* AddClientPoly();
	int            GetClientPolyCount() const     { return m_numUsedClientPolygons; }
	CREClientPoly* GetClientPoly(int index) const { assert(index < m_numUsedClientPolygons); return m_polygonsPool[index]; }
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	SDeferredDecal*              AddDeferredDecal(const SDeferredDecal& source);
	std::vector<SDeferredDecal>& GetDeferredDecals();
	size_t                       GetDeferredDecalsCount() const       { return m_deferredDecals.size(); }
	void                         SetDeferredNormalDecals(bool bValue) { m_bDeferrredNormalDecals = bValue; };
	bool                         GetDeferredNormalDecals() const      { return m_bDeferrredNormalDecals; }
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Operation on items.
	//////////////////////////////////////////////////////////////////////////
	void Clear();

private:
	void                   Job_PostWrite();
	void                   Job_SortRenderItemsInList(ERenderListID list);
	void                   ExpandPermanentRenderObjects();
	void                   CompileModifiedRenderObjects();
	void                   UpdateModifiedShaderItems();
	void                   ClearTemporaryCompiledObjects();
	void                   PrepareNearestShadows();
	void                   AddRenderItemsFromClientPolys();
	void                   CheckAndScheduleForUpdate(const SShaderItem& shaderItem);

	CCompiledRenderObject* AllocCompiledObject(CRenderObject* pObj, CRendElementBase* pElem, const SShaderItem& shaderItem);
	CCompiledRenderObject* AllocCompiledObjectTemporary(CRenderObject* pObj, CRendElementBase* pElem, const SShaderItem& shaderItem);
private:
	EUsageMode m_usageMode;
	EViewType  m_viewType;
	string     m_name;
	/// @See SRenderingPassInfo::ESkipRenderingFlags
	uint32     m_nSkipRenderingFlags;
	/// @see EShaderRenderingFlags
	uint32     m_nShaderRenderingFlags;

	uint64     m_frameId;

	// For shadows or recursive view parent will be main rendering view
	CRenderView*    m_pParentView;

	RenderItems     m_renderItems[EFSLIST_NUM];

	volatile uint32 m_BatchFlags[EFSLIST_NUM];
	// For general passes initialized as a pointers to the m_BatchFlags
	// But for shadow pass it will be a pointer to the shadow frustum side mask
	//volatile uint32* m_pFlagsPointer[EFSLIST_NUM];

	// Light sources affecting the view.
	std::vector<SRenderLight> m_lights[eDLT_NumLightTypes];

	//////////////////////////////////////////////////////////////////////////
	// Clip Volumes
	std::vector<SDeferredClipVolume> m_clipVolumes;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Simple polygons storage
	std::vector<CREClientPoly*> m_polygonsPool;
	int                         m_numUsedClientPolygons;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Deferred Decals
	std::vector<SDeferredDecal> m_deferredDecals;
	bool                        m_bDeferrredNormalDecals;

	//////////////////////////////////////////////////////////////////////////

	bool m_bTrackUncompiledItems;
	bool m_bAddingClientPolys;

	// Render objects modified by this view.
	lockfree_add_vector<CPermanentRenderObject*> m_permanentRenderObjectsToCompile;

	// Temporary compiled objects for this frame
	struct SCompiledPair { CRenderObject* pRenderObject; CCompiledRenderObject* pCompiledObject; };
	lockfree_add_vector<SCompiledPair> m_temporaryCompiledObjects;

	// shader items that need to be updated
	lockfree_add_vector<std::pair<CShaderResources*, CShader*>> m_shaderItemsToUpdate;

	// Persistent objects added to the view.
	struct SPermanentObjectRecord
	{
		CPermanentRenderObject* pRenderObject;
		uint32                  itemSorter;
		int                     shadowFrustumSide;
	};
	lockfree_add_vector<SPermanentObjectRecord> m_permanentObjects;

	CCamera       m_camera;       // Current camera
	CRenderCamera m_renderCamera; // Current render camera

	// Internal job states to control when view job processing is done.
	CryJobState                    m_jobstate_Sort;
	CryJobState                    m_jobstate_PostWrite;
	CryJobStateLambda              m_jobstate_Write;
	CryJobStateLambda              m_jobstate_ShadowGen;

	CryCriticalSectionNonRecursive m_lock_UsageMode;
	CryCriticalSectionNonRecursive m_lock_PostWrite;

	volatile int                   m_bPostWriteExecuted;

	struct SShadows
	{
		// Shadow frustums needed for a view.
		ShadowMapFrustum*                                             m_pShadowFrustumOwner;
		std::vector<SShadowFrustumToRender>                           m_renderFrustums;

		std::map<int, ShadowFrustumsPtr>                              m_frustumsByLight;
		std::array<ShadowFrustumsPtr, eShadowFrustumRenderType_Count> m_frustumsByType;

		void Clear();
		void CreateFrustumGroups();
		void PrepareNearestShadows();
	};

public:// temp
	SShadows m_shadows;

private:
	CRenderItemDrawer m_RenderItemDrawer;

public:
	void                     DrawCompiledRenderItems(const SGraphicsPipelinePassContext& passContext) const;

	CRenderItemDrawer&       GetDrawer()       { return m_RenderItemDrawer; }
	const CRenderItemDrawer& GetDrawer() const { return m_RenderItemDrawer; }
};

typedef _smart_ptr<CRenderView> CRenderViewPtr;
