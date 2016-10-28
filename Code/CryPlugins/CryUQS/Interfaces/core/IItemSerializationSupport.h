// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// IItemSerializationSupport
		//
		// - provides a support to serialize items to textual representation and deserialize them back
		//
		//===================================================================================

		struct IItemSerializationSupport
		{
			virtual                          ~IItemSerializationSupport() {}
			
			virtual bool                     DeserializeItemFromCStringLiteral(void* pOutItem, const client::IItemFactory& itemFactory, const char* szItemLiteral, shared::IUqsString* pErrorMessage) const = 0;
			virtual bool                     DeserializeItemIntoDictFromCStringLiteral(shared::IVariantDict& out, const char* szKey, client::IItemFactory& itemFactory, const char* szItemLiteral, shared::IUqsString* pErrorMessage) const = 0;

			virtual bool                     SerializeItemToStringLiteral(const void* pItem, const client::IItemFactory& itemFactory, shared::IUqsString& outString) const = 0;
		};

	}
}
