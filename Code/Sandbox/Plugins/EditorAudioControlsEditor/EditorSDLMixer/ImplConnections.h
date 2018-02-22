// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ImplConnection.h>

namespace ACE
{
namespace SDLMixer
{
enum class EEventType
{
	Start,
	Stop,
	Pause,
	Resume,
	NumTypes,
};

class CEventConnection final : public CImplConnection
{
public:

	explicit CEventConnection(CID const id)
		: CImplConnection(id)
		, m_type(EEventType::Start)
		, m_volume(-14.0f)
		, m_fadeInTime(0.0f)
		, m_fadeOutTime(0.0f)
		, m_minAttenuation(0.0f)
		, m_maxAttenuation(100.0f)
		, m_isPanningEnabled(true)
		, m_isAttenuationEnabled(true)
		, m_isInfiniteLoop(false)
		, m_loopCount(1)
	{}

	virtual ~CEventConnection() override = default;

	CEventConnection() = delete;

	// CImplConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CImplConnection

	void       SetType(EEventType const type)                { m_type = type; }
	EEventType GetType() const                               { return m_type; }

	void       SetVolume(float const volume)                 { m_volume = volume; }
	float      GetVolume() const                             { return m_volume; }

	void       SetFadeInTime(float const fadeInTime)         { m_fadeInTime = fadeInTime; }
	float      GetFadeInTime() const                         { return m_fadeInTime; }
	void       SetFadeOutTime(float const fadeInTime)        { m_fadeOutTime = fadeInTime; }
	float      GetFadeOutTime() const                        { return m_fadeOutTime; }

	void       SetMinAttenuation(float const minAttenuation) { m_minAttenuation = minAttenuation; }
	float      GetMinAttenuation() const                     { return m_minAttenuation; }
	void       SetMaxAttenuation(float const maxAttenuation) { m_maxAttenuation = maxAttenuation; }
	float      GetMaxAttenuation() const                     { return m_maxAttenuation; }

	void       SetPanningEnabled(bool const isEnabled)       { m_isPanningEnabled = isEnabled; }
	bool       IsPanningEnabled() const                      { return m_isPanningEnabled; }

	void       SetAttenuationEnabled(bool const isEnabled)   { m_isAttenuationEnabled = isEnabled; }
	bool       IsAttenuationEnabled() const                  { return m_isAttenuationEnabled; }

	void       SetInfiniteLoop(bool const isInfinite)        { m_isInfiniteLoop = isInfinite; }
	bool       IsInfiniteLoop() const                        { return m_isInfiniteLoop; }

	void       SetLoopCount(uint32 const loopCount)          { m_loopCount = loopCount; }
	uint32     GetLoopCount() const                          { return m_loopCount; }

private:

	EEventType m_type;
	float      m_volume;
	float      m_fadeInTime;
	float      m_fadeOutTime;
	float      m_minAttenuation;
	float      m_maxAttenuation;
	bool       m_isPanningEnabled;
	bool       m_isAttenuationEnabled;
	bool       m_isInfiniteLoop;
	uint32     m_loopCount;
};

class CParameterConnection final : public CImplConnection
{
public:

	explicit CParameterConnection(CID const id)
		: CImplConnection(id)
		, m_mult(1.0f)
		, m_shift(0.0f)
	{}

	virtual ~CParameterConnection() override = default;

	CParameterConnection() = delete;

	// CImplConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CImplConnection

	void  SetMultiplier(float const mult) { m_mult = mult; }
	float GetMultiplier() const           { return m_mult; }
	void  SetShift(float const shift)     { m_shift = shift; }
	float GetShift() const                { return m_shift; }

private:

	float m_mult;
	float m_shift;
};

class CStateConnection final : public CImplConnection
{
public:

	explicit CStateConnection(CID const id)
		: CImplConnection(id)
		, m_value(0.0f)
	{}

	virtual ~CStateConnection() override = default;

	CStateConnection() = delete;

	// CImplConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CImplConnection

	void  SetValue(float const value) { m_value = value; }
	float GetValue() const            { return m_value; }

private:

	float m_value;
};
} // namespace SDLMixer
} // namespace ACE
