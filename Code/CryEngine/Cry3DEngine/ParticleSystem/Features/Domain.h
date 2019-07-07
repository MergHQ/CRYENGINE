// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ParticleSystem/ParticleCommon.h"
#include "ParticleSystem/ParticleAttributes.h"
#include "ParticleSystem/ParticleEmitter.h"
#include "ParticleSystem/ParticleDataTypes.h"
#include "ParamMod.h"
#include "FeatureColor.h"

namespace pfx2
{

SERIALIZATION_ENUM_DECLARE(EDomain, ,
	Age,
	SpawnFraction,
	SpawnId,
	Speed,
	Field,
	Attribute,
	ViewAngle,
	CameraDistance,
	Global,
	Random,

	// old version
	_ParentTime,
	_ParentOrder,
	_ParentSpeed,
	_ParentField,

	_SelfTime = Age,
	_SelfSpeed = Speed,
	_LevelTime = Global
)

SERIALIZATION_ENUM_DECLARE(EDomainOwner, ,
    _None,
	Self,
	Parent
)

SERIALIZATION_ENUM_DECLARE(EDomainGlobal, ,
	LevelTime,
	TimeOfDay,
	ExposureValue
)

typedef DynamicEnum<struct SDomainField> EDomainField;
bool Serialize(Serialization::IArchive& ar, EDomainField& value, cstr name, cstr label);

struct CDomain
{
	void             Serialize(Serialization::IArchive& ar);
	void             AddToParam(CParticleComponent* pComponent);
	EDataDomain      GetDomain() const;
	string           GetSourceDescription() const;
	float            GetGlobalValue(EDomainGlobal source) const;
	TDataType<float> GetDataType() const;

protected:
	CAttributeReference m_attribute;
	EDomain             m_domain       = EDomain::Age;
	EDomainField        m_fieldSource  = EDomainField(EPDT_LifeTime);
	EDomainOwner        m_sourceOwner  = EDomainOwner::Self;
	EDomainGlobal       m_sourceGlobal = EDomainGlobal::LevelTime;
	PosInt              m_idModulus;
	bool                m_spawnOnly    = true;
	float               m_domainScale  = 1;
	float               m_domainBias   = 0;
	floatv              m_scaleV;
	floatv              m_biasV;

	ILINE floatv AdjustDomain(floatv in) const { return MAdd(in, m_scaleV, m_biasV); }
};

template<typename TChild, typename T, typename TFrom = T>
struct TModFunction: ITypeModifier<T, TFrom>, CDomain
{
	void        AddToParam(CParticleComponent* pComponent) override { CDomain::AddToParam(pComponent); }
	EDataDomain GetDomain() const override { return CDomain::GetDomain(); }
	void        Modify(const CParticleComponentRuntime& runtime, const SUpdateRange& range, TIOStream<T> stream, EDataDomain domain) const override;
	void        Sample(TVarArray<TFrom> samples) const override;

private:
	TChild const& Child() const { return (TChild const&)*this; }

	template<typename S, typename ...Args>
	void ModifyFromSampler(const CParticleComponentRuntime& runtime, const SUpdateRange& range, TIOStream<T> stream, EDataDomain domain, Args ...args) const;
};

}

#include "DomainImpl.h"
