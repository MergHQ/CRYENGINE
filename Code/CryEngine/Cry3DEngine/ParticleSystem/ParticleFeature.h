// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ParticleCommon.h"
#include "Features/ParamTraits.h"
#include <CryRenderer/IGpuParticles.h>
#include <CryCore/Dispatcher.h>

namespace pfx2
{

class CParticleEmitter;
class CParticleComponent;
struct SComponentParams;
class CParticleComponentRuntime;
struct SInstance;

enum EFeatureType
{
	EFT_Generic = 0,          // this feature does nothing in particular. Can have many of this per component.
	EFT_Spawn   = BIT(0),     // this feature spawns particles. At least one is needed in a component.
	EFT_Life    = BIT(1),     // this feature changes particles life time. At least one is required per component.
	EFT_Render  = BIT(2),     // this feature renders particles. Each component can only have either none or just one of this.
	EFT_Effect  = BIT(3),     // this feature renders particles. Each component can only have either none or just one of this.
	EFT_Size    = BIT(4),     // this feature changes particles sizes. At least one is required per component.
	EFT_Motion  = BIT(5),     // this feature moves particles around. Each component can only have either none or just one of this.
	EFT_Child   = BIT(6),     // this feature spawns instances from parent particles. At least one is needed for child components

	EFT_END     = BIT(7)
};

class CParticleFeature : public IParticleFeature
{
public:
	static bool RegisterFeature(const SParticleFeatureParams& params);
	~CParticleFeature() {}

	// IParticleFeature
	void                        SetEnabled(bool enabled) override                 { m_enabled.Set(enabled); }
	bool                        IsEnabled() const override                        { return m_enabled; }
	void                        Serialize(Serialization::IArchive& ar) override;
	uint                        GetNumConnectors() const override                 { return 0; }
	const char*                 GetConnectorName(uint connectorId) const override { return nullptr; }
	void                        ConnectTo(const char* pOtherName) override        {}
	void                        DisconnectFrom(const char* pOtherName) override   {}
	uint                        GetNumResources() const override                  { return 0; }
	const char*                 GetResourceName(uint resourceId) const override   { return nullptr; }
	// ~IParticleFeature

	// Parameters
	static bool HasConnector()   { return false; }
	static uint DefaultForType() { return 0; }

	// Initialization
	virtual CParticleFeature* ResolveDependency(CParticleComponent* pComponent)                         { return this; }
	virtual void              AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) {}
	virtual EFeatureType      GetFeatureType()                                                          { return EFT_Generic; }
	virtual bool              CanMakeRuntime(CParticleEmitter* pEmitter) const                          { return true; }

	// Runtime and instance initialization
	virtual void OnEdit(CParticleComponentRuntime& runtime) {}

	virtual void OnPreRun(CParticleComponentRuntime& runtime) {}

	virtual void MainPreUpdate(CParticleComponentRuntime& runtime) {}

	virtual void AddSubInstances(CParticleComponentRuntime& runtime, TDynArray<SInstance>& instances) {}

	virtual void CullSubInstances(CParticleComponentRuntime& runtime, TDynArray<SInstance>& instances) {}

	virtual void InitSubInstances(CParticleComponentRuntime& runtime, SUpdateRange instanceRange) {}

	virtual void GetSpatialExtents(const CParticleComponentRuntime& runtime, TConstArray<float> scales, TVarArray<float> extents) {}

	virtual void GetEmitOffsets(const CParticleComponentRuntime& runtime, TVarArray<Vec3> offsets, uint firstInstance) {}

	// Particle initialization
	virtual void KillParticles(CParticleComponentRuntime& runtime) {}

	virtual void SpawnParticles(CParticleComponentRuntime& runtime) {}

	virtual void PreInitParticles(CParticleComponentRuntime& runtime) {}

	virtual void InitParticles(CParticleComponentRuntime& runtime) {}

	virtual void PostInitParticles(CParticleComponentRuntime& runtime) {}

	virtual void PastUpdateParticles(CParticleComponentRuntime& runtime) {}

	virtual void DestroyParticles(CParticleComponentRuntime& runtime) {}

	// Particle update
	virtual void PreUpdateParticles(CParticleComponentRuntime& runtime) {}

	virtual void UpdateParticles(CParticleComponentRuntime& runtime) {}

	virtual void PostUpdateParticles(CParticleComponentRuntime& runtime) {}

	// Rendering
	virtual void Render(CParticleComponentRuntime& runtime, const SRenderContext& renderContext) {}
	virtual void RenderDeferred(const CParticleComponentRuntime& runtime, const SRenderContext& renderContext) {}
	virtual void ComputeVertices(const CParticleComponentRuntime& runtime, const SCameraInfo& camInfo, CREParticle* pRE, uint64 uRenderFlags, float fMaxPixels) {}

	// GPU interface
	virtual void UpdateGPUParams(CParticleComponentRuntime& runtime, gpu_pfx2::SUpdateParams& params) {}

	gpu_pfx2::IParticleFeature* GetGpuInterface() { return m_gpuInterface; }
	gpu_pfx2::IParticleFeature* MakeGpuInterface(CParticleComponent* pComponent, gpu_pfx2::EGpuFeatureType feature);

private:
	SEnable                                m_enabled;
	_smart_ptr<gpu_pfx2::IParticleFeature> m_gpuInterface;
};

template<class... Args> using TFeatureDispatcher = Dispatcher<CParticleFeature, Args...>;

struct SFeatureDispatchers
{
	TFeatureDispatcher<CParticleComponentRuntime&> OnEdit { &CParticleFeature::OnEdit };
	TFeatureDispatcher<CParticleComponentRuntime&> OnPreRun { &CParticleFeature::OnPreRun };
	TFeatureDispatcher<CParticleComponentRuntime&> MainPreUpdate { &CParticleFeature::MainPreUpdate };

	TFeatureDispatcher<CParticleComponentRuntime&, TDynArray<SInstance>&> AddSubInstances { &CParticleFeature::AddSubInstances };
	TFeatureDispatcher<CParticleComponentRuntime&, TDynArray<SInstance>&> CullSubInstances { &CParticleFeature::CullSubInstances };
	TFeatureDispatcher<CParticleComponentRuntime&, SUpdateRange> InitSubInstances { &CParticleFeature::InitSubInstances };
	TFeatureDispatcher<CParticleComponentRuntime&> KillParticles { &CParticleFeature::KillParticles };
	TFeatureDispatcher<CParticleComponentRuntime&> SpawnParticles { &CParticleFeature::SpawnParticles };

	TFeatureDispatcher<const CParticleComponentRuntime&, TConstArray<float>, TVarArray<float>> GetSpatialExtents { &CParticleFeature::GetSpatialExtents };
	TFeatureDispatcher<const CParticleComponentRuntime&, TVarArray<Vec3>, uint> GetEmitOffsets { &CParticleFeature::GetEmitOffsets };

	TFeatureDispatcher<CParticleComponentRuntime&> PreInitParticles { &CParticleFeature::PreInitParticles };
	TFeatureDispatcher<CParticleComponentRuntime&> InitParticles { &CParticleFeature::InitParticles };
	TFeatureDispatcher<CParticleComponentRuntime&> PostInitParticles { &CParticleFeature::PostInitParticles };
	TFeatureDispatcher<CParticleComponentRuntime&> PastUpdateParticles { &CParticleFeature::PastUpdateParticles };
	TFeatureDispatcher<CParticleComponentRuntime&> DestroyParticles { &CParticleFeature::DestroyParticles };

	TFeatureDispatcher<CParticleComponentRuntime&> PreUpdateParticles { &CParticleFeature::PreUpdateParticles };
	TFeatureDispatcher<CParticleComponentRuntime&> UpdateParticles { &CParticleFeature::UpdateParticles };
	TFeatureDispatcher<CParticleComponentRuntime&> PostUpdateParticles { &CParticleFeature::PostUpdateParticles };

	TFeatureDispatcher<CParticleComponentRuntime&, gpu_pfx2::SUpdateParams&> UpdateGPUParams { &CParticleFeature::UpdateGPUParams };

	TFeatureDispatcher<CParticleComponentRuntime&, const SRenderContext&> Render { &CParticleFeature::Render };
	TFeatureDispatcher<const CParticleComponentRuntime&, const SRenderContext&> RenderDeferred { &CParticleFeature::RenderDeferred };
	TFeatureDispatcher<const CParticleComponentRuntime&, const SCameraInfo&, CREParticle*, uint64, float> ComputeVertices { &CParticleFeature::ComputeVertices };
};

ILINE ColorB HexToColor(uint hex)
{
	return ColorB((hex >> 16) & 0xff, (hex >> 8) & 0xff, hex & 0xff);
}

static const ColorB colorAppearance = HexToColor(0x00ffba);
static const ColorB colorField      = HexToColor(0x02ba25);
static const ColorB colorRender     = HexToColor(0xc2fbbe);
static const ColorB colorLocation   = HexToColor(0x30a8fd);
static const ColorB colorAngles     = HexToColor(0x84d7fa);
static const ColorB colorSpawn      = HexToColor(0xfc7070);
static const ColorB colorLife       = HexToColor(0xd0c0ac);
static const ColorB colorKill       = HexToColor(0xd0c0ac);
static const ColorB colorVelocity   = HexToColor(0xcea639);
static const ColorB colorMotion     = HexToColor(0xfb9563);
static const ColorB colorLight      = HexToColor(0xfffdd0);
static const ColorB colorAudio      = HexToColor(0xd671f7);
static const ColorB colorGeneral    = HexToColor(0xececec);
static const ColorB colorChild      = HexToColor(0xc0c0c0);
static const ColorB colorProject    = HexToColor(0xc080c0);
static const ColorB colorGPU        = HexToColor(0x00e87e);
static const ColorB colorComponent  = HexToColor(0x80c0c0);

#define CRY_PFX2_DECLARE_FEATURE \
  struct SFeatureParams; \
  virtual const SParticleFeatureParams& GetFeatureParams() const override;

// Implement a Feature that can be added to Components
#define CRY_PFX2_IMPLEMENT_COMPONENT_FEATURE(BaseType, Type, GroupName, FeatureName, Color)                              \
  struct Type::SFeatureParams: SParticleFeatureParams { SFeatureParams() {                                               \
    m_fullName = GroupName ": " FeatureName;                                                                             \
    m_groupName = GroupName;                                                                                             \
    m_featureName = FeatureName;                                                                                         \
    m_color = Color;                                                                                                     \
    m_pFactory = []() -> IParticleFeature* { return new Type(); };                                                       \
    m_hasComponentConnector = Type::HasConnector();                                                                      \
    m_defaultForType = Type::DefaultForType();                                                                           \
  } };                                                                                                                   \
  const SParticleFeatureParams& Type::GetFeatureParams() const { static Type::SFeatureParams params; return params; }    \
  static bool sInit ## Type = CParticleFeature::RegisterFeature(Type::SFeatureParams());                                 \
  SERIALIZATION_CLASS_NAME(BaseType, Type, GroupName FeatureName, GroupName FeatureName);                                \

// Implement a Feature that can be added to Components and Emitters
#define CRY_PFX2_IMPLEMENT_FEATURE(BaseType, Type, GroupName, FeatureName, Color)                                        \
  CRY_PFX2_IMPLEMENT_COMPONENT_FEATURE(BaseType, Type, GroupName, FeatureName, Color)                                    \
  SERIALIZATION_CLASS_NAME(IParticleFeature, Type, GroupName FeatureName, GroupName ":" FeatureName);                    \

// Implement a legacy class name for serializing a Feature
#define CRY_PFX2_LEGACY_FEATURE(Type, GroupName, FeatureName) \
	SERIALIZATION_CLASS_NAME(CParticleFeature, Type, GroupName FeatureName, GroupName FeatureName);

}

namespace yasli
{
	// Copied and specialized from Serialization library to provide error message for nonexistent features
	template<> inline
	pfx2::CParticleFeature* ClassFactory<pfx2::CParticleFeature>::create(cstr registeredName) const
	{
		if (!registeredName || !*registeredName)
			return nullptr;
		auto it = typeToCreatorMap_.find(registeredName);
		if (it == typeToCreatorMap_.end())
		{
			gEnv->pLog->LogError("Particle effect has nonexistent feature %s", registeredName);
			return nullptr;
		}
		return it->second->create();
	}
}
