// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// use one instance of this class in your object manager
// quite efficient implementation, avoids use of pointer to save memory and reduce cache misses
struct SSaltBufferArray
{
	SSaltBufferArray()
	{
		Reset();
	}

	// reset (don't use old handles after calling reset)
	void Reset()
	{
		// build free list from front to back
		auto it = m_buffer.begin();
		for(auto end = m_buffer.cend(); it != end; ++it)
		{
			it->m_salt = 0u;
			it->m_nextIndex = static_cast<EntityIndex>(std::distance(m_buffer.begin(), it)) + 1;
		}

		m_maxUsedEntityIndex = 0;
		m_freeListStartIndex = 1;

		// 0 is not used because it's nil
		m_buffer[0].m_salt = std::numeric_limits<EntityIndex>::max();
		m_buffer[0].MarkUsed();
	}

	bool IsEmpty() const
	{
		return m_maxUsedEntityIndex == 0;
	}

	bool IsFull() const
	{
		return m_freeListStartIndex == EntityArraySize;
	}

	// O(n) n=FreeList Size
	// useful for serialization (Reset should be called before inserting the first known element)
	// Arguments:
	//   Handle - must be not nil
	void InsertKnownHandle(const SEntityIdentifier& handle)
	{
		CRY_ASSERT(handle.GetId() != 0, "Entity index 0 cannot be reserved!");
		CRY_ASSERT(handle.GetIndex() < EntityArraySize, "Entity index exceeded maximum bounds!");

		if (!IsUsed(handle.GetIndex()))
		{
			RemoveFromFreeList(handle.GetIndex());
		}

		if (handle.GetIndex() > m_maxUsedEntityIndex)
		{
			m_maxUsedEntityIndex = handle.GetIndex();
		}

		SSaltBufferElement& rElement = m_buffer[handle.GetIndex()];

		rElement.m_salt = handle.GetSalt();
		rElement.MarkUsed();
	}

	// O(1) = fast
	// Returns:
	//   nil if there was not enough space, valid SaltHandle otherwise
	SEntityIdentifier Insert()
	{
		if (IsFull())
		{
			return SEntityIdentifier();
		}

		// update bounds
		if (m_freeListStartIndex > m_maxUsedEntityIndex)
		{
			m_maxUsedEntityIndex = m_freeListStartIndex;
		}

		const SEntityIdentifier entityId(m_buffer[m_freeListStartIndex].m_salt, m_freeListStartIndex);

		SSaltBufferElement& saltBufferElement = m_buffer[m_freeListStartIndex];

		m_freeListStartIndex = saltBufferElement.m_nextIndex;
		saltBufferElement.MarkUsed();

		CRY_ASSERT(IsUsed(entityId.GetIndex()), "Entity identifier was not correctly marked as used!");

		return entityId;
	}

	// O(1) - don't call for invalid handles and don't remove objects twice
	void Remove(const SEntityIdentifier& handle)
	{
		CRY_ASSERT(handle.GetId() != 0, "Validity checks should not be performed on entity index 0");
		CRY_ASSERT(handle.GetIndex() < EntityArraySize);

		const EntityIndex index = handle.GetIndex();

#if defined(USE_CRY_ASSERT)
		const EntitySalt oldSalt = m_buffer[index].m_salt;
#endif

		CRY_ASSERT(IsUsed(index), "Tried to remove entity identifier that was already marked as not in use!");
		CRY_ASSERT(handle.GetSalt() == oldSalt);

		m_buffer[index].m_salt++;

		CRY_ASSERT(m_buffer[index].m_salt > oldSalt, "Entity salt overflowed, consider decreasing EntityIndexBitCount!");
		
		m_buffer[index].m_nextIndex = m_freeListStartIndex;
		m_freeListStartIndex = index;
	}

	// O(1)
	// Returns:
	//   true=handle is referencing to a valid object, false=handle is not or referencing to an object that was removed
	bool IsValid(const SEntityIdentifier& handle) const
	{
		CRY_ASSERT(handle.GetId() != 0, "Validity checks should not be performed on entity index 0");
		CRY_ASSERT(handle.GetIndex() < EntityArraySize);

		return m_buffer[handle.GetIndex()].m_salt == handle.GetSalt();
	}

	// O(1) - useful for iterating the used elements, use together with GetMaxUsed()
	bool IsUsed(const EntityIndex index) const
	{
		CRY_ASSERT(index != 0);
		CRY_ASSERT(index < EntityArraySize);
		return m_buffer[index].IsInUse();
	}

	// useful for iterating the used elements, use together with IsUsed()
	EntityIndex GetMaxUsedEntityIndex() const
	{
		return m_maxUsedEntityIndex;
	}

private:
	// O(n) n=FreeList Size
	// Returns:
	//   Index must be part of the FreeList
	void RemoveFromFreeList(const EntityIndex index)
	{
		if (m_freeListStartIndex == index)     // first index
		{
			m_freeListStartIndex = m_buffer[index].m_nextIndex;
		}
		else                                // not the first index
		{
			for (EntityIndex previousIndex = m_freeListStartIndex, currentIndex = m_buffer[previousIndex].m_nextIndex;; )
			{
				const EntityIndex nextIndex = m_buffer[currentIndex].m_nextIndex;

				if (currentIndex == index)
				{
					m_buffer[previousIndex].m_nextIndex = nextIndex;
					break;
				}

				CRY_ASSERT(!m_buffer[currentIndex].IsLastItem(), "Tried to remove non-existing entity index from the free list!");

				previousIndex = currentIndex;
				currentIndex = nextIndex;
			}
		}

		m_buffer[index].MarkUsed();
	}

	// ------------------------------------------------------------------------------------------------

	struct SSaltBufferElement
	{
		// Used to indicate that this element is currently in use
		static constexpr EntityIndex UsedMarker = EntityArraySize + 1;

		void MarkUsed() { m_nextIndex = UsedMarker; }
		bool IsInUse() const { return m_nextIndex == UsedMarker; }
		bool IsLastItem() const { return m_nextIndex == EntityArraySize; }

		//! 0.. is counting up on every remove, should never wrap around
		EntityIndex m_salt      : EntitySaltBitCount;
		//! linked list of free or used elements, EntityArraySize is used as end marker and for not valid elements, UsedMarker is used for used elements
		EntityIndex m_nextIndex : EntityIndexBitCount;
	};

	//! free list and salt buffer elements in one
	std::array<SSaltBufferElement, EntityArraySize> m_buffer;
	//! Next available entity index (relative to m_buffer)
	//! 1 if empty (due to 0 being an invalid index), EntityArraySize if we are full
	EntityIndex m_freeListStartIndex : EntityIndexBitCount;
	//! Index of the max used entity index
	//! Allows for fast iteration through only used elements and entities, instead of the whole array
	//! Constantly grows, only resetting when Reset is called
	EntityIndex m_maxUsedEntityIndex : EntityIndexBitCount;
};