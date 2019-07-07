// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <IFile.h>
#include <PoolObject.h>
#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
namespace Fmod
{
class CBank final : public IFile, public CPoolObject<CBank, stl::PSyncNone>
{
public:

	CBank(CBank const&) = delete;
	CBank(CBank&&) = delete;
	CBank& operator=(CBank const&) = delete;
	CBank& operator=(CBank&&) = delete;

	CBank() = default;
	virtual ~CBank() override = default;

	FMOD::Studio::Bank*                pBank = nullptr;
	FMOD::Studio::Bank*                pStreamsBank = nullptr;

	CryFixedStringT<MaxFilePathLength> m_streamsBankPath;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
