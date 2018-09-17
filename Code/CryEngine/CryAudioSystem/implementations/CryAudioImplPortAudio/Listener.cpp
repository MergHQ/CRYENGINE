// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Listener.h"

namespace CryAudio
{
namespace Impl
{
namespace PortAudio
{
//////////////////////////////////////////////////////////////////////////
CListener::CListener(CObjectTransformation const& transformation)
	: m_transformation(transformation)
{
}

//////////////////////////////////////////////////////////////////////////
void CListener::SetName(char const* const szName)
{
#if defined(INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE)
	m_name = szName;
#endif  // INCLUDE_PORTAUDIO_IMPL_PRODUCTION_CODE
}
} // namespace PortAudio
} // namespace Impl
} // namespace CryAudio
