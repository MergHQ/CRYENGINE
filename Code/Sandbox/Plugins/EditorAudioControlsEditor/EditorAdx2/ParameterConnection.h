// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "../Common/IConnection.h"

#include <PoolObject.h>
#include <CryAudioImplAdx2/GlobalData.h>

namespace ACE
{
namespace Impl
{
namespace Adx2
{
class CParameterConnection final : public IConnection, public CryAudio::CPoolObject<CParameterConnection, stl::PSyncNone>
{
public:

	CParameterConnection() = delete;
	CParameterConnection(CParameterConnection const&) = delete;
	CParameterConnection(CParameterConnection&&) = delete;
	CParameterConnection& operator=(CParameterConnection const&) = delete;
	CParameterConnection& operator=(CParameterConnection&&) = delete;

	explicit CParameterConnection(ControlId const id, EAssetType const type)
		: m_id(id)
		, m_isAdvanced(false)
		, m_type(type)
		, m_minValue(CryAudio::Impl::Adx2::g_defaultParamMinValue)
		, m_maxValue(CryAudio::Impl::Adx2::g_defaultParamMaxValue)
		, m_mult(CryAudio::Impl::Adx2::g_defaultParamMultiplier)
		, m_shift(CryAudio::Impl::Adx2::g_defaultParamShift)
	{}

	explicit CParameterConnection(
		ControlId const id,
		bool const isAdvanced,
		EAssetType const type,
		float const minValue,
		float const maxValue,
		float const mult,
		float const shift)
		: m_id(id)
		, m_isAdvanced(isAdvanced)
		, m_type(type)
		, m_minValue(minValue)
		, m_maxValue(maxValue)
		, m_mult(mult)
		, m_shift(shift)
	{}

	explicit CParameterConnection(
		ControlId const id,
		bool const isAdvanced,
		EAssetType const type,
		float const mult,
		float const shift)
		: m_id(id)
		, m_isAdvanced(isAdvanced)
		, m_type(type)
		, m_minValue(CryAudio::Impl::Adx2::g_defaultParamMinValue)
		, m_maxValue(CryAudio::Impl::Adx2::g_defaultParamMaxValue)
		, m_mult(mult)
		, m_shift(shift)
	{}

	virtual ~CParameterConnection() override = default;

	// CBaseConnection
	virtual ControlId GetID() const override         { return m_id; }
	virtual bool      HasProperties() const override { return true; }
	virtual void      Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	bool       IsAdvanced() const;

	EAssetType GetType() const       { return m_type; }

	float      GetMinValue() const   { return m_minValue; }
	float      GetMaxValue() const   { return m_maxValue; }
	float      GetMultiplier() const { return m_mult; }
	float      GetShift() const      { return m_shift; }

private:

	void Reset();

	ControlId const  m_id;
	bool             m_isAdvanced;
	EAssetType const m_type;
	float            m_minValue;
	float            m_maxValue;
	float            m_mult;
	float            m_shift;
};
} // namespace Adx2
} // namespace Impl
} // namespace ACE
