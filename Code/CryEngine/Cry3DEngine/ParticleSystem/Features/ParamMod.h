// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "../ParticleDataTypes.h"
#include "../ParticleDataStreams.h"
#include "../ParticleComponentRuntime.h"
#include "ParamTraits.h"
#include <CrySerialization/SmartPtr.h>
#include <CrySerialization/IClassFactory.h>
#include <cmath>

namespace pfx2
{

struct IModifier : public _i_reference_target_t
{
public:
	bool                IsEnabled() const { return m_enabled; }
	virtual EDataDomain GetDomain() const = 0;
	virtual void        AddToParam(CParticleComponent* pComponent) {}
	virtual void        SetDataType(EParticleDataType dataType) {}
	virtual void        Serialize(Serialization::IArchive& ar) { ar(m_enabled); }
private:
	SEnable m_enabled;
};

template<typename T, typename TFrom = T>
struct ITypeModifier: IModifier
{
	virtual void           Modify(const CParticleComponentRuntime& runtime, const SUpdateRange& range, TIOStream<T> stream, EDataDomain domain) const = 0;
	virtual void           Sample(TVarArray<TFrom> samples) const {}
	virtual TRange<TFrom>  GetMinMax() const { return TRange<TFrom>(TFrom(0), TFrom(1)); }
	virtual ITypeModifier* VersionFixReplace() const { return nullptr; }
};

template<typename T, typename TFrom = T>
struct IFieldModifier: ITypeModifier<T, TFrom>
{
	virtual IFieldModifier* VersionFixReplace() const { return nullptr; }
};

template<EDataDomain Domain, typename T = SFloat>
class CParamMod
{
public:
	typedef T TValue;
	typedef typename T::TStore TType;
	typedef typename T::TType TFrom;
	typedef TDataType<TType> ThisDataType;

	using TModifier = typename std::conditional<!!(Domain & EDD_HasUpdate),
		IFieldModifier<TType, TFrom>, 
		ITypeModifier<TType, TFrom>
	>::type;
	using PModifier = _smart_ptr<TModifier>;

	CParamMod(TFrom defaultValue = TFrom(1))
		: m_baseValue(defaultValue) {}

	void          AddToComponent(CParticleComponent* pComponent, CParticleFeature* pFeature);
	void          AddToComponent(CParticleComponent* pComponent, CParticleFeature* pFeature, ThisDataType dataType);
	void          Serialize(Serialization::IArchive& ar);

	void          Init(CParticleComponentRuntime& runtime, ThisDataType dataType) const;
	void          ModifyInit(const CParticleComponentRuntime& runtime, TIOStream<TType>& stream, SUpdateRange range) const;

	void          Update(CParticleComponentRuntime& runtime, ThisDataType dataType) const;
	void          ModifyUpdate(const CParticleComponentRuntime& runtime, TIOStream<TType>& stream, SUpdateRange range) const;

	TRange<TFrom> GetValues(const CParticleComponentRuntime& runtime, TVarArray<TType> data, EDataDomain domain) const;
	TRange<TFrom> GetValueRange(const CParticleComponentRuntime& runtime) const;
	TRange<TFrom> GetValueRange() const;
	void          Sample(TVarArray<TFrom> samples) const;

	bool          HasInitModifiers() const   { return !m_modInit.empty(); }
	bool          HasUpdateModifiers() const { return !m_modUpdate.empty(); }
	bool          HasModifiers() const       { return !m_modifiers.empty(); }
	TType         GetBaseValue() const       { return m_baseValue; }
	bool          IsEnabled() const          { return crymath::valueisfinite(m_baseValue); }

protected:
	T                      m_baseValue;
	std::vector<PModifier> m_modifiers;
	std::vector<PModifier> m_modInit;
	std::vector<PModifier> m_modUpdate;
};

//////////////////////////////////////////////////////////////////////////
// Copy factory creators from base class to derived class
template<typename BaseType, typename Type>
struct ClassFactoryInheritor
{
	using BaseFactory = Serialization::ClassFactory<BaseType>;
	using TypeFactory = Serialization::ClassFactory<Type>;

	ClassFactoryInheritor()
	{
		auto creators = BaseFactory::the().creatorChain();
		TypeFactory::the().registerChain(reinterpret_cast<typename TypeFactory::CreatorBase*>(creators));
	}
};

#define SERIALIZATION_INHERIT_CREATORS(BaseType, Type) \
	static ClassFactoryInheritor<BaseType, Type> ClassFactoryInherit_ ## Type;

///////////////////////////////////////////////////////////////////////////////
// Temp buffers

template<typename T>
struct STempBuffer: TIStream<T>
{
	template<typename TParamMod>
	STempBuffer(const CParticleComponentRuntime& runtime, TParamMod& paramMod)
		: TIStream<T>(nullptr, paramMod.GetBaseValue())
		, m_buffer(runtime.MemHeap())
	{}

protected:

	THeapArray<T> m_buffer;

	void Allocate(SGroupRange range)
	{
		m_buffer.resize(range.size());
		T* data = m_buffer.data() - +*range.begin();
		static_cast<TIStream<T>&>(*this) = TIStream<T>(data);
	}
};

template<typename T>
struct STempInitBuffer: STempBuffer<T>
{
	template<typename TParamMod>
	STempInitBuffer(const CParticleComponentRuntime& runtime, TParamMod& paramMod, SUpdateRange range)
		: STempBuffer<T>(runtime, paramMod)
	{
		if (paramMod.HasInitModifiers())
		{
			this->Allocate(range);
			paramMod.ModifyInit(runtime, *this, range);
		}
	}

	template<typename TParamMod>
	STempInitBuffer(const CParticleComponentRuntime& runtime, TParamMod& paramMod)
		: STempInitBuffer<T>(runtime, paramMod, runtime.SpawnedRange())
	{}
};

template<typename T>
struct STempUpdateBuffer: STempBuffer<T>
{
	template<typename TParamMod>
	STempUpdateBuffer(const CParticleComponentRuntime& runtime, TParamMod& paramMod, SUpdateRange range)
		: STempBuffer<T>(runtime, paramMod)
	{
		if (paramMod.HasModifiers())
		{
			this->Allocate(range);
			paramMod.ModifyInit(runtime, *this, range);
			paramMod.ModifyUpdate(runtime, *this, range);
		}
	}
};

template<typename T>
struct SInstanceUpdateBuffer: STempBuffer<T>
{
	template<typename TParamMod>
	SInstanceUpdateBuffer(const CParticleComponentRuntime& runtime, TParamMod& paramMod)
		: STempBuffer<T>(runtime, paramMod), m_runtime(runtime), m_range(1, 1)
	{
		if (paramMod.HasModifiers())
		{
			this->Allocate(SUpdateRange(0, runtime.GetNumInstances()));
			m_range = paramMod.GetValues(runtime, this->m_buffer, EDD_PerInstance);
		}
	}

	ILINE T operator[](uint id) const
	{
		const TParticleId parentId = m_runtime.GetInstance(id).m_parentId;
		return this->SafeLoad(parentId);
	}

	TRange<T> const& Range() const { return m_range; }

private:
	const CParticleComponentRuntime& m_runtime;
	TRange<T>                        m_range;
};



}

#include "ParamModImpl.h"

