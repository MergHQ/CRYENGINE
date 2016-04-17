// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

namespace pfx2
{

namespace detail
{

ILINE float AntiAliasParentAge(const float deltaTime, const float selfAge, const float parentInvLifeTime, const float parentAge)
{
	if (selfAge >= 0.0f)
		return 0.0f;
	return selfAge * parentInvLifeTime * deltaTime + parentAge;
}

#ifdef CRY_PFX2_USE_SSE
ILINE floatv AntiAliasParentAge(const floatv deltaTime, const floatv selfAge, const floatv parentInvLifeTime, const floatv parentAge)
{
	const floatv tempAntAliasParentAge = MAdd(Mul(selfAge, parentInvLifeTime), deltaTime, parentAge);
	const floatv sample = FSel(selfAge, parentAge, tempAntAliasParentAge);
	return sample;
}
#endif

class CSelfStreamSampler
{
public:
	CSelfStreamSampler(const SUpdateContext& context, EParticleDataType sourceStreamType, EModDomain domain = EMD_PerParticle)
		: sourceStream(domain == EMD_PerParticle ? context.m_container.GetIFStream(sourceStreamType) : context.m_parentContainer.GetIFStream(sourceStreamType))
	{}
	ILINE floatv Sample(TParticleGroupId particleId) const
	{
		return sourceStream.Load(particleId);
	}
private:
	IFStream sourceStream;
};

class CParentStreamSampler
{
public:
	CParentStreamSampler(const SUpdateContext& context, EParticleDataType sourceStreamType)
		: deltaTime(ToFloatv(context.m_deltaTime))
		, selfAges(context.m_container.GetIFStream(EPDT_NormalAge))
		, parentSourceStream(context.m_parentContainer.GetIFStream(sourceStreamType))
		, parentInvLifeTimes(context.m_parentContainer.GetIFStream(EPDT_InvLifeTime))
		, parentIds(context.m_container.GetIPidStream(EPDT_ParentId))
	{}
	ILINE floatv Sample(TParticleGroupId particleId) const
	{
		const uint32v parentId = parentIds.Load(particleId);
		const floatv selfAge = selfAges.Load(particleId);
		const floatv parentData = parentSourceStream.Load(parentId, 1.0f);
		const floatv parentInvLifeTime = parentInvLifeTimes.Load(parentId, 1.0f);
		return AntiAliasParentAge(deltaTime, selfAge, parentInvLifeTime, parentData);
	}
private:
	IFStream   selfAges;
	IFStream   parentSourceStream;
	IFStream   parentInvLifeTimes;
	IPidStream parentIds;
	floatv     deltaTime;
};

class CLevelTimeSampler
{
public:
	CLevelTimeSampler(const SUpdateContext& context)
		: deltaTime(ToFloatv(context.m_deltaTime))
		, selfAges(context.m_container.GetIFStream(EPDT_NormalAge))
	{}
	ILINE floatv Sample(TParticleGroupId particleId) const
	{
		const floatv selfAge = selfAges.Load(particleId);
		const floatv levelTime = ToFloatv(gEnv->pTimer->GetCurrTime());
		return AntiAliasParentAge(deltaTime, selfAge, ToFloatv(1.0f), levelTime);
	}
private:
	IFStream selfAges;
	floatv   deltaTime;
};

class CSelfSpeedSampler
{
public:
	CSelfSpeedSampler(const SUpdateContext& context, EModDomain domain = EMD_PerParticle)
		: velocities(domain == EMD_PerParticle ? context.m_container.GetIVec3Stream(EPVF_Velocity) : context.m_parentContainer.GetIVec3Stream(EPVF_Velocity))
	{}
	ILINE floatv Sample(TParticleGroupId particleId) const
	{
		const Vec3v velocity = velocities.Load(particleId);
		return Length(velocity);
	}
private:
	IVec3Stream velocities;
};

class CParentSpeedSampler
{
public:
	CParentSpeedSampler(const SUpdateContext& context, EModDomain domain)
		: parentVelocities(context.m_parentContainer.GetIVec3Stream(EPVF_Velocity))
		, parentIds(domain == EMD_PerInstance ? context.m_runtime.GetInstanceParentIds() : context.m_container.GetIPidStream(EPDT_ParentId))
	{}
	ILINE floatv Sample(TParticleGroupId particleId) const
	{
		const uint32v parentId = parentIds.Load(particleId);
		const Vec3v velocity = parentVelocities.Load(parentId);
		const floatv speed = Length(velocity);
		// PFX2_TODO : refactor this code into hight level instructions
#ifndef CRY_PFX2_USE_SSE
		const float sample = (parentId == gInvalidId) ? 0.0f : speed;
#else
		const __m128i mask = _mm_cmpeq_epi32(parentId, _mm_set1_epi32(gInvalidId));
		const floatv sample = CMov(_mm_castsi128_ps(mask), _mm_set1_ps(0.0f), speed);
#endif
		return sample;
	}
private:
	IVec3Stream parentVelocities;
	IPidStream  parentIds;
};

class CAttributeSampler
{
public:
	CAttributeSampler(const SUpdateContext& context, const string& m_attributeName)
	{
		const CAttributeInstance& attributes = context.m_runtime.GetEmitter()->GetAttributeInstance();
		auto attributeId = attributes.FindAttributeIdByName(m_attributeName.c_str());
		m_attributeValue = ToFloatv(attributes.GetAsFloat(attributeId, 1.0f));
	}
	ILINE floatv Sample(TParticleGroupId particleId) const
	{
		return m_attributeValue;
	}
private:
	floatv m_attributeValue;
};

}

ILINE CTimeSource::CTimeSource()
	: m_timeSource(ETimeSource::SelfTime)
	, m_fieldSource(ETimeSourceField::Age)
	, m_scale(1.0f)
	, m_bias(0.0f)
	, m_spawnOnly(true)
{
}

template<typename TParam, typename TMod>
ILINE void CTimeSource::AddToParam(CParticleComponent* pComponent, TParam* pParam, TMod* pModifier)
{
	CParticleComponent* pParent = pComponent->GetParentComponent();
	if (m_spawnOnly)
		pParam->AddToInitParticles(pModifier);
	else
		pParam->AddToUpdate(pModifier);

	//	if (m_timeSource == ETimeSource::SelfOrder)
	if (m_timeSource == ETimeSource::SpawnFraction)
		pComponent->AddParticleData(EPDT_SpawnFraction);
	else if (m_timeSource == ETimeSource::SelfSpeed)
		pComponent->AddParticleData(EPVF_Velocity);
	// else if (m_timeSource == ETimeSource::SelfField)
	else if (m_timeSource == ETimeSource::Field)
		pComponent->AddParticleData((EParticleDataType)m_fieldSource);

	else if (pParent && m_timeSource == ETimeSource::ParentOrder)
		pParent->AddParticleData(EPDT_SpawnFraction);
	else if (pParent && m_timeSource == ETimeSource::ParentSpeed)
		pParent->AddParticleData(EPVF_Velocity);
	else if (pParent && m_timeSource == ETimeSource::ParentField)
		pParent->AddParticleData((EParticleDataType)m_fieldSource);
}

ILINE void CTimeSource::SerializeInplace(Serialization::IArchive& ar)
{
	const auto& context = GetContext(ar);
	ar(m_timeSource, "TimeSource", "^>120>");

	if (m_timeSource == ETimeSource::Attribute)
		ar(m_attributeName, "AttributeName", "Attribute Name");

	//	if (m_timeSource == ETimeSource::SelfField || m_timeSource == ETimeSource::ParentField)
	if (m_timeSource == ETimeSource::Field || m_timeSource == ETimeSource::ParentField)
		ar(m_fieldSource, "Field", "Field");

	ar(m_scale, "Scale", "Scale");
	ar(m_bias, "Bias", "Bias");

	if (!context.HasUpdate())
		m_spawnOnly = true;
	else if (context.GetDomain() == EMD_PerParticle && m_timeSource == ETimeSource::SelfTime)
		m_spawnOnly = false;
	else
		ar(m_spawnOnly, "SpawnOnly", "Spawn Only");
}

ILINE EModDomain CTimeSource::GetDomain() const
{
	switch (m_timeSource)
	{
	case ETimeSource::SelfTime:
	//	case ETimeSource::SelfOrder:
	case ETimeSource::SpawnFraction:
	case ETimeSource::SelfSpeed:
	//	case ETimeSource::SelfField:
	case ETimeSource::Field:
		return EMD_PerParticle;
	case ETimeSource::ParentTime:
	case ETimeSource::ParentOrder:
	case ETimeSource::ParentSpeed:
	case ETimeSource::ParentField:
		return EMD_PerInstance;
	case ETimeSource::LevelTime:
	case ETimeSource::Attribute:
	default:
		return EMD_PerEffect;
	}
}

template<typename TBase, typename TStream>
ILINE void CTimeSource::Dispatch(const SUpdateContext& context, const SUpdateRange& range, TStream stream, EModDomain domain) const
{
	switch (m_timeSource)
	{
	case ETimeSource::SelfTime:
		((TBase*)this)->DoModify(
		  context, range, stream,
		  detail::CSelfStreamSampler(context, EPDT_NormalAge));
		break;
	case ETimeSource::ParentTime:
		if (domain == EMD_PerInstance)
			((TBase*)this)->DoModify(
			  context, range, stream,
			  detail::CSelfStreamSampler(context, EPDT_NormalAge, domain));
		else
			((TBase*)this)->DoModify(
			  context, range, stream,
			  detail::CParentStreamSampler(context, EPDT_NormalAge));
		break;
	case ETimeSource::LevelTime:
		((TBase*)this)->DoModify(
		  context, range, stream,
		  detail::CLevelTimeSampler(context));
		break;
	// case ETimeSource::SelfOrder:
	case ETimeSource::SpawnFraction:
		((TBase*)this)->DoModify(
		  context, range, stream,
		  detail::CSelfStreamSampler(context, EPDT_SpawnFraction));
		break;
	case ETimeSource::ParentOrder:
		if (domain == EMD_PerInstance)
			((TBase*)this)->DoModify(
			  context, range, stream,
			  detail::CSelfStreamSampler(context, EPDT_SpawnFraction, domain));
		else
			((TBase*)this)->DoModify(
			  context, range, stream,
			  detail::CParentStreamSampler(context, EPDT_SpawnFraction));
		break;
	case ETimeSource::SelfSpeed:
		((TBase*)this)->DoModify(
		  context, range, stream,
		  detail::CSelfSpeedSampler(context));
		break;
	case ETimeSource::ParentSpeed:
		if (domain == EMD_PerInstance)
			((TBase*)this)->DoModify(
			  context, range, stream,
			  detail::CSelfSpeedSampler(context, domain));
		else
			((TBase*)this)->DoModify(
			  context, range, stream,
			  detail::CParentSpeedSampler(context, domain));
		break;
	// case ETimeSource::SelfField:
	case ETimeSource::Field:
		((TBase*)this)->DoModify(
		  context, range, stream,
		  detail::CSelfStreamSampler(context, EParticleDataType(m_fieldSource)));
		break;
	case ETimeSource::ParentField:
		if (domain == EMD_PerInstance)
			((TBase*)this)->DoModify(
			  context, range, stream,
			  detail::CSelfStreamSampler(context, EParticleDataType(m_fieldSource), domain));
		else
			((TBase*)this)->DoModify(
			  context, range, stream,
			  detail::CParentStreamSampler(context, EParticleDataType(m_fieldSource)));
		break;
	case ETimeSource::Attribute:
		((TBase*)this)->DoModify(
		  context, range, stream,
		  detail::CAttributeSampler(context, m_attributeName));
		break;
	}
}

ILINE IParamModContext& CTimeSource::GetContext(Serialization::IArchive& ar) const
{
	IParamModContext* pContext = ar.context<IParamModContext>();
	CRY_PFX2_ASSERT(pContext != nullptr);
	return *pContext;
}

}
