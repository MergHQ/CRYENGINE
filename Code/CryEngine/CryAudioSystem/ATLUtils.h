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

//////////////////////////////////////////////////////////////////////////
class CSmoothFloat
{
public:

	explicit CSmoothFloat(float const alpha, float const precision, float const initValue = 0.0f)
		: m_value(initValue)
		, m_target(initValue)
		, m_bIsActive(false)
		, m_alpha(fabs_tpl(alpha))
		, m_precision(fabs_tpl(precision))
	{}

	~CSmoothFloat() {}

	void Update(float const deltaTime)
	{
		if (m_bIsActive)
		{
			if (fabs_tpl(m_target - m_value) > m_precision)
			{
				// still need not reached the target within the specified precision
				m_value += (m_target - m_value) * m_alpha;
			}
			else
			{
				//reached the target within the last update frame
				m_value = m_target;
				m_bIsActive = false;
			}
		}
	}

	float GetCurrent() const { return m_value; }

	void  SetNewTarget(float const newTarget, bool const bReset = false)
	{
		if (bReset)
		{
			m_target = newTarget;
			m_value = newTarget;
		}
		else if (fabs_tpl(newTarget - m_target) > m_precision)
		{
			m_target = newTarget;
			m_bIsActive = true;
		}
	}

	void Reset(float const initValue = 0.0f)
	{
		m_value = m_target = initValue;
		m_bIsActive = false;
	}

private:

	float       m_value;
	float       m_target;
	bool        m_bIsActive;
	float const m_alpha;
	float const m_precision;
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
