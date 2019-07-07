// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EntitySystem.h"
#include "Entity.h"

class CEntity;
struct IEntity;

class CEntityItMap final : public IEntityIt
{
public:
	CEntityItMap(const CEntitySystem::SEntityArray& array)
		: m_array(array)
	{
		m_referenceCount = 0;
		MoveFirst();
	}
	virtual ~CEntityItMap() = default;

	virtual bool IsEnd() override
	{
		const CEntitySystem::SEntityArray::const_iterator end = m_array.end();

		// jump over gaps
		while (m_it != end)
		{
			if (*m_it != nullptr)
				return false;

			++m_it;
		}

		// Check if we passed the last element
		return m_it == end;
	}

	virtual IEntity* This() override { return !IsEnd() ? *m_it : nullptr; }
	virtual IEntity* Next() override { return !IsEnd() ? *m_it++ : nullptr; }
	virtual void     MoveFirst() override { m_it = m_array.begin(); }
	virtual void     AddRef() override { m_referenceCount++; }
	virtual void     Release() override { if (--m_referenceCount <= 0) { delete this; } }

	// Only needed for unit tests
	int GetReferenceCount() const { return m_referenceCount; }

protected:
	int m_referenceCount;
	const CEntitySystem::SEntityArray& m_array;
	CEntitySystem::SEntityArray::const_iterator m_it;
};
