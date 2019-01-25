// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "SelectorLabel.h"
#include "BaseObject.h"

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	#include <Logger.h>
#endif // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

#include <cri_atom_ex.h>

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
void CSelectorLabel::Set(IObject* const pIObject)
{
	if (pIObject != nullptr)
	{
		auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);

		CriAtomExPlayerHn const pPlayer = pBaseObject->GetPlayer();
		criAtomExPlayer_SetSelectorLabel(pPlayer, static_cast<CriChar8 const*>(m_selectorName), static_cast<CriChar8 const*>(m_selectorLabelName));
		criAtomExPlayer_UpdateAll(pPlayer);
	}
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "Adx2 - Invalid Object passed to the Adx2 implementation of %s", __FUNCTION__);
	}
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CSelectorLabel::SetGlobally()
{
	for (auto const pBaseObject : g_constructedObjects)
	{
		Set(pBaseObject);
	}
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
