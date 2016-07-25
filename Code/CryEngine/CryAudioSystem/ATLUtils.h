// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <AudioLogger.h>
#include <STLSoundAllocator.h>

#define ATL_FLOAT_EPSILON 1.0e-6

///////////////////////////////////////////////////////////////////////////
template<typename TMap, typename TKey>
bool FindPlace(TMap& map, TKey const& key, typename TMap::iterator& iPlace);

template<typename TMap, typename TKey>
bool FindPlaceConst(TMap const& map, TKey const& key, typename TMap::const_iterator& iPlace);

//////////////////////////////////////////////////////////////////////////
template<typename ObjType, typename IDType = size_t>
class CInstanceManager
{
public:

	CInstanceManager() {}
	~CInstanceManager() {}

	typedef std::vector<ObjType*, STLSoundAllocator<ObjType*>> TPointerContainer;

	TPointerContainer m_reserved;
	IDType            m_idCounter;
	IDType const      m_reserveSize;
	IDType const      m_minCounterValue;

	explicit CInstanceManager(size_t const reserveSize, IDType const minCounterValue)
		: m_idCounter(minCounterValue)
		, m_reserveSize(reserveSize)
		, m_minCounterValue(minCounterValue)
	{
		m_reserved.reserve(reserveSize);
	}

	IDType GetNextID()
	{
		if (m_idCounter >= m_minCounterValue)
		{
			return m_idCounter++;
		}
		else
		{
			g_audioLogger.Log(eAudioLogType_Error, "An AudioSystem InstanceManager ID counter wrapped around.");
			m_idCounter = m_minCounterValue;
			return m_idCounter;
		}
	}
};

//--------------------------- Implementations ------------------------------
template<typename TMap, typename TKey>
bool FindPlace(TMap& map, TKey const& key, typename TMap::iterator& iPlace)
{
	iPlace = map.find(key);
	return (iPlace != map.end());
}

///////////////////////////////////////////////////////////////////////////
template<typename TMap, typename TKey>
bool FindPlaceConst(TMap const& map, TKey const& key, typename TMap::const_iterator& iPlace)
{
	iPlace = map.find(key);
	return (iPlace != map.end());
}
