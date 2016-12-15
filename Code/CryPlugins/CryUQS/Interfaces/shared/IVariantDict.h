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
			virtual void                        __AddOrReplace(const char* key, client::IItemFactory& itemFactory, void* pObject) = 0;  // this method can easily be used in a wrong way (passing in an object that was not created via given item-factory), but needs to be public, hence prefixed with underscores to discourage direct usage by client code
			virtual void                        AddSelfToOtherAndReplace(IVariantDict& out) const = 0;
			virtual bool                        Exists(const char* key) const = 0;
			virtual client::IItemFactory*       FindItemFactory(const char* key) const = 0;
			virtual bool                        FindItemFactoryAndObject(const char* key, client::IItemFactory* &outItemItemFactory, void* &outObject) const = 0;
		};

	}
}
