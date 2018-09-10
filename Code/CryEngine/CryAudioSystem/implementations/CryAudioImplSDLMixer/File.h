// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common.h"
#include <ATLEntityData.h>

namespace CryAudio
{
namespace Impl
{
namespace SDL_mixer
{
class CFile final : public IFile
{
public:

	CFile(CFile const&) = delete;
	CFile(CFile&&) = delete;
	CFile& operator=(CFile const&) = delete;
	CFile& operator=(CFile&&) = delete;

	CFile() = default;
	virtual ~CFile() override = default;

	SampleId sampleId;
};
} // namespace SDL_mixer
} // namespace Impl
} // namespace CryAudio