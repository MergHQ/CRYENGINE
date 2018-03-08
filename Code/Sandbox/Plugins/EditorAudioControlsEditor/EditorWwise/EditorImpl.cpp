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
CImplItem* SearchForItem(CImplItem* const pImplItem, string const& name, ItemType const type)
{
	CImplItem* pSearchedImplItem = nullptr;

	if ((pImplItem->GetName() == name) && (pImplItem->GetType() == type))
	{
		pSearchedImplItem = pImplItem;
	}
	else
	{
		int const count = pImplItem->GetNumChildren();

		for (int i = 0; i < count; ++i)
		{
			CImplItem* const pFoundImplItem = SearchForItem(static_cast<CImplItem* const>(pImplItem->GetChildAt(i)), name, type);

			if (pFoundImplItem != nullptr)
			{
				pSearchedImplItem = pFoundImplItem;
				break;
			}
		}
	}

	return pSearchedImplItem;
}

//////////////////////////////////////////////////////////////////////////
CImplSettings::CImplSettings()
	: m_projectPath(AUDIO_SYSTEM_DATA_ROOT "/wwise_project")
	, m_assetsPath(AUDIO_SYSTEM_DATA_ROOT "/" + string(CryAudio::Impl::Wwise::s_szImplFolderName) + "/" + string(CryAudio::s_szAssetsFolderName))
{}

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

	CProjectLoader(GetSettings()->GetProjectPath(), GetSettings()->GetAssetsPath(), m_rootItem, m_itemCache);

	if (preserveConnectionStatus)
	{
		for (auto const& connection : m_connectionsByID)
		{
			if (connection.second > 0)
			{
				auto const pImplItem = static_cast<CImplItem* const>(GetImplItem(connection.first));

				if (pImplItem != nullptr)
				{
					pImplItem->SetConnected(true);
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
IImplItem* CEditorImpl::GetImplItem(CID const id) const
{
	IImplItem* pImplItem = nullptr;

	if (id >= 0)
	{
		pImplItem = stl::find_in_map(m_itemCache, id, nullptr);
	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
char const* CEditorImpl::GetTypeIcon(IImplItem const* const pImplItem) const
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
bool CEditorImpl::IsTypeCompatible(ESystemItemType const systemType, IImplItem const* const pImplItem) const
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
ESystemItemType CEditorImpl::ImplTypeToSystemType(IImplItem const* const pImplItem) const
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
ConnectionPtr CEditorImpl::CreateConnectionToControl(ESystemItemType const controlType, IImplItem* const pImplItem)
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

			CImplItem* pImplItem = SearchForItem(&m_rootItem, name, static_cast<ItemType>(type));

			// If item not found, create a placeholder.
			// We want to keep that connection even if it's not in the middleware.
			// The user could be using the engine without the wwise project
			if (pImplItem == nullptr)
			{
				CID const id = GenerateID(name, isLocalized, &m_rootItem);
				EImplItemFlags const flags = isLocalized ? (EImplItemFlags::IsPlaceHolder | EImplItemFlags::IsLocalized) : EImplItemFlags::IsPlaceHolder;

				pImplItem = new CImplItem(name, id, static_cast<ItemType>(type), flags);

				m_itemCache[id] = pImplItem;
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

						CImplItem* pStateItem = nullptr;
						size_t const count = pImplItem->GetNumChildren();

						for (size_t i = 0; i < count; ++i)
						{
							auto const pChild = static_cast<CImplItem* const>(pImplItem->GetChildAt(i));

							if ((pChild != nullptr) && (pChild->GetName() == childName))
							{
								pStateItem = pChild;
							}
						}

						if (pStateItem == nullptr)
						{
							CID const id = GenerateID(childName);
							pStateItem = new CImplItem(childName, id, static_cast<ItemType>(type == EImpltemType::SwitchGroup ? EImpltemType::Switch : EImpltemType::State), EImplItemFlags::IsPlaceHolder);
							pImplItem->AddChild(pStateItem);

							m_itemCache[id] = pStateItem;
						}

						pImplItem = pStateItem;
					}
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[Audio Controls Editor] [Wwise] Error reading connection to Wwise control %s", name);
				}
			}

			if (pImplItem != nullptr)
			{
				if (type == EImpltemType::Parameter)
				{
					switch (controlType)
					{
					case ESystemItemType::Parameter:
					case ESystemItemType::Environment:
						{
							ParameterConnectionPtr const pConnection = std::make_shared<CParameterConnection>(pImplItem->GetId());
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
							StateConnectionPtr const pConnection = std::make_shared<CStateToParameterConnection>(pImplItem->GetId());
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
						pConnectionPtr = std::make_shared<CImplConnection>(pImplItem->GetId());
						break;
					}
				}
				else
				{
					pConnectionPtr = std::make_shared<CImplConnection>(pImplItem->GetId());
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

	auto const pImplItem = static_cast<CImplItem const* const>(GetImplItem(pConnection->GetID()));

	if (pImplItem != nullptr)
	{
		auto const itemType = static_cast<EImpltemType>(pImplItem->GetType());

		switch (itemType)
		{
		case EImpltemType::Switch:
		case EImpltemType::SwitchGroup:
		case EImpltemType::State:
		case EImpltemType::StateGroup:
			{
				IImplItem const* const pParent = pImplItem->GetParent();

				if (pParent != nullptr)
				{
					XmlNodeRef const pSwitchNode = GetISystem()->CreateXmlNode(TypeToTag(static_cast<EImpltemType>(pParent->GetType())));
					pSwitchNode->setAttr(CryAudio::s_szNameAttribute, pParent->GetName());

					XmlNodeRef const pStateNode = pSwitchNode->createNode(CryAudio::Impl::Wwise::s_szValueTag);
					pStateNode->setAttr(CryAudio::s_szNameAttribute, pImplItem->GetName());
					pSwitchNode->addChild(pStateNode);

					pNode = pSwitchNode;
				}
			}
			break;
		case EImpltemType::Parameter:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(itemType));
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pImplItem->GetName());

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
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pImplItem->GetName());
				pNode = pConnectionNode;
			}
			break;
		case EImpltemType::AuxBus:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(itemType));
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pImplItem->GetName());
				pNode = pConnectionNode;
			}
			break;
		case EImpltemType::SoundBank:
			{
				XmlNodeRef pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(itemType));
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pImplItem->GetName());

				if (pImplItem->IsLocalized())
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
	auto const pImplItem = static_cast<CImplItem* const>(GetImplItem(pConnection->GetID()));

	if (pImplItem != nullptr)
	{
		++m_connectionsByID[pImplItem->GetId()];
		pImplItem->SetConnected(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::DisableConnection(ConnectionPtr const pConnection)
{
	auto const pImplItem = static_cast<CImplItem* const>(GetImplItem(pConnection->GetID()));

	if (pImplItem != nullptr)
	{
		int connectionCount = m_connectionsByID[pImplItem->GetId()] - 1;

		if (connectionCount <= 0)
		{
			connectionCount = 0;
			pImplItem->SetConnected(false);
		}

		m_connectionsByID[pImplItem->GetId()] = connectionCount;
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::Clear()
{
	// Delete all the items
	for (auto const& itemPair : m_itemCache)
	{
		CImplItem const* const pImplItem = itemPair.second;

		if (pImplItem != nullptr)
		{
			delete pImplItem;
		}
	}

	m_itemCache.clear();

	// Clean up the root item
	m_rootItem.Clear();
}

//////////////////////////////////////////////////////////////////////////
CID CEditorImpl::GenerateID(string const& fullPathName) const
{
	return CryAudio::StringToId(fullPathName);
}

//////////////////////////////////////////////////////////////////////////
CID CEditorImpl::GenerateID(string const& name, bool isLocalized, CImplItem* pParent) const
{
	string pathName = (pParent != nullptr && !pParent->GetName().empty()) ? pParent->GetName() + "/" + name : name;

	if (isLocalized)
	{
		pathName = PathUtil::GetLocalizationFolder() + "/" + pathName;
	}

	return GenerateID(pathName);
}
} // namespace Wwise
} // namespace ACE
