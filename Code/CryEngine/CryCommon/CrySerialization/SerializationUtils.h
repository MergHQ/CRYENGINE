// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySerialization/StringList.h>
#include <CrySerialization/IArchive.h>
#include <CrySystem/XML/XMLAttrReader.h>

namespace SerializationUtils
{		
	template<class T, typename F>
	Serialization::StringList ContainerToStringList(const T& container, F toString)
	{
		Serialization::StringList stringList;
		stringList.reserve(container.size() + 1);
		stringList.push_back("");

		for (const typename T::value_type& element : container)
		{
			stringList.push_back(toString(element));
		}

		return stringList;
	}

	namespace
	{
		// Converts the given container to a string list applying the toString function on each element of the container
		// If forceAdd is True
		//  The function adds the value parameter to the string list if it doesnt exist and sets valueAddedToStringList to true
		template<class T, typename F>
		Serialization::StringListValue ContainerToStringListValuesHelper(const T& container, F toString, const string& value, const bool forceAdd, bool& valueAddedToStringList)
		{
			valueAddedToStringList = false;

			Serialization::StringList stringList = ContainerToStringList(container, toString);

			string selectedString = value;
			if (Serialization::StringList::npos == stringList.find(value.c_str()))
			{
				if (forceAdd)
				{
					stringList.push_back(value);
					valueAddedToStringList = true;
				}
				else
				{
					selectedString = "";
				}
			}

			// Attach StringListValue to input string parameter 'value'
			return Serialization::StringListValue(stringList, selectedString, &value, Serialization::TypeID::get<string>());
		}
	}
	
	// Converts the given container to a string list applying the toString function on each element of the container
	template<class T, typename F>
	Serialization::StringListValue ContainerToStringListValues(const T& container, F toString, const string& value)
	{
		const bool forceAdd = false;
		bool valueAddedToStringList = false;
		return ContainerToStringListValuesHelper(container, toString, value, forceAdd, valueAddedToStringList);
	}

	// Same as ContainerToStringListValues but adds the parameter value to the list if it doesnt exist and sets valueAddedToStringList to true
	template<class T, typename F>
	Serialization::StringListValue ContainerToStringListValuesForceAdd(const T& container, F toString, const string& value, bool& valueAddedToStringList)
	{
		const bool forceAdd = true;
		return ContainerToStringListValuesHelper(container, toString, value, forceAdd, valueAddedToStringList);
	}

	template < class Enum >
	void SerializeEnumList(Serialization::IArchive& archive, const char* szKey, const char* szLabel, const Serialization::StringList& stringList, const CXMLAttrReader < Enum >& xmlAttrReader, Enum& value)
	{
		Serialization::StringListValue stringListValue(stringList, stringList.find(xmlAttrReader.GetFirstRecordName(value)));
		archive(stringListValue, szKey, szLabel);
		value = *xmlAttrReader.GetFirstValue(stringList[stringListValue.index()]);
	}

	namespace Messages
	{

		inline string ErrorEmptyHierachy(const string& childType)
		{
			string emptyHierachyMesage;
			emptyHierachyMesage.Format("Must contain at least one %s", childType.c_str());
			return emptyHierachyMesage;
		}

		inline string ErrorEmptyValue(const string& fieldName)
		{
			string emptyValueMesage;
			emptyValueMesage.Format("Field '%s' must be specified. It cannot be empty.", fieldName.c_str());
			return emptyValueMesage;
		}

		inline string ErrorNonExistingValue(const string& fieldName, const string& oldValue)
		{
			string emptyValueMesage;
			emptyValueMesage.Format("Field '%s' is using an old value %s that no longer exists. Please, select a valid value", fieldName.c_str(), oldValue.c_str());
			return emptyValueMesage;
		}

		inline string ErrorDuplicatedValue(const string& fieldName, const string& duplicatedValue)
		{
			string duplicatedValueMessage;
			duplicatedValueMessage.Format("Duplicated field '%s' with value '%s'.", fieldName.c_str(), duplicatedValue.c_str());
			return duplicatedValueMessage;
		}

		inline string ErrorInvalidValueWithReason(const string& fieldName, const string& providedInvalidValue, const string& reason)
		{
			string invalidValueMessage;
			invalidValueMessage.Format("Field %s has invalid value '%s'. Cause: %s.", fieldName.c_str(), providedInvalidValue.c_str(), reason.c_str());
			return invalidValueMessage;
		}




	}
} // SerializationUtils