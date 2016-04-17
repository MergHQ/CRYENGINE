// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ParticleCommon.h"
#include "ParticleAttributes.h"
#include "ParticleEmitter.h"
#include "ParamMod.h"
#include "FeatureColor.h"

namespace pfx2
{

SERIALIZATION_ENUM_DECLARE(ETimeSource, ,
                           SelfTime,
                           ParentTime,
                           LevelTime,
                           // _SpawnFraction, SelfOrder = _SpawnFraction,
                           SpawnFraction,
                           ParentOrder,
                           SelfSpeed,
                           ParentSpeed,
                           // _Field, SelfField = _Field,
                           Field,
                           ParentField,
                           Attribute
                           )

SERIALIZATION_ENUM_DECLARE(ETimeSourceField, ,
                           LifeTime = EPDT_LifeTime,
                           Age = EPDT_NormalAge,
                           Size = EPDT_Size,
                           Opacity = EPDT_Alpha,
                           GravityMultiplier = EPDT_Gravity,
                           Drag = EPDT_Drag,
                           Angle = EPDT_Angle2D,
                           Spin = EPDT_Spin2D,
                           Random = EPDT_UNormRand
                           )

class CTimeSource
{
public:
	CTimeSource();

	template<typename TParam, typename TMod>
	void        AddToParam(CParticleComponent* pComponent, TParam* pParam, TMod* pModifier);
	template<typename TBase, typename TStream>
	void        Dispatch(const SUpdateContext& context, const SUpdateRange& range, TStream stream, EModDomain domain) const;

	ETimeSource GetTimeSource() const       { return m_timeSource; }
	EModDomain  GetDomain() const;
	float       Adjust(float sample) const  { return sample * m_scale + m_bias; }
#ifdef CRY_PFX2_USE_SSE
	floatv      Adjust(floatv sample) const { return MAdd(sample, ToFloatv(m_scale), ToFloatv(m_bias)); }
#endif //CRY_PFX2_USE_SSE
	void        SerializeInplace(Serialization::IArchive& ar);

private:
	IParamModContext& GetContext(Serialization::IArchive& ar) const;

	string           m_attributeName;
	ETimeSource      m_timeSource;
	ETimeSourceField m_fieldSource;
	float            m_scale;
	float            m_bias;
	bool             m_spawnOnly;
};

}

#include "TimeSourceImpl.h"
