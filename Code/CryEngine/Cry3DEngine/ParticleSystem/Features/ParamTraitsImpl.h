// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

namespace pfx2
{

ILINE bool Serialize(Serialization::IArchive& ar, SEnable& val, cstr name, cstr label)
{
	name = (name && *name != 0) ? name : "Enabled";
	if (!ar.isEdit())
	{
		if (ar.isInput() || (ar.isOutput() && val.m_value == false))
		{
			if (!ar(val.m_value, name, label))
			{
				if (ar.isInput())
					val.m_value = true;
				else
					return false;
			}
		}
	}
	else
	{
		return ar(val.m_value, name, name);
	}
	return true;
}

template<typename TTraits>
bool TValue<TTraits>::Serialize(Serialization::IArchive& ar, cstr name, cstr label)
{
	T v = TTraits::From(m_value);
	if (TTraits::HideDefault() && ar.isEdit())
	{
		// Create a toggle in editor to override default
		struct EnabledValue
		{
			T& m_value;

			void Serialize(Serialization::IArchive& ar)
			{
				bool enabled = m_value != TTraits::Default();
				ar(enabled, "enabled", "^");
				if (enabled)
				{
					ar(Range(m_value), "value", "^");
					if (m_value == TTraits::Default())
						m_value = TTraits::NonDefault();
				}
				else
				{
					m_value = TTraits::Default();
					string display = TTraits::DefaultName();
					ar(display, "value", "!^");
				}
			}
		};

		if (!ar(EnabledValue {v}, name, label))
			return false;
	}
	else
	{
		if (!ar(Range(v), name, label))
			return false;
	}
	if (ar.isInput())
		Set(v);
	return true;
}

template<>
inline bool TValue<TColor>::Serialize(Serialization::IArchive& ar, cstr name, cstr label)
{
	ColorB color(m_value.r, m_value.g, m_value.b, m_value.a);
	bool b = ar(color, name, label);
	if (b && ar.isInput())
		m_value.dcolor = color.pack_argb8888();
	return b;
}


}
