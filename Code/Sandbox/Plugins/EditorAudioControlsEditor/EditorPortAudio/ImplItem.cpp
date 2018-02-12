// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ImplItem.h"

namespace ACE
{
namespace PortAudio
{
//////////////////////////////////////////////////////////////////////////
void CImplItem::SetConnected(bool const isConnected)
{
	if (isConnected)
	{
		m_flags |= EImplItemFlags::IsConnected;
	}
	else
	{
		m_flags &= ~EImplItemFlags::IsConnected;
	}
}

//////////////////////////////////////////////////////////////////////////
void CImplItem::AddChild(CImplItem* const pChild)
{
	m_children.push_back(pChild);
	pChild->SetParent(this);
}

//////////////////////////////////////////////////////////////////////////
void CImplItem::Clear()
{
	m_children.clear();
}
} // namespace PortAudio
} // namespace ACE
