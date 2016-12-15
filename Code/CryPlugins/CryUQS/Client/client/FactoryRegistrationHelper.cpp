// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{

		void CFactoryRegistrationHelper::RegisterAllFactoryInstancesInHub(core::IHub& hub)
		{
			internal::CItemFactoryBase::RegisterAllInstancesInFactoryDatabase(hub.GetItemFactoryDatabase());
			internal::CFunctionFactoryBase::RegisterAllInstancesInFactoryDatabase(hub.GetFunctionFactoryDatabase());
			internal::CGeneratorFactoryBase::RegisterAllInstancesInFactoryDatabase(hub.GetGeneratorFactoryDatabase());
			internal::CInstantEvaluatorFactoryBase::RegisterAllInstancesInFactoryDatabase(hub.GetInstantEvaluatorFactoryDatabase());
			internal::CDeferredEvaluatorFactoryBase::RegisterAllInstancesInFactoryDatabase(hub.GetDeferredEvaluatorFactoryDatabase());
		}

	}
}
