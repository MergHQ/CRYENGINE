// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CrySystem/IEngineModule.h>
#include <CrySystem/ISystem.h>

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace Cry
{
	namespace UDR
	{
		struct IHub;

		// TODO: Split Module from System.
		struct IUDR : public Cry::IDefaultModule
		{
			CRYINTERFACE_DECLARE_GUID(IUDR, "560ddced-bd31-44c0-96ba-550cc1317a39"_cry_guid);

			struct SConfig
			{
				//! Specifies if, while in Sandbox, UDR data should be reset when user switches to game mode.
				bool resetDataOnSwitchToGameMode = false;
			};
			
			//! Set UDR module configuration.
			//! \param config To be set.
			//! \return True if successfully set. False otherwise.
			virtual bool            SetConfig(const SConfig& config) = 0;

			//! Get current configuration of the UDR Module.
			//! \return Current configuration.
			virtual const SConfig&  GetConfig() const = 0;

			//! Resets the UDR module.
			//! /note This will cause UDR to wipe all existing recorded data and reset the base time. It does not change the module configuration.
			virtual void            Reset() = 0;

			//! Gets the elapsed time since UDR was created or reset.
			//! \return Elapsed time since UDR was created or reset.
			virtual CTimeValue      GetElapsedTime() const = 0;
			virtual Cry::UDR::IHub& GetHub() = 0;
		};

	}
}

