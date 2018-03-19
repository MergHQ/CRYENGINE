// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IConnection.h>
#include <CryAudioImplWwise/GlobalData.h>

namespace ACE
{
namespace Wwise
{
class CBaseConnection : public IConnection
{
public:

	explicit CBaseConnection(ControlId const id)
		: m_id(id)
	{}

	virtual ~CBaseConnection() override = default;

	CBaseConnection() = delete;

	// IImplConnection
	virtual ControlId GetID() const override final                                                             { return m_id; }
	virtual bool      HasProperties() const override                                                           { return false; }
	virtual void      Serialize(Serialization::IArchive& ar) override                                          {}
	virtual void      SetPlatformEnabled(PlatformIndexType const platformIndex, bool const isEnabled) override {}
	virtual bool      IsPlatformEnabled(PlatformIndexType const platformIndex) const override                  { return true; }
	virtual void      ClearPlatforms() override                                                                {}
	// ~IImplConnection

private:

	ControlId const m_id;
};

class CParameterConnection final : public CBaseConnection
{
public:

	explicit CParameterConnection(ControlId const id, float const mult = CryAudio::Impl::Wwise::s_defaultParamMultiplier, float const shift = CryAudio::Impl::Wwise::s_defaultParamShift)
		: CBaseConnection(id)
		, m_mult(mult)
		, m_shift(shift)
	{}

	CParameterConnection() = delete;

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	float GetMultiplier() const { return m_mult; }
	float GetShift() const      { return m_shift; }

private:

	float m_mult;
	float m_shift;
};

class CParameterToStateConnection final : public CBaseConnection
{
public:

	explicit CParameterToStateConnection(ControlId const id, float const value = CryAudio::Impl::Wwise::s_defaultStateValue)
		: CBaseConnection(id)
		, m_value(value)
	{}

	CParameterToStateConnection() = delete;

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	float GetValue() const { return m_value; }

private:

	float m_value;
};

class CSoundBankConnection final : public CBaseConnection
{
public:

	explicit CSoundBankConnection(ControlId const id)
		: CBaseConnection(id)
		, m_configurationsMask(std::numeric_limits<PlatformIndexType>::max())
	{}

	CSoundBankConnection() = delete;

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	virtual void SetPlatformEnabled(PlatformIndexType const platformIndex, bool const isEnabled) override;
	virtual bool IsPlatformEnabled(PlatformIndexType const platformIndex) const override;
	virtual void ClearPlatforms() override;
	// ~CBaseConnection

private:

	PlatformIndexType m_configurationsMask;
};
} // namespace Wwise
} // namespace ACE

