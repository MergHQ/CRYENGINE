// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ParameterInfo.h"
#include <IParameterConnection.h>
#include <PoolObject.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CParameterAdvanced final : public IParameterConnection, public CPoolObject<CParameterAdvanced, stl::PSyncNone>
{
public:

	CParameterAdvanced() = delete;
	CParameterAdvanced(CParameterAdvanced const&) = delete;
	CParameterAdvanced(CParameterAdvanced&&) = delete;
	CParameterAdvanced& operator=(CParameterAdvanced const&) = delete;
	CParameterAdvanced& operator=(CParameterAdvanced&&) = delete;

	explicit CParameterAdvanced(
		CParameterInfo const& parameterInfo,
		float const multiplier,
		float const shift)
		: m_parameterInfo(parameterInfo)
		, m_multiplier(multiplier)
		, m_shift(shift)
	{}

	virtual ~CParameterAdvanced() override;

	// IParameterConnection
	virtual void Set(IObject* const pIObject, float const value) override;
	virtual void SetGlobally(float const value) override;
	// ~IParameterConnection

private:

	CParameterInfo m_parameterInfo;
	float const    m_multiplier;
	float const    m_shift;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
