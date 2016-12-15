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

		void CTextualGlobalConstantParamsBlueprint::AddParameter(const char* name, const char* type, const char* value, datasource::SyntaxErrorCollectorUniquePtr syntaxErrorCollector)
		{
			m_parameters.emplace_back(name, type, value, std::move(syntaxErrorCollector));
		}

		size_t CTextualGlobalConstantParamsBlueprint::GetParameterCount() const
		{
			return m_parameters.size();
		}

		ITextualGlobalConstantParamsBlueprint::SParameterInfo CTextualGlobalConstantParamsBlueprint::GetParameter(size_t index) const
		{
			assert(index < m_parameters.size());
			const SStoredParameterInfo& pi = m_parameters[index];
			return SParameterInfo(pi.name.c_str(), pi.type.c_str(), pi.value.c_str(), pi.pSyntaxErrorCollector.get());
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

		bool CGlobalConstantParamsBlueprint::Resolve(const ITextualGlobalConstantParamsBlueprint& source)
		{
			const size_t numParams = source.GetParameterCount();

			for (size_t i = 0; i < numParams; ++i)
			{
				const ITextualGlobalConstantParamsBlueprint::SParameterInfo p = source.GetParameter(i);

				// ensure each parameter exists only once
				if (m_constantParams.Exists(p.name))
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
				if (!itemSerializationSupport.DeserializeItemIntoDictFromCStringLiteral(m_constantParams, p.name, *pItemFactory, p.value, pErrorMessage))
				{
					if (datasource::ISyntaxErrorCollector* pSE = p.pSyntaxErrorCollector)
					{
						pSE->AddErrorMessage("Parameter '%s' could not be parsed from its serialized representation ('%s'). Reason:\n%s", p.name, p.value, errorMessage.c_str());
					}
					return false;
				}
			}

			return true;
		}

		const shared::CVariantDict& CGlobalConstantParamsBlueprint::GetParams() const
		{
			return m_constantParams;
		}

		void CGlobalConstantParamsBlueprint::PrintToConsole(CLogger& logger) const
		{
			const std::map<string, shared::CVariantDict::SDataEntry>& entries = m_constantParams.GetEntries();

			if (entries.empty())
			{
				logger.Printf("Global constant params: (none)");
			}
			else
			{
				logger.Printf("Global constant params:");
				CLoggerIndentation _indent;
				CItemSerializationSupport& itemSerializationSupport = g_hubImpl->GetItemSerializationSupport();
				for (const auto& e : entries)
				{
					const char* paramName = e.first.c_str();
					const client::IItemFactory* pItemFactory = e.second.pItemFactory;
					shared::CUqsString itemAsString;
					assert(pItemFactory->CanBePersistantlySerialized()); // constant params can *always* be represented in textual form (how else should the query author provide them via e. g. an XML file?)
					itemSerializationSupport.SerializeItemToStringLiteral(e.second.pObject, *pItemFactory, itemAsString);
					logger.Printf("\"%s\" = %s [%s]", paramName, itemAsString.c_str(), pItemFactory->GetName());
				}
			}
		}

	}
}
