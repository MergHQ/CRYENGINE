// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
class CEnvironment final : public IEnvironment
{
public:

	CEnvironment(CEnvironment const&) = delete;
	CEnvironment(CEnvironment&&) = delete;
	CEnvironment& operator=(CEnvironment const&) = delete;
	CEnvironment& operator=(CEnvironment&&) = delete;

	CEnvironment() = default;
	virtual ~CEnvironment() override = default;
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio