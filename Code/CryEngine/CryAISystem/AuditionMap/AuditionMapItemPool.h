// Copyright 2001-2017 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

namespace Perception
{
namespace AuditionHelpers
{

template<class TYPE, int SORT_AFTER = 32> class ItemPool
{
public:
	typedef typename std::vector<TYPE>::size_type size_type;

	size_type Allocate()
	{
		size_type index;
		if (!freeIndices.empty())
		{
			index = freeIndices.back();
			freeIndices.pop_back();
		}
		else
		{
			index = itemsArray.size();
			itemsArray.resize(index + 1);
		}
		allocatedIndices.push_back(index);

		CRY_ASSERT(index >= 0 && index < itemsArray.size());
		itemsArray[index] = TYPE();
		return index;
	}

	void Release(size_type index)
	{
		CRY_ASSERT(index >= 0 && index < itemsArray.size());

		if (!stl::find_and_erase(allocatedIndices, index))
		{
			CRY_ASSERT(false);
			return;
		}

		freeIndices.push_back(index);

		if (--releasesBeforeSortingCounter <= 0)
		{
			// Sort descendantly so that the lowest indices can be popped of the end
			// again. This will make us re-use lowest indices faster which means
			// that newly 'allocated' stimuli will remain clustered at the start of
			// the memory buffers (more or less).
			std::sort(freeIndices.begin(), freeIndices.end(), std::greater<size_type>());

			// Sort ascendantly so that we run through all the allocated items
			// we will access them in sequential order in memory.
			std::sort(allocatedIndices.begin(), allocatedIndices.end());

			releasesBeforeSortingCounter = SORT_AFTER;
		}
	}

	void Clear()
	{
		itemsArray.clear();
		freeIndices.clear();
		allocatedIndices.clear();
		releasesBeforeSortingCounter = SORT_AFTER;
	}

	size_type Size() const
	{
		return allocatedIndices.size();
	}
	size_type GetIndex(size_t idx)
	{
		return allocatedIndices[idx];
	}

	TYPE&       operator[](size_type index)       { return itemsArray[index]; }
	const TYPE& operator[](size_type index) const { return itemsArray[index]; }

protected:
	std::vector<TYPE>      itemsArray;
	std::vector<size_type> freeIndices;
	std::vector<size_type> allocatedIndices;
	int                    releasesBeforeSortingCounter = SORT_AFTER;
};



template<class TYPE, int SORT_AFTER = 32> class ItemPoolRefCount : public ItemPool<TYPE, SORT_AFTER>
{
public:
	typedef ItemPool<TYPE, SORT_AFTER> base;
	typedef typename base::size_type size_type;

	void AddRef(size_type index)
	{
		CRY_ASSERT(index >= 0 && index < base::itemsArray.size());
		base::itemsArray[index].AddRef();
	}
	void DecRef(size_type index)
	{
		CRY_ASSERT(index >= 0 && index < base::itemsArray.size());
		TYPE& item = base::itemsArray[index];
		item.DecRef();

		CRY_ASSERT(item.GetRefCount() >= 0);
		if (item.GetRefCount() <= 0)
		{
			Release(index);
		}
	}
	void Release(size_type index)
	{
		CRY_ASSERT(index >= 0 && index < base::itemsArray.size());
		CRY_ASSERT(base::itemsArray[index].GetRefCount() == 0);
		ItemPool<TYPE, SORT_AFTER>::Release(index);
	}
};

class ItemCounted
{
public:
	ItemCounted()
		: refCount(0)
	{}

	size_t GetRefCount() { return refCount; }
	void   AddRef()      { ++refCount; }
	void   DecRef()      { --refCount; }

private:
	size_t refCount;
};

}   // AuditionHelpers

} // Perception
