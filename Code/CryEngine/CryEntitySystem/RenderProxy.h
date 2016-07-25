// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   RenderProxy.h
//  Version:     v1.00
//  Created:     19/5/2004 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#ifndef __RenderProxy_h__
#define __RenderProxy_h__
#pragma once

#include "EntitySystem.h"
#include <CryRenderer/IRenderer.h>
#include <CryEntitySystem/IEntityRenderState.h>

// forward declarations.
class CEntityObject;
class CEntity;
struct SRendParams;
struct IShaderPublicParams;
class CEntityTimeoutList;
class CRenderProxy;
struct AnimEventInstance;

//////////////////////////////////////////////////////////////////////////
// Description:
//    CRenderProxy provides default implementation of IEntityRenderProxy interface.
//    This is a renderable object that is registered in 3D engine sector.
//    It can contain multiple sub object slots that can have their own relative transformation, and
//    each slot can represent specific renderable interface (IStatObj,ICharacterInstance,etc..)
//    Slots can also form hierarchical transformation tree, every slot can reference parent slot for transformation.
///////////////////////////////////////////////////////////// /////////////
class CRenderProxy : public IRenderNode, public IEntityRenderProxy
{
public:

	enum EFlags // Must be limited to 32 flags.
	{
		FLAG_CUSTOM_POST_EFFECT             = BIT(0), // Has custom post effect ID stored in custom data
		FLAG_BBOX_VALID_LOCAL               = BIT(1),
		FLAG_BBOX_FORCED                    = BIT(2),
		FLAG_BBOX_INVALID                   = BIT(3),
		FLAG_HIDDEN                         = BIT(4), // If render proxy is hidden.
		//FLAG_HAS_ENV_LIGHTING  = 0x0020, // If render proxy have environment lighting.
		FLAG_UPDATE                         = BIT(5),  // If render proxy needs to be updated.
		FLAG_NOW_VISIBLE                    = BIT(6),  // If render proxy currently visible.
		FLAG_REGISTERED_IN_3DENGINE         = BIT(7),  // If render proxy have been registered in 3d engine.
		FLAG_POST_INIT                      = BIT(8),  // If render proxy have received Post init event.
		FLAG_HAS_LIGHTS                     = BIT(9),  // If render proxy has some lights.
		FLAG_GEOMETRY_MODIFIED              = BIT(10), // Geometry for this render slot was modified at runtime.
		FLAG_HAS_CHILDRENDERNODES           = BIT(11), // If render proxy contains child render nodes
		FLAG_HAS_PARTICLES                  = BIT(12), // If render proxy contains particle emitters
		FLAG_SHADOW_DISSOLVE                = BIT(13), // If render proxy requires dissolving shadows
		FLAG_FADE_CLOAK_BY_DISTANCE         = BIT(14), // If render proxy requires fading cloak by distance
		FLAG_IGNORE_HUD_INTERFERENCE_FILTER = BIT(15), // HUD render proxy ignores hud interference filter post effect settings
		FLAG_IGNORE_HEAT_VALUE              = BIT(16), // Don't appear hot in nano vision
		FLAG_POST_3D_RENDER                 = BIT(17), // Render proxy in post 3D pass
		FLAG_IGNORE_CLOAK_REFRACTION_COLOR  = BIT(18), // Will ignore cloak refraction color
		FLAG_HUD_REQUIRE_DEPTHTEST          = BIT(19), // If 3D HUD Object requires to be rendered at correct depth (i.e. behind weapon)
		FLAG_CLOAK_INTERFERENCE             = BIT(20), // When set the cloak will use the cloak interference parameters
		FLAG_CLOAK_HIGHLIGHTS               = BIT(21), // When set the cloak will use the cloak highlight parameters
		FLAG_HUD_DISABLEBLOOM               = BIT(22), // Allows 3d hud object to disable bloom (Required to avoid overglow and cutoff with alien hud ghosted planes)
		FLAG_ANIMATE_OFFSCREEN_SHADOW       = BIT(23), // Update the animation if object drawn in the shadow pass
		FLAG_DISABLE_MOTIONBLUR             = BIT(24), // Disable motion blur
		FLAG_EXECUTE_AS_JOB_FLAG            = BIT(25), // set if this CRenderProxy can be executed as a Job from the 3DEngine
		FLAG_RECOMPUTE_EXECUTE_AS_JOB_FLAG  = BIT(26), // set if the slots changed, to recheck if this renderproxy can execute as a job
	};

	CRenderProxy();
	~CRenderProxy();

	EERType      GetRenderNodeType() override;
	virtual void GetMemoryUsage(ICrySizer* pSizer) const override;
	virtual bool CanExecuteRenderAsJob() override;

	// Must be called after constructor.
	void PostInit();

	//////////////////////////////////////////////////////////////////////////
	// IEntityEvent implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void Initialize(const SComponentInitializer& init) override;
	virtual void ProcessEvent(SEntityEvent& event) override;
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityProxy implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual EEntityProxy GetType() override                                          { return ENTITY_PROXY_RENDER; }
	virtual void         Release() override;
	virtual void         Done() override                                             {}
	// Called when the subsystem initialize.
	virtual bool         Init(IEntity* pEntity, SEntitySpawnParams& params) override { return true; }
	virtual void         Reload(IEntity* pEntity, SEntitySpawnParams& params) override;
	virtual void         Update(SEntityUpdateContext& ctx) override;
	virtual void         SerializeXML(XmlNodeRef& entityNode, bool bLoading) override;
	virtual void         Serialize(TSerialize ser) override;
	virtual bool         NeedSerialize() override;
	virtual bool         GetSignature(TSerialize signature) override;
	virtual void         SetViewDistRatio(int nViewDistRatio) override;
	virtual void         SetLodRatio(int nLodRatio) override;
	//////////////////////////////////////////////////////////////////////////

	virtual void             SetMinSpec(int nMinSpec) override;
	virtual bool             GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const override;
	void                     UpdateLodDistance(const SFrameLodInfo& frameLodInfo);

	virtual bool             PhysicalizeFoliage(bool bPhysicalize = true, int iSource = 0, int nSlot = 0) override;
	virtual IPhysicalEntity* GetBranchPhys(int idx, int nSlot = 0) override { IFoliage* pFoliage = GetFoliage(); return pFoliage ? pFoliage->GetBranchPhysics(idx) : 0; }
	virtual struct IFoliage* GetFoliage(int nSlot = 0) override;

	virtual void             OnRenderNodeBecomeVisible(const SRenderingPassInfo& passInfo) override;

	//////////////////////////////////////////////////////////////////////////
	// IEntityRenderProxy interface.
	//////////////////////////////////////////////////////////////////////////
	// Description:
	//    Force local bounds.
	// Arguments:
	//    bounds - Bounding box in local space.
	//    bDoNotRecalculate - when set to true entity will never try to recalculate local bounding box set by this call.
	virtual void SetLocalBounds(const AABB& bounds, bool bDoNotRecalculate) override;
	virtual void GetWorldBounds(AABB& bounds) override;
	virtual void GetLocalBounds(AABB& bounds) override;
	virtual void InvalidateLocalBounds() override;

	//////////////////////////////////////////////////////////////////////////
	virtual IMaterial* GetRenderMaterial(int nSlot = -1) override;
	// Set shader parameters for this instance
	//virtual void SetShaderParam(const char *pszName, UParamVal &pParam, int nType = 0) ;

	virtual struct IRenderNode*  GetRenderNode() override { return this; };
	virtual IShaderPublicParams* GetShaderPublicParams(bool bCreate = true) override;

	virtual void                 AddShaderParamCallback(IShaderParamCallbackPtr pCallback) override;
	virtual bool                 RemoveShaderParamCallback(IShaderParamCallbackPtr pCallback) override;
	virtual void                 CheckShaderParamCallbacks() override;
	virtual void                 ClearShaderParamCallbacks() override;

	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// IEntityRenderProxy implementation.
	//////////////////////////////////////////////////////////////////////////
	bool                  IsSlotValid(int nIndex) const { return nIndex >= 0 && nIndex < (int)m_slots.size() && m_slots[nIndex] != NULL; };
	int                   GetSlotCount() const override;
	bool                  SetParentSlot(int nParentIndex, int nChildIndex);
	bool                  GetSlotInfo(int nIndex, SEntitySlotInfo& slotInfo) const;
	void                  SetSlotMaterial(int nSlot, IMaterial* pMaterial) override;
	IMaterial*            GetSlotMaterial(int nSlot) override;
	void                  SetSubObjHideMask(int nSlot, hidemask nSubObjHideMask) override;
	hidemask              GetSubObjHideMask(int nSlot) const override;
	void                  ClearSlots() override { FreeAllSlots(); }

	void                  SetSlotFlags(int nSlot, uint32 nFlags);
	uint32                GetSlotFlags(int nSlot);

	ICharacterInstance*   GetCharacter(int nSlot);
	IStatObj*             GetStatObj(int nSlot);
	IParticleEmitter*     GetParticleEmitter(int nSlot);
#if defined(USE_GEOM_CACHES)
	IGeomCacheRenderNode* GetGeomCacheRenderNode(int nSlot);
#endif

	int  SetSlotGeometry(int nSlot, IStatObj* pStatObj);
	void QueueSlotGeometryChange(int nSlot, IStatObj* pStatObj);
	int  SetSlotCharacter(int nSlot, ICharacterInstance* pCharacter);
	int  LoadGeometry(int nSlot, const char* sFilename, const char* sGeomName = NULL, int nLoadFlags = 0);
	int  LoadCharacter(int nSlot, const char* sFilename, int nLoadFlags = 0);
	int  LoadParticleEmitter(int nSlot, IParticleEffect* pEffect, SpawnParams const* params = NULL, bool bPrime = false, bool bSerialize = false);
	int  SetParticleEmitter(int nSlot, IParticleEmitter* pEmitter, bool bSerialize = false);
	int  LoadLight(int nSlot, CDLight* pLight, uint16 layerId);
	int  LoadCloud(int nSlot, const char* sFilename);
	int  SetCloudMovementProperties(int nSlot, const SCloudMovementProperties& properties);
	int  LoadCloudBlocker(int nSlot, const SCloudBlockerProperties& properties);
	int  LoadFogVolume(int nSlot, const SFogVolumeProperties& properties);
	int  FadeGlobalDensity(int nSlot, float fadeTime, float newGlobalDensity);
#if defined(USE_GEOM_CACHES)
	int  LoadGeomCache(int nSlot, const char* sFilename);
#endif

#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
	int LoadPrismObject(int nSlot);
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

	int LoadVolumeObject(int nSlot, const char* sFilename);
	int SetVolumeObjectMovementProperties(int nSlot, const SVolumeObjectMovementProperties& properties);
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Slots.
	//////////////////////////////////////////////////////////////////////////
	ILINE CEntityObject* GetSlot(int nIndex) const { return (nIndex >= 0 && nIndex < (int)m_slots.size()) ? m_slots[nIndex] : NULL; }
	CEntityObject*       GetParentSlot(int nIndex) const;

	// Allocate a new object slot.
	CEntityObject* AllocSlot(int nIndex);
	// Frees existing object slot, also optimizes size of slot array.
	void           FreeSlot(int nIndex);
	void           FreeAllSlots();
	void           ExpandCompoundSlot0(); // if there's a compound cgf in slot 0, expand it into subobjects (slots)

	// Returns world transformation matrix of the object slot.
	const Matrix34& GetSlotWorldTM(int nIndex) const;
	// Returns local relative to host entity transformation matrix of the object slot.
	const Matrix34& GetSlotLocalTM(int nIndex, bool bRelativeToParent) const;
	// Set local transformation matrix of the object slot.
	void            SetSlotLocalTM(int nIndex, const Matrix34& localTM, int nWhyFlags = 0);
	// Set camera space position of the object slot
	void            SetSlotCameraSpacePos(int nIndex, const Vec3& cameraSpacePos);
	// Get camera space position of the object slot
	void            GetSlotCameraSpacePos(int nSlot, Vec3& cameraSpacePos) const;

	// Invalidates local or world space bounding box.
	void InvalidateBounds(bool bLocal, bool bWorld);

	// Register render proxy in 3d engine.
	void        RegisterForRendering(bool bEnable);
	static void RegisterCharactersForRendering();

	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	// IRenderNode interface implementation.
	//////////////////////////////////////////////////////////////////////////
	virtual void                       SetLayerId(uint16 nLayerId) override;
	virtual void                       SetMaterial(IMaterial* pMat) override;
	virtual IMaterial*                 GetMaterial(Vec3* pHitPos = NULL) const override;
	virtual IMaterial*                 GetMaterialOverride() override { return m_pMaterial; }
	virtual const char*                GetEntityClassName() const override;
	virtual Vec3                       GetPos(bool bWorldOnly = true) const override;
	virtual const Ang3&                GetAngles(int realA = 0) const;
	virtual float                      GetScale() const;
	virtual const char*                GetName() const override;
	virtual void                       GetRenderBBox(Vec3& mins, Vec3& maxs);
	virtual void                       GetBBox(Vec3& mins, Vec3& maxs);
	virtual void                       FillBBox(AABB& aabb) override;
	virtual bool                       HasChanged() { return false; }
	virtual void                       Render(const struct SRendParams& EntDrawParams, const SRenderingPassInfo& passInfo) override;
	virtual struct IStatObj*           GetEntityStatObj(unsigned int nPartId, unsigned int nSubPartId, Matrix34A* pMatrix = NULL, bool bReturnOnlyVisible = false) override;
	virtual void                       SetEntityStatObj(unsigned int nSlot, IStatObj* pStatObj, const Matrix34A* pMatrix = NULL) override {}
	virtual IMaterial*                 GetEntitySlotMaterial(unsigned int nPartId, bool bReturnOnlyVisible, bool* pbDrawNear) override;
	virtual struct ICharacterInstance* GetEntityCharacter(unsigned int nSlot, Matrix34A* pMatrix = NULL, bool bReturnOnlyVisible = false) override;
#if defined(USE_GEOM_CACHES)
	virtual IGeomCacheRenderNode*      GetGeomCacheRenderNode(unsigned int nSlot, Matrix34A* pMatrix = NULL, bool bReturnOnlyVisible = false) override;
#endif
	virtual bool                       IsRenderProxyVisAreaVisible() const override;
	virtual struct IPhysicalEntity* GetPhysics() const override;
	virtual void                    SetPhysics(IPhysicalEntity* pPhys) override                                           {}
	virtual void                    PreloadInstanceResources(Vec3 vPrevPortalPos, float fPrevPortalDistance, float fTime) {}

	// cppcheck-suppress passedByValue
	void          Render_JobEntry(const SRendParams inRenderParams, const SRenderingPassInfo passInfo);

	virtual float GetMaxViewDist() override;

	virtual void  SetMaterialLayersMask(uint8 nMtlLayers) override;
	virtual uint8 GetMaterialLayersMask() const override
	{
		return m_nMaterialLayers;
	}

	virtual void SetMaterialLayersBlend(uint32 nMtlLayersBlend) override
	{
		m_nMaterialLayersBlend = nMtlLayersBlend;
	}

	virtual uint32 GetMaterialLayersBlend() const override
	{
		return m_nMaterialLayersBlend;
	}

	virtual void SetCloakHighlightStrength(float highlightStrength) override
	{
		m_fCustomData[0] = clamp_tpl(highlightStrength, 0.0f, 1.0f);

		if (highlightStrength > 0.0f)
		{
			AddFlags(FLAG_CLOAK_HIGHLIGHTS);
		}
		else
		{
			ClearFlags(FLAG_CLOAK_HIGHLIGHTS);
		}
	}

	virtual void SetMotionBlur(bool enable) override
	{
		if (enable)
		{
			AddFlags(FLAG_DISABLE_MOTIONBLUR);
		}
		else
		{
			ClearFlags(FLAG_DISABLE_MOTIONBLUR);
		}
	}

	virtual void SetCloakInterferenceState(bool bHasCloakInterference) override
	{
		if (bHasCloakInterference)
		{
			AddFlags(FLAG_CLOAK_INTERFERENCE);
		}
		else
		{
			ClearFlags(FLAG_CLOAK_INTERFERENCE);
		}
	}

	virtual bool GetCloakInterferenceState() const override
	{
		return (m_nFlags & FLAG_CLOAK_INTERFERENCE) ? true : false;
	}

	virtual void SetCloakColorChannel(uint8 nCloakColorChannel) override
	{
		m_nCloakColorChannel = nCloakColorChannel;
	}

	virtual uint8 GetCloakColorChannel() const override
	{
		return m_nCloakColorChannel;
	}

	virtual void SetCloakFadeByDistance(bool bCloakFadeByDistance) override
	{
		if (bCloakFadeByDistance)
		{
			AddFlags(FLAG_FADE_CLOAK_BY_DISTANCE);
		}
		else
		{
			ClearFlags(FLAG_FADE_CLOAK_BY_DISTANCE);
		}
	}

	virtual bool DoesCloakFadeByDistance() const override
	{
		return (m_nFlags & FLAG_FADE_CLOAK_BY_DISTANCE) ? true : false;
	}

	virtual void SetCloakBlendTimeScale(float fCloakBlendTimeScale) override
	{
		m_nCloakBlendTimeScale = (uint8) min(255.0f, max(1.0f, fCloakBlendTimeScale * 64.0f));
	}

	virtual float GetCloakBlendTimeScale() const override
	{
		return ((float)m_nCloakBlendTimeScale) * (1.0f / 64.0f);
	}

	virtual void SetIgnoreCloakRefractionColor(bool bIgnoreCloakRefractionColor) override
	{
		if (bIgnoreCloakRefractionColor)
		{
			AddFlags(FLAG_IGNORE_CLOAK_REFRACTION_COLOR);
		}
		else
		{
			ClearFlags(FLAG_IGNORE_CLOAK_REFRACTION_COLOR);
		}
	}

	virtual bool DoesIgnoreCloakRefractionColor() const override
	{
		return (m_nFlags & FLAG_IGNORE_CLOAK_REFRACTION_COLOR) ? true : false;
	}

	virtual void SetCustomPostEffect(const char* pPostEffectName) override;

	virtual void SetAsPost3dRenderObject(bool bPost3dRenderObject, uint8 groupId, float groupScreenRect[4]) override;

	virtual void SetIgnoreHudInterferenceFilter(const bool bIgnoreFiler) override
	{
		if (bIgnoreFiler)
		{
			AddFlags(FLAG_IGNORE_HUD_INTERFERENCE_FILTER);
		}
		else
		{
			ClearFlags(FLAG_IGNORE_HUD_INTERFERENCE_FILTER);
		}
	}

	virtual void SetHUDRequireDepthTest(const bool bRequire) override
	{
		if (bRequire)
		{
			AddFlags(FLAG_HUD_REQUIRE_DEPTHTEST);
		}
		else
		{
			ClearFlags(FLAG_HUD_REQUIRE_DEPTHTEST);
		}
	}

	virtual void SetHUDDisableBloom(const bool bDisableBloom) override
	{
		if (bDisableBloom)
		{
			AddFlags(FLAG_HUD_DISABLEBLOOM);
		}
		else
		{
			ClearFlags(FLAG_HUD_DISABLEBLOOM);
		}
	}

	virtual void SetIgnoreHeatAmount(bool bIgnoreHeat) override
	{
		if (bIgnoreHeat)
		{
			AddFlags(FLAG_IGNORE_HEAT_VALUE);
		}
		else
		{
			ClearFlags(FLAG_IGNORE_HEAT_VALUE);
		}
	}

	//! Allows to adjust vision params:
	virtual void SetVisionParams(float r, float g, float b, float a) override
	{
		m_nVisionParams = (uint32) (int_round(r * 255.0f) << 24) | (int_round(g * 255.0f) << 16) | (int_round(b * 255.0f) << 8) | (int_round(a * 255.0f));
	}

	//! return vision params
	virtual uint32 GetVisionParams() const override { return m_nVisionParams; }

	//! Allows to adjust hud silhouettes params:
	// . binocular view uses color and alpha for blending
	virtual void SetHUDSilhouettesParams(float r, float g, float b, float a) override
	{
		m_nHUDSilhouettesParams = (uint32) (int_round(r * 255.0f) << 24) | (int_round(g * 255.0f) << 16) | (int_round(b * 255.0f) << 8) | (int_round(a * 255.0f));
	}

	//! return vision params
	virtual uint32 GetHUDSilhouettesParams() const override { return m_nHUDSilhouettesParams; }

	//! set shadow dissolve state
	virtual void SetShadowDissolve(bool enable) override
	{
		if (enable)
			AddFlags(FLAG_SHADOW_DISSOLVE);
		else
			ClearFlags(FLAG_SHADOW_DISSOLVE);
	}

	//! return shadow dissolve state
	virtual bool GetShadowDissolve() const override { return CheckFlags(FLAG_SHADOW_DISSOLVE); }

	//! Allows to adjust effect layer params
	virtual void SetEffectLayerParams(const Vec4& pParams) override
	{
		m_pLayerEffectParams = (uint32) (int_round(pParams.x * 255.0f) << 24) | (int_round(pParams.y * 255.0f) << 16) | (int_round(pParams.z * 255.0f) << 8) | (int_round(pParams.w * 255.0f));
	}

	//! Allows to adjust effect layer params
	virtual void SetEffectLayerParams(uint32 nEncodedParams) override
	{
		m_pLayerEffectParams = nEncodedParams;
	}

	//! return effect layer params
	virtual const uint32 GetEffectLayerParams() const override { return m_pLayerEffectParams; }

	virtual void         SetOpacity(float fAmount) override
	{
		m_nOpacity = int_round(clamp_tpl<float>(fAmount, 0.0f, 1.0f) * 255);
	}

	virtual float      GetOpacity() const override                { return (float) m_nOpacity / 255.0f; }

	virtual const AABB GetBBox() const override                   { return m_WSBBox; }
	virtual void       SetBBox(const AABB& WSBBox) override       { m_WSBBox = WSBBox; }
	virtual void       OffsetPosition(const Vec3& delta) override {}

	virtual float      GetLastSeenTime() const override           { return m_fLastSeenTime; }
	//////////////////////////////////////////////////////////////////////////

	// Internal slot access function.
	ILINE CEntityObject* Slot(int nIndex) const { assert(nIndex >= 0 && nIndex < (int)m_slots.size()); return m_slots[nIndex]; }
	IStatObj*            GetCompoundObj() const;

	//////////////////////////////////////////////////////////////////////////
	// Flags
	//////////////////////////////////////////////////////////////////////////
	ILINE void   SetFlags(uint32 flags)                { m_nFlags = flags; };
	ILINE uint32 GetFlags()                            { return m_nFlags; };
	ILINE void   AddFlags(uint32 flagsToAdd)           { SetFlags(m_nFlags | flagsToAdd); };
	ILINE void   ClearFlags(uint32 flagsToClear)       { SetFlags(m_nFlags & ~flagsToClear); };
	ILINE bool   CheckFlags(uint32 flagsToCheck) const { return (m_nFlags & flagsToCheck) == flagsToCheck; };
	//////////////////////////////////////////////////////////////////////////

	// Change custom material.
	void       SetCustomMaterial(IMaterial* pMaterial);
	// Get assigned custom material.
	IMaterial* GetCustomMaterial() { return m_pMaterial; }

	void       UpdateEntityFlags();
	void       SetLastSeenTime(float fNewTime) { m_fLastSeenTime = fNewTime; }

	void       DebugDraw(const SGeometryDebugDrawInfo& info);

	//////////////////////////////////////////////////////////////////////////
	// Custom new/delete.
	//////////////////////////////////////////////////////////////////////////
	void*             operator new(size_t nSize);
	void              operator delete(void* ptr);

	static void       SetTimeoutList(CEntityTimeoutList* pList)          { s_pTimeoutList = pList; }

	static ILINE void SetViewDistMin(float fViewDistMin)                 { s_fViewDistMin = fViewDistMin; }
	static ILINE void SetViewDistRatio(float fViewDistRatio)             { s_fViewDistRatio = fViewDistRatio; }
	static ILINE void SetViewDistRatioCustom(float fViewDistRatioCustom) { s_fViewDistRatioCustom = fViewDistRatioCustom; }
	static ILINE void SetViewDistRatioDetail(float fViewDistRatioDetail) { s_fViewDistRatioDetail = fViewDistRatioDetail; }

	static int        AnimEventCallback(ICharacterInstance* pCharacter, void* userdata);

private:
	int  GetSlotId(CEntityObject* pSlot) const;
	void CalcLocalBounds();
	void OnEntityXForm(int nWhyFlags);
	void OnHide(bool bHide);
	void OnReset();

	// Get existing slot or make a new slot if not exist.
	// Is nSlot is negative will allocate a new available slot and return it Index in nSlot parameter.
	CEntityObject* GetOrMakeSlot(int& nSlot);

	I3DEngine*     GetI3DEngine() { return gEnv->p3DEngine; }
	bool           CheckCharacterForMorphs(ICharacterInstance* pCharacter);
	void           AnimationEvent(ICharacterInstance* pCharacter, const AnimEventInstance& event);
	void           UpdateMaterialLayersRendParams(SRendParams& pRenderParams, const SRenderingPassInfo& passInfo);
	void           UpdateEffectLayersParams(SRendParams& pRenderParams);
	bool           IsMovableByGame() const override;

private:
	friend class CEntityObject;

	static float                      gsWaterLevel; //static cached water level, updated each ctor call

	static float                      s_fViewDistMin;
	static float                      s_fViewDistRatio;
	static float                      s_fViewDistRatioCustom;
	static float                      s_fViewDistRatioDetail;
	static std::vector<CRenderProxy*> s_arrCharactersToRegisterForRendering;

	//////////////////////////////////////////////////////////////////////////
	// Variables.
	//////////////////////////////////////////////////////////////////////////
	// Host entity.
	CEntity* m_pEntity;

	// Object Slots.
	typedef std::vector<CEntityObject*> Slots;
	Slots m_slots;

	// Local bounding box of render proxy.
	AABB m_localBBox;

	//! Override material.
	_smart_ptr<IMaterial> m_pMaterial;

	// per instance shader public params
	_smart_ptr<IShaderPublicParams> m_pShaderPublicParams;

	// per instance callback object
	typedef std::vector<IShaderParamCallbackPtr> TCallbackVector;
	TCallbackVector m_Callbacks;

	// Time passed since this entity was seen last time  (wrong: this is an absolute time, todo: fix float absolute time values)
	float                      m_fLastSeenTime;

	static CEntityTimeoutList* s_pTimeoutList;
	EntityId                   m_entityId;

	uint32                     m_nEntityFlags;

	// Render proxy flags.
	uint32 m_nFlags;

	// Vision modes stuff
	uint32 m_nVisionParams;
	// Hud silhouetes param
	uint32 m_nHUDSilhouettesParams;
	// Material layers blending amount
	uint32 m_nMaterialLayersBlend;

	typedef std::pair<int, IStatObj*> SlotGeometry;
	typedef std::vector<SlotGeometry> SlotGeometries;
	SlotGeometries m_queuedGeometryChanges;

	// Layer effects
	uint32 m_pLayerEffectParams;

	float  m_fCustomData[4];

	float  m_fLodDistance;
	uint8  m_nCloakBlendTimeScale;
	uint8  m_nOpacity;
	uint8  m_nCloakColorChannel;
	uint8  m_nCustomData;

	AABB   m_WSBBox;
};

extern stl::PoolAllocatorNoMT<sizeof(CRenderProxy)>* g_Alloc_RenderProxy;

//////////////////////////////////////////////////////////////////////////
DECLARE_COMPONENT_POINTERS(CRenderProxy);

//////////////////////////////////////////////////////////////////////////
inline void* CRenderProxy::operator new(size_t nSize)
{
	void* ptr = g_Alloc_RenderProxy->Allocate();
	return ptr;
}

//////////////////////////////////////////////////////////////////////////
inline void CRenderProxy::operator delete(void* ptr)
{
	if (ptr)
		g_Alloc_RenderProxy->Deallocate(ptr);
}

#endif // __RenderProxy_h__
