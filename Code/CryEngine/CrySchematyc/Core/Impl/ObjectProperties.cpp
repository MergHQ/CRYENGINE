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
CObjectProperties::SComponent::SComponent() {}

CObjectProperties::SComponent::SComponent(const char* _szName, const IPropertiesPtr& _pProperties, EOverridePolicy _overridePolicy)
	: name(_szName)
	, pProperties(_pProperties)
	, overridePolicy(_overridePolicy)
{}

CObjectProperties::SComponent::SComponent(const SComponent& rhs)
	: name(rhs.name)
	, pProperties(rhs.pProperties->Clone())
	, overridePolicy(rhs.overridePolicy)
{}

void CObjectProperties::SComponent::Serialize(Serialization::IArchive& archive)
{
	if (archive.isEdit())
	{
		if (overridePolicy == EOverridePolicy::Default)
		{
			archive(Serialization::ActionButton(functor(*this, &CObjectProperties::SComponent::Edit)), "edit", "^Edit");
		}
		else
		{
			archive(Serialization::ActionButton(functor(*this, &CObjectProperties::SComponent::Revert)), "revert", "^Revert");
			pProperties->Serialize(archive);
		}
	}
	else
	{
		archive(*pProperties, "properties");
		archive(overridePolicy, "overridePolicy");
	}
}

void CObjectProperties::SComponent::Edit()
{
	overridePolicy = EOverridePolicy::Override;
}

void CObjectProperties::SComponent::Revert()
{
	overridePolicy = EOverridePolicy::Default;
}

CObjectProperties::SVariable::SVariable() {}

CObjectProperties::SVariable::SVariable(const char* _szName, const CAnyValuePtr& _pValue, EOverridePolicy _overridePolicy)
	: name(_szName)
	, pValue(_pValue)
	, overridePolicy(_overridePolicy)
{}

CObjectProperties::SVariable::SVariable(const SVariable& rhs)
	: name(rhs.name)
	, pValue(CAnyValue::CloneShared(*rhs.pValue))
	, overridePolicy(rhs.overridePolicy)
{}

void CObjectProperties::SVariable::Serialize(Serialization::IArchive& archive)
{
	if (archive.isEdit())
	{
		if (overridePolicy == EOverridePolicy::Default)
		{
			archive(Serialization::ActionButton(functor(*this, &CObjectProperties::SVariable::Edit)), "edit", "^Edit");
		}
		else
		{
			archive(Serialization::ActionButton(functor(*this, &CObjectProperties::SVariable::Revert)), "revert", "^Revert");
			archive(*pValue, "value", "^Value");
		}
	}
	else
	{
		archive(*pValue, "value");
		archive(overridePolicy, "overridePolicy");
	}
}

void CObjectProperties::SVariable::Edit()
{
	overridePolicy = EOverridePolicy::Override;
}

void CObjectProperties::SVariable::Revert()
{
	overridePolicy = EOverridePolicy::Default;
}

CObjectProperties::CObjectProperties() {}

CObjectProperties::CObjectProperties(const CObjectProperties& rhs)
	: m_components(rhs.m_components)
	, m_variables(rhs.m_variables)
{}

IObjectPropertiesPtr CObjectProperties::Clone() const
{
	return std::make_shared<CObjectProperties>(*this);
}

void CObjectProperties::Serialize(Serialization::IArchive& archive)
{
	typedef std::map<SGUID, Serialization::SBlackBox> BlackBoxComponents;
	typedef std::map<SGUID, Serialization::SBlackBox> BlackBoxVariables;

	if (archive.isEdit())
	{
		if (!m_components.empty())
		{
			archive.openBlock("components", "Components");

			ComponentsByName componentsByName;
			for (Components::value_type& component : m_components)
			{
				componentsByName.insert(ComponentsByName::value_type(component.second.name.c_str(), component.second));
			}
			for (ComponentsByName::value_type& component : componentsByName)
			{
				archive(component.second, component.first, component.first);
			}

			archive.closeBlock();
		}

		if (!m_variables.empty())
		{
			archive.openBlock("variables", "Variables");

			VariablesByName variablesByName;
			for (Variables::value_type& variable : m_variables)
			{
				variablesByName.insert(VariablesByName::value_type(variable.second.name.c_str(), variable.second));
			}
			for (VariablesByName::value_type& variable : variablesByName)
			{
				archive(variable.second, variable.first, variable.first);
			}

			archive.closeBlock();
		}
	}
	else if (archive.isInput() && !archive.caps(Serialization::IArchive::BINARY))  // Check for binary archive because binary archives do not currently support black box serialization.
	{
		BlackBoxComponents blackBoxComponents;
		archive(blackBoxComponents, "components");
		for (BlackBoxComponents::value_type& blackBoxComponent : blackBoxComponents)
		{
			Components::iterator itComponent = m_components.find(blackBoxComponent.first);
			if (itComponent != m_components.end())
			{
				Serialization::LoadBlackBox(itComponent->second, blackBoxComponent.second);
			}
		}

		BlackBoxVariables blackBoxVariables;
		archive(blackBoxVariables, "variables");
		for (BlackBoxVariables::value_type& blackBoxVariable : blackBoxVariables)
		{
			Variables::iterator itVariable = m_variables.find(blackBoxVariable.first);
			if (itVariable != m_variables.end())
			{
				Serialization::LoadBlackBox(itVariable->second, blackBoxVariable.second);
			}
		}
	}
	else
	{
		archive(m_components, "components");
		archive(m_variables, "variables");
	}
}

const IProperties* CObjectProperties::GetComponentProperties(const SGUID& guid) const
{
	Components::const_iterator itComponent = m_components.find(guid);
	return itComponent != m_components.end() ? itComponent->second.pProperties.get() : nullptr;
}

bool CObjectProperties::ReadVariable(const CAnyRef& value, const SGUID& guid) const
{
	Variables::const_iterator itVariable = m_variables.find(guid);
	if ((itVariable != m_variables.end()))
	{
		if (itVariable->second.overridePolicy == EOverridePolicy::Override)
		{
			return Any::CopyAssign(value, *itVariable->second.pValue);
		}
		return true;
	}
	return false;
}

void CObjectProperties::AddComponent(const SGUID& guid, const char* szName, const IProperties& properties)
{
	m_components.insert(Components::value_type(guid, SComponent(szName, properties.Clone(), EOverridePolicy::Default)));
}

void CObjectProperties::AddVariable(const SGUID& guid, const char* szName, const CAnyConstRef& value)
{
	m_variables.insert(Variables::value_type(guid, SVariable(szName, CAnyValue::CloneShared(value), EOverridePolicy::Default)));
}
} // Schematyc
