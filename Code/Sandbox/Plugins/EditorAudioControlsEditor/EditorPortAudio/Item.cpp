// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Item.h"

namespace ACE
{
namespace PortAudio
{
//////////////////////////////////////////////////////////////////////////
void CItem::SetConnected(bool const isConnected)
{
	if (isConnected)
	{
		m_flags |= EItemFlags::IsConnected;
	}
	else
	{
		m_flags &= ~EItemFlags::IsConnected;
	}
}

//////////////////////////////////////////////////////////////////////////
void CItem::AddChild(CItem* const pChild)
{
	m_children.push_back(pChild);
	pChild->SetParent(this);
}

//////////////////////////////////////////////////////////////////////////
void CItem::Clear()
{
	m_children.clear();
}
} // namespace PortAudio
} // namespace ACE
