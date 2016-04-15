// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Color.h"
#include <CryMath/Cry_Color.h>

//////////////////////////////////////////////////////////////////////////
template<typename T>
struct SerializableColor_tpl : Color_tpl<T>
{
	void Serialize(Serialization::IArchive& ar) {}
};

template<typename T>
bool Serialize(Serialization::IArchive& ar, Color_tpl<T>& c, const char* name, const char* label)
{
	if (ar.isEdit())
		return Serialize(ar, static_cast<SerializableColor_tpl<T>&>(c), name, label);
	else
	{
		typedef T (& Array)[4];
		return ar((Array)c, name, label);
	}
}
