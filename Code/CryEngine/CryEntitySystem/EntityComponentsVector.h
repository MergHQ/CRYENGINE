#pragma once

#include <CryExtension/ICryFactoryRegistry.h>

// Implemented in a similar way to CListenerSet

// Forward decl.
template<typename T>
class CEntityComponentsIterator;

//! SEntityComponentRecord takes over ownership of the component and stores additional data.
//! Components will be shutdown and cleaned up.
struct SEntityComponentRecord
{
	int                               proxyType;            //!< Proxy id associated with this component (Only when component correspond to know proxy, -1 overwise)
	int                               eventPriority;        //!< Event dispatch priority
	EntityEventMask                   registeredEventsMask; //!< Bitmask of the EEntityEvent values
	CryInterfaceID                    typeId;               //!< Interface IDD for the registered component.
	std::shared_ptr<IEntityComponent> pComponent;           //!< Pointer to the owned component, Only the entity owns the component life time

	IEntityComponent* GetComponent() const
	{
		return pComponent.get();
	}

	int GetProxyType() const
	{
		return proxyType;
	}

	int GetEventPriority() const
	{
		return eventPriority;
	}

	// Mark the component was removed
	// This is done since we are not always able to remove the record from storage immediately
	void Invalidate()
	{
		pComponent.reset();
	}

	// Calls the shutdown function of the component
	void Shutdown();

	// Check whether the component has been removed
	bool IsValid() const { return pComponent != nullptr; }
};

//! SMinimalEntityComponent is used to store component without taking ownership of them.
//! On delete the component won't be shutdown or deleted, just the entry in the vector will be remove.
//! This struct should be used if components should be stored temporarily.
struct SMinimalEntityComponentRecord
{
	IEntityComponent* pComponent;

	IEntityComponent* GetComponent() const
	{
		return pComponent;
	}

	int GetProxyType() const
	{
		return pComponent->GetProxyType();
	}

	int GetEventPriority() const
	{
		return pComponent->GetEventPriority();
	}

	void Invalidate()
	{
		pComponent = nullptr;
	}

	// Don't shutdown the component since this struct doesn't take ownership of the components
	void Shutdown()
	{
	}

	// Check whether the component has been removed
	bool IsValid() const { return pComponent != nullptr; }
};

//! Main component collection class used in conjunction with CEntityComponentsIterator.
template<typename TRecord>
class CEntityComponentsVector
{
private:
	std::vector<TRecord> m_vector;   //!< Collection of unique elements.
	size_t               m_unsortedElements;      //!< Number of unsorted entries in the vector
	int                  m_activeScopes : 29;     //!< Counts current iteration scopes (cleanup cannot occur unless this is 0).
	int                  m_cleanupRequired : 1;   //!< Indicates NULL elements in listener.
	int                  m_sortingValid : 1;      //!< Indicates that sorting order of the elements maybe not be valid.
	CryCriticalSection   m_lock;

public:
	//! \note No default constructor in favor of forcing users to provide an expected capacity.
	CEntityComponentsVector() : m_activeScopes(0), m_cleanupRequired(false), m_sortingValid(false), m_unsortedElements(0) {}
	~CEntityComponentsVector()
	{
		CRY_ASSERT(m_activeScopes == 0);
		CRY_ASSERT(!m_cleanupRequired);
	};

	const std::vector<TRecord>& GetVector() const { return m_vector; }

	//! Appends a component to the end of the collection.
	TRecord& Add()
	{
		CryAutoCriticalSection lock(m_lock);

		m_vector.emplace_back();
		m_sortingValid = false;

		return m_vector.back();
	}

	//! Sorted insertion based on the event priority
	//! \param componentRecord Component record which should be inserted into the collection
	void SortedEmplace(TRecord&& componentRecord)
	{
		CryAutoCriticalSection lock(m_lock);

		// Add the component to the back of the vector
		// The component will be sorted later on
		m_vector.emplace_back(componentRecord);

		m_unsortedElements++;

		m_sortingValid = false;
	}

	//! Removes a component from the collection.
	void Remove(IEntityComponent* pComponent)
	{
		CryAutoCriticalSection lock(m_lock);

		TRecord tempComponentRecord;

		auto endIter = m_vector.end();
		auto iter = m_vector.end();
		for (iter = m_vector.begin(); iter != m_vector.end(); ++iter)
		{
			if (iter->GetComponent() == pComponent)
			{
				tempComponentRecord = *iter;

				// If no iteration in progress
				if (m_activeScopes == 0)
				{
					// check if we remove a unsorted element
					if (iter >= endIter - m_unsortedElements)
					{
						m_unsortedElements--;
					}

					// Just delete the entry immediately
					m_sortingValid = false;
					m_vector.erase(iter);
				}
				else  // Iteration(s) in progress, cannot re-order vector
				{
					m_sortingValid = false;
					// Mark for cleanup
					iter->Invalidate();
					m_cleanupRequired = true;
				}

				break;
			}
		}

		if (tempComponentRecord.IsValid())
		{
			tempComponentRecord.Shutdown();
		}
	}

	//! Removes all components from the collection
	void Clear()
	{
		CRY_ASSERT(m_activeScopes == 0);
		CRY_ASSERT(!m_cleanupRequired);
		CryAutoCriticalSection lock(m_lock);

		//////////////////////////////////////////////////////////////////////////
		// ShutDown all components.
		TRecord legacyGameObjectComponentRecord;

		ForEach([this, &legacyGameObjectComponentRecord](TRecord& rec) -> bool
		{
			if (rec.IsValid())
			{
				if (rec.GetProxyType() == ENTITY_PROXY_USER)
				{
					// User component must be deleted last after all other components are destroyed (Required by CGameObject)
					legacyGameObjectComponentRecord = rec;
				}

				rec.Shutdown();
			}

			return true;
		});

		// Remove all entity components in a temporary vector, in case any components try to modify components in their destructor.
		auto tempComponents = std::move(m_vector);
		tempComponents.clear();

		m_unsortedElements = 0;
	}

	//! Reserves space to help avoid runtime reallocation.
	void Reserve(size_t capacity)
	{
		CryAutoCriticalSection lock(m_lock);
		m_vector.reserve(capacity);
	}

	size_t Size() const
	{
		return m_vector.size();
	}

	//! Invokes the provided function f once with each component.
	template<typename LambdaFunction>
	void ForEach(LambdaFunction f)
	{
		CryAutoCriticalSection lock(m_lock);
		++m_activeScopes;

		// In case the vector isn't sorted
		if (m_unsortedElements != 0)
		{
			for (; m_unsortedElements != 0; --m_unsortedElements)
			{
				TRecord& record = m_vector.back();

				auto upperBoundIt = std::upper_bound(m_vector.begin(), m_vector.end() - m_unsortedElements, record, [](const TRecord& a, const TRecord& b) -> bool
				{
					return a.GetEventPriority() < b.GetEventPriority();
				});

				m_vector.insert(upperBoundIt, record);

				m_vector.pop_back();
			}

			m_sortingValid = true;
		}

		// Iterate only until current count, ignore new elements added during the loop
		for (size_t index = 0; index < m_vector.size(); ++index)
		{
			TRecord& rec = m_vector[index];
			if (rec.GetComponent())
			{
				if (!f(rec))
				{
					break;
				}
			}
		}

		// End the scope
		// Ensure at least one notification scope was started
		CRY_ASSERT(m_activeScopes > 0);

		// If this is the last notification, remove null elements
		if (--m_activeScopes == 0 && m_cleanupRequired)
		{
			// Shuffles all elements != value to the front and returns the start of the removed elements.
			auto newEndIter = std::remove_if(m_vector.begin(), m_vector.end(),
			                                 [](const TRecord& rec) { return rec.GetComponent() == nullptr; }
			                                 );
			// Delete the removed range at the back of the container (low-cost for vector).
			m_vector.erase(newEndIter, m_vector.end());

			m_sortingValid = false;
			m_cleanupRequired = false;
		}
	}

	// Iterate through all components, assumes that m_vector will not be changed during iteration!
	template<typename LambdaFunction>
	void ForEachUnchecked(LambdaFunction f) const
	{
		CryAutoCriticalSection lock(m_lock);

		for (size_t index = 0; index < m_vector.size(); ++index)
		{
			const TRecord& rec = m_vector[index];
			if (rec.GetComponent())
			{
				f(rec);
			}
		}
	}

	//! Sorts the components based on their event priority.
	//! \param forceSorting Forces sorting even when the list might already be sorted
	void Sort(bool forceSorting = false)
	{
		CryAutoCriticalSection lock(m_lock);

		if (m_sortingValid == false || forceSorting == true)
		{
			std::stable_sort(m_vector.begin(), m_vector.end(), [](const TRecord& p1, const TRecord& p2)
			{
				return p1.GetEventPriority() < p2.GetEventPriority();
			});

			m_sortingValid = true;
			m_unsortedElements = 0;
		}
	}
};
