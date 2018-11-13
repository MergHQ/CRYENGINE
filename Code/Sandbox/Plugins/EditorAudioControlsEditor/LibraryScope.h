// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Common/SharedData.h"
#include <CryAudio/IAudioInterfacesCommonData.h>
#include <CrySystem/ISystem.h>
#include <CrySystem/XML/IXml.h>

namespace ACE
{
struct SLibraryScope final
{
	SLibraryScope()
	{
		pNodes[0] = GetISystem()->CreateXmlNode(CryAudio::s_szTriggersNodeTag);
		pNodes[1] = GetISystem()->CreateXmlNode(CryAudio::s_szParametersNodeTag);
		pNodes[2] = GetISystem()->CreateXmlNode(CryAudio::s_szSwitchesNodeTag);
		pNodes[3] = GetISystem()->CreateXmlNode(CryAudio::s_szEnvironmentsNodeTag);
		pNodes[4] = GetISystem()->CreateXmlNode(CryAudio::s_szPreloadsNodeTag);
		pNodes[5] = GetISystem()->CreateXmlNode(CryAudio::s_szSettingsNodeTag);
	}

	XmlNodeRef GetXmlNode(EAssetType const type) const
	{
		XmlNodeRef pNode = nullptr;

		switch (type)
		{
		case EAssetType::Trigger:
			pNode = pNodes[0];
			break;
		case EAssetType::Parameter:
			pNode = pNodes[1];
			break;
		case EAssetType::Switch:
			pNode = pNodes[2];
			break;
		case EAssetType::Environment:
			pNode = pNodes[3];
			break;
		case EAssetType::Preload:
			pNode = pNodes[4];
			break;
		case EAssetType::Setting:
			pNode = pNodes[5];
			break;
		default:
			pNode = nullptr;
			break;
		}

		return pNode;
	}

	XmlNodeRef pNodes[6]; // Trigger, Parameter, Switch, Environment, Preload, Setting
	bool       isDirty = false;
	uint32     numTriggers = 0;
	uint32     numParameters = 0;
	uint32     numSwitches = 0;
	uint32     numStates = 0;
	uint32     numEnvironments = 0;
	uint32     numPreloads = 0;
	uint32     numSettings = 0;
	uint32     numFiles = 0;
};
} // namespace ACE
