// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace client
	{

		//===================================================================================
		//
		// CFactoryRegistrationHelper
		//
		// - registers all factories of item types, evaluators, generators, etc. in the IHub
		// - it's a convenient way for client code to register all factories in just one call
		// - should be called upon core::EHubEvent::RegisterYourFactoriesNow
		//
		//===================================================================================

		class CFactoryRegistrationHelper
		{
		public:
			static void RegisterAllFactoryInstancesInHub(core::IHub& hub);
		};

	}
}
