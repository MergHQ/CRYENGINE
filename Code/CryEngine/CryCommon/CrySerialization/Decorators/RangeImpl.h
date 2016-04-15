// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace Serialization
{

template<class T>
bool Serialize(IArchive& ar, RangeDecorator<T>& value, const char* name, const char* label)
{
	if (ar.isEdit())
	{
		if (!ar(SStruct::forEdit(value), name, label))
			return false;
	}
	else if (!ar(*value.value, name, label))
		return false;

	if (ar.isInput())
	{
		if (*value.value < value.hardMin)
			*value.value = value.hardMin;
		if (*value.value > value.hardMax)
			*value.value = value.hardMax;
	}
	return true;
}

}
