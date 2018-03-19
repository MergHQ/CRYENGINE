// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     25/03/2015 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef PARAMMOD_H
#define PARAMMOD_H

#pragma once

#include "../ParticleComponent.h"
#include "ParamTraits.h"
#include <CrySerialization/SmartPtr.h>
#include <CrySerialization/IClassFactory.h>

namespace pfx2
{

enum EModDomain
{
	EMD_PerParticle,
	EMD_PerInstance,
	EMD_PerEffect,
};

struct IModifier;
struct IFieldModifier;

struct IParamMod
{
	virtual void AddToInitParticles(IModifier* pMod) = 0;
	virtual void AddToUpdate(IModifier* pMod) = 0;
};

struct IParamModContext
{
	using TModifier = IModifier;
	virtual EModDomain GetDomain() const = 0;
	virtual bool       HasInit() const = 0;
	virtual bool       HasUpdate() const = 0;
	virtual bool       CanInheritParent() const = 0;
};

struct SModParticleField : public IParamModContext
{
	using TModifier = IFieldModifier;
	EModDomain GetDomain() const        { return EMD_PerParticle; }
	bool       HasInit() const          { return true; }
	bool       HasUpdate() const        { return true; }
	bool       CanInheritParent() const { return true; }
};
struct SModParticleSpawnInit : public IParamModContext
{
	EModDomain GetDomain() const        { return EMD_PerParticle; }
	bool       HasInit() const          { return true; }
	bool       HasUpdate() const        { return false; }
	bool       CanInheritParent() const { return false; }
};
struct SModInstanceCounter : public IParamModContext
{
	EModDomain GetDomain() const        { return EMD_PerInstance; }
	bool       HasInit() const          { return true; }
	bool       HasUpdate() const        { return true; }
	bool       CanInheritParent() const { return false; }
};
struct SModInstanceTimer : public IParamModContext
{
	EModDomain GetDomain() const        { return EMD_PerInstance; }
	bool       HasInit() const          { return true; }
	bool       HasUpdate() const        { return false; }
	bool       CanInheritParent() const { return false; }
};
struct SModEffectField : public IParamModContext
{
	EModDomain GetDomain() const        { return EMD_PerEffect; }
	bool       HasInit() const          { return true; }
	bool       HasUpdate() const        { return true; }
	bool       CanInheritParent() const { return false; }
};

struct IModifier : public _i_reference_target_t
{
public:
	bool               IsEnabled() const                                { return m_enabled; }
	virtual EModDomain GetDomain() const = 0;
	virtual Range      GetMinMax() const = 0;
	virtual void       AddToParam(CParticleComponent* pComponent, IParamMod* pParam)                                                                             {}
	virtual void       Modify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, EParticleDataType streamType, EModDomain domain) const {}
	virtual void       Sample(float* samples, const int numSamples) const                                                                                        {}
	virtual void       Serialize(Serialization::IArchive& ar);
	virtual IModifier* VersionFixReplace() const                                                                                                                 { return nullptr; }
private:
	SEnable m_enabled;
};

struct IFieldModifier: IModifier
{
	virtual IFieldModifier* VersionFixReplace() const { return nullptr; }
};

template<typename TParamModContext, typename T = SFloat>
class CParamMod : public IParamMod
{
public:
	typedef T TValue;
	typedef typename T::TType TType;

	CParamMod(TType defaultValue = TType(1));

	virtual void                   AddToInitParticles(IModifier* pMod);
	virtual void                   AddToUpdate(IModifier* pMod);

	void                           AddToComponent(CParticleComponent* pComponent, CParticleFeature* pFeature);
	void                           AddToComponent(CParticleComponent* pComponent, CParticleFeature* pFeature, EParticleDataType dataType);
	void                           Serialize(Serialization::IArchive& ar);

	void                           InitParticles(const SUpdateContext& context, EParticleDataType dataType) const;
	void                           ModifyInit(const SUpdateContext& context, TType* data, SUpdateRange range) const;

	void                           Update(const SUpdateContext& context, EParticleDataType dataType) const;
	void                           ModifyUpdate(const SUpdateContext& context, TType* data, SUpdateRange range) const;

	TRange<TType>                  GetValues(const SUpdateContext& context, TType* data, SUpdateRange range, EModDomain domain, bool updating) const;
	TRange<TType>                  GetValues(const SUpdateContext& context, TVarArray<TType> data, EModDomain domain, bool updating) const;
	TRange<TType>                  GetValueRange(const SUpdateContext& context) const;
	TRange<TType>                  GetValueRange() const;
	void                           Sample(TType* samples, int numSamples) const;

	bool                           HasInitModifiers() const   { return !m_modInit.empty(); }
	bool                           HasUpdateModifiers() const { return !m_modUpdate.empty(); }
	bool                           HasModifiers() const       { return !m_modInit.empty() || !m_modUpdate.empty(); }
	TType                          GetBaseValue() const       { return m_baseValue; }
	bool                           IsEnabled() const          { return std::isfinite(m_baseValue); }
	static const TParamModContext& Context()                  { static TParamModContext context; return context; }

private:
	using TModifier = typename TParamModContext::TModifier;
	using PModifier = _smart_ptr<TModifier>;

	T                       m_baseValue;
	std::vector<PModifier>  m_modifiers;
	std::vector<IModifier*> m_modInit;
	std::vector<IModifier*> m_modUpdate;
};

template<typename T>
struct STempModBuffer
{
	template<typename TParamMod>
	STempModBuffer(const SUpdateContext& context, TParamMod& paramMod)
		: m_buffer(*context.m_pMemHeap)
		, m_stream(nullptr, paramMod.GetBaseValue()) {}

	T* Allocate(SGroupRange range, T baseValue)
	{
		m_buffer.resize(range.size());
		T* data = m_buffer.data() - +*range.begin();
		m_stream = TIStream<T>(data, baseValue);
		return data;
	}

	template<typename TParamMod>
	void ModifyInit(const SUpdateContext& context, TParamMod& paramMod, SUpdateRange range)
	{
		if (paramMod.HasInitModifiers())
		{
			T* data = Allocate(range, paramMod.GetBaseValue());
			paramMod.ModifyInit(context, data, range);
		}
	}

	template<typename TParamMod>
	void ModifyUpdate(const SUpdateContext& context, TParamMod& paramMod, SUpdateRange range)
	{
		if (paramMod.HasUpdateModifiers())
		{
			T* data = Allocate(range, paramMod.GetBaseValue());
			m_stream.Fill(range, paramMod.GetBaseValue());
			paramMod.ModifyUpdate(context, data, range);
		}
	}

	THeapArray<T> m_buffer;
	TIStream<T>   m_stream;
};

template<typename T>
struct STempInitBuffer: STempModBuffer<T>
{
	template<typename TParamMod>
	STempInitBuffer(const SUpdateContext& context, TParamMod& paramMod)
		: STempModBuffer<T>(context, paramMod)
	{
		this->ModifyInit(context, paramMod, context.GetSpawnedRange());
	}
};

}

#include "ParamModImpl.h"

#endif // PARAMMOD_H
