// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "ObjectProperties.h"

#include <CrySerialization/BlackBox.h>
#include <CrySerialization/IArchiveHost.h>
#include <CrySerialization/STL.h>
#include <CrySerialization/Decorators/ActionButton.h>
#include <Schematyc/Utils/Any.h>

namespace Schematyc
{
CObjectProperties::SProperty::SProperty()
	: overridePolicy(EOverridePolicy::Default)
{}

CObjectProperties::SProperty::SProperty(const char* _szName, const CAnyValuePtr& _pValue, EOverridePolicy _overridePolicy)
	: name(_szName)
	, pValue(_pValue)
	, overridePolicy(_overridePolicy)
{}

CObjectProperties::SProperty::SProperty(const SProperty& rhs)
	: name(rhs.name)
	, pValue(CAnyValue::CloneShared(*rhs.pValue))
	, overridePolicy(rhs.overridePolicy)
{}

void CObjectProperties::SProperty::Serialize(Serialization::IArchive& archive)
{
	if (archive.isEdit())
	{
		if (overridePolicy == EOverridePolicy::Default)
		{
			archive(Serialization::ActionButton(functor(*this, &CObjectProperties::SProperty::Edit)), "edit", "^Edit");
		}
		else
		{
			archive(Serialization::ActionButton(functor(*this, &CObjectProperties::SProperty::Revert)), "revert", "^Revert");
			archive(*pValue, "value", "^Value");
		}
	}
	else
	{
		archive(*pValue, "value");
		archive(overridePolicy, "overridePolicy");
	}
}

void CObjectProperties::SProperty::Edit()
{
	overridePolicy = EOverridePolicy::Override;
}

void CObjectProperties::SProperty::Revert()
{
	overridePolicy = EOverridePolicy::Default;
}

CObjectProperties::CObjectProperties() {}

CObjectProperties::CObjectProperties(const CObjectProperties& rhs)
	: m_properties(rhs.m_properties)
{}

IObjectPropertiesPtr CObjectProperties::Clone() const
{
	return std::make_shared<CObjectProperties>(*this);
}

void CObjectProperties::Serialize(Serialization::IArchive& archive)
{
	if (archive.isEdit())
	{
		PropertiesByName propertiesByName;
		for (Properties::value_type& property : m_properties)
		{
			propertiesByName.insert(PropertiesByName::value_type(property.second.name.c_str(), property.second));
		}
		for (PropertiesByName::value_type& property : propertiesByName)
		{
			archive(property.second, property.first, property.first);
		}
	}
	else if (archive.isInput() && !archive.caps(Serialization::IArchive::BINARY))  // Check for binary archive because binary archives do not currently support black box serialization.
	{
		typedef std::map<SGUID, Serialization::SBlackBox> BlackBoxProperties;

		BlackBoxProperties blackBoxProperties;
		archive(blackBoxProperties, "properties");
		for (BlackBoxProperties::value_type& blackBoxProperty : blackBoxProperties)
		{
			Properties::iterator itProperty = m_properties.find(blackBoxProperty.first);
			if (itProperty != m_properties.end())
			{
				Serialization::LoadBlackBox(itProperty->second, blackBoxProperty.second);
			}
		}
	}
	else
	{
		archive(m_properties, "properties");
	}
}

bool CObjectProperties::Read(const CAnyRef& value, const SGUID& guid) const
{
	Properties::const_iterator itProperty = m_properties.find(guid);
	if ((itProperty != m_properties.end()))
	{
		if (itProperty->second.overridePolicy == EOverridePolicy::Override)
		{
			return Any::CopyAssign(value, *itProperty->second.pValue);
		}
		return true;
	}
	return false;
}

void CObjectProperties::Add(const SGUID& guid, const char* szName, const CAnyConstRef& value)
{
	m_properties.insert(Properties::value_type(guid, SProperty(szName, CAnyValue::CloneShared(value), EOverridePolicy::Default)));
}
} // Schematyc
