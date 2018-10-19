// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>
#include <PoolObject.h>

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CFile final : public IFile, public CPoolObject<CFile, stl::PSyncNone>
{
public:

	CFile(CFile const&) = delete;
	CFile(CFile&&) = delete;
	CFile& operator=(CFile const&) = delete;
	CFile& operator=(CFile&&) = delete;

	CFile() = default;
	virtual ~CFile() override = default;

	CriAtomExAcbHn pAcb = nullptr;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
