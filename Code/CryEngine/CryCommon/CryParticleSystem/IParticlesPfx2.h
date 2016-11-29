// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "IParticles.h"
#include "CryExtension/ICryUnknown.h"
#include "CryExtension/CryCreateClassInstance.h"

// fwd
namespace gpu_pfx2
{
struct SUpdateContext;
struct IParticleFeatureGpuInterface;
struct SComponentParams;
class IParticleComponentRuntime;
}

namespace pfx2
{

class CParticleComponentRuntime;
struct IParticleFeature;
struct IParticleEffectPfx2;
typedef _smart_ptr<IParticleEffectPfx2> PParticleEffect;
typedef _smart_ptr<IParticleEmitter>    PParticleEmitter;

struct SParticleFeatureParams
{
	const char*       m_groupName;
	const char*       m_featureName;
	ColorB            m_color;
	IParticleFeature* (* m_pFactory)();
	bool              m_hasComponentConnector;
};

typedef uint32 TComponentId;
typedef uint32 TParticleId;

struct IParticleFeature
{
	virtual bool                                    IsEnabled() const = 0;
	virtual void                                    SetEnabled(bool enabled) = 0;
	virtual void                                    Serialize(Serialization::IArchive& ar) = 0;
	virtual const SParticleFeatureParams&           GetFeatureParams() const = 0;
	virtual uint                                    GetNumConnectors() const = 0;
	virtual const char*                             GetConnectorName(uint connectorId) const = 0;
	virtual void                                    ConnectTo(const char* pOtherName) = 0;
	virtual void                                    DisconnectFrom(const char* pOtherName) = 0;

	virtual gpu_pfx2::IParticleFeatureGpuInterface* GetGpuInterface() = 0;
};

enum EUpdateList
{
	EUL_MainPreUpdate,    // this feature will update once per frame on the main thread
	EUL_InitSubInstance,  // this feature has sub instance data to initialize
	EUL_GetExtents,       // this feature has a spatial extent
	EUL_GetEmitOffset,    // this feature moves the effective emit location
	EUL_Spawn,            // this feature creates new particles
	EUL_InitUpdate,       // this feature needs to initialize newborn particle data
	EUL_PostInitUpdate,   // this feature needs to initialize newborn particle data after main InitUpdate
	EUL_KillUpdate,       // this feature needs to run logic for particles that are being killed
	EUL_PreUpdate,        // this feature changes particles over time before the main update
	EUL_Update,           // this feature changes particle data over time
	EUL_Render,           // this feature has geometry to render
	EUL_RenderDeferred,   // this feature has geometry to render but can only render after all updates are done

	EUL_Count,
};

struct IParticleComponent : public _i_reference_target_t
{
	virtual void                                     SetChanged() = 0;
	virtual bool                                     IsEnabled() const = 0;
	virtual void                                     SetEnabled(bool enabled) = 0;
	virtual bool                                     IsVisible() const = 0;
	virtual void                                     SetVisible(bool visible) = 0;
	virtual void                                     Serialize(Serialization::IArchive& ar) = 0;
	virtual void                                     SetName(const char* name) = 0;
	virtual const char*                              GetName() const = 0;
	virtual uint                                     GetNumFeatures() const = 0;
	virtual IParticleFeature*                        GetFeature(uint featureIdx) const = 0;
	virtual void                                     AddFeature(uint placeIdx, const SParticleFeatureParams& featureParams) = 0;
	virtual void                                     RemoveFeature(uint featureIdx) = 0;
	virtual void                                     SwapFeatures(const uint* swapIds, uint numSwapIds) = 0;
	virtual Vec2                                     GetNodePosition() const = 0;
	virtual void                                     SetNodePosition(Vec2 position) = 0;

	virtual gpu_pfx2::IParticleFeatureGpuInterface** GetGpuUpdateList(EUpdateList list, int& size) const = 0;
};

enum EGpuSortMode
{
	eGpuSortMode_None = 0,
	eGpuSortMode_BackToFront,
	eGpuSortMode_FrontToBack,
	eGpuSortMode_OldToNew,
	eGpuSortMode_NewToOld
};

enum EGpuFacingMode
{
	eGpuFacingMode_Screen = 0,
	eGpuFacingMode_Velocity
};

struct SRuntimeInitializationParameters
{

	SRuntimeInitializationParameters() : usesGpuImplementation(false), maxParticles(0), maxNewBorns(0), sortMode(eGpuSortMode_None), isSecondGen(false), parentId(0), version(0) {}
	bool           usesGpuImplementation;
	// the following parameters are only used by the GPU particle runtime
	int            maxParticles;
	int            maxNewBorns;
	EGpuSortMode   sortMode;
	EGpuFacingMode facingMode;
	float          axisScale;
	bool           isSecondGen;
	int            parentId;
	int            version;
};

class ICommonParticleComponentRuntime : public _i_reference_target_t
{
public:
	struct SInstance
	{
		SInstance() {}
		TParticleId m_parentId;
	};
	virtual ~ICommonParticleComponentRuntime() {}

	virtual bool                                 IsValidRuntimeForInitializationParameters(const SRuntimeInitializationParameters& parameters) = 0;

	virtual gpu_pfx2::IParticleComponentRuntime* GetGpuRuntime() { return nullptr; }
	virtual pfx2::CParticleComponentRuntime*     GetCpuRuntime() { return nullptr; }

	virtual bool                                 IsActive() const = 0;
	virtual const AABB& GetBounds() const = 0;

	virtual void        AccumCounts(SParticleCounts& counts) = 0;

	virtual void        AddSubInstances(SInstance* pInstances, size_t count) = 0;
	virtual void        ReparentParticles(const uint* swapIds, const uint numSwapIds) = 0;
	virtual void        MainPreUpdate() = 0;
};

struct IParticleEffectPfx2 : public IParticleEffect
{
	virtual void                   SetChanged() = 0;
	virtual uint                   GetNumComponents() const = 0;
	virtual IParticleComponent*    GetComponent(uint componentIdx) const = 0;
	virtual void                   AddComponent(uint componentIdx) = 0;
	virtual void                   RemoveComponent(uint componentIdx) = 0;
	virtual Serialization::SStruct GetEffectOptionsSerializer() const = 0;
};

struct IParticleSystem : public ICryUnknown
{
	CRYINTERFACE_DECLARE(IParticleSystem, 0xCC3AA21D44A24DED, 0x9AA1DADED21D570A);

	virtual PParticleEffect         CreateEffect() = 0;
	virtual PParticleEffect         ConvertEffect(const IParticleEffect* pOldEffect, bool bReplace = false) = 0;
	virtual void                    RenameEffect(PParticleEffect pEffect, cstr name) = 0;
	virtual PParticleEffect         FindEffect(cstr name) = 0;
	virtual PParticleEmitter        CreateEmitter(PParticleEffect pEffect) = 0;
	virtual uint                    GetNumFeatureParams() const = 0;
	virtual SParticleFeatureParams& GetFeatureParam(uint featureIdx) const = 0;

	virtual void                    OnFrameStart() = 0;
	virtual void                    Update() = 0;
	virtual void                    Reset() = 0;

	virtual void                    Serialize(TSerialize ser) = 0;

	virtual void                    GetCounts(SParticleCounts& counts) = 0;
	virtual void                    GetMemoryUsage(ICrySizer* pSizer) const = 0;
};

static std::shared_ptr<IParticleSystem> GetIParticleSystem()
{
	std::shared_ptr<IParticleSystem> pParticleSystem;
	CryCreateClassInstance("CryEngine_ParticleSystem", pParticleSystem);
	return pParticleSystem;
}

}
