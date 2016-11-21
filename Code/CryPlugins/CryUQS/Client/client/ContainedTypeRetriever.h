// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{
		namespace internal
		{

			//===================================================================================
			//
			// SContainedTypeRetriever
			//
			// - this is a helper for CFunctionFactory<>::GetContainedType()
			// - it's used to retrieve the underlying type of items that have been shuttled from one query to another
			// - also see client::internal::CFunc_ShuttledItems<> and observe that it's making use of CItemListProxy_Readable<>, hence the template specialization for it
			//
			//===================================================================================

			template <class TItem>
			struct SContainedTypeRetriever
			{
				static const shared::CTypeInfo* GetTypeInfo()
				{
					return nullptr;
				}
			};

			template <class TItem>
			struct SContainedTypeRetriever<CItemListProxy_Readable<TItem>>
			{
				static const shared::CTypeInfo* GetTypeInfo()
				{
					static const shared::CTypeInfo& type = shared::SDataTypeHelper<TItem>::GetTypeInfo();
					return &type;
				}
			};

		}
	}
}
