// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ISetting.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CSetting final : public ISetting
{
public:

	CSetting(CSetting const&) = delete;
	CSetting(CSetting&&) = delete;
	CSetting& operator=(CSetting const&) = delete;
	CSetting& operator=(CSetting&&) = delete;

	CSetting() = default;
	virtual ~CSetting() override = default;

	// ISetting
	virtual void Load() const override   {}
	virtual void Unload() const override {}
	// ~ISetting
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
