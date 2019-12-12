// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "BehaviorTreeDefines.h"

#include <CrySerialization/IArchive.h>
#include <CrySerialization/CryStrings.h>
#include <CrySerialization/SerializationUtils.h>

#include <bitset>

namespace BehaviorTree
{
#ifdef USING_BEHAVIOR_TREE_SERIALIZATION

#define NODE_COMBOBOX_FIXED_WIDTH          "300"
#define STATE_TRANSITION_EVENT_FIXED_WIDTH "300"
#define LINE_NUMBER_FIXED_WIDTH            "58"

//! A simple helper class in which we store hints to the serialization code.
//! These hints can be used to control the layout/contents of the property tree in the editor.
struct NodeSerializationHints
{
	enum EEventsFlags
	{
		EEventsFlags_CryEngine = 0,
		EEventsFlags_GameSDK = 1,
		EEventsFlags_Deprecated = 2,

		EEventsFlags_Count = 3
	};

	typedef std::bitset<EEventsFlags_Count> EventsFlags;

	NodeSerializationHints()
		: showXmlLineNumbers(false)
		, showComments(false)
		, eventsFlags()
	{
	}

	bool showXmlLineNumbers;
	bool showComments;
	EventsFlags eventsFlags;
};

static void HandleXmlLineNumberSerialization(Serialization::IArchive& archive, const uint32 xmlLineNumber)
{
	const NodeSerializationHints* hintsContext = archive.context<NodeSerializationHints>();
	IF_LIKELY (hintsContext != nullptr)
	{
		if (hintsContext->showXmlLineNumbers)
		{
			stack_string xmlLineText;
			const bool isXmlLineKnown = (xmlLineNumber != 0u);     // XML lines start at 1, so this works when the document has not been saved to XML yet.
			if (isXmlLineKnown)
			{
				xmlLineText.Format("::: %u :::", xmlLineNumber);
			}
			else
			{
				xmlLineText = "::: ? :::";
			}

			archive(xmlLineText, "", "!^^>" LINE_NUMBER_FIXED_WIDTH ">");
		}
	}
}

static void HandleCommentSerialization(Serialization::IArchive& archive, string& comment)
{
	const NodeSerializationHints* pHintsContext = archive.context<NodeSerializationHints>();
	IF_LIKELY (pHintsContext != nullptr)
	{
		if (pHintsContext->showComments)
		{
			archive(comment, "comment", "^Comment");
		}
	}
}

// T::value_type should implement SerializeToString method
template<class T>
static void SerializeContainerAsStringList(Serialization::IArchive& archive, const char* szKey, const char* szLabel, const T& container, const string& fieldName, /* OUT */ string& selectedElement, const bool showErrorEmptyMessage = true, const bool showErrorNonExistingValueMessage = true)
{
	const auto toString = [](const T::value_type& t) -> string
	{
		return t.SerializeToString();
	};

	bool elementAddedToStringList;
	Serialization::StringListValue stringListValue = SerializationUtils::ContainerToStringListValuesForceAdd(container, toString, selectedElement, elementAddedToStringList);

	archive(stringListValue, szKey, szLabel);
	selectedElement = stringListValue.c_str();

	if (showErrorNonExistingValueMessage && elementAddedToStringList)
	{
		archive.error(stringListValue.handle(), stringListValue.type(), SerializationUtils::Messages::ErrorNonExistingValue(fieldName, selectedElement));
	}

	if (showErrorEmptyMessage && selectedElement.empty())
	{
		archive.error(stringListValue.handle(), stringListValue.type(), SerializationUtils::Messages::ErrorEmptyValue(fieldName));
	}
}

// T::value_type should implement SerializeToString method
// T::value_type should implement operator <
template<class T>
static void SerializeContainerAsSortedStringList(Serialization::IArchive& archive, const char* szKey, const char* szLabel, T container, const string& fieldName, /* OUT */ string& selectedElement, const bool showErrorEmptyMessage = true, const bool showErrorNonExistingValueMessage = true)
{
	std::stable_sort(container.begin(), container.end());

	SerializeContainerAsStringList(archive, szKey, szLabel, container, fieldName, selectedElement, showErrorEmptyMessage, showErrorNonExistingValueMessage);
}

#endif // USING_BEHAVIOR_TREE_SERIALIZATION
}
