// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CStandaloneFile final : public IStandaloneFile
{
public:

	CStandaloneFile(CStandaloneFile const&) = delete;
	CStandaloneFile(CStandaloneFile&&) = delete;
	CStandaloneFile& operator=(CStandaloneFile const&) = delete;
	CStandaloneFile& operator=(CStandaloneFile&&) = delete;

	CStandaloneFile() = default;
	virtual ~CStandaloneFile() override = default;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
