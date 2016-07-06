// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     25/03/2015 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <CryMath/PNoise3.h>
#include "ParamMod.h"
#include "ParticleComponentRuntime.h"
#include "TimeSource.h"

CRY_PFX2_DBG

volatile bool gFeatureModifiers = false;

namespace pfx2
{

class CFTimeSource : public CTimeSource, public IModifier
{
public:
	virtual EModDomain GetDomain() const
	{
		return CTimeSource::GetDomain();
	}
};

//////////////////////////////////////////////////////////////////////////
// CModCurve

class CModCurve : public CFTimeSource
{
public:
	CModCurve() {}

	virtual bool CanCreate(const IParamModContext& context) const
	{
		return context.HasInit();
	}

	virtual void AddToParam(CParticleComponent* pComponent, IParamMod* pParam)
	{
		if (m_spline.HasKeys())
			CTimeSource::AddToParam(pComponent, pParam, this);
	}

	virtual void Serialize(Serialization::IArchive& ar)
	{
		IModifier::Serialize(ar);
		CTimeSource::SerializeInplace(ar);
		Serialization::SContext _splineContext(ar, Serialization::getEnumDescription<ETimeSource>().label(int(CTimeSource::GetTimeSource())));
		ar(m_spline, "Curve", "Curve");
	}

	virtual void Sample(float* samples, const int numSamples) const
	{
		for (int i = 0; i < numSamples; ++i)
		{
			const float point = (float)i / numSamples;
			float dataIn = samples[i];
			float spline = m_spline.Interpolate(point);
			float dataOut = dataIn * spline;
			samples[i] = dataOut;
		}
	}

	virtual void Modify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, EParticleDataType streamType, EModDomain domain) const
	{
		CRY_PFX2_PROFILE_DETAIL;
		CTimeSource::Dispatch<CModCurve>(context, range, stream, domain);
	}

	template<typename TTimeKernel>
	void DoModify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, const TTimeKernel& timeKernel) const
	{
		const CParticleComponentRuntime& runtime = context.m_runtime;
		const CParticleContainer& container = context.m_container;

		CRY_PFX2_FOR_RANGE_PARTICLESGROUP(range)
		{
			const floatv inValue = stream.Load(particleGroupId);
			const floatv time = timeKernel.Sample(particleGroupId);
			const floatv value = m_spline.Interpolate(time);
			const floatv outvalue = Mul(inValue, value);
			stream.Store(particleGroupId, outvalue);
		}
		CRY_PFX2_FOR_END;
	}

	virtual Range GetMinMax() const
	{
		return m_spline.GetValueRange();
	}

private:
	CParticleSpline m_spline;
};

SERIALIZATION_CLASS_NAME(IModifier, CModCurve, "Curve", "Curve");

//////////////////////////////////////////////////////////////////////////
// CModDoubleCurve

class CModDoubleCurve : public CFTimeSource
{
public:
	CModDoubleCurve() {}

	virtual bool CanCreate(const IParamModContext& context) const
	{
		return context.HasInit();
	}

	virtual void AddToParam(CParticleComponent* pComponent, IParamMod* pParam)
	{
		if (m_spline.HasKeys())
		{
			CTimeSource::AddToParam(pComponent, pParam, this);
			pComponent->AddParticleData(EPDT_UNormRand);
		}
	}

	virtual void Serialize(Serialization::IArchive& ar)
	{
		IModifier::Serialize(ar);
		CTimeSource::SerializeInplace(ar);
		Serialization::SContext _splineContext(ar, Serialization::getEnumDescription<ETimeSource>().label(int(CTimeSource::GetTimeSource())));
		ar(m_spline, "DoubleCurve", "Double Curve");
	}

	virtual void Modify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, EParticleDataType streamType, EModDomain domain) const
	{
		CRY_PFX2_PROFILE_DETAIL;
		CTimeSource::Dispatch<CModDoubleCurve>(context, range, stream, domain);
	}

	template<typename TimeKernel>
	void DoModify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, const TimeKernel& timeKernel) const
	{
		CRY_PFX2_PROFILE_DETAIL;

		const CParticleComponentRuntime& runtime = context.m_runtime;
		const CParticleContainer& container = context.m_container;
		const IFStream unormRands = container.GetIFStream(EPDT_UNormRand);

		CRY_PFX2_FOR_RANGE_PARTICLESGROUP(range)
		{
			const floatv unormRand = unormRands.Load(particleGroupId);
			const floatv inValue = stream.Load(particleGroupId);
			const floatv time = timeKernel.Sample(particleGroupId);
			const floatv value = m_spline.Interpolate(time, unormRand);
			const floatv outvalue = Mul(inValue, value);
			stream.Store(particleGroupId, outvalue);
		}
		CRY_PFX2_FOR_END;
	}

	virtual Range GetMinMax() const
	{
		return m_spline.GetValueRange();
	}

private:
	CParticleDoubleSpline m_spline;
};

SERIALIZATION_CLASS_NAME(IModifier, CModDoubleCurve, "DoubleCurve", "Double Curve");

//////////////////////////////////////////////////////////////////////////
// CModRandom

class CModRandom : public IModifier
{
public:
	CModRandom(float amount = 0.0f)
		: m_amount(amount)
	{}

	virtual bool CanCreate(const IParamModContext& context) const
	{
		return context.HasInit();
	}

	virtual EModDomain GetDomain() const
	{
		return EMD_PerParticle;
	}

	virtual void AddToParam(CParticleComponent* pComponent, IParamMod* pParam)
	{
		pParam->AddToInitParticles(this);
	}

	virtual void Serialize(Serialization::IArchive& ar)
	{
		IModifier::Serialize(ar);
		ar(m_amount, "Amount", "Amount");
	}

	virtual void Modify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, EParticleDataType streamType, EModDomain domain) const
	{
		CRY_PFX2_PROFILE_DETAIL;

		SChaosKeyV chaos;
		SChaosKeyV::Range randRange(1.0f - m_amount, 1.0f);

		CRY_PFX2_FOR_RANGE_PARTICLESGROUP(range)
		{
			const floatv inValue = stream.Load(particleGroupId);
			const floatv value = chaos.Rand(randRange);
			const floatv outvalue = Mul(inValue, value);
			stream.Store(particleGroupId, outvalue);
		}
		CRY_PFX2_FOR_END;
	}

	virtual Range GetMinMax() const
	{
		return Range(1.0f - m_amount, 1.0f);
	}

private:

	UFloat m_amount;
};

SERIALIZATION_CLASS_NAME(IModifier, CModRandom, "Random", "Random");

//////////////////////////////////////////////////////////////////////////
// CModNoise

SERIALIZATION_DECLARE_ENUM(EParamNoiseMode,
                           Smooth,
                           Fractal,
                           Pulse
                           )

class CModNoise : public CFTimeSource
{
public:
	CModNoise()
		: m_amount(0.0f)
		, m_rate(1.0f)
		, m_pulseWidth(0.5f)
		, m_mode(EParamNoiseMode::Smooth) {}

	virtual bool CanCreate(const IParamModContext& context) const
	{
		return context.HasInit();
	}

	virtual void AddToParam(CParticleComponent* pComponent, IParamMod* pParam)
	{
		CTimeSource::AddToParam(pComponent, pParam, this);
	}

	virtual void Serialize(Serialization::IArchive& ar)
	{
		IModifier::Serialize(ar);
		if (!VersionFix(ar))
			ar(m_mode, "Mode", "Mode");
		ar(m_amount, "Amount", "Amount");
		CTimeSource::SerializeInplace(ar);
		ar(m_rate, "Rate", "Rate");
		if (m_mode == EParamNoiseMode::Pulse)
			ar(m_pulseWidth, "PulseWidth", "Pulse Width");
	}

	virtual IModifier* VersionFixReplace() const
	{
		if (m_mode == EParamNoiseMode(-1))
			return new CModRandom(m_amount);
		return nullptr;
	}

	virtual void Modify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, EParticleDataType streamType, EModDomain domain) const
	{
		CRY_PFX2_PROFILE_DETAIL;
		CTimeSource::Dispatch<CModNoise>(context, range, stream, domain);
	}

	template<typename TimeKernel>
	void DoModify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, const TimeKernel& timeKernel) const
	{
		CRY_PFX2_PROFILE_DETAIL;

		switch (m_mode)
		{
		case EParamNoiseMode::Smooth:
			Modify(context, range, stream, timeKernel,
			       [](floatv time){ return Smooth(time); });
			break;
		case EParamNoiseMode::Fractal:
			Modify(context, range, stream, timeKernel,
			       [](floatv time){ return Fractal(time); });
			break;
		case EParamNoiseMode::Pulse:
			Modify(context, range, stream, timeKernel,
			       [this](floatv time){ return Pulse(time); });
			break;
		}
	}

	virtual Range GetMinMax() const
	{
		return Range(1.0f - m_amount, 1.0f);
	}

private:
	template<typename TimeKernel, typename ModeFn>
	ILINE void Modify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, const TimeKernel& timeKernel, ModeFn modeFn) const
	{
		const floatv one = ToFloatv(1.0f);
		const floatv amount = ToFloatv(m_amount);
		const floatv rate = ToFloatv(m_rate);

		CRY_PFX2_FOR_RANGE_PARTICLESGROUP(range)
		{
			const floatv inValue = stream.Load(particleGroupId);
			const floatv time = timeKernel.Sample(particleGroupId);
			const floatv value = modeFn(Mul(time, rate));
			const floatv outvalue = Mul(inValue, Sub(one, Mul(value, amount)));
			stream.Store(particleGroupId, outvalue);
		}
		CRY_PFX2_FOR_END;
	}

	// PFX2_TODO : properly vectorize this code and make it not dependent on SSE4.1
	ILINE static floatv UPNoise(floatv x)
	{
		CPNoise3& noiseGen = *gEnv->pSystem->GetNoiseGen();
#ifndef CRY_PFX2_USE_SSE
		return noiseGen.Noise1D(x) * 2.5f;
#else
		const float x0 = detail::ExtractF32<0>(x);
		const float x1 = detail::ExtractF32<1>(x);
		const float x2 = detail::ExtractF32<2>(x);
		const float x3 = detail::ExtractF32<3>(x);
		const float v0 = noiseGen.Noise1D(x0);
		const float v1 = noiseGen.Noise1D(x1);
		const float v2 = noiseGen.Noise1D(x2);
		const float v3 = noiseGen.Noise1D(x3);
		return _mm_mul_ps(_mm_set_ps(v3, v2, v1, v0), _mm_set1_ps(2.5f));
#endif
	}

	ILINE static floatv Smooth(floatv time)
	{
		const floatv half = ToFloatv(0.5f);
		return MAdd(UPNoise(time), half, half);
	}

	ILINE static floatv Fractal(floatv time)
	{
		const floatv two = ToFloatv(2.0f);
		const floatv four = ToFloatv(4.0f);
		const floatv half = ToFloatv(0.5f);
		const floatv quarter = ToFloatv(0.25f);
		floatv fractal = UPNoise(time);
		fractal = MAdd(UPNoise(Mul(time, two)), half, fractal);
		fractal = MAdd(UPNoise(Mul(time, four)), quarter, fractal);
		return MAdd(fractal, half, half);
	}

	ILINE floatv Pulse(floatv time) const
	{
		const floatv one = ToFloatv(1.0f);
		const floatv pulseWidth = ToFloatv(m_pulseWidth);
		return if_else_zero(UPNoise(time) < pulseWidth, one);
	}

	bool VersionFix(Serialization::IArchive& ar)
	{
		if (ar.isInput() && GetVersion(ar) < 5)
		{
			// Read deprecated Random mode
			string mode;
			ar(mode, "Mode");
			if (mode == "Random")
			{
				m_mode = EParamNoiseMode(-1);
				return true;
			}
		}
		return false;
	}

	SFloat          m_amount;
	UFloat10        m_rate;
	UUnitFloat      m_pulseWidth;
	EParamNoiseMode m_mode;
};

SERIALIZATION_CLASS_NAME(IModifier, CModNoise, "Noise", "Noise");

//////////////////////////////////////////////////////////////////////////
// CModWave

SERIALIZATION_DECLARE_ENUM(EWaveType,
                           Sin,
                           Saw,
                           Pulse
                           )

class CModWave : public CFTimeSource
{
public:
	CModWave()
		: m_waveType(EWaveType::Sin)
		, m_rate(1.0f)
		, m_amplitude(1.0f)
		, m_phase(0.0f)
		, m_bias(0.5f)
		, m_inverse(false) {}

	virtual bool CanCreate(const IParamModContext& context) const
	{
		return context.HasInit();
	}

	virtual void AddToParam(CParticleComponent* pComponent, IParamMod* pParam)
	{
		CTimeSource::AddToParam(pComponent, pParam, this);
	}

	virtual void Serialize(Serialization::IArchive& ar)
	{
		IModifier::Serialize(ar);
		CTimeSource::SerializeInplace(ar);
		ar(m_waveType, "Wave", "Wave");
		ar(m_rate, "Rate", "Rate");
		ar(m_amplitude, "Amplitude", "Amplitude");
		ar(m_phase, "Phase", "Phase");
		ar(m_bias, "Bias", "Bias");
		ar(m_inverse, "Inverse", "Inverse");
	}

	virtual void Modify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, EParticleDataType streamType, EModDomain domain) const
	{
		CRY_PFX2_PROFILE_DETAIL;
		CTimeSource::Dispatch<CModWave>(context, range, stream, domain);
	}

	template<typename TimeKernel>
	void DoModify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, const TimeKernel& timeKernel) const
	{
		switch (m_waveType)
		{
		case EWaveType::Sin:
			Modify(context, range, stream, timeKernel,
			       [](floatv time){ return Sin(time); });
			break;
		case EWaveType::Saw:
			Modify(context, range, stream, timeKernel,
			       [](floatv time){ return Saw(time); });
			break;
		case EWaveType::Pulse:
			Modify(context, range, stream, timeKernel,
			       [](floatv time){ return Pulse(time); });
			break;
		}
	}

	virtual Range GetMinMax() const
	{
		return Range(m_bias - m_amplitude * 0.5f, m_bias + m_amplitude * 0.5f);
	}

private:
	template<typename TimeKernel, typename WaveFn>
	void Modify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, const TimeKernel& timeKernel, WaveFn waveFn) const
	{
		const floatv mult = ToFloatv(m_amplitude * (m_inverse ? -1.0f : 1.0f) * 0.5f);
		const floatv bias = ToFloatv(m_bias);
		const floatv phase = ToFloatv(m_phase);
		const floatv rate = ToFloatv(m_rate);

		CRY_PFX2_FOR_RANGE_PARTICLESGROUP(range);
		{
			const floatv inValue = stream.Load(particleGroupId);
			const floatv time = timeKernel.Sample(particleGroupId);
			const floatv sample = waveFn(Mul(Sub(time, phase), rate));
			const floatv value = MAdd(sample, mult, bias);
			const floatv outvalue = Mul(inValue, value);
			stream.Store(particleGroupId, outvalue);
		}
		CRY_PFX2_FOR_END;
	}

	// PFX2_TODO : improve functions and place them in a common library
#ifdef CRY_PFX2_USE_SSE
	ILINE static floatv SinApproax(floatv x)
	{
		const floatv one = ToFloatv(1.0f);
		const floatv half = ToFloatv(0.5f);
		const floatv negHalfPi = ToFloatv(-gf_PI * 0.5f);
		const floatv pi = ToFloatv(gf_PI);
		const floatv ipi = ToFloatv(1.0f / gf_PI);
		const floatv ipi2 = ToFloatv(1.0f / gf_PI2);

		const floatv x1 = MAdd(Frac(Mul(x, ipi)), pi, negHalfPi);
		const floatv m = sign(Frac(Mul(x, ipi2)) - half);

		const floatv p0 = ToFloatv(-0.4964738f);
		const floatv p1 = ToFloatv(0.036957536f);
		const floatv x2 = Mul(x1, x1);
		const floatv x4 = Mul(x2, x2);
		const floatv result = Mul(MAdd(x4, p1, MAdd(x2, p0, one)), m);

		return result;
	}

	ILINE static floatv CosApproax(floatv x)
	{
		const floatv halfPi = ToFloatv(gf_PI * 0.5f);
		return SinApproax(Sub(x, halfPi));
	}
#else

	ILINE static floatv CosApproax(floatv x)
	{
		return cosf(x);
	}

#endif

	ILINE static floatv Sin(floatv time)
	{
		return -CosApproax(Mul(time, ToFloatv(gf_PI2)));
	}

	ILINE static floatv Saw(floatv time)
	{
		const floatv minusOne = ToFloatv(-1.0f);
		const floatv two = ToFloatv(2.0f);
		return MAdd(Frac(time), two, minusOne);
	}

	ILINE static floatv Pulse(floatv time)
	{
		const floatv half = ToFloatv(0.5f);
		return sign(half - Frac(time));
	}

	EWaveType m_waveType;
	UFloat10  m_rate;
	UFloat    m_amplitude;
	SFloat    m_phase;
	SFloat    m_bias;
	bool      m_inverse;
};

SERIALIZATION_CLASS_NAME(IModifier, CModWave, "Wave", "Wave");

//////////////////////////////////////////////////////////////////////////
// CModInherit

class CModInherit : public IModifier
{
public:
	CModInherit()
		: m_spawnOnly(true) {}

	virtual bool CanCreate(const IParamModContext& context) const
	{
		return context.GetDomain() >= EMD_PerParticle && context.CanInheritParent();
	}

	virtual EModDomain GetDomain() const
	{
		return EMD_PerParticle;
	}

	virtual void AddToParam(CParticleComponent* pComponent, IParamMod* pParam)
	{
		if (m_spawnOnly)
			pParam->AddToInitParticles(this);
		else
			pParam->AddToUpdate(this);
	}

	virtual void Serialize(Serialization::IArchive& ar)
	{
		IModifier::Serialize(ar);
		ar(m_spawnOnly, "SpawnOnly", "Spawn Only");
	}

	virtual void Modify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, EParticleDataType streamType, EModDomain domain) const
	{
		CRY_PFX2_PROFILE_DETAIL;

		CParticleContainer& container = context.m_container;
		CParticleContainer& parentContainer = context.m_parentContainer;
		if (!parentContainer.HasData(streamType))
			return;
		IPidStream parentIds = context.m_container.GetIPidStream(EPDT_ParentId);
		IFStream parentStream = parentContainer.GetIFStream(streamType);

		CRY_PFX2_FOR_RANGE_PARTICLESGROUP(range)
		{
			const TParticleIdv parentId = parentIds.Load(particleGroupId);
			const floatv input = stream.Load(particleGroupId);
			const floatv parent = parentStream.Load(parentId, 0.0f);
			const floatv output = Mul(input, parent);
			stream.Store(particleGroupId, output);
		}
		CRY_PFX2_FOR_END;
	}

	virtual Range GetMinMax() const
	{
		// To do: Wrong! Depends on inherited value
		return Range(0.0f, 1.0f);
	}
private:
	bool m_spawnOnly;
};

SERIALIZATION_CLASS_NAME(IModifier, CModInherit, "Inherit", "Inherit");

//////////////////////////////////////////////////////////////////////////
// CModLinear

class CModLinear : public CFTimeSource
{
public:
	CModLinear() {}

	virtual bool CanCreate(const IParamModContext& context) const
	{
		return true;
	}

	virtual void AddToParam(CParticleComponent* pComponent, IParamMod* pParam)
	{
		CTimeSource::AddToParam(pComponent, pParam, this);
	}

	virtual void Serialize(Serialization::IArchive& ar)
	{
		IModifier::Serialize(ar);
		CTimeSource::SerializeInplace(ar);
	}

	virtual void Modify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, EParticleDataType streamType, EModDomain domain) const
	{
		CRY_PFX2_PROFILE_DETAIL;
		CTimeSource::Dispatch<CModLinear>(context, range, stream, domain);
	}

	template<typename TimeKernel>
	void DoModify(const SUpdateContext& context, const SUpdateRange& range, IOFStream stream, const TimeKernel& timeKernel) const
	{
		const CParticleComponentRuntime& runtime = context.m_runtime;
		const CParticleContainer& container = context.m_container;

		CRY_PFX2_FOR_RANGE_PARTICLESGROUP(range)
		{
			const floatv inValue = stream.Load(particleGroupId);
			const floatv sample = timeKernel.Sample(particleGroupId);
			const floatv outvalue = Mul(inValue, sample);
			stream.Store(particleGroupId, outvalue);
		}
		CRY_PFX2_FOR_END;
	}

	virtual Range GetMinMax() const
	{
		// To do: Wrong! Depends on TimeSource range
		return Range(Adjust(0.0f), Adjust(1.0f));
	}
};

SERIALIZATION_CLASS_NAME(IModifier, CModLinear, "Linear", "Linear");

}
