// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "CategoryState.h"

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	#include "Common.h"
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CCategoryState::Set(IObject* const pIObject)
{
	criAtomExCategory_SetVolumeByName(static_cast<CriChar8 const*>(m_name), m_value);

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	g_categoryValues[m_name] = m_value;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CCategoryState::SetGlobally()
{
	Set(nullptr);
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
