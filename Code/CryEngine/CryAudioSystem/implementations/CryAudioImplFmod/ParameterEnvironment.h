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
class CParameterEnvironment final : public IEnvironmentConnection, public CPoolObject<CParameterEnvironment, stl::PSyncNone>
{
public:

	CParameterEnvironment() = delete;
	CParameterEnvironment(CParameterEnvironment const&) = delete;
	CParameterEnvironment(CParameterEnvironment&&) = delete;
	CParameterEnvironment& operator=(CParameterEnvironment const&) = delete;
	CParameterEnvironment& operator=(CParameterEnvironment&&) = delete;

	explicit CParameterEnvironment(CParameterInfo const& parameterInfo)
		: m_parameterInfo(parameterInfo)
	{}

	virtual ~CParameterEnvironment() override;

	// IEnvironmentConnection
	virtual void Set(IObject* const pIObject, float const amount) override;
	// ~IEnvironmentConnection

private:

	CParameterInfo m_parameterInfo;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
