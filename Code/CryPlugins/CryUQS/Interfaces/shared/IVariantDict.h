// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client { struct IItemFactory; }

	namespace shared
	{

		struct IVariantDict
		{
			virtual                             ~IVariantDict() {}
			virtual void                        AddOrReplace(const char* szKey, client::IItemFactory& itemFactory, const void* pItemToClone) = 0;
			virtual void                        AddSelfToOtherAndReplace(IVariantDict& out) const = 0;
			virtual bool                        Exists(const char* key) const = 0;
			virtual client::IItemFactory*       FindItemFactory(const char* key) const = 0;
			virtual bool                        FindItemFactoryAndObject(const char* key, client::IItemFactory* &outItemItemFactory, void* &outObject) const = 0;
		};

	}
}
