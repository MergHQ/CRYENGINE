// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// IUtils
		//
		//===================================================================================

		struct IUtils
		{
			virtual                          ~IUtils() {}
			virtual client::IItemFactory*    FindItemFactoryByType(const shared::CTypeInfo& type) const = 0;
#if UQS_SCHEMATYC_SUPPORT
			virtual client::IItemFactory*    FindItemFactoryBySchematycTypeName(const Schematyc::CTypeName& schematycTypeNameToSearchFor) const = 0;
#endif
		};

	}
}
