// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IGameObject.h>

#define WATER_RIPPLES_EDITING_ENABLED 1

// Description: Generates water ripples when moving across a water surface

class CWaterRipplesGenerator
	: public CGameObjectExtensionHelper<CWaterRipplesGenerator, IGameObjectExtension>
	  , public IEntityComponentPreviewer
{
	struct SProperties
	{
		SProperties()
			: m_scale(1.0f)
			, m_strength(1.0f)
			, m_frequency(1.0f)
			, m_randFrequency(0.0f)
			, m_randScale(0.0f)
			, m_randStrength(0.0f)
			, m_enabled(true)
			, m_autoSpawn(false)
			, m_spawnOnMovement(true)
			, m_randomOffset(0, 0)
		{
		}

		void InitFromScript(const IEntity& entity);

		float m_scale;
		float m_strength;
		float m_frequency;
		float m_randFrequency;
		float m_randScale;
		float m_randStrength;
		bool  m_enabled;
		bool  m_autoSpawn;
		bool  m_spawnOnMovement;
		Vec2  m_randomOffset;
	};

public:
	CWaterRipplesGenerator();
	virtual ~CWaterRipplesGenerator();

	// IGameObjectExtension
	virtual bool                       Init(IGameObject* pGameObject) override;
	virtual void                       InitClient(int channelId) override                                                       {}
	virtual void                       PostInit(IGameObject* pGameObject) override;
	virtual void                       PostInitClient(int channelId) override                                                   {}
	virtual bool                       ReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override;
	virtual void                       PostReloadExtension(IGameObject* pGameObject, const SEntitySpawnParams& params) override {}
	virtual bool                       NetSerialize(TSerialize ser, EEntityAspects aspect, uint8 profile, int pflags) override  { return true; }
	virtual void                       FullSerialize(TSerialize ser) override;
	virtual void                       PostSerialize() override                                                                 {}
	virtual void                       SerializeSpawnInfo(TSerialize ser) override                                              {}
	virtual ISerializableInfoPtr       GetSpawnInfo() override                                                                  { return 0; }
	virtual void                       Update(SEntityUpdateContext& ctx, int updateSlot) override;
	virtual void                       PostUpdate(float frameTime) override                                                     {}
	virtual void                       PostRemoteSpawn() override                                                               {}
	virtual void                       HandleEvent(const SGameObjectEvent& gameObjectEvent) override;
	virtual void                       ProcessEvent(const SEntityEvent&) override;
	virtual Cry::Entity::EventFlags    GetEventMask() const override;
	virtual void                       SetChannelId(uint16 id) override                 {}
	virtual void                       GetMemoryUsage(ICrySizer* pSizer) const override { pSizer->Add(*this); }
	virtual IEntityComponentPreviewer* GetPreviewer() override                          { return this; }
	// ~IGameObjectExtension

	// IEntityComponentPreviewer
	virtual void SerializeProperties(Serialization::IArchive& archive) override {}
	virtual void Render(const IEntity& entity, const IEntityComponent& component, SEntityPreviewContext& context) const override;
	// ~IEntityComponentPreviewer

	virtual bool GetEntityPoolSignature(TSerialize signature);
	virtual void Release();
	virtual void ProcessHit(bool isMoving);

private:

	void Reset();
	void ActivateGeneration(const bool activate);

	SProperties m_properties;
	float       m_lastSpawnTime;

#if WATER_RIPPLES_EDITING_ENABLED
	bool m_currentLocationOk;
#endif
};
