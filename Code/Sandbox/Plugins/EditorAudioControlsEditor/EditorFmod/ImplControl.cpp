// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "StdAfx.h"
#include "ImplControl.h"

namespace ACE
{
namespace Fmod
{
//////////////////////////////////////////////////////////////////////////
CImplControl::CImplControl(string const& name, CID const id, ItemType const type)
	: CImplItem(name, id, type)
{
}
} // namespace Fmod
} //namespace ACE
