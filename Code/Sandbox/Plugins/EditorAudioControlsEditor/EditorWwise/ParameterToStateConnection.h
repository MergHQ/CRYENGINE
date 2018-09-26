// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BaseConnection.h"

#include <PoolObject.h>
#include <CryAudioImplWwise/GlobalData.h>

namespace ACE
{
namespace Impl
{
namespace Wwise
{
class CParameterToStateConnection final : public CBaseConnection, public CryAudio::CPoolObject<CParameterToStateConnection, stl::PSyncNone>
{
public:

	CParameterToStateConnection() = delete;
	CParameterToStateConnection(CParameterToStateConnection const&) = delete;
	CParameterToStateConnection(CParameterToStateConnection&&) = delete;
	CParameterToStateConnection& operator=(CParameterToStateConnection const&) = delete;
	CParameterToStateConnection& operator=(CParameterToStateConnection&&) = delete;

	explicit CParameterToStateConnection(ControlId const id, float const value = CryAudio::Impl::Wwise::s_defaultStateValue)
		: CBaseConnection(id)
		, m_value(value)
	{}

	virtual ~CParameterToStateConnection() override = default;

	// CBaseConnection
	virtual bool HasProperties() const override { return true; }
	virtual void Serialize(Serialization::IArchive& ar) override;
	// ~CBaseConnection

	float GetValue() const { return m_value; }

private:

	float m_value;
};
} // namespace Wwise
} // namespace Impl
} // namespace ACE
