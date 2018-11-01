// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

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
class CFile final : public IFile, public CPoolObject<CFile, stl::PSyncNone>
{
public:

	CFile(CFile const&) = delete;
	CFile(CFile&&) = delete;
	CFile& operator=(CFile const&) = delete;
	CFile& operator=(CFile&&) = delete;

	CFile() = default;
	virtual ~CFile() override = default;

	FMOD::Studio::Bank*                pBank = nullptr;
	FMOD::Studio::Bank*                pStreamsBank = nullptr;

	CryFixedStringT<MaxFilePathLength> m_streamsBankPath;
};
} // namespace Fmod
} // namespace Impl
} // namespace CryAudio
