// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "StdAfx.h"
#include "AudioSystemEditor_wwise.h"
#include "AudioSystemControl_wwise.h"
#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>
#include <CryCore/CryCrc32.h>
#include <ACETypes.h>
#include <CryString/CryPath.h>
#include <CryCore/StlUtils.h>
#include <CrySerialization/IArchiveHost.h>
#include "AudioWwiseLoader.h"

namespace ACE
{

const string g_userSettingsFile = "%USER%/audiocontrolseditor_wwise.user";

class CStringAndHash
{
	//a small helper class to make comparison of the stored c-string faster, by checking a hash-representation first.
public:
	CStringAndHash(const char* pText) : szText(pText), hash(CCrc32::Compute(pText)) {}
	operator string() const { return szText; }
	operator const char*() const { return szText; }
	bool operator ==(const CStringAndHash& other) { return (hash == other.hash && strcmp(szText, other.szText) == 0); }

private:
	const char* szText;
	const uint32 hash;
};

// XML tags
const CStringAndHash g_switchTag = "WwiseSwitch";
const CStringAndHash g_stateTag = "WwiseState";
const CStringAndHash g_fileTag = "WwiseFile";
const CStringAndHash g_rtpcTag = "WwiseRtpc";
const CStringAndHash g_eventTag = "WwiseEvent";
const CStringAndHash g_auxBusTag = "WwiseAuxBus";
const CStringAndHash g_valueTag = "WwiseValue";

// XML attributes
const string g_nameAttribute = "wwise_name";
const string g_shiftAttribute = "wwise_value_shift";
const string g_multAttribute = "wwise_value_multiplier";
const string g_valueAttribute = "wwise_value";
const string g_localizedAttribute = "wwise_localised";
const string g_trueAttribute = "true";
const string g_soundBanksInfoFilename = "SoundbanksInfo.xml";

ItemType TagToType(const char* szTag)
{
	CStringAndHash hashedTag(szTag);
	if (hashedTag == g_eventTag)
	{
		return eWwiseItemTypes_Event;
	}
	if (hashedTag == g_fileTag)
	{
		return eWwiseItemTypes_SoundBank;
	}
	if (hashedTag == g_rtpcTag)
	{
		return eWwiseItemTypes_Rtpc;
	}
	if (hashedTag == g_switchTag)
	{
		return eWwiseItemTypes_SwitchGroup;
	}
	if (hashedTag == g_stateTag)
	{
		return eWwiseItemTypes_StateGroup;
	}
	if (hashedTag == g_auxBusTag)
	{
		return eWwiseItemTypes_AuxBus;
	}
	return eWwiseItemTypes_Invalid;
}

string TypeToTag(const ItemType type)
{
	switch (type)
	{
	case eWwiseItemTypes_Event:
		return g_eventTag;
	case eWwiseItemTypes_Rtpc:
		return g_rtpcTag;
	case eWwiseItemTypes_Switch:
		return g_valueTag;
	case eWwiseItemTypes_AuxBus:
		return g_auxBusTag;
	case eWwiseItemTypes_SoundBank:
		return g_fileTag;
	case eWwiseItemTypes_State:
		return g_valueTag;
	case eWwiseItemTypes_SwitchGroup:
		return g_switchTag;
	case eWwiseItemTypes_StateGroup:
		return g_stateTag;
	}
	return "";
}

CAudioSystemEditor_wwise::CAudioSystemEditor_wwise()
{
	Serialization::LoadJsonFile(m_settings, g_userSettingsFile.c_str());
	Reload();
}

CAudioSystemEditor_wwise::~CAudioSystemEditor_wwise()
{
}

void CAudioSystemEditor_wwise::Reload(bool bPreserveConnectionStatus)
{
	// set all the controls as placeholder as we don't know if
	// any of them have been removed but still have connections to them
	for (auto controlPair : m_controls)
	{
		TControlPtr pControl = controlPair.second;
		if (pControl)
		{
			pControl->SetPlaceholder(true);
			pControl->SetConnected(false);
		}
	}

	// reload data
	LoadEventsMetadata();
	CAudioWwiseLoader loader;
	loader.Load(this);

	if (bPreserveConnectionStatus)
	{
		UpdateConnectedStatus();
	}
	else
	{
		m_connectionsByID.clear();
	}
}

IAudioSystemItem* CAudioSystemEditor_wwise::CreateControl(const SControlDef& controlDefinition)
{
	string fullname = controlDefinition.name;
	IAudioSystemItem* pParent = controlDefinition.pParent;
	if (pParent)
	{
		fullname = controlDefinition.pParent->GetName() + CRY_NATIVE_PATH_SEPSTR + fullname;
	}

	if (controlDefinition.bLocalised)
	{
		fullname = PathUtil::GetLocalizationFolder() + CRY_NATIVE_PATH_SEPSTR + fullname;
	}

	CID id = GetID(fullname);

	IAudioSystemItem* pControl = GetControl(id);
	if (pControl)
	{
		if (pControl->IsPlaceholder())
		{
			pControl->SetPlaceholder(false);
			if (pParent && pParent->IsPlaceholder())
			{
				pParent->SetPlaceholder(false);
			}
		}
		return pControl;
	}
	else
	{
		TControlPtr pNewControl = std::make_shared<IAudioSystemControl_wwise>(controlDefinition.name, id, controlDefinition.type);
		if (pParent == nullptr)
		{
			pParent = &m_rootControl;
		}

		if (controlDefinition.type == eWwiseItemTypes_Event)
		{
			pNewControl->SetRadius(m_eventsInfoMap[id].maxRadius);
		}
		pParent->AddChild(pNewControl.get());
		pNewControl->SetParent(pParent);
		pNewControl->SetLocalised(controlDefinition.bLocalised);
		m_controls[id] = pNewControl;
		return pNewControl.get();
	}
}

IAudioSystemItem* CAudioSystemEditor_wwise::GetControl(CID id) const
{
	if (id >= 0)
	{
		return stl::find_in_map(m_controls, id, TControlPtr()).get();
	}
	return nullptr;
}

IAudioSystemItem* CAudioSystemEditor_wwise::GetControlByName(const string& name, bool bIsLocalised, IAudioSystemItem* pParent) const
{
	string fullName = (pParent) ? pParent->GetName() + CRY_NATIVE_PATH_SEPSTR + name : name;
	if (bIsLocalised)
	{
		fullName = PathUtil::GetLocalizationFolder() + CRY_NATIVE_PATH_SEPSTR + fullName;
	}
	return GetControl(GetID(fullName));
}

ConnectionPtr CAudioSystemEditor_wwise::CreateConnectionToControl(EACEControlType eATLControlType, IAudioSystemItem* pMiddlewareControl)
{
	if (pMiddlewareControl)
	{
		if (pMiddlewareControl->GetType() == eWwiseItemTypes_Rtpc)
		{
			switch (eATLControlType)
			{
			case EACEControlType::eACEControlType_RTPC:
				{
					return std::make_shared<CRtpcConnection>(pMiddlewareControl->GetId());
				}
			case EACEControlType::eACEControlType_State:
				{
					return std::make_shared<CStateToRtpcConnection>(pMiddlewareControl->GetId());
				}
			}
		}

		return std::make_shared<IAudioConnection>(pMiddlewareControl->GetId());
	}
	return nullptr;
}

ConnectionPtr CAudioSystemEditor_wwise::CreateConnectionFromXMLNode(XmlNodeRef pNode, EACEControlType eATLControlType)
{
	if (pNode)
	{
		const char* szTag = pNode->getTag();
		ItemType type = TagToType(szTag);
		if (type != AUDIO_SYSTEM_INVALID_TYPE)
		{
			const string name = pNode->getAttr(g_nameAttribute);
			const string localisedAttribute = pNode->getAttr(g_localizedAttribute);
			const bool bLocalised = (localisedAttribute.compareNoCase(g_trueAttribute) == 0);

			// If control not found, create a placeholder.
			// We want to keep that connection even if it's not in the middleware.
			// The user could be using the engine without the wwise project
			IAudioSystemItem* pControl = GetControlByName(name, bLocalised);
			if (pControl == nullptr)
			{
				pControl = CreateControl(SControlDef(name, type));
				if (pControl)
				{
					pControl->SetPlaceholder(true);
					pControl->SetLocalised(bLocalised);
				}
			}

			// If it's a switch we actually connect to one of the states within the switch
			if (type == eWwiseItemTypes_SwitchGroup || type == eWwiseItemTypes_StateGroup)
			{
				if (pNode->getChildCount() == 1)
				{
					pNode = pNode->getChild(0);
					if (pNode)
					{
						string childName = pNode->getAttr(g_nameAttribute);

						IAudioSystemItem* pChildControl = GetControlByName(childName, false, pControl);
						if (pChildControl == nullptr)
						{
							pChildControl = CreateControl(SControlDef(childName, type == eWwiseItemTypes_SwitchGroup ? eWwiseItemTypes_Switch : eWwiseItemTypes_State, false, pControl));
							if (pChildControl)
							{
								pChildControl->SetPlaceholder(true);
								pChildControl->SetLocalised(bLocalised);
							}
						}
						pControl = pChildControl;
					}
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[Audio Controls Editor] [Wwise] Error reading connection to Wwise control %s", name);
				}
			}

			if (pControl)
			{
				if (type == eWwiseItemTypes_Rtpc)
				{
					switch (eATLControlType)
					{
					case EACEControlType::eACEControlType_RTPC:
						{
							RtpcConnectionPtr pConnection = std::make_shared<CRtpcConnection>(pControl->GetId());

							pNode->getAttr(g_multAttribute, pConnection->mult);
							pNode->getAttr(g_shiftAttribute, pConnection->shift);
							return pConnection;
						}
					case EACEControlType::eACEControlType_State:
						{
							StateConnectionPtr pConnection = std::make_shared<CStateToRtpcConnection>(pControl->GetId());
							pNode->getAttr(g_valueAttribute, pConnection->value);
							return pConnection;
						}
					}
				}
				return std::make_shared<IAudioConnection>(pControl->GetId());
			}
		}
	}
	return nullptr;
}

XmlNodeRef CAudioSystemEditor_wwise::CreateXMLNodeFromConnection(const ConnectionPtr pConnection, const EACEControlType eATLControlType)
{
	const IAudioSystemItem* pControl = GetControl(pConnection->GetID());
	if (pControl)
	{
		switch (pControl->GetType())
		{
		case ACE::eWwiseItemTypes_Switch:
		case ACE::eWwiseItemTypes_SwitchGroup:
		case ACE::eWwiseItemTypes_State:
		case ACE::eWwiseItemTypes_StateGroup:
			{
				const IAudioSystemItem* pParent = pControl->GetParent();
				if (pParent)
				{
					XmlNodeRef pSwitchNode = GetISystem()->CreateXmlNode(TypeToTag(pParent->GetType()));
					pSwitchNode->setAttr(g_nameAttribute, pParent->GetName());

					XmlNodeRef pStateNode = pSwitchNode->createNode(g_valueTag);
					pStateNode->setAttr(g_nameAttribute, pControl->GetName());
					pSwitchNode->addChild(pStateNode);

					return pSwitchNode;
				}
			}
			break;

		case ACE::eWwiseItemTypes_Rtpc:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()));
				pConnectionNode->setAttr(g_nameAttribute, pControl->GetName());

				if (eATLControlType == eACEControlType_RTPC)
				{
					std::shared_ptr<const CRtpcConnection> pRtpcConnection = std::static_pointer_cast<const CRtpcConnection>(pConnection);
					if (pRtpcConnection->mult != 1.0f)
					{
						pConnectionNode->setAttr(g_multAttribute, pRtpcConnection->mult);
					}
					if (pRtpcConnection->shift != 0.0f)
					{
						pConnectionNode->setAttr(g_shiftAttribute, pRtpcConnection->shift);
					}

				}
				else if (eATLControlType == eACEControlType_State)
				{
					std::shared_ptr<const CStateToRtpcConnection> pStateConnection = std::static_pointer_cast<const CStateToRtpcConnection>(pConnection);
					pConnectionNode->setAttr(g_valueAttribute, pStateConnection->value);
				}
				return pConnectionNode;
			}
			break;

		case ACE::eWwiseItemTypes_Event:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()));
				pConnectionNode->setAttr(g_nameAttribute, pControl->GetName());
				return pConnectionNode;
			}
			break;

		case ACE::eWwiseItemTypes_AuxBus:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()));
				pConnectionNode->setAttr(g_nameAttribute, pControl->GetName());
				return pConnectionNode;
			}
			break;

		case ACE::eWwiseItemTypes_SoundBank:
			{
				XmlNodeRef pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()));
				pConnectionNode->setAttr(g_nameAttribute, pControl->GetName());
				if (pControl->IsLocalised())
				{
					pConnectionNode->setAttr(g_localizedAttribute, g_trueAttribute);
				}
				return pConnectionNode;
			}
			break;
		}
	}
	return nullptr;
}

const char* CAudioSystemEditor_wwise::GetTypeIcon(ItemType type) const
{
	switch (type)
	{
	case eWwiseItemTypes_Event:
		return "Editor/Icons/audio/wwise/event_nor.png";
		break;
	case eWwiseItemTypes_Rtpc:
		return "Editor/Icons/audio/wwise/gameparameter_nor.png";
		break;
	case eWwiseItemTypes_Switch:
		return "Editor/Icons/audio/wwise/switch_nor.png";
		break;
	case eWwiseItemTypes_AuxBus:
		return "Editor/Icons/audio/wwise/auxbus_nor.png";
		break;
	case eWwiseItemTypes_SoundBank:
		return "Editor/Icons/audio/wwise/soundbank_nor.png";
		break;
	case eWwiseItemTypes_State:
		return "Editor/Icons/audio/wwise/state_nor.png";
		break;
	case eWwiseItemTypes_SwitchGroup:
		return "Editor/Icons/audio/wwise/switchgroup_nor.png";
		break;
	case eWwiseItemTypes_StateGroup:
		return "Editor/Icons/audio/wwise/stategroup_nor.png";
		break;
	}
	return "Editor/Icons/audio/wwise/switchgroup_nor.png";
}

ACE::EACEControlType CAudioSystemEditor_wwise::ImplTypeToATLType(ItemType type) const
{
	switch (type)
	{
	case eWwiseItemTypes_Event:
		return eACEControlType_Trigger;
		break;
	case eWwiseItemTypes_Rtpc:
		return eACEControlType_RTPC;
		break;
	case eWwiseItemTypes_Switch:
	case eWwiseItemTypes_State:
		return eACEControlType_State;
		break;
	case eWwiseItemTypes_AuxBus:
		return eACEControlType_Environment;
		break;
	case eWwiseItemTypes_SoundBank:
		return eACEControlType_Preload;
		break;
	case eWwiseItemTypes_StateGroup:
	case eWwiseItemTypes_SwitchGroup:
		return eACEControlType_Switch;
		break;
	}
	return eACEControlType_NumTypes;
}

ACE::TImplControlTypeMask CAudioSystemEditor_wwise::GetCompatibleTypes(EACEControlType atlControlType) const
{
	switch (atlControlType)
	{
	case eACEControlType_Trigger:
		return eWwiseItemTypes_Event;
		break;
	case eACEControlType_RTPC:
		return eWwiseItemTypes_Rtpc;
		break;
	case eACEControlType_Switch:
		return AUDIO_SYSTEM_INVALID_TYPE;
		break;
	case eACEControlType_State:
		return (eWwiseItemTypes_Switch | eWwiseItemTypes_State | eWwiseItemTypes_Rtpc);
		break;
	case eACEControlType_Environment:
		return (eWwiseItemTypes_AuxBus | eWwiseItemTypes_Switch | eWwiseItemTypes_State | eWwiseItemTypes_Rtpc);
		break;
	case eACEControlType_Preload:
		return eWwiseItemTypes_SoundBank;
		break;
	}
	return AUDIO_SYSTEM_INVALID_TYPE;
}

ACE::CID CAudioSystemEditor_wwise::GetID(const string& name) const
{
	return CCrc32::ComputeLowercase(name);
}

string CAudioSystemEditor_wwise::GetName() const
{
	return "Wwise";
}

void CAudioSystemEditor_wwise::UpdateConnectedStatus()
{
	TConnectionsMap::iterator it = m_connectionsByID.begin();
	TConnectionsMap::iterator end = m_connectionsByID.end();
	for (; it != end; ++it)
	{
		if (it->second > 0)
		{
			IAudioSystemItem* pControl = GetControl(it->first);
			if (pControl)
			{
				pControl->SetConnected(true);
			}
		}
	}
}

void CAudioSystemEditor_wwise::DisableConnection(ConnectionPtr pConnection)
{
	IAudioSystemItem* pControl = GetControl(pConnection->GetID());
	if (pControl)
	{
		int connectionCount = m_connectionsByID[pControl->GetId()] - 1;
		if (connectionCount <= 0)
		{
			connectionCount = 0;
			pControl->SetConnected(false);
		}
		m_connectionsByID[pControl->GetId()] = connectionCount;
	}
}

void CAudioSystemEditor_wwise::EnableConnection(ConnectionPtr pConnection)
{
	IAudioSystemItem* pControl = GetControl(pConnection->GetID());
	if (pControl)
	{
		++m_connectionsByID[pControl->GetId()];
		pControl->SetConnected(true);
	}
}

void CAudioSystemEditor_wwise::LoadEventsMetadata()
{
	m_eventsInfoMap.clear();
	string path = m_settings.GetSoundBanksPath();
	path += CRY_NATIVE_PATH_SEPSTR + g_soundBanksInfoFilename;
	XmlNodeRef pRootNode = GetISystem()->LoadXmlFromFile(path.c_str());
	if (pRootNode)
	{
		XmlNodeRef pSoundBanksNode = pRootNode->findChild("SoundBanks");
		if (pSoundBanksNode)
		{
			const int numSoundBankNodes = pSoundBanksNode->getChildCount();
			for (int i = 0; i < numSoundBankNodes; ++i)
			{
				XmlNodeRef pSoundBankNode = pSoundBanksNode->getChild(i);
				if (pSoundBankNode)
				{
					XmlNodeRef pIncludedEventsNode = pSoundBankNode->findChild("IncludedEvents");
					if (pIncludedEventsNode)
					{
						const int numEventNodes = pIncludedEventsNode->getChildCount();
						for (int j = 0; j < numEventNodes; ++j)
						{
							XmlNodeRef pEventNode = pIncludedEventsNode->getChild(j);
							if (pEventNode)
							{
								SEventInfo info;
								if (!pEventNode->getAttr("MaxAttenuation", info.maxRadius))
								{
									info.maxRadius = 0.0f;
								}
								const char* szName = pEventNode->getAttr("Name");
								m_eventsInfoMap[CCrc32::ComputeLowercase(szName)] = info;
							}
						}
					}
				}
			}
		}
	}
}

void CImplementationSettings_wwise::SetProjectPath(const char* szPath)
{
	m_projectPath = szPath;
	Serialization::SaveJsonFile(g_userSettingsFile.c_str(), *this);
}

}
