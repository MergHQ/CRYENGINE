// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ParticleSystem/ParticleFeature.h"
#include <CrySerialization/Decorators/ResourcesAudio.h>
#include <CryAudio/IObject.h>
#include "ParamMod.h"

CRY_PFX2_DBG

namespace pfx2
{
static const ColorB audioColor = ColorB(172, 196, 138);
using IOAudioObjects = TIOStream<CryAudio::IObject*>;

EParticleDataType PDT(EPDT_AudioObject, CryAudio::IObject*);

SERIALIZATION_DECLARE_ENUM(ETriggerType,
                           OnSpawn,
                           OnDeath
                           )

SERIALIZATION_ENUM_BEGIN_NESTED(CryAudio, EOcclusionType, "Occlusion Type")
SERIALIZATION_ENUM(CryAudio::EOcclusionType::None, "None", "None")
SERIALIZATION_ENUM(CryAudio::EOcclusionType::Ignore, "Ignore", "Ignore")
SERIALIZATION_ENUM(CryAudio::EOcclusionType::Adaptive, "Adaptive", "Adaptive")
SERIALIZATION_ENUM(CryAudio::EOcclusionType::Low, "Low", "Low")
SERIALIZATION_ENUM(CryAudio::EOcclusionType::Medium, "Medium", "Medium")
SERIALIZATION_ENUM(CryAudio::EOcclusionType::High, "High", "High")
SERIALIZATION_ENUM_END()

class CFeatureAudioTrigger final : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureAudioTrigger()
		: m_triggerType(ETriggerType::OnSpawn)
		, m_occlusionType(CryAudio::EOcclusionType::Ignore)
		, m_audioId(CryAudio::InvalidControlId)
		, m_followParticle(true) {}

	void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		gEnv->pAudioSystem->GetAudioTriggerId(m_audioName.c_str(), m_audioId);
		if (m_audioId != CryAudio::InvalidControlId)
		{
			pComponent->AddToUpdateList(EUL_MainPreUpdate, this);
			pComponent->AddParticleData(EPVF_Position);
			pComponent->AddParticleData(EPDT_AudioObject);
		}
	}

	void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(Serialization::AudioTrigger(m_audioName), "Name", "Name");
		ar(m_triggerType, "Trigger", "Trigger");
		ar(m_occlusionType, "Occlusion", "Occlusion");
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

	uint GetNumResources() const override
	{
		return m_audioName.empty() ? 0 : 1;
	}

	const char* GetResourceName(uint resourceId) const override
	{
		return m_audioName.c_str();
	}

private:
	void TriggerSingleAudioEvents(CParticleComponentRuntime* pComponentRuntime, const char* proxyName)
	{
		CRY_PFX2_PROFILE_DETAIL;

		const SUpdateContext context(pComponentRuntime);
		CParticleContainer& container = context.m_container;
		const IVec3Stream positions = container.GetIVec3Stream(EPVF_Position);
		const auto states = container.GetTIStream<uint8>(EPDT_State);
		IOAudioObjects audioObjects = container.GetTIOStream<CryAudio::IObject*>(EPDT_AudioObject);

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			const uint8 state = states.Load(particleId);
			const bool onSpawn = (m_triggerType == ETriggerType::OnSpawn) && (state == ES_NewBorn);
			const bool onDeath = (m_triggerType == ETriggerType::OnDeath) && (state == ES_Expired);
			if (onSpawn || onDeath)
			{
				const Vec3 position = positions.Load(particleId);
				CryAudio::SExecuteTriggerData const data(proxyName, m_occlusionType, position, true, m_audioId);
				gEnv->pAudioSystem->ExecuteTriggerEx(data);
			}
		}
		CRY_PFX2_FOR_END;
	}

	void TriggerFollowAudioEvents(CParticleComponentRuntime* pComponentRuntime, const char* proxyName)
	{
		CRY_PFX2_PROFILE_DETAIL;

		const SUpdateContext context(pComponentRuntime);
		CParticleContainer& container = context.m_container;
		const IVec3Stream positions = container.GetIVec3Stream(EPVF_Position);
		const auto states = container.GetTIStream<uint8>(EPDT_State);
		IOAudioObjects audioObjects = container.GetTIOStream<CryAudio::IObject*>(EPDT_AudioObject);

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			CryAudio::IObject* pIObject = audioObjects.Load(particleId);
			const uint8 state = states.Load(particleId);
			if ((state & ESB_NewBorn) != 0)
			{
				CRY_ASSERT(pIObject == nullptr);
				pIObject = MakeAudioObject(gEnv->pAudioSystem, positions, particleId, proxyName);
				audioObjects.Store(particleId, pIObject);
				pIObject->ExecuteTrigger(m_audioId);
			}
			if ((state & ESB_Dead) != 0 && pIObject != nullptr)
			{
				pIObject->StopTrigger(m_audioId);
				gEnv->pAudioSystem->ReleaseObject(pIObject);
				audioObjects.Store(particleId, nullptr);
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
		IOAudioObjects audioObjects = container.GetTIOStream<CryAudio::IObject*>(EPDT_AudioObject);

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			CryAudio::IObject* const pIObject = audioObjects.Load(particleId);
			if (pIObject != nullptr)
			{
				const Vec3 position = positions.Load(particleId);
				pIObject->SetTransformation(position);
			}
		}
		CRY_PFX2_FOR_END;
	}

	ILINE CryAudio::IObject* MakeAudioObject(CryAudio::IAudioSystem* pAudioSystem, IVec3Stream positions, TParticleId particleId, const char* const szName)
	{
		const Vec3 position = positions.Load(particleId);

		CryAudio::SCreateObjectData const data(szName, m_occlusionType, position, INVALID_ENTITYID, true);
		return pAudioSystem->CreateObject(data);
	}

	string                   m_audioName;
	CryAudio::ControlId      m_audioId;
	ETriggerType             m_triggerType;
	CryAudio::EOcclusionType m_occlusionType;
	bool                     m_followParticle;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureAudioTrigger, "Audio", "Trigger", colorAudio);

class CFeatureAudioParameter : public CParticleFeature
{
public:
	CRY_PFX2_DECLARE_FEATURE

	CFeatureAudioParameter()
		: m_value(1.0f)
		, m_parameterId(CryAudio::InvalidControlId) {}

	void AddToComponent(CParticleComponent* pComponent, SComponentParams* pParams) override
	{
		gEnv->pAudioSystem->GetAudioParameterId(m_parameterName.c_str(), m_parameterId);
		if (m_parameterId != CryAudio::InvalidControlId)
		{
			pComponent->AddToUpdateList(EUL_MainPreUpdate, this);
			m_value.AddToComponent(pComponent, this);
		}
	}

	void Serialize(Serialization::IArchive& ar) override
	{
		CParticleFeature::Serialize(ar);
		ar(Serialization::AudioRTPC(m_parameterName), "Name", "Name");
		ar(m_value, "Value", "Value");
	}

	void MainPreUpdate(CParticleComponentRuntime* pComponentRuntime) override
	{
		const SUpdateContext context(pComponentRuntime);
		CParticleContainer& container = context.m_container;
		if (!container.HasData(EPDT_AudioObject))
			return;
		IOAudioObjects audioObjects = container.GetTIOStream<CryAudio::IObject*>(EPDT_AudioObject);

		STempModBuffer values(context, m_value);
		values.ModifyUpdate(context, m_value, container.GetFullRange());

		CRY_PFX2_FOR_ACTIVE_PARTICLES(context)
		{
			const float value = values.m_stream.Load(particleId);
			CryAudio::IObject* const pIObject = audioObjects.Load(particleId);
			if (pIObject != nullptr)
			{
				pIObject->SetParameter(m_parameterId, value);
			}
		}
		CRY_PFX2_FOR_END;
	}

private:
	string                               m_parameterName;
	CryAudio::ControlId                  m_parameterId;
	CParamMod<SModParticleField, SFloat> m_value;
};

CRY_PFX2_IMPLEMENT_FEATURE(CParticleFeature, CFeatureAudioParameter, "Audio", "Rtpc", colorAudio);
}// namespace pfx2
