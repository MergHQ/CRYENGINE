// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IFile.h>

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
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
};
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
