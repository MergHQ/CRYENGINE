// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ParameterInfo.h"
#include <IEnvironmentConnection.h>
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CParameterEnvironmentAdvanced final : public IEnvironmentConnection, public CPoolObject<CParameterEnvironmentAdvanced, stl::PSyncNone>
{
public:

	CParameterEnvironmentAdvanced() = delete;
	CParameterEnvironmentAdvanced(CParameterEnvironmentAdvanced const&) = delete;
	CParameterEnvironmentAdvanced(CParameterEnvironmentAdvanced&&) = delete;
	CParameterEnvironmentAdvanced& operator=(CParameterEnvironmentAdvanced const&) = delete;
	CParameterEnvironmentAdvanced& operator=(CParameterEnvironmentAdvanced&&) = delete;

	explicit CParameterEnvironmentAdvanced(
		CParameterInfo const& parameterInfo,
		float const multiplier,
		float const shift)
		: m_parameterInfo(parameterInfo)
		, m_multiplier(multiplier)
		, m_shift(shift)
	{}

	virtual ~CParameterEnvironmentAdvanced() override;

	// IEnvironmentConnection
	virtual void Set(IObject* const pIObject, float const amount) override;
	// ~IEnvironmentConnection

private:

	CParameterInfo m_parameterInfo;
	float const    m_multiplier;
	float const    m_shift;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
