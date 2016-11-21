// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

#include <CryAISystem/INavigationSystem.h>

namespace uqs
{
	namespace stdlib
	{

		//===================================================================================
		//
		// string-conversion functions for item types that can have a textual representation
		//
		//===================================================================================

		// int
		bool Int_Serialize(Serialization::IArchive& archive, int& value, const char* szName, const char* szLabel);

		// bool
		bool Bool_Serialize(Serialization::IArchive& archive, bool& value, const char* szName, const char* szLabel);

		// float
		bool Float_Serialize(Serialization::IArchive& archive, float& value, const char* szName, const char* szLabel);

		// Vec3
		bool Vec3_Serialize(Serialization::IArchive& archive, Vec3& value, const char* szName, const char* szLabel);

		// NavigationAgentTypeID
		bool NavigationAgentTypeID_Serialize(Serialization::IArchive& archive, NavigationAgentTypeID& value, const char* szName, const char* szLabel);

	}
}
