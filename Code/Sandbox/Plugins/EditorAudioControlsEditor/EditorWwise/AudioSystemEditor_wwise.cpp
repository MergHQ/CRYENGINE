// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "StdAfx.h"
#include "AudioSystemEditor_wwise.h"

#include "AudioSystemControl_wwise.h"
#include "AudioWwiseLoader.h"

#include <CryAudio/IAudioSystem.h>
#include <CryAudio/IProfileData.h>
#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>
#include <CryCore/CryCrc32.h>
#include <ACETypes.h>
#include <CryString/CryPath.h>
#include <CryCore/StlUtils.h>
#include <CrySerialization/IArchiveHost.h>

namespace ACE
{
string  const g_userSettingsFile = "%USER%/audiocontrolseditor_wwise.user";

class CStringAndHash
{
	//a small helper class to make comparison of the stored c-string faster, by checking a hash-representation first.
public:

	CStringAndHash(char const* pText)
		: szText(pText), hash(CCrc32::Compute(pText))
	{}

	operator string() const                      { return szText; }
	operator char const*() const                 { return szText; }
	bool operator==(const CStringAndHash& other) { return (hash == other.hash && strcmp(szText, other.szText) == 0); }

private:

	char const*  szText;
	uint32 const hash;
};

// XML tags
CStringAndHash const g_switchTag = "WwiseSwitch";
CStringAndHash const g_stateTag = "WwiseState";
CStringAndHash const g_fileTag = "WwiseFile";
CStringAndHash const g_parameterTag = "WwiseRtpc";
CStringAndHash const g_eventTag = "WwiseEvent";
CStringAndHash const g_auxBusTag = "WwiseAuxBus";
CStringAndHash const g_valueTag = "WwiseValue";

// XML attributes
string const g_nameAttribute = "wwise_name";
string const g_shiftAttribute = "wwise_value_shift";
string const g_multAttribute = "wwise_value_multiplier";
string const g_valueAttribute = "wwise_value";
string const g_localizedAttribute = "wwise_localised";
string const g_trueAttribute = "true";

//////////////////////////////////////////////////////////////////////////
ItemType TagToType(char const* szTag)
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
	if (hashedTag == g_parameterTag)
	{
		return eWwiseItemTypes_Parameter;
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

//////////////////////////////////////////////////////////////////////////
string TypeToTag(ItemType const type)
{
	switch (type)
	{
	case eWwiseItemTypes_Event:
		return g_eventTag;
	case eWwiseItemTypes_Parameter:
		return g_parameterTag;
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

//////////////////////////////////////////////////////////////////////////
IAudioSystemItem* SearchForControl(IAudioSystemItem* const pItem, string const& name, ItemType const type)
{
	if ((pItem->GetName() == name) && (pItem->GetType() == type))
	{
		return pItem;
	}

	int const count = pItem->ChildCount();

	for (int i = 0; i < count; ++i)
	{
		IAudioSystemItem* const pFound = SearchForControl(pItem->GetChildAt(i), name, type);

		if (pFound != nullptr)
		{
			return pFound;
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CImplementationSettings_wwise::SetProjectPath(char const* szPath)
{
	m_projectPath = szPath;
	Serialization::SaveJsonFile(g_userSettingsFile.c_str(), *this);
}

//////////////////////////////////////////////////////////////////////////
CAudioSystemEditor_wwise::CAudioSystemEditor_wwise()
{
	Serialization::LoadJsonFile(m_settings, g_userSettingsFile.c_str());
}

//////////////////////////////////////////////////////////////////////////
CAudioSystemEditor_wwise::~CAudioSystemEditor_wwise()
{
	Clear();
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemEditor_wwise::Reload(bool const preserveConnectionStatus)
{
	Clear();

	// reload data
	CAudioWwiseLoader loader(GetSettings()->GetProjectPath(), GetSettings()->GetSoundBanksPath(), m_rootControl);

	CreateControlCache(&m_rootControl);

	if (preserveConnectionStatus)
	{
		UpdateConnectedStatus();
	}
	else
	{
		m_connectionsByID.clear();
	}
}

//////////////////////////////////////////////////////////////////////////
IAudioSystemItem* CAudioSystemEditor_wwise::GetControl(CID const id) const
{
	if (id >= 0)
	{
		return stl::find_in_map(m_controlsCache, id, nullptr);
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
TImplControlTypeMask CAudioSystemEditor_wwise::GetCompatibleTypes(EItemType const controlType) const
{
	switch (controlType)
	{
	case EItemType::Trigger:
		return eWwiseItemTypes_Event;
		break;
	case EItemType::Parameter:
		return eWwiseItemTypes_Parameter;
		break;
	case EItemType::Switch:
		return AUDIO_SYSTEM_INVALID_TYPE;
		break;
	case EItemType::State:
		return (eWwiseItemTypes_Switch | eWwiseItemTypes_State | eWwiseItemTypes_Parameter);
		break;
	case EItemType::Environment:
		return (eWwiseItemTypes_AuxBus | eWwiseItemTypes_Switch | eWwiseItemTypes_State | eWwiseItemTypes_Parameter);
		break;
	case EItemType::Preload:
		return eWwiseItemTypes_SoundBank;
		break;
	}
	return AUDIO_SYSTEM_INVALID_TYPE;
}

//////////////////////////////////////////////////////////////////////////
char const* CAudioSystemEditor_wwise::GetTypeIcon(ItemType const type) const
{
	switch (type)
	{
	case eWwiseItemTypes_Event:
		return "Editor/Icons/audio/wwise/event_nor.png";
		break;
	case eWwiseItemTypes_Parameter:
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
	case eWwiseItemTypes_WorkUnit:
		return "Editor/Icons/audio/wwise/workunit_nor.png";
		break;
	case eWwiseItemTypes_VirtualFolder:
		return "Editor/Icons/audio/wwise/folder_nor.png";
		break;
	case eWwiseItemTypes_PhysicalFolder:
		return "Editor/Icons/audio/wwise/physical_folder_nor.png";
		break;
	}
	return "Editor/Icons/audio/wwise/switchgroup_nor.png";
}

//////////////////////////////////////////////////////////////////////////
string CAudioSystemEditor_wwise::GetName() const
{
	return gEnv->pAudioSystem->GetProfileData()->GetImplName();
}

//////////////////////////////////////////////////////////////////////////
EItemType CAudioSystemEditor_wwise::ImplTypeToSystemType(ItemType const itemType) const
{
	switch (itemType)
	{
	case eWwiseItemTypes_Event:
		return EItemType::Trigger;
		break;
	case eWwiseItemTypes_Parameter:
		return EItemType::Parameter;
		break;
	case eWwiseItemTypes_Switch:
	case eWwiseItemTypes_State:
		return EItemType::State;
		break;
	case eWwiseItemTypes_AuxBus:
		return EItemType::Environment;
		break;
	case eWwiseItemTypes_SoundBank:
		return EItemType::Preload;
		break;
	case eWwiseItemTypes_StateGroup:
	case eWwiseItemTypes_SwitchGroup:
		return EItemType::Switch;
		break;
	}
	return EItemType::Invalid;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CAudioSystemEditor_wwise::CreateConnectionToControl(EItemType const controlType, IAudioSystemItem* const pMiddlewareControl)
{
	if (pMiddlewareControl != nullptr)
	{
		if (pMiddlewareControl->GetType() == eWwiseItemTypes_Parameter)
		{
			switch (controlType)
			{
			case EItemType::Parameter:
				{
					return std::make_shared<CParameterConnection>(pMiddlewareControl->GetId());
				}
			case EItemType::State:
				{
					return std::make_shared<CStateToParameterConnection>(pMiddlewareControl->GetId());
				}
			}
		}

		return std::make_shared<IAudioConnection>(pMiddlewareControl->GetId());
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CAudioSystemEditor_wwise::CreateConnectionFromXMLNode(XmlNodeRef pNode, EItemType const controlType)
{
	if (pNode != nullptr)
	{
		char const* const szTag = pNode->getTag();
		ItemType const type = TagToType(szTag);

		if (type != AUDIO_SYSTEM_INVALID_TYPE)
		{
			string const name = pNode->getAttr(g_nameAttribute);
			string const localizedAttribute = pNode->getAttr(g_localizedAttribute);
			bool const isLocalized = (localizedAttribute.compareNoCase(g_trueAttribute) == 0);

			IAudioSystemItem* pControl = SearchForControl(&m_rootControl, name, type);

			// If control not found, create a placeholder.
			// We want to keep that connection even if it's not in the middleware.
			// The user could be using the engine without the wwise project
			if (pControl == nullptr)
			{
				CID const id = GenerateID(name, isLocalized, &m_rootControl);
				pControl = new IAudioSystemControl_wwise(name, id, type);
				pControl->SetLocalised(isLocalized);
				pControl->SetPlaceholder(true);

				m_controlsCache[id] = pControl;
			}

			// If it's a switch we actually connect to one of the states within the switch
			if ((type == eWwiseItemTypes_SwitchGroup) || (type == eWwiseItemTypes_StateGroup))
			{
				if (pNode->getChildCount() == 1)
				{
					pNode = pNode->getChild(0);

					if (pNode != nullptr)
					{
						string const childName = pNode->getAttr(g_nameAttribute);

						IAudioSystemItem* pStateControl = nullptr;
						size_t const count = pControl->ChildCount();

						for (size_t i = 0; i < count; ++i)
						{
							IAudioSystemItem* const pChild = pControl->GetChildAt(i);

							if ((pChild != nullptr) && (pChild->GetName() == childName))
							{
								pStateControl = pChild;
							}
						}

						if (pStateControl == nullptr)
						{
							CID const id = GenerateID(childName);
							pStateControl = new IAudioSystemControl_wwise(childName, id, type == eWwiseItemTypes_SwitchGroup ? eWwiseItemTypes_Switch : eWwiseItemTypes_State);
							pStateControl->SetLocalised(false);
							pStateControl->SetPlaceholder(true);
							pControl->AddChild(pStateControl);
							pStateControl->SetParent(pControl);

							m_controlsCache[id] = pStateControl;
						}

						pControl = pStateControl;
					}
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[Audio Controls Editor] [Wwise] Error reading connection to Wwise control %s", name);
				}
			}

			if (pControl != nullptr)
			{
				if (type == eWwiseItemTypes_Parameter)
				{
					switch (controlType)
					{
					case EItemType::Parameter:
						{
							ParameterConnectionPtr const pConnection = std::make_shared<CParameterConnection>(pControl->GetId());

							pNode->getAttr(g_multAttribute, pConnection->mult);
							pNode->getAttr(g_shiftAttribute, pConnection->shift);
							return pConnection;
						}
					case EItemType::State:
						{
							StateConnectionPtr const pConnection = std::make_shared<CStateToParameterConnection>(pControl->GetId());
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

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CAudioSystemEditor_wwise::CreateXMLNodeFromConnection(ConnectionPtr const pConnection, EItemType const controlType)
{
	IAudioSystemItem const* const pControl = GetControl(pConnection->GetID());

	if (pControl != nullptr)
	{
		switch (pControl->GetType())
		{
		case eWwiseItemTypes_Switch:
		case eWwiseItemTypes_SwitchGroup:
		case eWwiseItemTypes_State:
		case eWwiseItemTypes_StateGroup:
			{
				IAudioSystemItem const* const pParent = pControl->GetParent();

				if (pParent != nullptr)
				{
					XmlNodeRef const pSwitchNode = GetISystem()->CreateXmlNode(TypeToTag(pParent->GetType()));
					pSwitchNode->setAttr(g_nameAttribute, pParent->GetName());

					XmlNodeRef const pStateNode = pSwitchNode->createNode(g_valueTag);
					pStateNode->setAttr(g_nameAttribute, pControl->GetName());
					pSwitchNode->addChild(pStateNode);

					return pSwitchNode;
				}
			}
			break;

		case eWwiseItemTypes_Parameter:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()));
				pConnectionNode->setAttr(g_nameAttribute, pControl->GetName());

				if (controlType == EItemType::Parameter)
				{
					std::shared_ptr<const CParameterConnection> pParameterConnection = std::static_pointer_cast<const CParameterConnection>(pConnection);
					if (pParameterConnection->mult != 1.0f)
					{
						pConnectionNode->setAttr(g_multAttribute, pParameterConnection->mult);
					}
					if (pParameterConnection->shift != 0.0f)
					{
						pConnectionNode->setAttr(g_shiftAttribute, pParameterConnection->shift);
					}

				}
				else if (controlType == EItemType::State)
				{
					std::shared_ptr<const CStateToParameterConnection> pStateConnection = std::static_pointer_cast<const CStateToParameterConnection>(pConnection);
					pConnectionNode->setAttr(g_valueAttribute, pStateConnection->value);
				}

				return pConnectionNode;
			}
			break;

		case eWwiseItemTypes_Event:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()));
				pConnectionNode->setAttr(g_nameAttribute, pControl->GetName());
				return pConnectionNode;
			}
			break;

		case eWwiseItemTypes_AuxBus:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()));
				pConnectionNode->setAttr(g_nameAttribute, pControl->GetName());
				return pConnectionNode;
			}
			break;

		case eWwiseItemTypes_SoundBank:
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

//////////////////////////////////////////////////////////////////////////
void CAudioSystemEditor_wwise::EnableConnection(ConnectionPtr const pConnection)
{
	IAudioSystemItem* const pControl = GetControl(pConnection->GetID());

	if (pControl != nullptr)
	{
		++m_connectionsByID[pControl->GetId()];
		pControl->SetConnected(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemEditor_wwise::DisableConnection(ConnectionPtr const pConnection)
{
	IAudioSystemItem* const pControl = GetControl(pConnection->GetID());

	if (pControl != nullptr)
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

//////////////////////////////////////////////////////////////////////////
void CAudioSystemEditor_wwise::Clear()
{
	// Delete all the controls
	for (auto const controlPair : m_controlsCache)
	{
		IAudioSystemItem const* const pControl = controlPair.second;

		if (pControl != nullptr)
		{
			delete pControl;
		}
	}

	m_controlsCache.clear();

	// Clean up the root control
	m_rootControl = IAudioSystemItem();
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemEditor_wwise::CreateControlCache(IAudioSystemItem const* const pParent)
{
	if (pParent != nullptr)
	{
		size_t const count = pParent->ChildCount();

		for (size_t i = 0; i < count; ++i)
		{
			IAudioSystemItem* const pChild = pParent->GetChildAt(i);

			if (pChild != nullptr)
			{
				m_controlsCache[pChild->GetId()] = pChild;
				CreateControlCache(pChild);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CAudioSystemEditor_wwise::UpdateConnectedStatus()
{
	TConnectionsMap::iterator it = m_connectionsByID.begin();
	TConnectionsMap::iterator end = m_connectionsByID.end();

	for (; it != end; ++it)
	{
		if (it->second > 0)
		{
			IAudioSystemItem* const pControl = GetControl(it->first);

			if (pControl != nullptr)
			{
				pControl->SetConnected(true);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CID CAudioSystemEditor_wwise::GenerateID(string const& fullPathName) const
{
	return CCrc32::ComputeLowercase(fullPathName);
}

//////////////////////////////////////////////////////////////////////////
CID CAudioSystemEditor_wwise::GenerateID(string const& controlName, bool isLocalized, IAudioSystemItem* pParent) const
{
	string pathName = (pParent != nullptr && !pParent->GetName().empty()) ? pParent->GetName() + CRY_NATIVE_PATH_SEPSTR + controlName : controlName;

	if (isLocalized)
	{
		pathName = PathUtil::GetLocalizationFolder() + CRY_NATIVE_PATH_SEPSTR + pathName;
	}

	return GenerateID(pathName);
}
} // namespace ACE
