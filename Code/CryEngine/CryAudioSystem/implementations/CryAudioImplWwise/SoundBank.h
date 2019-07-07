// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IFile.h>
#include <PoolObject.h>
#include <AK/SoundEngine/Common/AkTypes.h>

namespace CryAudio
{
namespace Impl
{
namespace Wwise
{
class CSoundBank final : public IFile, public CPoolObject<CSoundBank, stl::PSyncNone>
{
public:

	CSoundBank(CSoundBank const&) = delete;
	CSoundBank(CSoundBank&&) = delete;
	CSoundBank& operator=(CSoundBank const&) = delete;
	CSoundBank& operator=(CSoundBank&&) = delete;

	CSoundBank() = default;
	virtual ~CSoundBank() override = default;

	AkBankID bankId = AK_INVALID_BANK_ID;
};
} // namespace Wwise
} // namespace Impl
} // namespace CryAudio
