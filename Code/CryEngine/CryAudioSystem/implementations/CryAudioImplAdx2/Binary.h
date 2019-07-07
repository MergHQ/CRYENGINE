// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IFile.h>
#include <PoolObject.h>

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
class CBinary final : public IFile, public CPoolObject<CBinary, stl::PSyncNone>
{
public:

	CBinary(CBinary const&) = delete;
	CBinary(CBinary&&) = delete;
	CBinary& operator=(CBinary const&) = delete;
	CBinary& operator=(CBinary&&) = delete;

	CBinary() = default;
	virtual ~CBinary() override = default;

	CriAtomExAcbHn pAcb = nullptr;
};
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
