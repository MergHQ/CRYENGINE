// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "ISequencerSystem.h"

const int KEY_MENU_CMD_BASE = 1000;

template<class KeyType>
class TSequencerTrack
	: public CSequencerTrack
{
public:
	//! Return number of keys in track.
	virtual int GetNumKeys() const override { return m_keys.size(); }

	//! Set number of keys in track.
	//! If needed adds empty keys at end or remove keys from end.
	virtual void SetNumKeys(int numKeys) override { m_keys.resize(numKeys); }

	//! Remove specified key.
	virtual void RemoveKey(int num) override;

	virtual int  CreateKey(float time) override;
	virtual int  CloneKey(int fromKey) override;
	virtual int  CopyKey(CSequencerTrack* pFromTrack, int nFromKey) override;

	//! Get key at specified location.
	//! @param key Must be valid pointer to compatible key structure, to be filled with specified key location.
	virtual void                 GetKey(int index, CSequencerKey* key) const override;

	virtual const CSequencerKey* GetKey(int index) const override;
	virtual CSequencerKey*       GetKey(int index) override;

	//! Set key at specified location.
	//! @param key Must be valid pointer to compatible key structure.
	virtual void SetKey(int index, const CSequencerKey* key) override;

	/** Serialize this animation track to XML.
	    Do not override this method, prefer to override SerializeKey.
	 */
	virtual bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks = true) override;

	virtual bool SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected = false, float fTimeOffset = 0) override;

	/** Serialize single key of this track.
	    Override this in derived classes.
	    Do not save time attribute, it is already saved in Serialize of the track.
	 */
	virtual void SerializeKey(KeyType& key, XmlNodeRef& keyNode, bool bLoading) = 0;

protected:
	//! Sort keys in track (after time of keys was modified).
	virtual void DoSortKeys() override;

	virtual void OnTimeRangeChanged(const Range& oldRange, const Range& newRange) override;

protected:
	typedef std::vector<KeyType> Keys;
	Keys m_keys;
};

//////////////////////////////////////////////////////////////////////////
template<class KeyType>
inline void TSequencerTrack<KeyType >::RemoveKey(int index)
{
	assert(index >= 0 && index < (int)m_keys.size());
	m_keys.erase(m_keys.begin() + index);
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
template<class KeyType>
inline void TSequencerTrack<KeyType >::GetKey(int index, CSequencerKey* key) const
{
	assert(index >= 0 && index < (int)m_keys.size());
	assert(key != 0);
	*(KeyType*)key = m_keys[index];
}

//////////////////////////////////////////////////////////////////////////
template<class KeyType>
inline const CSequencerKey* TSequencerTrack<KeyType >::GetKey(int index) const
{
	assert(index >= 0 && index < (int)m_keys.size());
	return &m_keys[index];
}

//////////////////////////////////////////////////////////////////////////
template<class KeyType>
inline CSequencerKey* TSequencerTrack<KeyType >::GetKey(int index)
{
	assert(index >= 0 && index < (int)m_keys.size());
	return &m_keys[index];
}

//////////////////////////////////////////////////////////////////////////
template<class KeyType>
inline void TSequencerTrack<KeyType >::SetKey(int index, const CSequencerKey* key)
{
	assert(index >= 0 && index < (int)m_keys.size());
	assert(key != 0);
	m_keys[index] = *(KeyType*)key;
	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
template<class KeyType>
inline void TSequencerTrack<KeyType >::DoSortKeys()
{
	std::sort(m_keys.begin(), m_keys.end());
}

template<typename KeyType>
inline void TSequencerTrack<KeyType>::OnTimeRangeChanged(const Range& oldRange, const Range& newRange)
{
	const float leftDelta = newRange.start - oldRange.start;
	const float rightDelta = newRange.end - oldRange.end;

	for (CSequencerKey& key : m_keys)
	{
		switch (key.GetTimelineAlignment())
		{
		case CSequencerKey::ETimelineAlignment::Left:
			key.m_time += leftDelta;
			break;
		case CSequencerKey::ETimelineAlignment::Right:
			key.m_time += rightDelta;
			break;
		default:
			assert(false);
			break;
		}

		newRange.ClipValue(key.m_time);
	}

	Invalidate();
}

//////////////////////////////////////////////////////////////////////////
template<class KeyType>
inline bool TSequencerTrack<KeyType >::Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks)
{
	if (bLoading)
	{
		int num = xmlNode->getChildCount();

		Range timeRange;
		int flags = GetFlags();
		xmlNode->getAttr("Flags", flags);
		xmlNode->getAttr("StartTime", timeRange.start);
		xmlNode->getAttr("EndTime", timeRange.end);
		SetFlags(flags);
		SetTimeRange(timeRange);

		SetNumKeys(num);
		for (int i = 0; i < num; i++)
		{
			XmlNodeRef keyNode = xmlNode->getChild(i);
			keyNode->getAttr("time", m_keys[i].m_time);

			SerializeKey(m_keys[i], keyNode, bLoading);
		}

		if ((!num) && (!bLoadEmptyTracks))
			return false;
	}
	else
	{
		int num = GetNumKeys();
		MakeValid();
		xmlNode->setAttr("Flags", GetFlags());
		xmlNode->setAttr("StartTime", GetTimeRange().start);
		xmlNode->setAttr("EndTime", GetTimeRange().end);

		for (int i = 0; i < num; i++)
		{
			XmlNodeRef keyNode = xmlNode->newChild("Key");
			keyNode->setAttr("time", m_keys[i].m_time);

			SerializeKey(m_keys[i], keyNode, bLoading);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
template<class KeyType>
inline bool TSequencerTrack<KeyType >::SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected, float fTimeOffset)
{
	if (bLoading)
	{
		int numCur = GetNumKeys();
		int num = xmlNode->getChildCount();

		int type;
		xmlNode->getAttr("TrackType", type);

		SetNumKeys(num + numCur);
		for (int i = 0; i < num; i++)
		{
			XmlNodeRef keyNode = xmlNode->getChild(i);
			keyNode->getAttr("time", m_keys[i + numCur].m_time);
			m_keys[i + numCur].m_time += fTimeOffset;

			SerializeKey(m_keys[i + numCur], keyNode, bLoading);
			if (bCopySelected)
			{
				m_keys[i + numCur].flags |= AKEY_SELECTED;
			}
		}
		SortKeys();
	}
	else
	{
		int num = GetNumKeys();
		//CheckValid();

		for (int i = 0; i < num; i++)
		{
			if (!bCopySelected || GetKeyFlags(i) & AKEY_SELECTED)
			{
				XmlNodeRef keyNode = xmlNode->newChild("Key");
				keyNode->setAttr("time", m_keys[i].m_time + fTimeOffset);

				SerializeKey(m_keys[i], keyNode, bLoading);
			}
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
template<class KeyType>
inline int TSequencerTrack<KeyType >::CreateKey(float time)
{
	KeyType key, akey;
	int nkey = GetNumKeys();
	SetNumKeys(nkey + 1);
	key.m_time = time;
	SetKey(nkey, &key);

	return nkey;
}

//////////////////////////////////////////////////////////////////////////
template<class KeyType>
inline int TSequencerTrack<KeyType >::CloneKey(int fromKey)
{
	KeyType key;
	GetKey(fromKey, &key);
	int nkey = GetNumKeys();
	SetNumKeys(nkey + 1);
	SetKey(nkey, &key);
	return nkey;
}

//////////////////////////////////////////////////////////////////////////
template<class KeyType>
inline int TSequencerTrack<KeyType >::CopyKey(CSequencerTrack* pFromTrack, int nFromKey)
{
	KeyType key;
	pFromTrack->GetKey(nFromKey, &key);
	int nkey = GetNumKeys();
	SetNumKeys(nkey + 1);
	SetKey(nkey, &key);
	return nkey;
}
