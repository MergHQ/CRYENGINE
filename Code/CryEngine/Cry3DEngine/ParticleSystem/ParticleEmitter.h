// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     23/09/2014 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef PARTICLEEMITTER_H
#define PARTICLEEMITTER_H

#pragma once

#include "ParticleEffect.h"
#include <CryRenderer/IGpuParticles.h>

namespace pfx2
{

class CParticleEmitter : public IParticleEmitter, public Cry3DEngineBase
{
private:
	struct SRuntimeRef
	{
		SRuntimeRef(CParticleEffect* effect, CParticleEmitter* emitter, CParticleComponent* component, const SRuntimeInitializationParameters& params);
		SRuntimeRef() : pRuntime(nullptr), pComponent(nullptr) {}
		_smart_ptr<ICommonParticleComponentRuntime> pRuntime;
		CParticleComponent*                         pComponent;
	};

	typedef std::vector<SRuntimeRef> TComponentRuntimes;

public:
	CParticleEmitter();
	~CParticleEmitter();

	// IRenderNode
	virtual EERType          GetRenderNodeType() override;
	virtual const char*      GetEntityClassName() const override;
	virtual const char*      GetName() const override;
	virtual void             SetMatrix(const Matrix34& mat) override;
	virtual Vec3             GetPos(bool bWorldOnly = true) const override;
	virtual void             Render(const struct SRendParams& rParam, const SRenderingPassInfo& passInfo) override;
	virtual IPhysicalEntity* GetPhysics() const override;
	virtual void             SetPhysics(IPhysicalEntity*) override;
	virtual void             SetMaterial(IMaterial* pMat) override;
	virtual IMaterial*       GetMaterial(Vec3* pHitPos = 0) const override;
	virtual IMaterial*       GetMaterialOverride() override;
	virtual float            GetMaxViewDist() override;
	virtual void             Precache() override;
	virtual void             GetMemoryUsage(ICrySizer* pSizer) const override;
	virtual const AABB       GetBBox() const override;
	virtual void             FillBBox(AABB& aabb) override;
	virtual void             SetBBox(const AABB& WSBBox) override;
	virtual void             OffsetPosition(const Vec3& delta) override;
	virtual bool             IsAllocatedOutsideOf3DEngineDLL() override;
	virtual void             SetViewDistRatio(int nViewDistRatio) override;
	virtual void             ReleaseNode(bool bImmediate) override;
	// ~IRenderNode

	// pfx2 IParticleEmitter
	virtual const IParticleEffect* GetEffect() const override;
	virtual void                   Activate(bool activate) override;
	virtual void                   Kill() override;
	virtual bool                   IsActive() const;
	virtual void                   Prime();
	virtual void                   SetLocation(const QuatTS& loc) override;
	virtual QuatTS                 GetLocation() const override { return m_location; }
	virtual void                   EmitParticle(const EmitParticleData* pData = NULL)  override;
	virtual IParticleAttributes& GetAttributes()  override    { return m_attributeInstance; }
	virtual void                 SetEntity(IEntity* pEntity, int nSlot) override;
	virtual void                 SetTarget(const ParticleTarget& target) override;
	virtual bool                 UpdateStreamableComponents(float fImportance, const Matrix34A& objMatrix, IRenderNode* pRenderNode, float fEntDistance, bool bFullUpdate, int nLod) override;
	virtual void                 SetSpawnParams(const SpawnParams& spawnParams) override;
	// ~pfx2 IParticleEmitter

	// pfx1 IParticleEmitter
	virtual int                GetVersion() const override                             { return 2; }
	virtual bool               IsAlive() const override                                { return m_registered; }
	virtual bool               IsInstant() const override                              { return false; }
	virtual void               Restart() override                                      {}
	virtual void               SetEffect(const IParticleEffect* pEffect) override      {}
	virtual void               SetEmitGeom(const GeomRef& geom) override               { m_emitterGeometry = geom; }
	virtual const SpawnParams& GetSpawnParams() const override                         { static SpawnParams sp; return sp; }
	virtual void               Update() override;
	virtual unsigned int       GetAttachedEntityId() override                          { return 0; }
	virtual int                GetAttachedEntitySlot() override                        { return 0; }
	// ~pfx1 CPArticleEmitter

	void                      PostUpdate();
	CParticleContainer&       GetParentContainer() { return m_parentContainer; }
	const CParticleContainer& GetParentContainer() const { return m_parentContainer; }
	virtual void              GetParentData(const int parentComponentId, const uint* parentParticleIds, const int numParentParticleIds, SInitialData* data) const override;
	const TComponentRuntimes& GetRuntimes() const                   { return m_componentRuntimes; }
	void                      SetCEffect(CParticleEffect* pEffect);
	const CParticleEffect*    GetCEffect() const                    { return m_pEffect; }
	CParticleEffect*          GetCEffect()                          { return m_pEffect; }
	void                      Register();
	void                      Unregister();
	bool                      IsIndependent() const { return GetRefCount() == 1; }
	bool                      HasParticles() const;
	void                      UpdateEmitGeomFromEntity();
	const SVisEnviron&        GetVisEnv() const                     { return m_visEnviron; }
	const SPhysEnviron&       GetPhysicsEnv() const                 { return m_physEnviron; }
	const GeomRef&            GetEmitterGeometry() const            { return m_emitterGeometry; }
	QuatTS                    GetEmitterGeometryLocation() const;
	const CAttributeInstance& GetAttributeInstance() const          { return m_attributeInstance; }
	const ParticleTarget&     GetTarget() const                     { return m_target; }
	float                     GetViewDistRatio() const              { return m_viewDistRatio; }
	CRenderObject*            GetRenderObject(uint threadId, uint renderObjectIdx);
	void                      SetRenderObject(CRenderObject* pRenderObject, uint threadId, uint renderObjectIdx);
	float                     GetTime() const { return m_time; }
	uint32                    GetInitialSeed() const { return m_initialSeed; }
	uint32                    GetCurrentSeed() const { return m_currentSeed; }

	void                      AccumCounts(SParticleCounts& counts);
	void                      AddDrawCallCounts(int numRendererdParticles, int numClippedParticles);

private:
	void     UpdateRuntimeRefs();
	void     AddInstance();
	void     StopInstances();
	void     UpdateFromEntity();
	void     UpdateTargetFromEntity(IEntity* pEntity);
	IEntity* GetEmitGeometryEntity() const;
	void     ResetRenderObjects();

	std::vector<CRenderObject*> m_pRenderObjects[RT_COMMAND_BUF_COUNT];
	SVisEnviron        m_visEnviron;
	SPhysEnviron       m_physEnviron;
	CAttributeInstance m_attributeInstance;
	AABB               m_bounds;
	CParticleContainer m_parentContainer;
	TComponentRuntimes m_componentRuntimes;
	CParticleEffect*   m_pEffect;
	SContainerCounts   m_emitterCounts;
	CryMutex           m_countsMutex;
	QuatTS             m_location;
	ParticleTarget     m_target;
	EntityId           m_entityId;
	int                m_entitySlot;
	GeomRef            m_emitterGeometry;
	int                m_emitterGeometrySlot;
	float              m_viewDistRatio;
	float              m_time;
	int                m_editVersion;
	uint               m_initialSeed;
	uint               m_currentSeed;
	bool               m_registered;
	bool               m_active;
};

}

#endif // PARTICLEEMITTER_H
