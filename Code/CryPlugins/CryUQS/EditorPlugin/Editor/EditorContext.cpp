// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EditorContext.h"

#include "QueryListProvider.h"

//////////////////////////////////////////////////////////////////////////

template<typename TFactoryDb, typename TFilterFunc>
static void CUqsDatabaseSerializationCache::BuildNameStringListWithFilter(const TFactoryDb& factoryDb, TFilterFunc filterFunc, Serialization::StringList& outNamesList)
{
	outNamesList.clear();

	const size_t count = factoryDb.GetFactoryCount();
	outNamesList.reserve(count + 1);

	outNamesList.push_back("");

	for (size_t i = 0; i < count; ++i)
	{
		const auto& factory = factoryDb.GetFactory(i);
		if (filterFunc(factory))
		{
			outNamesList.push_back(factory.GetName());
		}
	}
}

struct SEmptyFilterFunc
{
	template<typename T>
	bool operator()(const T&) { return true; }
};

template<typename TFactoryDb>
static void CUqsDatabaseSerializationCache::BuildNameStringList(const TFactoryDb& factoryDb, Serialization::StringList& outNamesList)
{
	BuildNameStringListWithFilter(factoryDb, SEmptyFilterFunc(), outNamesList);
}

const Serialization::StringList& CUqsDatabaseSerializationCache::GetQueryFactoryNamesList() const
{
	if (m_queryFactoryNamesList.empty())
	{
		const auto& db = uqs::core::IHubPlugin::GetHub().GetQueryFactoryDatabase();
		// TODO pavloi 2015.12.08: fix const_cast inside lazy getter
		BuildNameStringList(db, const_cast<Serialization::StringList&>(m_queryFactoryNamesList));
	}

	return m_queryFactoryNamesList;
}

const Serialization::StringList& CUqsDatabaseSerializationCache::GetItemTypeNamesList() const
{
	if (m_itemTypeNamesList.empty())
	{
		const auto& db = uqs::core::IHubPlugin::GetHub().GetItemFactoryDatabase();
		// TODO pavloi 2015.12.08: fix const_cast inside lazy getter
		BuildNameStringList(db, const_cast<Serialization::StringList&>(m_itemTypeNamesList));
	}

	return m_itemTypeNamesList;
}

const Serialization::StringList& CUqsDatabaseSerializationCache::GetGeneratorNamesList() const
{
	if (m_generatorNamesList.empty())
	{
		const auto& db = uqs::core::IHubPlugin::GetHub().GetGeneratorFactoryDatabase();
		// TODO pavloi 2015.12.08: fix const_cast inside lazy getter
		BuildNameStringList(db, const_cast<Serialization::StringList&>(m_generatorNamesList));
	}

	return m_generatorNamesList;
}

const Serialization::StringList& CUqsDatabaseSerializationCache::GetFunctionNamesList() const
{
	if (m_functionNamesList.empty())
	{
		const auto& db = uqs::core::IHubPlugin::GetHub().GetFunctionFactoryDatabase();
		// TODO pavloi 2015.12.08: fix const_cast inside lazy getter
		BuildNameStringList(db, const_cast<Serialization::StringList&>(m_functionNamesList));
	}

	return m_functionNamesList;
}

const Serialization::StringList& CUqsDatabaseSerializationCache::GetFunctionNamesList(const SItemTypeName& typeToFilter) const
{
	if (typeToFilter.Empty())
	{
		return GetFunctionNamesList();
	}

	if (m_lastTypeToFilterFunctionNames != typeToFilter)
	{
		// TODO pavloi 2015.12.08: fix const_cast inside lazy getter
		const_cast<CUqsDatabaseSerializationCache*>(this)->BuildFilteredFunctionNamesList(typeToFilter);
	}

	return m_filteredFunctionNamesList;
}

const Serialization::StringList& CUqsDatabaseSerializationCache::GetEvaluatorNamesList() const
{
	if (m_evaluatorNamesList.empty())
	{
		// TODO pavloi 2015.12.08: fix const_cast inside lazy getter
		const_cast<CUqsDatabaseSerializationCache*>(this)->BuildEvaluatorNamesList();
	}

	return m_evaluatorNamesList;
}

SItemTypeName CUqsDatabaseSerializationCache::GetItemTypeNameFromType(const uqs::shared::CTypeInfo& typeInfo) const
{
	if (m_typeInfoToName.empty())
	{
		// TODO pavloi 2015.12.11: fix const_cast inside lazy getter
		const_cast<CUqsDatabaseSerializationCache*>(this)->BuildTypeInfoToNameMap();
	}

	auto iter = m_typeInfoToName.find(typeInfo);
	if (iter != m_typeInfoToName.end())
	{
		return iter->second;
	}
	else
	{
		return SItemTypeName(typeInfo.name());
	}
}

void CUqsDatabaseSerializationCache::BuildEvaluatorNamesList()
{
	m_instantEvaluatorNamesList.clear();
	m_deferredEvaluatorNamesList.clear();

	{
		const auto& db = uqs::core::IHubPlugin::GetHub().GetInstantEvaluatorFactoryDatabase();
		BuildNameStringList(db, m_instantEvaluatorNamesList);
	}
	{
		const auto& db = uqs::core::IHubPlugin::GetHub().GetDeferredEvaluatorFactoryDatabase();
		BuildNameStringList(db, m_deferredEvaluatorNamesList);
	}

	m_evaluatorNamesList = m_instantEvaluatorNamesList;
	if (m_deferredEvaluatorNamesList.size() > 1)
	{
		m_evaluatorNamesList.insert(m_evaluatorNamesList.end(), m_deferredEvaluatorNamesList.begin() + 1, m_deferredEvaluatorNamesList.end());
	}
}

void CUqsDatabaseSerializationCache::BuildFilteredFunctionNamesList(const SItemTypeName& typeToFilter)
{
	const auto& db = uqs::core::IHubPlugin::GetHub().GetFunctionFactoryDatabase();

	// TODO pavloi 2015.12.14: save filtered lists for different types
	auto filter = [this, &typeToFilter](const uqs::client::IFunctionFactory& factory)
	{
		return (GetItemTypeNameFromType(factory.GetReturnType()) == typeToFilter);
	};

	BuildNameStringListWithFilter(db, filter, m_filteredFunctionNamesList);
	m_lastTypeToFilterFunctionNames = typeToFilter;
}

void CUqsDatabaseSerializationCache::BuildTypeInfoToNameMap()
{
	m_typeInfoToName.clear();

	const auto& db = uqs::core::IHubPlugin::GetHub().GetItemFactoryDatabase();
	const size_t itemFactoryCount = db.GetFactoryCount();
	for (size_t i = 0; i < itemFactoryCount; ++i)
	{
		const auto& factory = db.GetFactory(i);
		const uqs::shared::CTypeInfo& typeInfo = factory.GetItemType();

		m_typeInfoToName[typeInfo] = SItemTypeName(factory.GetName());
	}
}

//////////////////////////////////////////////////////////////////////////

CUqsEditorContext::CUqsEditorContext()
{
	m_pQueryListProvider.reset(
	  new CQueryListProvider(uqs::core::IHubPlugin::GetHub().GetEditorLibraryProvider(), *this));
	m_pQueryListProvider->Populate();
}

CQueryListProvider& CUqsEditorContext::GetQueryListProvider() const
{
	if (!m_pQueryListProvider)
	{
		CryFatalError("m_pQueryListProvider is not created");
	}
	return *m_pQueryListProvider;
}

const SDocumentSettings& CUqsEditorContext::GetSettings() const
{
	return m_docSettings;
}

SDocumentSettings& CUqsEditorContext::GetSettings()
{
	return m_docSettings;
}

const CUqsDatabaseSerializationCache& CUqsEditorContext::GetSerializationCache() const
{
	return m_uqsDatabaseSerializationCache;
}
