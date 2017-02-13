// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "GlobalConstantParamsBlueprint.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CTextualGlobalConstantParamsBlueprint
		//
		//===================================================================================

		CTextualGlobalConstantParamsBlueprint::CTextualGlobalConstantParamsBlueprint()
		{
			// nothing
		}

		void CTextualGlobalConstantParamsBlueprint::AddParameter(const char* name, const char* type, const char* value, bool bAddToDebugRenderWorld, datasource::SyntaxErrorCollectorUniquePtr syntaxErrorCollector)
		{
			m_parameters.emplace_back(name, type, value, bAddToDebugRenderWorld, std::move(syntaxErrorCollector));
		}

		size_t CTextualGlobalConstantParamsBlueprint::GetParameterCount() const
		{
			return m_parameters.size();
		}

		ITextualGlobalConstantParamsBlueprint::SParameterInfo CTextualGlobalConstantParamsBlueprint::GetParameter(size_t index) const
		{
			assert(index < m_parameters.size());
			const SStoredParameterInfo& pi = m_parameters[index];
			return SParameterInfo(pi.name.c_str(), pi.type.c_str(), pi.value.c_str(), pi.bAddToDebugRenderWorld, pi.pSyntaxErrorCollector.get());
		}

		//===================================================================================
		//
		// CGlobalConstantParamsBlueprint
		//
		//===================================================================================

		CGlobalConstantParamsBlueprint::CGlobalConstantParamsBlueprint()
		{
			// nothing
		}

		CGlobalConstantParamsBlueprint::~CGlobalConstantParamsBlueprint()
		{
			// destroy all items (they have been created by Resolve())
			for (auto& pair : m_constantParams)
			{
				pair.second.pItemFactory->DestroyItems(pair.second.pItem);
			}
		}

		bool CGlobalConstantParamsBlueprint::Resolve(const ITextualGlobalConstantParamsBlueprint& source)
		{
			const size_t numParams = source.GetParameterCount();

			for (size_t i = 0; i < numParams; ++i)
			{
				const ITextualGlobalConstantParamsBlueprint::SParameterInfo p = source.GetParameter(i);

				// ensure each parameter exists only once
				if (m_constantParams.find(p.name) != m_constantParams.cend())
				{
					if (datasource::ISyntaxErrorCollector* pSE = p.pSyntaxErrorCollector)
					{
						pSE->AddErrorMessage("Duplicate parameter: '%s'", p.name);
					}
					return false;
				}

				// find the item factory
				client::IItemFactory* pItemFactory = g_hubImpl->GetItemFactoryDatabase().FindFactoryByName(p.type);
				if (!pItemFactory)
				{
					if (datasource::ISyntaxErrorCollector* pSE = p.pSyntaxErrorCollector)
					{
						pSE->AddErrorMessage("Unknown item type: '%s'", p.type);
					}
					return false;
				}

				if (!pItemFactory->CanBePersistantlySerialized())
				{
					if (datasource::ISyntaxErrorCollector* pSE = p.pSyntaxErrorCollector)
					{
						pSE->AddErrorMessage("Items of type '%s' cannot be represented in textual form", p.type);
					}
					return false;
				}

				IItemSerializationSupport& itemSerializationSupport = g_hubImpl->GetItemSerializationSupport();
				shared::CUqsString errorMessage;
				shared::IUqsString* pErrorMessage = (p.pSyntaxErrorCollector) ? &errorMessage : nullptr;
				void* pItem = pItemFactory->CreateItems(1, client::IItemFactory::EItemInitMode::UseDefaultConstructor);

				if (!itemSerializationSupport.DeserializeItemFromCStringLiteral(pItem, *pItemFactory, p.value, pErrorMessage))
				{
					if (datasource::ISyntaxErrorCollector* pSE = p.pSyntaxErrorCollector)
					{
						pSE->AddErrorMessage("Parameter '%s' could not be parsed from its serialized representation ('%s'). Reason:\n%s", p.name, p.value, errorMessage.c_str());
					}
					pItemFactory->DestroyItems(pItem);
					return false;
				}

				m_constantParams.insert(std::map<string, SParamInfo>::value_type(p.name, SParamInfo(pItemFactory, pItem, p.bAddToDebugRenderWorld)));  // notice: the item will get destroyed by CGlobalConstantParamsBlueprint's dtor
			}

			return true;
		}

		const std::map<string, CGlobalConstantParamsBlueprint::SParamInfo>& CGlobalConstantParamsBlueprint::GetParams() const
		{
			return m_constantParams;
		}

		void CGlobalConstantParamsBlueprint::AddSelfToDictAndReplace(shared::CVariantDict& out) const
		{
			for (const auto& pair : m_constantParams)
			{
				void* pClonedItem = pair.second.pItemFactory->CloneItem(pair.second.pItem);
				out.__AddOrReplace(pair.first.c_str(), *pair.second.pItemFactory, pClonedItem);
			}
		}

		void CGlobalConstantParamsBlueprint::PrintToConsole(CLogger& logger) const
		{
			if (m_constantParams.empty())
			{
				logger.Printf("Global constant params: (none)");
			}
			else
			{
				logger.Printf("Global constant params:");
				CLoggerIndentation _indent;
				CItemSerializationSupport& itemSerializationSupport = g_hubImpl->GetItemSerializationSupport();
				for (const auto& e : m_constantParams)
				{
					const char* paramName = e.first.c_str();
					const client::IItemFactory* pItemFactory = e.second.pItemFactory;
					shared::CUqsString itemAsString;
					assert(pItemFactory->CanBePersistantlySerialized()); // constant params can *always* be represented in textual form (how else should the query author provide them via e. g. an XML file?)
					itemSerializationSupport.SerializeItemToStringLiteral(e.second.pItem, *pItemFactory, itemAsString);
					logger.Printf("\"%s\" = %s [%s]", paramName, itemAsString.c_str(), pItemFactory->GetName());
				}
			}
		}

	}
}
