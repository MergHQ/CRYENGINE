// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

////////////////////////////////////////////////////////////////////////////////////////

template<const int TrackLatency = DEVRES_TRACK_LATENCY>
class SUsageTrackedItem
{
public:
	SUsageTrackedItem() { MarkUsed(); }
	SUsageTrackedItem(uint32 lastUseFrame) : m_lastUseFrame(lastUseFrame) {}

	void MarkUsed()
	{
		m_lastUseFrame = GetDeviceObjectFactory().GetCurrentFrameCounter() + TrackLatency;
	}

	bool IsInUse() const
	{
		return m_lastUseFrame > GetDeviceObjectFactory().GetCompletedFrameCounter();
	}

protected:
	uint32 m_lastUseFrame;
};

////////////////////////////////////////////////////////////////////////////////////////
// NOTE: this container is expected to hold very few (<3) items usually, so a linear search during allocate should be fine here
template<typename T>
class CTrackedItemAllocatorBase
{
public:
	CTrackedItemAllocatorBase()
	{}

	T* FindUnusedItem()
	{
		for (auto& item : m_items)
		{
			if (item.bAvailable && !item.pItem->IsInUse())
			{
				item.bAvailable = false;
				return item.pItem;
			}
		}
		return nullptr;
	}

	void Release(T*& pItem)
	{
		for (auto& storedItem : m_items)
		{
			if (storedItem.pItem == pItem)
			{
				CRY_ASSERT(!storedItem.bAvailable); // check for double release
				storedItem.bAvailable = true;

				break;
			}
		}

		pItem = nullptr;
	}

	size_t GetItemCount() const
	{
		return m_items.size();
	}

protected:
	T* AddItem(T* pItem)
	{
		m_items.emplace_back(pItem);
		return pItem;
	}

	void ResetBase(std::function<void(T*&)> deleteItem)
	{
		for (auto& storedItem : m_items)
			deleteItem(storedItem.pItem);

		m_items.clear();
	}

	struct SItem
	{
		SItem(T* pItem)
			: bAvailable(false)
			, pItem(pItem)
		{}

		bool bAvailable;
		T*   pItem;
	};

	std::vector<SItem>   m_items;

private:
	CTrackedItemAllocatorBase(const CTrackedItemAllocatorBase<T>&);
	CTrackedItemAllocatorBase<T>& operator=(const CTrackedItemAllocatorBase<T>&);
};

template<typename T>
class CTrackedItemAllocator : public CTrackedItemAllocatorBase<T>
{
public:
	CTrackedItemAllocator() {}
	~CTrackedItemAllocator() { Reset(); }

	void Reset()
	{
		auto deleteLambda = [](T*& pItem) { delete pItem; };
		this->ResetBase(deleteLambda);
	}

	// workaround for the lack of variadic templates in visual studio 2012
#define TRY_RETURN_UNUSED_BUFFER            \
  if (auto* pItem = this->FindUnusedItem()) \
  {                                         \
    if (pNewAlloc) * pNewAlloc = false;     \
    return pItem;                           \
  }                                         \

#define RETURN_NEW_BUFFER(item)      \
  if (pNewAlloc) * pNewAlloc = true; \
  return this->AddItem((item));      \

	T*                                    Allocate(bool* pNewAlloc = nullptr)                   { TRY_RETURN_UNUSED_BUFFER; RETURN_NEW_BUFFER(new T()); }
	template<typename A0> T*              Allocate(A0&& a0, bool* pNewAlloc = nullptr)          { TRY_RETURN_UNUSED_BUFFER; RETURN_NEW_BUFFER(new T(std::forward<A0>(a0))); }
	template<typename A0, typename A1> T* Allocate(A0&& a0, A1&& a1, bool* pNewAlloc = nullptr) { TRY_RETURN_UNUSED_BUFFER; RETURN_NEW_BUFFER(new T(std::forward<A0>(a0), std::forward<A1>(a1))); }

#undef TRY_RETURN_UNUSED_BUFFER
#undef RETURN_NEW_BUFFER

};

template<typename T>
class CTrackedItemCustomAllocator : public CTrackedItemAllocatorBase<T>
{
public:
	typedef std::function<T*()>      CustomAllocFunction;
	typedef std::function<void(T*&)> CustomDeleteFunction;

	CTrackedItemCustomAllocator(CustomAllocFunction customAlloc, CustomDeleteFunction customDelete)
		: m_customAllocate(customAlloc)
		, m_customDelete(customDelete)
	{}

	~CTrackedItemCustomAllocator()
	{
		Reset();
	}
	
	void Reset()
	{
		this->ResetBase(m_customDelete);
	}

	T* Allocate(bool* pNewAlloc = nullptr)
	{
		if (auto* pItem = this->FindUnusedItem())
		{
			if (pNewAlloc) *pNewAlloc = false;
			CRY_ASSERT_MESSAGE(!!pItem, "Allocation found a nullptr on the list of available allocations!");
			return pItem;
		}

		if (pNewAlloc) *pNewAlloc = true;
		T* pItem = m_customAllocate();
		CRY_ASSERT_MESSAGE(!!pItem, "Custom allocation lambda returns nullptr!");
		return this->AddItem(pItem);
	}

	void SetAllocFunctions(CustomAllocFunction customAllocate, CustomDeleteFunction customDelete)
	{
		m_customAllocate = customAllocate;
		m_customDelete = customDelete;
	}
protected:
	CustomAllocFunction  m_customAllocate;
	CustomDeleteFunction m_customDelete;
};
