// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleSystem/ParticleFeature.h"
#include <CrySerialization/Decorators/ResourcesAudio.h>
#include "ParamMod.h"

CRY_PFX2_DBG

volatile bool gFeatureAudio = false;

namespace pfx2
{

static const ColorB audioColor = ColorB(172, 196, 138);
typedef TIOStream<IAudioProxy*> TIOAudioProxies;

SERIALIZATION_DECLARE_ENUM(ETriggerType,
                           OnSpawn,
                           OnDeath
                           )

SERIALIZATION_DECLARE_ENUM(EAudioOcclusionMode,
                           Ignore = eAudioOcclusionType_Ignore,
                           SingleRay = eAudioOcclusionType_SingleRay,
                           MultipleRay = eAudioOcclusionType_MultiRay
                           )

class CFeatureAudioTrigger : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureAudioTrigger()
		: m_triggerType(ETriggerType::OnSpawn)
		, m_occlusionMode(EAudioOcclusionMode::Ignore)
		, m_audioId(INVALID_AUDIO_CONTROL_ID)
		, m_followParticle(true) {}

	void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		gEnv->pAudioSystem->GetAudioTriggerId(m_audioName.c_str(), m_audioId);
		if (m_audioId != INVALID_AUDIO_CONTROL_ID)
		{
			pComponent->AddToUpdateList(EUL_MainPreUpdate, this);
			pComponent->AddParticleData(EPVF_Position);
			pComponent->AddParticleData(EPDT_AudioProxy);
		}
	}

	void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(Serialization::AudioTrigger(m_audioName), "Name", "Name");
		ar(m_triggerType, "Trigger", "Trigger");
		ar(m_occlusionMode, "Occlusion", "Occlusion");
		if (m_triggerType == ETriggerType::OnSpawn)
			ar(m_followParticle, "FollowParticle", "Follow Particle");
		else
			m_followParticle = false;
	}

	void MainPreUpdate(CParticleComponentRuntime* pComponentRuntime) override
	{
		CryStackStringT<char, 512> proxyName;
		proxyName.append(pComponentRuntime->GetComponent()->GetEffect()->GetName());
		proxyName.append(" : ");
		proxyName.append(pComponentRuntime->GetComponent()->GetName());

		if (m_followParticle)
		{
			TriggerFollowAudioEvents(pComponentRuntime, proxyName.c_str());
			UpdateAudioProxies(pComponentRuntime);
		}
		else
			TriggerSingleAudioEvents(pComponentRuntime, proxyName.c_str());
	}

private:
	void TriggerSingleAudioEvents(CParticleComponentRuntime* pComponentRuntime, const char* proxyName)
	{
		CRY_PFX2_PROFILE_DETAIL;

		IAudioSystem* pAudioSystem = gEnv->pAudioSystem;
		const SUpdateContext context(pComponentRuntime);
		CParticleContainer& container = context.m_container;
		const IVec3Stream positions = container.GetIVec3Stream(EPVF_Position);
		const auto states = container.GetTIStream<uint8>(EPDT_State);
		TIOAudioProxies audioProxies = container.GetTIOStream<IAudioProxy*>(EPDT_AudioProxy);

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			IAudioProxy* pAudioProxy = nullptr;
			const uint8 state = states.Load(particleId);
			const bool onSpawn = (m_triggerType == ETriggerType::OnSpawn) && (state == ES_NewBorn);
			const bool onDeath = (m_triggerType == ETriggerType::OnDeath) && (state == ES_Expired);
			if (onSpawn || onDeath)
			{
				pAudioProxy = MakeProxy(pAudioSystem, positions, particleId, proxyName);
				pAudioProxy->ExecuteTrigger(m_audioId);
				pAudioProxy->Release();
			}
		}
		CRY_PFX2_FOR_END;
	}

	void TriggerFollowAudioEvents(CParticleComponentRuntime* pComponentRuntime, const char* proxyName)
	{
		CRY_PFX2_PROFILE_DETAIL;

		IAudioSystem* pAudioSystem = gEnv->pAudioSystem;
		const SUpdateContext context(pComponentRuntime);
		CParticleContainer& container = context.m_container;
		const IVec3Stream positions = container.GetIVec3Stream(EPVF_Position);
		const auto states = container.GetTIStream<uint8>(EPDT_State);
		TIOAudioProxies audioProxies = container.GetTIOStream<IAudioProxy*>(EPDT_AudioProxy);

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			IAudioProxy* pAudioProxy = audioProxies.Load(particleId);
			const uint8 state = states.Load(particleId);
			if ((state & ESB_NewBorn) != 0)
			{
				pAudioProxy = MakeProxy(pAudioSystem, positions, particleId, proxyName);
				audioProxies.Store(particleId, pAudioProxy);
				pAudioProxy->ExecuteTrigger(m_audioId);
			}
			if ((state & ESB_Dead) != 0 && pAudioProxy)
			{
				pAudioProxy->StopTrigger(m_audioId);
				pAudioProxy->Release();
				audioProxies.Store(particleId, nullptr);
			}
		}
		CRY_PFX2_FOR_END;
	}

	void UpdateAudioProxies(CParticleComponentRuntime* pComponentRuntime)
	{
		CRY_PFX2_PROFILE_DETAIL;

		const SUpdateContext context(pComponentRuntime);
		CParticleContainer& container = context.m_container;
		const IVec3Stream positions = container.GetIVec3Stream(EPVF_Position);
		TIOAudioProxies audioProxies = container.GetTIOStream<IAudioProxy*>(EPDT_AudioProxy);

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			IAudioProxy* pAudioProxy = audioProxies.Load(particleId);
			if (pAudioProxy)
			{
				const Vec3 position = positions.Load(particleId);
				pAudioProxy->SetPosition(position);
			}
		}
		CRY_PFX2_FOR_END;
	}

	ILINE IAudioProxy* MakeProxy(IAudioSystem* pAudioSystem, IVec3Stream positions, TParticleId particleId, const char* proxyName)
	{
		IAudioProxy* pAudioProxy;
		const Vec3 position = positions.Load(particleId);

		pAudioProxy = pAudioSystem->GetFreeAudioProxy();

		pAudioProxy->Initialize(proxyName);
		pAudioProxy->SetOcclusionType(EAudioOcclusionType(m_occlusionMode));
		pAudioProxy->SetPosition(position);
		pAudioProxy->SetCurrentEnvironments();

		return pAudioProxy;
	}

	string              m_audioName;
	AudioControlId      m_audioId;
	ETriggerType        m_triggerType;
	EAudioOcclusionMode m_occlusionMode;
	bool                m_followParticle;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureAudioTrigger, "Audio", "Trigger", defaultIcon, audioColor);

class CFeatureAudioRtpc : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureAudioRtpc()
		: m_value(1.0f)
		, m_rtpcId(INVALID_AUDIO_CONTROL_ID) {}

	void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		gEnv->pAudioSystem->GetAudioRtpcId(m_rtpcName.c_str(), m_rtpcId);
		if (m_rtpcId != INVALID_AUDIO_CONTROL_ID)
		{
			pComponent->AddToUpdateList(EUL_MainPreUpdate, this);
			m_value.AddToComponent(pComponent, this);
		}
	}

	void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(Serialization::AudioRTPC(m_rtpcName), "Name", "Name");
		ar(m_value, "Value", "Value");
	}

	void MainPreUpdate(CParticleComponentRuntime* pComponentRuntime) override
	{
		IAudioSystem* pAudioSystem = gEnv->pAudioSystem;
		const SUpdateContext context(pComponentRuntime);
		CParticleContainer& container = context.m_container;
		if (!container.HasData(EPDT_AudioProxy))
			return;
		TIOAudioProxies audioProxies = container.GetTIOStream<IAudioProxy*>(EPDT_AudioProxy);

		STempModBuffer values(context, m_value);
		values.ModifyUpdate(context, m_value, container.GetFullRange());

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			const float value = values.m_stream.Load(particleId);
			IAudioProxy* pAudioProxy = audioProxies.Load(particleId);
			if (pAudioProxy)
				pAudioProxy->SetRtpcValue(m_rtpcId, value);
		}
		CRY_PFX2_FOR_END;
	}

private:
	string                               m_rtpcName;
	AudioControlId                       m_rtpcId;
	CParamMod<SModParticleField, SFloat> m_value;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureAudioRtpc, "Audio", "Rtpc", defaultIcon, audioColor);

}
