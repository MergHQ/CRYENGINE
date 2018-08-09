// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Listener.h"

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
//////////////////////////////////////////////////////////////////////////
CListener::CListener(uint16 const id, CriAtomEx3dListenerHn const pHandle)
	: m_id(id)
	, m_pHandle(pHandle)
{
	ZeroStruct(m_transformation);
}

//////////////////////////////////////////////////////////////////////////
void CListener::SetTransformation(CObjectTransformation const& transformation)
{
	TranslateTransformation(transformation, m_transformation);

	criAtomEx3dListener_SetPosition(m_pHandle, &m_transformation.pos);
	criAtomEx3dListener_SetOrientation(m_pHandle, &m_transformation.fwd, &m_transformation.up);
	criAtomEx3dListener_Update(m_pHandle);
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
