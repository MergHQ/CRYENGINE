// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IParameterConnection.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
class CParameter final : public IParameterConnection
{
public:

	CParameter(CParameter const&) = delete;
	CParameter(CParameter&&) = delete;
	CParameter& operator=(CParameter const&) = delete;
	CParameter& operator=(CParameter&&) = delete;

	CParameter() = default;
	virtual ~CParameter() override = default;

	// IParameterConnection
	virtual void Set(IObject* const pIObject, float const value) override {}
	virtual void SetGlobally(float const value) override                  {}
	// ~IParameterConnection
};
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
