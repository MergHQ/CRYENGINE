// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Setting.h"

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	#include "Debug.h"
#endif // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CSetting::Load() const
{
	criAtomEx_AttachDspBusSetting(static_cast<CriChar8 const*>(m_name), nullptr, 0);

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	g_debugCurrentDspBusSettingName = m_name;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSetting::Unload() const
{
	criAtomEx_DetachDspBusSetting();

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	g_debugCurrentDspBusSettingName = g_debugNoneDspBusSetting;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
