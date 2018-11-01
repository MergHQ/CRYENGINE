// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/PoolObject.h"
#include <CryString/HashedString.h>

namespace CryAudio
{
namespace Impl
{
struct IStandaloneFile;
} // namespace Impl

enum class EStandaloneFileState : EnumFlagsType
{
	None,
	Playing,
	Stopping,
	Loading,
};

class CStandaloneFile final : public CPoolObject<CStandaloneFile, stl::PSyncNone>
{
public:

	explicit CStandaloneFile() = default;

	bool IsPlaying() const { return (m_state == EStandaloneFileState::Playing) || (m_state == EStandaloneFileState::Stopping); }

	CObject*               m_pObject = nullptr;
	Impl::IStandaloneFile* m_pImplData = nullptr;
	EStandaloneFileState   m_state = EStandaloneFileState::None;
	CHashedString          m_hashedFilename;

	// Needed only during middleware switch.
#if defined(INCLUDE_AUDIO_PRODUCTION_CODE)
	ControlId m_triggerId = InvalidControlId;
	bool      m_isLocalized = true;
#endif // INCLUDE_AUDIO_PRODUCTION_CODE
};
} // namespace CryAudio
