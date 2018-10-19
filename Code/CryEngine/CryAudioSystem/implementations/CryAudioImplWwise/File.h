// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <ATLEntityData.h>
#include <PoolObject.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
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

	AkBankID bankId = AK_INVALID_BANK_ID;
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
