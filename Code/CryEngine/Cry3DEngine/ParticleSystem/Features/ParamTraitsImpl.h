// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  Created:     04/03/2015 by Filipe amim
//  Description:
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#ifndef PARAMTRAITSIMPL_H
#define PARAMTRAITSIMPL_H

#pragma once

namespace pfx2
{

ILINE bool Serialize(Serialization::IArchive& ar, SEnable& val, const char* name, const char* label)
{
	if (ar.isEdit() || ar.isInput() || (ar.isOutput() && val.m_value == false))
	{
		name = (name && *name != 0) ? name : "Enabled";
		label = (label && *label != 0) ? label : "^^";
		if (!ar(val.m_value, name, label))
		{
			if (ar.isInput())
				val.m_value = true;
			else
				return false;
		}
	}
	return true;
}

template<typename T, typename D>
ILINE bool IsDefault(const T& value, D defaultValue)
{
	return (value.Min() == value.Max()) && (value.Max() == defaultValue);
}

}

#endif // PARAMTRAITSIMPL_H
