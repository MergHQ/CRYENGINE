// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "GpuParticleContainer.h"

#include "Gpu/Physics/GpuPhysicsParticleFluid.h"
#include "Gpu/GpuMergeSort.h"
#include "CREGpuParticle.h"
#include "GpuParticleFeatureBase.h"

namespace gpu_pfx2
{
class CREGpuParticle;

static const float kBoundsScale = 100.f;
static const uint32 kCalcBoundsBlocks = 16u;

struct SReadbackData
{
	Vec3i min;
	Vec3i max;
};

struct SParticleParameters
{
	Matrix44           viewProjection;
	Quat               emitterOrientation;
	Vec3               emitterPosition;
	f32                deltaTime;
	Vec3               physAccel;
	f32                currentTime;
	Vec3               physWind;
	float              farToNearDistance;
	Vec3               cameraPosition;
	float              lifeTime;
	int32              numParticles;
	int32              numNewBorns;
	int32              numKilled;
	pfx2::EGpuSortMode sortMode;
	int32              managerSlot;
};

struct SSpawnData
{
	SSpawnData() : amount(0.f), spawned(0.f), delay(0.0f), duration(0.0f), timer(0.f) {}
	float amount;
	float spawned;
	float delay;
	float duration;
	float timer;
};

enum EFeatureUpdateFlags
{
	eFeatureUpdateFlags_Motion                = 0x04,
	eFeatureUpdateFlags_Color                 = 0x08,
	eFeatureUpdateFlags_Size                  = 0x10,
	eFeatureUpdateFlags_Opacity               = 0x20,
	eFeatureUpdateFlags_Motion_LinearIntegral = 0x40,
	eFeatureUpdateFlags_Motion_DragFast       = 0x80,
	eFeatureUpdateFlags_Motion_Brownian       = 0x100,
	eFeatureUpdateFlags_Motion_Simplex        = 0x200,
	eFeatureUpdateFlags_Motion_Curl           = 0x400,
	eFeatureUpdateFlags_Motion_Gravity        = 0x800,
	eFeatureUpdateFlags_Motion_Vortex         = 0x1000,
	EFeatureUpdateFlags_Collision_ScreenSpace = 0x2000,
	EFeatureUpdateFlags_PixelSize             = 0x4000
};

enum EFeatureUpdateSrvSlot
{
	eFeatureUpdateSrvSlot_sizeTable,
	eFeatureUpdateSrvSlot_colorTable,
	eFeatureUpdateSrvSlot_opacityTable,
	eFeatureUpdateSrvSlot_depthBuffer,

	eFeatureUpdateSrvSlot_COUNT
};

enum EFeatureInitializationFlags
{
	eFeatureInitializationFlags_LocationOffset          = 0x04,
	eFeatureInitializationFlags_LocationBox             = 0x08,
	eFeatureInitializationFlags_LocationSphere          = 0x10,
	eFeatureInitializationFlags_LocationCircle          = 0x20,
	eFeatureInitializationFlags_VelocityCone            = 0x40,
	eFeatureInitializationFlags_VelocityDirectional     = 0x80,
	eFeatureInitializationFlags_VelocityOmniDirectional = 0x100,
};

enum EFeatureInitializationSrvSlot
{
	eFeatureInitializationSrvSlot_newBornIndices,
	eFeatureInitializationSrvSlot_parentData,

	eFeatureInitializationSrvSlot_COUNT
};

enum EConstantBufferSlot
{
	eConstantBufferSlot_Base = 4,
	eConstantBufferSlot_Initialization,
	eConstantBufferSlot_Motion,
	eConstantBufferSlot_Brownian,
	eConstantBufferSlot_Simplex,
	eConstantBufferSlot_Curl,
	eConstantBufferSlot_Gravity,
	eConstantBufferSlot_Vortex,
	eConstantBufferSlot_PixelSize,

	eConstantBufferSlot_COUNT
};

struct SParticleInitializationParameters
{
	Vec3  offset;
	float velocity;
	Vec3  box;
	float velocityScale;
	Vec3  scale;
	float radius;
	Vec3  color;
	float size;
	Vec3  direction;
	float opacity;
	float angle;
	float directionScale;
	float omniVelocity;
};

struct SVertexShaderParameters
{
	float axisScale;
};

class CParticleComponentRuntime;

// context passed to functions during update phase
struct SUpdateContext
{
	CParticleComponentRuntime* pRuntime;
	ID3D11UnorderedAccessView* pReadbackUAV;
	ID3D11UnorderedAccessView* pCounterUAV;
	float                      deltaTime;
};

class CParticleComponentRuntime : public IParticleComponentRuntime
{
public:
	static const int kThreadsInBlock;

	enum EGpuUpdateList
	{
		eGpuUpdateList_Spawn,
		eGpuUpdateList_InitUpdate,
		eGpuUpdateList_Update,
		eGpuUpdateList_InitSubInstance,
		eGpuUpdateList_COUNT
	};

	std::vector<_smart_ptr<gpu_pfx2::CFeature>> m_gpuUpdateLists[eGpuUpdateList_COUNT];

	typedef std::vector<uint> TInstances;

	CParticleComponentRuntime(
	  pfx2::IParticleComponent* pComponent,
	  const pfx2::SRuntimeInitializationParameters& params);

	virtual ~CParticleComponentRuntime()
	{
	}
	virtual void        SetEmitterData(::IParticleEmitter* pEmitter) override;
	virtual EState      GetState() const override       { return m_state; };
	virtual bool        IsActive() const override       { return m_active; }
	virtual void        SetActive(bool active) override { m_active = active; }
	virtual void        MainPreUpdate() override        {}
	virtual bool        IsSecondGen() override          { return m_isSecondGen; }

	virtual bool        IsValidRuntimeForInitializationParameters(const pfx2::SRuntimeInitializationParameters& parameters) override;
	virtual void        SetEnvironmentParameters(const SEnvironmentParameters& params) override { m_envParams = params; }

	virtual const AABB& GetBounds() const override;
	virtual void        AccumCounts(SParticleCounts& counts) override;
	virtual bool        HasParticles() override { return m_parameters->numParticles != 0; }

	void                Initialize();
	void                Reset();

	void                SetManagerSlot(int i) { m_parameters->managerSlot = i; }

	void                PrepareRelease();

	void                SetLifeTime(float lifeTime) { m_parameters->lifeTime = lifeTime; }

	int                 GetNumParticles()           { return m_parameters->numParticles; };

	// this is from the render thread
	void AddRemoveNewBornsParticles(SUpdateContext& context);
	void UpdateParticles(SUpdateContext& context);
	void CalculateBounds(ID3D11UnorderedAccessView* pReadbackUAV);
	void SetBoundsFromManager(const SReadbackData* pData);
	void SetCounterFromManager(const uint32* pCounter);

	void SpawnParticles(int instanceID, uint32 count);

	// this comes from the main thread
	virtual void                       Render(CRenderObject* pRenderObject, const SRenderingPassInfo& passInfo, const SRendParams& rParams) override;

	CParticleContainer*                GetContainer() { return &m_container; }

	void                               RunTechnique(CCryNameTSCRC tech);
	void                               SetSRVs(const int numSRVs, ID3D11ShaderResourceView** SRVs);

	void                               SetUpdateSRV(EFeatureUpdateSrvSlot slot, ID3D11ShaderResourceView* pSRV);
	void                               SetUpdateFlags(uint64 flags);

	SParticleInitializationParameters& GetParticleInitializationParameters() { return m_particleInitializationParameters.GetHostData(); }
	void                               SetInitializationSRV(EFeatureInitializationSrvSlot slot, ID3D11ShaderResourceView* pSRV);
	void                               SetInitializationFlags(uint64 flags);

	// code duplication :(
	virtual void AddSubInstances(SInstance* pInstances, size_t count) override;
	void         RemoveSubInstance(size_t instanceId);
	void         RemoveAllSubInstances() override;
	virtual void ReparentParticles(const uint* swapIds, const uint numSwapIds) override;
	size_t       GetNumInstances() const       { return m_subInstancesRenderThread.size(); }
	const uint&  GetInstance(size_t idx) const { return m_subInstancesRenderThread[idx]; }

	// currently called from the render element mfDraw, but may go somewhere else
	void                                   SwapToEnd(SUpdateContext& context);

	Vec3                                   GetPos() const { return m_parameters->emitterPosition; };

	gpu_physics::CParticleFluidSimulation* CreateFluidSimulation();
	gpu_physics::CParticleFluidSimulation* GetFluidSimulation() { return m_pFluidSimulation.get(); }
	void                                   FluidCollisions();
	void                                   EvolveParticles();

	bool                                   BindVertexShaderResources()
	{
		if (IsActive() && m_parameters->numParticles)
		{
			ID3D11ShaderResourceView* ppSRVs[] = { GetContainer()->GetDefaultParticleDataSRV() };
			gcpRendD3D->m_DevMan.BindSRV(CDeviceManager::TYPE_VS, ppSRVs, 7, 1);
			m_vertexShaderParams.BindVertexShader();
			return true;
		}
		return false;
	}

	SSpawnData& GetSpawnData(size_t idx) { return m_spawnData[idx]; }
	// /code duplication :(
private:

	struct SSpawnEntry
	{
		uint32 m_count;
		int32  m_instanceId;
	};

	void AddRemoveParticles(const SUpdateContext& context);
	void UpdateNewBorns(const SUpdateContext& context);
	void FillKillList(const SUpdateContext& context);
	void UpdateFeatures(const SUpdateContext& context);
	void Sort(const SUpdateContext& context);

	gpu::CTypedConstantBuffer<SParticleParameters, eConstantBufferSlot_Base> m_parameters;

	CParticleContainer                                  m_container;
	std::unique_ptr<CREGpuParticle>                     m_pRendElement;

	gpu::CTypedResource<int, gpu::BufferFlagsReadWrite> m_blockSums;
	gpu::CTypedResource<int, gpu::BufferFlagsReadWrite> m_killList;
	gpu::CTypedResource<uint, gpu::BufferFlagsDynamic>  m_newBornIndices;

	int32 m_parentId;
	int   m_version;

	// the subinstances are double buffered, so the render thread can work
	// with a different set while the main thread (or any other thread, there is a lock)
	// manipulates them. They are synchronized in AddRemoveNewBornsParticles
	TInstances m_subInstances;
	TInstances m_subInstancesRenderThread;

	// m_parentData corresponds to the subInstances in m_subInstances, while the buffer
	// in m_parentDataRenderThread corresponds to subInstances in m_subInstancesRenderThread
	std::vector<SInitialData>                                  m_parentData;
	gpu::CTypedResource<SInitialData, gpu::BufferFlagsDynamic> m_parentDataRenderThread;
	int                                                        m_parentDataSizeRenderThread;

	std::vector<SSpawnData>                                    m_spawnData;
	std::vector<SSpawnEntry>                                   m_spawnEntries;
	AABB                                                       m_bounds;
	bool                                                       m_active;
	bool                                                       m_isSecondGen;

	std::unique_ptr<gpu::CComputeBackend>                      m_pBackend;

	ID3D11ShaderResourceView*                                  m_updateSrvSlots[eFeatureUpdateSrvSlot_COUNT];
	uint64                                                     m_updateShaderFlags;

	ID3D11ShaderResourceView*                                  m_initializationSrvSlots[eFeatureInitializationSrvSlot_COUNT];
	uint64                                                     m_initializationShaderFlags;
	gpu::CTypedConstantBuffer<SParticleInitializationParameters, eConstantBufferSlot_Initialization> m_particleInitializationParameters;

	EState                                                 m_state;

	CryCriticalSection                                     m_cs;

	SEnvironmentParameters                                 m_envParams;

	int                                                    m_texSampler;
	int                                                    m_texPointSampler;
	std::unique_ptr<gpu_physics::CParticleFluidSimulation> m_pFluidSimulation;
	std::unique_ptr<gpu::CMergeSort>                       m_pMergeSort;

	// set only during initialization
	const int                                             m_maxParticles;
	const int                                             m_maxNewBorns;
	const pfx2::EGpuSortMode                              m_sortMode;
	const uint32                                          m_facingMode;
	const float                                           m_axisScale;

	gpu::CTypedConstantBuffer<SVertexShaderParameters, 4> m_vertexShaderParams;
};
}
