// Copyright 2001-2019 Crytek GmbH. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"

template<class IndexT>
struct SObjectIndexValidator
{
	static bool IsValid(const IndexT& index) { return true; }
};

template<>
struct SObjectIndexValidator<string>
{
	static bool IsValid(const string& index) { return !index.empty(); }
};

template<class IndexT, class ObjT, class HashT = std::hash<IndexT>, class EqT = std::equal_to<IndexT>>
class CObjectIndexContainerBase
{
public:
	//! Removes the index from the object.
	void RemoveIndex(const IndexT& index, const ObjT& obj)
	{
		if (!SObjectIndexValidator<IndexT>::IsValid(index)) return;
		auto it = Find(index, obj);
		if (it != m_files.end())
		{
			m_files.erase(it);
			return;
		}
	}

	//! Returns the number of objects that have the index.
	int GetIndexCount(const IndexT& index) const
	{
		if (index.empty()) return 0;
		return m_files.count(index);
	}

	//! Returns the list of objects that have the index.
	std::vector<ObjT*> GetObjectsForIndex(const IndexT& index) const
	{
		if (!SObjectIndexValidator<IndexT>::IsValid(index)) return {};
		std::vector<ObjT*> objects;
		auto range = m_files.equal_range(index);
		objects.reserve(std::distance(range.first, range.second));
		for (auto it = range.first; it != range.second; ++it)
		{
			objects.push_back(const_cast<ObjT*>(it->second));
		}
		return objects;
	}

	//! Iterates through the list of all indices.
	void ForEachIndex(std::function<void(const IndexT& index, int count)> f)
	{
		auto it = m_files.cbegin();
		while (it != m_files.cend())
		{
			auto range = m_files.equal_range(it->first);
			f(it->first, std::distance(range.first, range.second));
			it = range.second;
		}
	}

	//! Returns the list of objects that have the index.
	//! This method is slightly more expensive than the one without list of objects.
	void ForEachIndex(std::function<void(const IndexT& index, const std::vector<ObjT*>& objects)> f)
	{
		auto it = m_files.cbegin();
		std::vector<ObjT*> objects;
		objects.reserve(10);
		while (it != m_files.cend())
		{
			auto range = m_files.equal_range(it->first);
			objects.reserve(std::distance(range.first, range.second));
			std::transform(range.first, range.second, std::back_inserter(objects), [](const auto& it)
			{
				return const_cast<ObjT*>(it.second);
			});
			f(it->first, objects);
			objects.clear();
			it = range.second;
		}
	}

protected:
	using ObjectsFilesMap = std::unordered_multimap<IndexT, const ObjT*, HashT, EqT>;

	typename ObjectsFilesMap::iterator Find(const IndexT& index, const ObjT& obj)
	{
		auto range = m_files.equal_range(index);
		auto it = std::find_if(range.first, range.second, [&obj](const auto& pair)
		{
			return pair.second == &obj;
		});
		return it != range.second ? it : m_files.end();
	}

	ObjectsFilesMap m_files;
};

//! This class provides an effective way of attaching index to objects and finding them by indices.
template <class IndexT, class ObjT, bool isOnePerObject, class HashT = std::hash<IndexT>, class EqT = std::equal_to<IndexT>>
class CObjectIndexContainer : public CObjectIndexContainerBase<IndexT, ObjT, HashT, EqT>
{
public:
	using GetIndicesFunc = std::function<std::vector<IndexT>(const ObjT&)>;

	using Base = CObjectIndexContainerBase<IndexT, ObjT, HashT, EqT>;

	CObjectIndexContainer(GetIndicesFunc f)
		: m_getIndicesFunc(std::move(f))
	{}

	//! Add an index to the object.
	void AddIndex(const IndexT& index, const ObjT& obj)
	{
		if (!SObjectIndexValidator<IndexT>::IsValid(index)) return;
		auto it = Base::Find(index, obj);
		if (it == Base::m_files.end())
		{
			Base::m_files.insert(std::make_pair(index, &obj));
		}
	}

	//! Set indices to the object. Removes existing ones.
	void SetIndices(const std::vector<IndexT>& indices, const ObjT& obj)
	{
		RemoveIndices(m_getIndicesFunc(obj), obj);
		for (const IndexT& index : indices)
		{
			if (SObjectIndexValidator<IndexT>::IsValid(index))
			{
				Base::m_files.insert(std::make_pair(index, &obj));
			}
		}
	}

	//! Removes indices attached to the object.
	void RemoveIndices(const std::vector<IndexT>& indices, const ObjT& obj)
	{
		for (const IndexT& index : indices)
		{
			Base::RemoveIndex(index, obj);
		}
	}

private:
	GetIndicesFunc m_getIndicesFunc;
};

// This specialization support only one index per object.
template <class IndexT, class ObjT, class HashT, class EqT>
class CObjectIndexContainer<IndexT, ObjT, true, HashT, EqT> : public CObjectIndexContainerBase<IndexT, ObjT, HashT, EqT>
{
public:
	using GetIndexFunc = std::function<IndexT(const ObjT&)>;

	using Base = CObjectIndexContainerBase<IndexT, ObjT, HashT, EqT>;

	CObjectIndexContainer(GetIndexFunc f)
		: m_getIndexFunc(std::move(f))
	{}

	//! Sets the index to the object. Remove the existing one.
	void SetIndex(const IndexT& index, const ObjT& obj)
	{
		Base::RemoveIndex(m_getIndexFunc(obj), obj);
		if (SObjectIndexValidator<IndexT>::IsValid(index))
		{
			Base::m_files.insert(std::make_pair(index, &obj));
		}
	}

	GetIndexFunc m_getIndexFunc;
};
