// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/IEngineModule.h>
#include <CrySystem/ISystem.h>

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{

		struct IUDR : public Cry::IDefaultModule
		{
			CRYINTERFACE_DECLARE_GUID(IUDR, "560ddced-bd31-44c0-96ba-550cc1317a39"_cry_guid);

			virtual Cry::UDR::IHub& GetHub() = 0;
		};

	}
}

