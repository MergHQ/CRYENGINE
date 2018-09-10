// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
class CParameter final : public IParameter
{
public:

	CParameter(CParameter const&) = delete;
	CParameter(CParameter&&) = delete;
	CParameter& operator=(CParameter const&) = delete;
	CParameter& operator=(CParameter&&) = delete;

	CParameter() = default;
	virtual ~CParameter() override = default;
};
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
