// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

//! \cond INTERNAL

#pragma once

#include "BehaviorTreeDefines.h"

#if defined(USING_BEHAVIOR_TREE_SERIALIZATION)
	#include <CrySerialization/StringList.h>
	#include <CrySerialization/SerializationUtils.h>
	#include <CryAISystem/BehaviorTree/IBehaviorTree.h>
#endif

#if defined(DEBUG_MODULAR_BEHAVIOR_TREE) || defined(USING_BEHAVIOR_TREE_EDITOR)
	#define STORE_EXTRA_TIMESTAMP_DATA
#endif

#include <CryCore/Containers/VariableCollection.h>

namespace BehaviorTree
{
//! Unique identifier for a timestamp (the CRC of the timestamp name).
struct TimestampID
{
	TimestampID()
		: id(0)
	{
	}

	TimestampID(const char* name)
	{
#ifdef STORE_EXTRA_TIMESTAMP_DATA
		timestampName = name;
#endif

		id = CCrc32::Compute(name);
	}

	bool operator==(const TimestampID& rhs) const
	{
		return id == rhs.id;
	}

	bool operator!=(const TimestampID& rhs) const
	{
		return id != rhs.id;
	}

	operator bool() const
	{
		return id != 0;
	}

#ifdef STORE_EXTRA_TIMESTAMP_DATA
	string timestampName;
#endif

	unsigned int id;
};

struct Timestamp;
typedef std::vector<Timestamp> Timestamps;

//! A timestamp is a small piece of information on when a certain event occurred.
//! For example, this is useful if you want to query when a character was last
//! shot at or hit, when they last could see the player, when they last
//! moved to a new cover location, etc.
//! Optional:
//! Timestamps can be declared as exclusive to each other. For example,
//! <EnemySeenTime> could be said to be exclusive to <EnemyLostTime>.
//! You can't see the player and at the same time consider him lost.
struct Timestamp
{
	Timestamp()
		: hasBeenSetAtLeastOnce(false)
	{
		Invalidate();
	}

	TimestampID id;
	TimestampID exclusiveTo;
	CTimeValue  time;
	uint32      setOnEventNameCRC32;
	bool        hasBeenSetAtLeastOnce;

#if defined(STORE_EXTRA_TIMESTAMP_DATA)
	string setOnEventName;
#endif
	bool IsValid() const
	{
		return time.GetMilliSecondsAsInt64() >= 0;
	}

	void Invalidate()
	{
		time = CTimeValue(-1.0f);
	}

#if defined (USING_BEHAVIOR_TREE_SERIALIZATION)
	void Serialize(Serialization::IArchive& archive)
	{
		// Serialize timestamp name
		archive(id.timestampName, "name", "^<Name");
		archive.doc("Timestamp name");

		if (id.timestampName.empty())
		{
			archive.error(id.timestampName, SerializationUtils::Messages::ErrorEmptyValue("Name"));
		}

		// Serialize events
		const Variables::EventsDeclaration* eventsDeclaration = archive.context<Variables::EventsDeclaration>();
		IF_LIKELY (!eventsDeclaration)
			return;

		SerializeContainerAsSortedStringList(archive, "setOnEventName", "^Set on event", eventsDeclaration->GetEventsWithFlags(), "Event", setOnEventName);
		archive.doc("Event that triggers the start of the Timestamp");

		const Timestamps* timestamps = archive.context<Timestamps>();
		if (!timestamps)
		{
			return;
		}
		
		Timestamps timestampsWithoutThis;
		if (!timestamps->empty())
		{
			timestampsWithoutThis.reserve(timestamps->size() - 1);

			for (const Timestamp& timestamp : *timestamps)
			{
				if (timestamp.id != id)
				{
					timestampsWithoutThis.push_back(timestamp);
				}
			}
		}

		SerializeContainerAsSortedStringList(archive,  "exclusiveTo", "^<Exclusive To", timestampsWithoutThis, "Exclusive To", exclusiveTo.timestampName, false, true);
	}

	const string& SerializeToString() const
	{
		return id.timestampName;
	}

	bool operator<(const Timestamp& rhs) const
	{
		return (id.timestampName < rhs.id.timestampName);
	}

	bool operator ==(const Timestamp& rhs) const
	{
		return setOnEventNameCRC32 == rhs.setOnEventNameCRC32;
	}
#endif

};

//! Contains information about when certain events occurred.
//! Allows us to get answers to the following questions:
//! Q: How long was it since X happened?
//! Q: How long has Y been going on?
class TimestampCollection
{
	struct MatchesID
	{
		const TimestampID& id;
		MatchesID(const TimestampID& _id) : id(_id) {}
		bool operator()(const Timestamp& timestamp) const { return timestamp.id == id; }
	};

public:
	bool HasBeenSetAtLeastOnce(const TimestampID& id) const
	{
		const Timestamps::const_iterator it = std::find_if(m_timestamps.begin(), m_timestamps.end(), MatchesID(id));

		if (it != m_timestamps.end())
			return it->hasBeenSetAtLeastOnce;

		return false;
	}

	void GetElapsedTimeSince(const TimestampID& id, CTimeValue& elapsedTime, bool& valid) const
	{
		const Timestamps::const_iterator it = std::find_if(m_timestamps.begin(), m_timestamps.end(), MatchesID(id));

		if (it != m_timestamps.end())
		{
			if (it->IsValid())
			{
				valid = true;
				elapsedTime = gEnv->pTimer->GetFrameStartTime() - it->time;
				return;
			}
		}

		valid = false;
		elapsedTime = CTimeValue(0.0f);
	}

	void HandleEvent(uint32 eventNameCRC32)
	{
		TimestampIDs timestampsToInvalidate;

		{
			// Go through timestamps, refresh the time value and see if there
			// are any exclusive timestamps that should be removed.

			Timestamps::iterator it = m_timestamps.begin();
			Timestamps::iterator end = m_timestamps.end();

			for (; it != end; ++it)
			{
				Timestamp& timestamp = *it;

				if (timestamp.setOnEventNameCRC32 == eventNameCRC32)
				{
					timestamp.time = gEnv->pTimer->GetFrameStartTime();
					timestamp.hasBeenSetAtLeastOnce = true;

					if (timestamp.exclusiveTo)
					{
						timestampsToInvalidate.push_back(timestamp.exclusiveTo);
					}
				}
			}
		}

		{
			// Invalidate the timestamps that were pushed out via exclusivity

			TimestampIDs::iterator it = timestampsToInvalidate.begin();
			TimestampIDs::iterator end = timestampsToInvalidate.end();

			for (; it != end; ++it)
			{
				Timestamps::iterator timestamp = std::find_if(m_timestamps.begin(), m_timestamps.end(), MatchesID(*it));

				if (timestamp != m_timestamps.end())
				{
					timestamp->Invalidate();
				}
			}
		}
	}

	bool LoadFromXml(Variables::EventsDeclaration& eventsDeclaration, const XmlNodeRef& xml, const char* fileName, const bool isLoadingFromEditor)
	{
		for (int i = 0; i < xml->getChildCount(); ++i)
		{
			XmlNodeRef child = xml->getChild(i);

			// name
			const char* name;
			if (child->haveAttr("name"))
			{
				name = child->getAttr("name");
			}
			else
			{
				gEnv->pLog->LogWarning("(%d) [Tree='%s'] Missing 'name' attribute for tag '%s'", child->getLine(), fileName, child->getTag());
				return false;
			}

			// Event
			const char* setOnEvent;
			if (child->haveAttr("setOnEvent"))
			{
				setOnEvent = child->getAttr("setOnEvent");
			}
			else
			{
				gEnv->pLog->LogWarning("(%d) [Tree='%s'] Missing 'setOnEvent' attribute for tag '%s'", child->getLine(), fileName, child->getTag());
				return false;
			}

#if defined(USING_BEHAVIOR_TREE_SERIALIZATION)
			// Automatically declare user-defined signals
			if (!eventsDeclaration.IsDeclared(setOnEvent, isLoadingFromEditor))
			{
				if (isLoadingFromEditor)
				{
					eventsDeclaration.DeclareGameEvent(setOnEvent);
					gEnv->pLog->LogWarning("(%d) [File='%s'] Unknown event '%s' used in Timestamp. Event will be declared automatically", child->getLine(), fileName, setOnEvent);
				}
				else
				{
					gEnv->pLog->LogError("(%d) [File='%s'] Unknown event '%s' used in Timestamp.", child->getLine(), fileName, setOnEvent);
					return false;
				}
			}
#endif // USING_BEHAVIOR_TREE_SERIALIZATION

			// Exclusive
			const char* exclusiveTo = child->getAttr("exclusiveTo");

			Timestamp timestamp;
			timestamp.id = TimestampID(name);
			timestamp.setOnEventNameCRC32 = CCrc32::Compute(setOnEvent);
			timestamp.exclusiveTo = TimestampID(exclusiveTo);
#if defined (STORE_EXTRA_TIMESTAMP_DATA)
			timestamp.setOnEventName = setOnEvent;
#endif
			m_timestamps.push_back(timestamp);

		}

		return true;
	}

	const Timestamps& GetTimestamps() const { return m_timestamps; }

#if defined (USING_BEHAVIOR_TREE_SERIALIZATION)
	void Serialize(Serialization::IArchive& archive)
	{
		const std::vector<size_t> duplicatedIndices = Variables::GetIndicesOfDuplicatedEntries(m_timestamps);
		for (const size_t i : duplicatedIndices)
		{
			archive.error(m_timestamps[i].id.timestampName, SerializationUtils::Messages::ErrorDuplicatedValue("Timestamp", m_timestamps[i].id.timestampName));
		}

		Serialization::SContext variableDeclarations(archive, &m_timestamps);
		archive(m_timestamps, "timestamps", "^[<>]");
	}
#endif

#if defined (USING_BEHAVIOR_TREE_XML_DESCRIPTION_CREATION)
	XmlNodeRef CreateXmlDescription()
	{
		if (m_timestamps.size() == 0)
			return XmlNodeRef();

		XmlNodeRef timestampsXml = GetISystem()->CreateXmlNode("Timestamps");
		for (Timestamps::const_iterator it = m_timestamps.begin(), end = m_timestamps.end(); it != end; ++it)
		{
			const Timestamp& timestamp = *it;

			XmlNodeRef timestampXml = GetISystem()->CreateXmlNode("Timestamp");
			timestampXml->setAttr("name", timestamp.id.timestampName);
			timestampXml->setAttr("setOnEvent", timestamp.setOnEventName);
			if (!timestamp.exclusiveTo.timestampName.empty())
				timestampXml->setAttr("exclusiveTo", timestamp.exclusiveTo.timestampName);

			timestampsXml->addChild(timestampXml);
		}

		return timestampsXml;
	}
#endif

private:
	typedef std::vector<TimestampID> TimestampIDs;
	Timestamps m_timestamps;
};
}

//! \endcond