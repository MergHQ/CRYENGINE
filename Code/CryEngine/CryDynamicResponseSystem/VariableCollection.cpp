// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "VariableCollection.h"
#include <CryDynamicResponseSystem/IDynamicResponseCondition.h>
#include "ResponseSystem.h"
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>
#include "Variable.h"

using namespace CryDRS;

const CHashedString CVariableCollection::s_globalCollectionName = CHashedString("Global");
const CHashedString CVariableCollection::s_localCollectionName = CHashedString("Local");
const CHashedString CVariableCollection::s_contextCollectionName = CHashedString("Context");
const CVariableValue CVariableCollection::s_newVariableValue;  //the initial value for new/not existing variables is 0. remark: so checking an not existing variable to 0 will actually pass. but this is the desired behavior in most cases

CVariableCollection::CVariableCollection(const CHashedString& name) : m_name(name)
{
}

//--------------------------------------------------------------------------------------------------
CVariableCollection::~CVariableCollection()
{
	for (VariableList::iterator it = m_allResponseVariables.begin(); it != m_allResponseVariables.end(); ++it)
	{
		delete *it;
	}
}

//--------------------------------------------------------------------------------------------------
CVariable* CVariableCollection::CreateVariable(const CHashedString& name, const CVariableValue& initialValue)
{
#if defined ENABLE_VARIABLE_VALUE_TYPE_CHECKINGS
	CRY_ASSERT_MESSAGE(GetVariable(name) == 0, "Variable with this name already exists");
#endif

	//drs-todo optimize me, the variables should be more cache friendly allocated (but they need to stay in exactly the position were they were generated, because we are giving out native PTRs
	//we could store the names separately in a vector, so that we can iterate fast when searching by name)

	CVariable* newVariable = new CVariable(name, initialValue);
	m_allResponseVariables.push_back(newVariable);
	return newVariable;
}

//--------------------------------------------------------------------------------------------------
CVariable* CVariableCollection::CreateVariable(const CHashedString& name, int initialValue)
{
	return CreateVariable(name, CVariableValue(initialValue));
}

//--------------------------------------------------------------------------------------------------
CVariable* CVariableCollection::CreateVariable(const CHashedString& name, float initialValue)
{
	return CreateVariable(name, CVariableValue(initialValue));
}

//--------------------------------------------------------------------------------------------------
CVariable* CVariableCollection::CreateVariable(const CHashedString& name, const CHashedString& initialValue)
{
	return CreateVariable(name, CVariableValue(initialValue));
}

//--------------------------------------------------------------------------------------------------
CVariable* CVariableCollection::CreateVariable(const CHashedString& name, bool initialValue)
{
	return CreateVariable(name, CVariableValue(initialValue));
}

//--------------------------------------------------------------------------------------------------
CVariable* CVariableCollection::GetVariable(const CHashedString& name) const
{
	for (CVariable* variable : m_allResponseVariables)
	{
		if (variable->m_name == name)
		{
			return variable;
		}
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
bool CVariableCollection::SetVariableValue(const CHashedString& name, const CVariableValue& newValue, bool createIfNotExisting, float resetTime)
{
	for (CVariable* variable : m_allResponseVariables)
	{
		if (variable->m_name == name)
		{
#if defined(ENABLE_VARIABLE_VALUE_TYPE_CHECKINGS)
			if (!newValue.DoTypesMatch(variable->m_value))
			{
				DrsLogWarning((string("SetVariableValue: Type of variable \'" + string(name.GetText()) + "\' and type of newValue do not match! NewValueType: ") \
				  + newValue.GetTypeAsString() + " - variableType: " + variable->m_value.GetTypeAsString()).c_str());
			}
#endif
			if (resetTime <= 0.0f)
			{
				//check if there is already an cooldown for this variable, and if yes, remove it
				for (CoolingDownVariableList::iterator itCooling = m_coolingDownVariables.begin(); itCooling != m_coolingDownVariables.end(); ++itCooling)
				{
					if (itCooling->variable == variable)
					{
						m_coolingDownVariables.erase(itCooling);
						break;
					}
				}
			}
			return SetVariableValue(variable, newValue, resetTime);
		}
	}

	if (createIfNotExisting)
	{
		DRS_DEBUG_DATA_ACTION(AddVariableSet(name.GetText(), m_name.GetText(), CVariableValue(), newValue, CResponseSystem::GetInstance()->GetCurrentDrsTime()));
		if (resetTime > 0.0f)
		{
			CVariable* newVariable = CreateVariable(name, s_newVariableValue);
			return SetVariableValueForSomeTime(newVariable, newValue, resetTime);
		}
		else
		{
			return CreateVariable(name, newValue) != nullptr;
		}
	}
	else
	{
		DrsLogWarning((string("SetVariable Value failed, because there was no variable named ") + name.GetText() + "in this collection " + m_name.GetText()).c_str());
	}

	return false;
}

//--------------------------------------------------------------------------------------------------
bool CVariableCollection::SetVariableValue(CVariable* pVariable, const CVariableValue& newValue, float resetTime /*= -1.0f*/)
{
	if (!pVariable)
	{
		DrsLogError("Trying to set a value on a 0-ptr variable!");
		return false;
	}

	DRS_DEBUG_DATA_ACTION(AddVariableSet(pVariable->m_name.GetText(), m_name.GetText(), pVariable->m_value, newValue, CResponseSystem::GetInstance()->GetCurrentDrsTime()));

	if (resetTime > FLT_EPSILON)
	{
		return SetVariableValueForSomeTime(pVariable, newValue, resetTime);
	}
	else
	{
		pVariable->m_value = newValue;
		return true;
	}
}

//--------------------------------------------------------------------------------------------------
bool CVariableCollection::SetVariableValue(const CHashedString& name, int newValue, bool createIfNotExisting /*= true*/, float resetTime /*= -1.0f*/)
{
	return SetVariableValue(name, CVariableValue(newValue), createIfNotExisting, resetTime);
}

//--------------------------------------------------------------------------------------------------
bool CVariableCollection::SetVariableValue(const CHashedString& name, float newValue, bool createIfNotExisting /*= true*/, float resetTime /*= -1.0f*/)
{
	return SetVariableValue(name, CVariableValue(newValue), createIfNotExisting, resetTime);
}

//--------------------------------------------------------------------------------------------------
bool CVariableCollection::SetVariableValue(const CHashedString& name, const CHashedString& newValue, bool createIfNotExisting /*= true*/, float resetTime /*= -1.0f*/)
{
	return SetVariableValue(name, CVariableValue(newValue), createIfNotExisting, resetTime);
}

//--------------------------------------------------------------------------------------------------
bool CVariableCollection::SetVariableValue(const CHashedString& name, bool newValue, bool createIfNotExisting /*= true*/, float resetTime /*= -1.0f*/)
{
	return SetVariableValue(name, CVariableValue(newValue), createIfNotExisting, resetTime);
}

//--------------------------------------------------------------------------------------------------
bool CVariableCollection::SetVariableValueForSomeTime(CVariable* pVariable, const CVariableValue& newValue, float timeBeforeReset)
{
#if defined(ENABLE_VARIABLE_VALUE_TYPE_CHECKINGS)
	if (!newValue.DoTypesMatch(pVariable->m_value))
	{
		DrsLogWarning((string("SetVariableValue: Type of variable \'" + string(pVariable->m_name.GetText()) + "\' and type of newValue do not match! NewValueType: ") + newValue.GetTypeAsString() + " - variableType: " + pVariable->m_value.GetTypeAsString()).c_str());
	}
#endif
	const float currentTime = CResponseSystem::GetInstance()->GetCurrentDrsTime();

	//check if there is already an cooldown for this variable, in that case we dont change the "oldValue"
	for (CoolingDownVariableList::iterator it = m_coolingDownVariables.begin(), itEnd = m_coolingDownVariables.end(); it != itEnd; ++it)
	{
		if (it->variable == pVariable)
		{
			pVariable->m_value = newValue;  //actual change the value of the variable
			it->timeOfReset = currentTime + timeBeforeReset;
			return true;
		}
	}

	VariableCooldownInfo newCooldown;
	newCooldown.variable = pVariable;
	newCooldown.timeOfReset = currentTime + timeBeforeReset;
	newCooldown.oldValue = pVariable->m_value.GetValue();
	m_coolingDownVariables.push_back(newCooldown);

	pVariable->m_value = newValue;  //actual change the value of the variable

	return true;
}

//--------------------------------------------------------------------------------------------------
void CVariableCollection::Update()
{
	if (m_coolingDownVariables.empty())
	{
		return;
	}

	const float currentTime = CResponseSystem::GetInstance()->GetCurrentDrsTime();
	SET_DRS_USER_SCOPED("Variable Cooldowns");

	for (CoolingDownVariableList::iterator it = m_coolingDownVariables.begin(); it != m_coolingDownVariables.end(); )
	{
		if (currentTime > it->timeOfReset)
		{
			DRS_DEBUG_DATA_ACTION(AddVariableSet(it->variable->m_name.GetText(), m_name.GetText(), it->variable->m_value, it->oldValue, CResponseSystem::GetInstance()->GetCurrentDrsTime()));
			DrsLogInfo((string("Reset Variable '") + it->variable->m_name.GetText() + " from '" + it->variable->m_value.GetValueAsString() + "' back to '" + CVariableValue(it->oldValue).GetValueAsString() + "' because the reset-cooldown has expired").c_str());
			it->variable->m_value.SetValue(it->oldValue);
			it = m_coolingDownVariables.erase(it);
		}
		else
		{
			++it;
		}
	}
}

//--------------------------------------------------------------------------------------------------
const CVariableValue& CVariableCollection::GetVariableValue(const CHashedString& name) const
{
	for (const CVariable* variable : m_allResponseVariables)
	{
		if (variable->m_name == name)
		{
			return variable->m_value;
		}
	}
	return s_newVariableValue;
}

//--------------------------------------------------------------------------------------------------
CVariable* CVariableCollection::CreateOrGetVariable(const CHashedString& variableName)
{
	CVariable* pVariable = GetVariable(variableName);
	if (!pVariable)
	{
		pVariable = CreateVariable(variableName, s_newVariableValue);
	}
	return pVariable;
}

//--------------------------------------------------------------------------------------------------
void CVariableCollection::Reset()
{
	SET_DRS_USER_SCOPED("Variable Reset");
	for (VariableList::iterator it = m_allResponseVariables.begin(), itEnd = m_allResponseVariables.end(); it != itEnd; ++it)
	{
		DRS_DEBUG_DATA_ACTION(AddVariableSet((*it)->m_name.GetText(), m_name.GetText(), (*it)->m_value, CVariableValue(), CResponseSystem::GetInstance()->GetCurrentDrsTime()));
		(*it)->m_value.Reset();
	}
}

//--------------------------------------------------------------------------------------------------
void CVariableCollection::Serialize(Serialization::IArchive& ar)
{
	ar(m_name, "collection-name", "!>");

	std::list<CVariable> variablesCopy;

	if (ar.isOutput())
	{
		for (auto it = m_allResponseVariables.cbegin(); it != m_allResponseVariables.cend(); ++it)
		{
			variablesCopy.push_back(*(*it));
		}
	}

	ar(variablesCopy, "Variables", "^ ");

	if (ar.isInput())
	{
		for (auto it = variablesCopy.begin(); it != variablesCopy.end(); ++it)
		{
			CVariable* variable = this->CreateOrGetVariable(it->m_name);
			variable->m_value = it->m_value;
		}
	}
	//do we need to serialize m_coolingDownVariables?
}

//--------------------------------------------------------------------------------------------------
string CVariableCollection::GetVariablesAsString() const
{
	string output;
	for (VariableList::const_iterator it = m_allResponseVariables.begin(), itEnd = m_allResponseVariables.end(); it != itEnd; ++it)
	{
		if (!output.empty())
		{
			output += ", ";
		}
		output += (*it)->m_name.GetText();
		output += " = ";
		output += (*it)->m_value.GetValueAsString();
	}
	return output;
}

//--------------------------------------------------------------------------------------------------
CVariableCollectionManager::~CVariableCollectionManager()
{
	for (CollectionList::iterator it = m_variableCollections.begin(); it != m_variableCollections.end(); ++it)
	{
		delete *it;
	}
}

//--------------------------------------------------------------------------------------------------
CVariableCollection* CVariableCollectionManager::CreateVariableCollection(const CHashedString& collectionName)
{
	if (!collectionName.IsValid())
	{
		return nullptr;
	}

#if defined(ENABLE_VARIABLE_VALUE_TYPE_CHECKINGS)
	CVariableCollection* alreadyExisting = GetCollection(collectionName);
	if (alreadyExisting)
	{
		DrsLogError((string("Variable Collection with name '") + string(collectionName.GetText()) + "\' was already existing!").c_str());
	}
#endif

	CVariableCollection* newCollection = new CVariableCollection(collectionName);
	m_variableCollections.push_back(newCollection);
	return newCollection;
}

//--------------------------------------------------------------------------------------------------
void CVariableCollectionManager::ReleaseVariableCollection(CVariableCollection* pCollectionToFree)
{
	for (CollectionList::iterator it = m_variableCollections.begin(); it != m_variableCollections.end(); ++it)
	{
		if (*it == pCollectionToFree)
		{
			delete *it;
			m_variableCollections.erase(it);
			return;
		}
	}
	DrsLogError("Could not find the specified Collection!");
}

//--------------------------------------------------------------------------------------------------
CVariableCollection* CVariableCollectionManager::GetCollection(const CHashedString& collectionName) const
{
	for (CVariableCollection* collection : m_variableCollections)
	{
		if (collection->GetName() == collectionName)
		{
			return collection;
		}
	}
	return nullptr;
}

//--------------------------------------------------------------------------------------------------
void CVariableCollectionManager::Update()
{
	for (CVariableCollection* const collection : m_variableCollections)
	{
		collection->Update();
	}
}

//--------------------------------------------------------------------------------------------------
void CVariableCollectionManager::Reset()
{
	for (CVariableCollection* const collection : m_variableCollections)
	{
		collection->Reset();
	}
}

//--------------------------------------------------------------------------------------------------
void CVariableCollectionManager::Serialize(Serialization::IArchive& ar)
{
#if defined(HASHEDSTRING_STORES_SOURCE_STRING)
	//todo drs this will not work for adding/removing variables, also the response manager need to serialize after we change move the variables around here
	for (size_t i = 0; i < m_variableCollections.size(); ++i)
	{
		CVariableCollection& currentCollection = *m_variableCollections[i];
		ar(currentCollection, currentCollection.GetName().m_textCopy, currentCollection.GetName().m_textCopy);
	}
#else
	DrsLogError("Not implemented");
#endif
}
