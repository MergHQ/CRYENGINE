// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		//===================================================================================
		//
		// IFactoryDatabase<>
		//
		//===================================================================================

		template <class TFactory>
		struct IFactoryDatabase
		{
			virtual               ~IFactoryDatabase() {}
			virtual void          RegisterFactory(TFactory* pFactoryToRegister, const char* name) = 0;
			virtual TFactory*     FindFactoryByName(const char* name) const = 0;
			virtual size_t        GetFactoryCount() const = 0;
			virtual TFactory&     GetFactory(size_t index) const = 0;
		};

	}
}
