// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EditorImpl.h"

#include "ImplConnections.h"
#include "ProjectLoader.h"

#include <CryAudioImplWwise/GlobalData.h>
#include <CrySystem/ISystem.h>
#include <CryCore/CryCrc32.h>
#include <CryCore/StlUtils.h>
#include <CrySerialization/IArchiveHost.h>

namespace ACE
{
namespace Wwise
{
string const g_userSettingsFile = "%USER%/audiocontrolseditor_wwise.user";

//////////////////////////////////////////////////////////////////////////
EImpltemType TagToType(string const& tag)
{
	EImpltemType type = EImpltemType::Invalid;

	if (tag == CryAudio::s_szEventTag)
	{
		type = EImpltemType::Event;
	}
	else if (tag == CryAudio::Impl::Wwise::s_szFileTag)
	{
		type = EImpltemType::SoundBank;
	}
	else if (tag == CryAudio::Impl::Wwise::s_szParameterTag)
	{
		type = EImpltemType::Parameter;
	}
	else if (tag == CryAudio::Impl::Wwise::s_szSwitchGroupTag)
	{
		type = EImpltemType::SwitchGroup;
	}
	else if (tag == CryAudio::Impl::Wwise::s_szStateGroupTag)
	{
		type = EImpltemType::StateGroup;
	}
	else if (tag == CryAudio::Impl::Wwise::s_szAuxBusTag)
	{
		type = EImpltemType::AuxBus;
	}

	// Backwards compatibility will be removed before March 2019.
#if defined (USE_BACKWARDS_COMPATIBILITY)
	else if (tag == "WwiseEvent")
	{
		type = EImpltemType::Event;
	}
	else if (tag == "WwiseFile")
	{
		type = EImpltemType::SoundBank;
	}
	else if (tag == "WwiseRtpc")
	{
		type = EImpltemType::Parameter;
	}
	else if (tag == "WwiseSwitch")
	{
		type = EImpltemType::SwitchGroup;
	}
	else if (tag == "WwiseState")
	{
		type = EImpltemType::StateGroup;
	}
	else if (tag == "WwiseAuxBus")
	{
		type = EImpltemType::AuxBus;
	}
#endif // USE_BACKWARDS_COMPATIBILITY

	else
	{
		type = EImpltemType::Invalid;
	}

	return type;
}

//////////////////////////////////////////////////////////////////////////
char const* TypeToTag(EImpltemType const type)
{
	char const* tag = nullptr;

	switch (type)
	{
	case EImpltemType::Event:
		tag = CryAudio::s_szEventTag;
		break;
	case EImpltemType::Parameter:
		tag = CryAudio::Impl::Wwise::s_szParameterTag;
		break;
	case EImpltemType::Switch:
		tag = CryAudio::Impl::Wwise::s_szValueTag;
		break;
	case EImpltemType::AuxBus:
		tag = CryAudio::Impl::Wwise::s_szAuxBusTag;
		break;
	case EImpltemType::SoundBank:
		tag = CryAudio::Impl::Wwise::s_szFileTag;
		break;
	case EImpltemType::State:
		tag = CryAudio::Impl::Wwise::s_szValueTag;
		break;
	case EImpltemType::SwitchGroup:
		tag = CryAudio::Impl::Wwise::s_szSwitchGroupTag;
		break;
	case EImpltemType::StateGroup:
		tag = CryAudio::Impl::Wwise::s_szStateGroupTag;
		break;
	default:
		tag = nullptr;
		break;
	}

	return tag;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* SearchForControl(CImplItem* const pImplItem, string const& name, ItemType const type)
{
	CImplItem* pImplControl = nullptr;

	if ((pImplItem->GetName() == name) && (pImplItem->GetType() == type))
	{
		pImplControl = pImplItem;
	}
	else
	{
		int const count = pImplItem->ChildCount();

		for (int i = 0; i < count; ++i)
		{
			CImplItem* const pFoundImplControl = SearchForControl(pImplItem->GetChildAt(i), name, type);

			if (pFoundImplControl != nullptr)
			{
				pImplControl = pFoundImplControl;
				break;
			}
		}
	}

	return pImplControl;
}

//////////////////////////////////////////////////////////////////////////
CImplSettings::CImplSettings()
	: m_projectPath(PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "wwise_project")
	, m_assetsPath(PathUtil::GetGameFolder() +
	               CRY_NATIVE_PATH_SEPSTR
	               AUDIO_SYSTEM_DATA_ROOT
	               CRY_NATIVE_PATH_SEPSTR +
	               CryAudio::Impl::Wwise::s_szImplFolderName +
	               CRY_NATIVE_PATH_SEPSTR +
	               CryAudio::s_szAssetsFolderName)
{
}

//////////////////////////////////////////////////////////////////////////
void CImplSettings::SetProjectPath(char const* szPath)
{
	m_projectPath = szPath;
	Serialization::SaveJsonFile(g_userSettingsFile.c_str(), *this);
}

//////////////////////////////////////////////////////////////////////////
void CImplSettings::Serialize(Serialization::IArchive& ar)
{
	ar(m_projectPath, "projectPath", "Project Path");
}

//////////////////////////////////////////////////////////////////////////
CEditorImpl::CEditorImpl()
{
	Serialization::LoadJsonFile(m_implSettings, g_userSettingsFile.c_str());
	gEnv->pAudioSystem->GetImplInfo(m_implInfo);
	m_implName = m_implInfo.name.c_str();
	m_implFolderName = CryAudio::Impl::Wwise::s_szImplFolderName;
}

//////////////////////////////////////////////////////////////////////////
CEditorImpl::~CEditorImpl()
{
	Clear();
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::Reload(bool const preserveConnectionStatus)
{
	Clear();

	CProjectLoader(GetSettings()->GetProjectPath(), GetSettings()->GetAssetsPath(), m_rootControl, m_controlsCache);

	if (preserveConnectionStatus)
	{
		for (auto const& connection : m_connectionsByID)
		{
			if (connection.second > 0)
			{
				CImplItem* const pImplControl = GetControl(connection.first);

				if (pImplControl != nullptr)
				{
					pImplControl->SetConnected(true);
				}
			}
		}
	}
	else
	{
		m_connectionsByID.clear();
	}
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CEditorImpl::GetControl(CID const id) const
{
	CImplItem* pImplItem = nullptr;

	if (id >= 0)
	{
		pImplItem = stl::find_in_map(m_controlsCache, id, nullptr);
	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
char const* CEditorImpl::GetTypeIcon(CImplItem const* const pImplItem) const
{
	char const* szIconPath = "icons:Dialogs/dialog-error.ico";
	auto const type = static_cast<EImpltemType>(pImplItem->GetType());

	switch (type)
	{
	case EImpltemType::Event:
		szIconPath = "icons:audio/wwise/event.ico";
		break;
	case EImpltemType::Parameter:
		szIconPath = "icons:audio/wwise/gameparameter.ico";
		break;
	case EImpltemType::Switch:
		szIconPath = "icons:audio/wwise/switch.ico";
		break;
	case EImpltemType::AuxBus:
		szIconPath = "icons:audio/wwise/auxbus.ico";
		break;
	case EImpltemType::SoundBank:
		szIconPath = "icons:audio/wwise/soundbank.ico";
		break;
	case EImpltemType::State:
		szIconPath = "icons:audio/wwise/state.ico";
		break;
	case EImpltemType::SwitchGroup:
		szIconPath = "icons:audio/wwise/switchgroup.ico";
		break;
	case EImpltemType::StateGroup:
		szIconPath = "icons:audio/wwise/stategroup.ico";
		break;
	case EImpltemType::WorkUnit:
		szIconPath = "icons:audio/wwise/workunit.ico";
		break;
	case EImpltemType::VirtualFolder:
		szIconPath = "icons:audio/wwise/virtualfolder.ico";
		break;
	case EImpltemType::PhysicalFolder:
		szIconPath = "icons:audio/wwise/physicalfolder.ico";
		break;
	default:
		szIconPath = "icons:Dialogs/dialog-error.ico";
		break;
	}

	return szIconPath;
}

//////////////////////////////////////////////////////////////////////////
string const& CEditorImpl::GetName() const
{
	return m_implName;
}

//////////////////////////////////////////////////////////////////////////
string const& CEditorImpl::GetFolderName() const
{
	return m_implFolderName;
}

//////////////////////////////////////////////////////////////////////////
bool CEditorImpl::IsTypeCompatible(ESystemItemType const systemType, CImplItem const* const pImplItem) const
{
	bool isCompatible = false;
	auto const implType = static_cast<EImpltemType>(pImplItem->GetType());

	switch (systemType)
	{
	case ESystemItemType::Trigger:
		isCompatible = (implType == EImpltemType::Event);
		break;
	case ESystemItemType::Parameter:
		isCompatible = (implType == EImpltemType::Parameter);
		break;
	case ESystemItemType::Switch:
		isCompatible = (implType == EImpltemType::Invalid);
		break;
	case ESystemItemType::State:
		isCompatible = (implType == EImpltemType::Switch) || (implType == EImpltemType::State) || (implType == EImpltemType::Parameter);
		break;
	case ESystemItemType::Environment:
		isCompatible = (implType == EImpltemType::AuxBus) || (implType == EImpltemType::Parameter);
		break;
	case ESystemItemType::Preload:
		isCompatible = (implType == EImpltemType::SoundBank);
		break;
	}

	return isCompatible;
}

//////////////////////////////////////////////////////////////////////////
ESystemItemType CEditorImpl::ImplTypeToSystemType(CImplItem const* const pImplItem) const
{
	ESystemItemType systemType = ESystemItemType::Invalid;
	auto const itemType = static_cast<EImpltemType>(pImplItem->GetType());

	switch (itemType)
	{
	case EImpltemType::Event:
		systemType = ESystemItemType::Trigger;
		break;
	case EImpltemType::Parameter:
		systemType = ESystemItemType::Parameter;
		break;
	case EImpltemType::Switch:
	case EImpltemType::State:
		systemType = ESystemItemType::State;
		break;
	case EImpltemType::AuxBus:
		systemType = ESystemItemType::Environment;
		break;
	case EImpltemType::SoundBank:
		systemType = ESystemItemType::Preload;
		break;
	case EImpltemType::StateGroup:
	case EImpltemType::SwitchGroup:
		systemType = ESystemItemType::Switch;
		break;
	default:
		systemType = ESystemItemType::Invalid;
		break;
	}

	return systemType;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CEditorImpl::CreateConnectionToControl(ESystemItemType const controlType, CImplItem* const pImplItem)
{
	ConnectionPtr pConnection = nullptr;

	if (pImplItem != nullptr)
	{
		if (static_cast<EImpltemType>(pImplItem->GetType()) == EImpltemType::Parameter)
		{
			switch (controlType)
			{
			case ESystemItemType::Parameter:
			case ESystemItemType::Environment:
				pConnection = std::make_shared<CParameterConnection>(pImplItem->GetId());
				break;
			case ESystemItemType::State:
				pConnection = std::make_shared<CStateToParameterConnection>(pImplItem->GetId());
				break;
			default:
				pConnection = std::make_shared<CImplConnection>(pImplItem->GetId());
				break;
			}
		}
		else
		{
			pConnection = std::make_shared<CImplConnection>(pImplItem->GetId());
		}

	}

	return pConnection;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CEditorImpl::CreateConnectionFromXMLNode(XmlNodeRef pNode, ESystemItemType const controlType)
{
	ConnectionPtr pConnectionPtr = nullptr;

	if (pNode != nullptr)
	{
		char const* const szTag = pNode->getTag();
		EImpltemType const type = TagToType(szTag);

		if (type != EImpltemType::Invalid)
		{
			string name = pNode->getAttr(CryAudio::s_szNameAttribute);
			string localizedAttribute = pNode->getAttr(CryAudio::Impl::Wwise::s_szLocalizedAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
			if (name.IsEmpty() && pNode->haveAttr("wwise_name"))
			{
				name = pNode->getAttr("wwise_name");
			}

			if (localizedAttribute.IsEmpty() && pNode->haveAttr("wwise_localised"))
			{
				localizedAttribute = pNode->getAttr("wwise_localised");
			}
#endif    // USE_BACKWARDS_COMPATIBILITY
			bool const isLocalized = (localizedAttribute.compareNoCase(CryAudio::Impl::Wwise::s_szTrueValue) == 0);

			CImplItem* pImplControl = SearchForControl(&m_rootControl, name, static_cast<ItemType>(type));

			// If control not found, create a placeholder.
			// We want to keep that connection even if it's not in the middleware.
			// The user could be using the engine without the wwise project
			if (pImplControl == nullptr)
			{
				CID const id = GenerateID(name, isLocalized, &m_rootControl);
				pImplControl = new CImplControl(name, id, static_cast<ItemType>(type));
				pImplControl->SetLocalised(isLocalized);
				pImplControl->SetPlaceholder(true);

				m_controlsCache[id] = pImplControl;
			}

			// If it's a switch we actually connect to one of the states within the switch
			if ((type == EImpltemType::SwitchGroup) || (type == EImpltemType::StateGroup))
			{
				if (pNode->getChildCount() == 1)
				{
					pNode = pNode->getChild(0);

					if (pNode != nullptr)
					{
						string childName = pNode->getAttr(CryAudio::s_szNameAttribute);

#if defined (USE_BACKWARDS_COMPATIBILITY)
						if (childName.IsEmpty() && pNode->haveAttr("wwise_name"))
						{
							childName = pNode->getAttr("wwise_name");
						}
#endif          // USE_BACKWARDS_COMPATIBILITY

						CImplItem* pStateControl = nullptr;
						size_t const count = pImplControl->ChildCount();

						for (size_t i = 0; i < count; ++i)
						{
							CImplItem* const pChild = pImplControl->GetChildAt(i);

							if ((pChild != nullptr) && (pChild->GetName() == childName))
							{
								pStateControl = pChild;
							}
						}

						if (pStateControl == nullptr)
						{
							CID const id = GenerateID(childName);
							pStateControl = new CImplControl(childName, id, static_cast<ItemType>(type == EImpltemType::SwitchGroup ? EImpltemType::Switch : EImpltemType::State));
							pStateControl->SetLocalised(false);
							pStateControl->SetPlaceholder(true);
							pImplControl->AddChild(pStateControl);

							m_controlsCache[id] = pStateControl;
						}

						pImplControl = pStateControl;
					}
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[Audio Controls Editor] [Wwise] Error reading connection to Wwise control %s", name);
				}
			}

			if (pImplControl != nullptr)
			{
				if (type == EImpltemType::Parameter)
				{
					switch (controlType)
					{
					case ESystemItemType::Parameter:
					case ESystemItemType::Environment:
						{
							ParameterConnectionPtr const pConnection = std::make_shared<CParameterConnection>(pImplControl->GetId());
							float mult = pConnection->GetMultiplier();
							float shift = pConnection->GetShift();

							pNode->getAttr(CryAudio::Impl::Wwise::s_szMutiplierAttribute, mult);
							pNode->getAttr(CryAudio::Impl::Wwise::s_szShiftAttribute, shift);
#if defined (USE_BACKWARDS_COMPATIBILITY)
							if (pNode->haveAttr("wwise_value_multiplier"))
							{
								pNode->getAttr("wwise_value_multiplier", mult);
							}
							if (pNode->haveAttr("wwise_value_shift"))
							{
								pNode->getAttr("wwise_value_shift", shift);
							}
#endif            // USE_BACKWARDS_COMPATIBILITY
							pConnection->SetMultiplier(mult);
							pConnection->SetShift(shift);

							pConnectionPtr = pConnection;
						}
						break;
					case ESystemItemType::State:
						{
							StateConnectionPtr const pConnection = std::make_shared<CStateToParameterConnection>(pImplControl->GetId());
							float value = pConnection->GetValue();

							pNode->getAttr(CryAudio::Impl::Wwise::s_szValueAttribute, value);
#if defined (USE_BACKWARDS_COMPATIBILITY)
							if (pNode->haveAttr("wwise_value"))
							{
								pNode->getAttr("wwise_value", value);
							}
#endif            // USE_BACKWARDS_COMPATIBILITY
							pConnection->SetValue(value);

							pConnectionPtr = pConnection;
						}
						break;
					default:
						pConnectionPtr = std::make_shared<CImplConnection>(pImplControl->GetId());
						break;
					}
				}
				else
				{
					pConnectionPtr = std::make_shared<CImplConnection>(pImplControl->GetId());
				}
			}
		}
	}

	return pConnectionPtr;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CEditorImpl::CreateXMLNodeFromConnection(ConnectionPtr const pConnection, ESystemItemType const controlType)
{
	XmlNodeRef pNode = nullptr;

	CImplItem const* const pImplControl = GetControl(pConnection->GetID());

	if (pImplControl != nullptr)
	{
		auto const itemType = static_cast<EImpltemType>(pImplControl->GetType());

		switch (static_cast<EImpltemType>(pImplControl->GetType()))
		{
		case EImpltemType::Switch:
		case EImpltemType::SwitchGroup:
		case EImpltemType::State:
		case EImpltemType::StateGroup:
			{
				CImplItem const* const pParent = pImplControl->GetParent();

				if (pParent != nullptr)
				{
					XmlNodeRef const pSwitchNode = GetISystem()->CreateXmlNode(TypeToTag(static_cast<EImpltemType>(pParent->GetType())));
					pSwitchNode->setAttr(CryAudio::s_szNameAttribute, pParent->GetName());

					XmlNodeRef const pStateNode = pSwitchNode->createNode(CryAudio::Impl::Wwise::s_szValueTag);
					pStateNode->setAttr(CryAudio::s_szNameAttribute, pImplControl->GetName());
					pSwitchNode->addChild(pStateNode);

					pNode = pSwitchNode;
				}
			}
			break;
		case EImpltemType::Parameter:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(itemType));
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pImplControl->GetName());

				if ((controlType == ESystemItemType::Parameter) || (controlType == ESystemItemType::Environment))
				{
					std::shared_ptr<const CParameterConnection> pParameterConnection = std::static_pointer_cast<const CParameterConnection>(pConnection);

					float const mult = pParameterConnection->GetMultiplier();

					if (mult != 1.0f)
					{
						pConnectionNode->setAttr(CryAudio::Impl::Wwise::s_szMutiplierAttribute, mult);
					}

					float const shift = pParameterConnection->GetShift();

					if (shift != 0.0f)
					{
						pConnectionNode->setAttr(CryAudio::Impl::Wwise::s_szShiftAttribute, shift);
					}

				}
				else if (controlType == ESystemItemType::State)
				{
					std::shared_ptr<const CStateToParameterConnection> pStateConnection = std::static_pointer_cast<const CStateToParameterConnection>(pConnection);
					pConnectionNode->setAttr(CryAudio::Impl::Wwise::s_szValueAttribute, pStateConnection->GetValue());
				}

				pNode = pConnectionNode;
			}
			break;
		case EImpltemType::Event:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(itemType));
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pImplControl->GetName());
				pNode = pConnectionNode;
			}
			break;
		case EImpltemType::AuxBus:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(itemType));
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pImplControl->GetName());
				pNode = pConnectionNode;
			}
			break;
		case EImpltemType::SoundBank:
			{
				XmlNodeRef pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(itemType));
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pImplControl->GetName());

				if (pImplControl->IsLocalised())
				{
					pConnectionNode->setAttr(CryAudio::Impl::Wwise::s_szLocalizedAttribute, CryAudio::Impl::Wwise::s_szTrueValue);
				}

				pNode = pConnectionNode;
			}
			break;
		}
	}

	return pNode;
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::EnableConnection(ConnectionPtr const pConnection)
{
	CImplItem* const pImplControl = GetControl(pConnection->GetID());

	if (pImplControl != nullptr)
	{
		++m_connectionsByID[pImplControl->GetId()];
		pImplControl->SetConnected(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::DisableConnection(ConnectionPtr const pConnection)
{
	CImplItem* const pImplControl = GetControl(pConnection->GetID());

	if (pImplControl != nullptr)
	{
		int connectionCount = m_connectionsByID[pImplControl->GetId()] - 1;

		if (connectionCount <= 0)
		{
			connectionCount = 0;
			pImplControl->SetConnected(false);
		}

		m_connectionsByID[pImplControl->GetId()] = connectionCount;
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::Clear()
{
	// Delete all the controls
	for (auto const& controlPair : m_controlsCache)
	{
		CImplItem const* const pImplControl = controlPair.second;

		if (pImplControl != nullptr)
		{
			delete pImplControl;
		}
	}

	m_controlsCache.clear();

	// Clean up the root control
	m_rootControl = CImplItem();
}

//////////////////////////////////////////////////////////////////////////
CID CEditorImpl::GenerateID(string const& fullPathName) const
{
	return CryAudio::StringToId(fullPathName);
}

//////////////////////////////////////////////////////////////////////////
CID CEditorImpl::GenerateID(string const& controlName, bool isLocalized, CImplItem* pParent) const
{
	string pathName = (pParent != nullptr && !pParent->GetName().empty()) ? pParent->GetName() + CRY_NATIVE_PATH_SEPSTR + controlName : controlName;

	if (isLocalized)
	{
		pathName = PathUtil::GetLocalizationFolder() + CRY_NATIVE_PATH_SEPSTR + pathName;
	}

	return GenerateID(pathName);
}
} // namespace Wwise
} // namespace ACE
