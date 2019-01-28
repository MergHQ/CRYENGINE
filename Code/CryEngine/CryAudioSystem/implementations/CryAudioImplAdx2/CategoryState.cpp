// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "CategoryState.h"

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	#include "BaseObject.h"
	#include <Logger.h>
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CCategoryState::Set(IObject* const pIObject)
{
	SetGlobally();

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);
	Cry::Audio::Log(ELogType::Warning, R"(Adx2 - Category "%s" was set to %f on object "%s". Consider setting it globally.)",
	                m_name.c_str(), static_cast<float>(m_value), pBaseObject->GetName());
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CCategoryState::SetGlobally()
{
	criAtomExCategory_SetVolumeByName(static_cast<CriChar8 const*>(m_name.c_str()), m_value);

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	g_categoryValues[m_name] = m_value;
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
