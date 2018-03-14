// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EditorImpl.h"

#include "Connection.h"
#include "ProjectLoader.h"

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
EItemType TagToType(char const* const szTag)
{
	EItemType type = EItemType::None;

	if (_stricmp(szTag, CryAudio::s_szEventTag) == 0)
	{
		type = EItemType::Event;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Wwise::s_szFileTag) == 0)
	{
		type = EItemType::SoundBank;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Wwise::s_szParameterTag) == 0)
	{
		type = EItemType::Parameter;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Wwise::s_szSwitchGroupTag) == 0)
	{
		type = EItemType::SwitchGroup;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Wwise::s_szStateGroupTag) == 0)
	{
		type = EItemType::StateGroup;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Wwise::s_szAuxBusTag) == 0)
	{
		type = EItemType::AuxBus;
	}

	// Backwards compatibility will be removed before March 2019.
#if defined (USE_BACKWARDS_COMPATIBILITY)
	else if (_stricmp(szTag, "WwiseEvent") == 0)
	{
		type = EItemType::Event;
	}
	else if (_stricmp(szTag, "WwiseFile") == 0)
	{
		type = EItemType::SoundBank;
	}
	else if (_stricmp(szTag, "WwiseRtpc") == 0)
	{
		type = EItemType::Parameter;
	}
	else if (_stricmp(szTag, "WwiseSwitch") == 0)
	{
		type = EItemType::SwitchGroup;
	}
	else if (_stricmp(szTag, "WwiseState") == 0)
	{
		type = EItemType::StateGroup;
	}
	else if (_stricmp(szTag, "WwiseAuxBus") == 0)
	{
		type = EItemType::AuxBus;
	}
#endif // USE_BACKWARDS_COMPATIBILITY

	else
	{
		type = EItemType::None;
	}

	return type;
}

//////////////////////////////////////////////////////////////////////////
char const* TypeToTag(EItemType const type)
{
	char const* szTag = nullptr;

	switch (type)
	{
	case EItemType::Event:
		szTag = CryAudio::s_szEventTag;
		break;
	case EItemType::Parameter:
		szTag = CryAudio::Impl::Wwise::s_szParameterTag;
		break;
	case EItemType::Switch:
		szTag = CryAudio::Impl::Wwise::s_szValueTag;
		break;
	case EItemType::AuxBus:
		szTag = CryAudio::Impl::Wwise::s_szAuxBusTag;
		break;
	case EItemType::SoundBank:
		szTag = CryAudio::Impl::Wwise::s_szFileTag;
		break;
	case EItemType::State:
		szTag = CryAudio::Impl::Wwise::s_szValueTag;
		break;
	case EItemType::SwitchGroup:
		szTag = CryAudio::Impl::Wwise::s_szSwitchGroupTag;
		break;
	case EItemType::StateGroup:
		szTag = CryAudio::Impl::Wwise::s_szStateGroupTag;
		break;
	default:
		szTag = nullptr;
		break;
	}

	return szTag;
}

//////////////////////////////////////////////////////////////////////////
CItem* SearchForItem(CItem* const pItem, string const& name, EItemType const type)
{
	CItem* pSearchedItem = nullptr;

	if ((pItem->GetName().compareNoCase(name) == 0) && (pItem->GetType() == type))
	{
		pSearchedItem = pItem;
	}
	else
	{
		int const count = pItem->GetNumChildren();

		for (int i = 0; i < count; ++i)
		{
			CItem* const pFoundItem = SearchForItem(static_cast<CItem* const>(pItem->GetChildAt(i)), name, type);

			if (pFoundItem != nullptr)
			{
				pSearchedItem = pFoundItem;
				break;
			}
		}
	}

	return pSearchedItem;
}

//////////////////////////////////////////////////////////////////////////
CSettings::CSettings()
	: m_projectPath(AUDIO_SYSTEM_DATA_ROOT "/wwise_project")
	, m_assetsPath(AUDIO_SYSTEM_DATA_ROOT "/" + string(CryAudio::Impl::Wwise::s_szImplFolderName) + "/" + string(CryAudio::s_szAssetsFolderName))
{}

//////////////////////////////////////////////////////////////////////////
void CSettings::SetProjectPath(char const* szPath)
{
	m_projectPath = szPath;
	Serialization::SaveJsonFile(g_userSettingsFile.c_str(), *this);
}

//////////////////////////////////////////////////////////////////////////
void CSettings::Serialize(Serialization::IArchive& ar)
{
	ar(m_projectPath, "projectPath", "Project Path");
}

//////////////////////////////////////////////////////////////////////////
CEditorImpl::CEditorImpl()
{
	Serialization::LoadJsonFile(m_settings, g_userSettingsFile.c_str());
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
				auto const pItem = static_cast<CItem* const>(GetItem(connection.first));

				if (pItem != nullptr)
				{
					pItem->SetConnected(true);
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
IImplItem* CEditorImpl::GetItem(ControlId const id) const
{
	IImplItem* pIImplItem = nullptr;

	if (id >= 0)
	{
		pIImplItem = stl::find_in_map(m_itemCache, id, nullptr);
	}

	return pIImplItem;
}

//////////////////////////////////////////////////////////////////////////
char const* CEditorImpl::GetTypeIcon(IImplItem const* const pIImplItem) const
{
	char const* szIconPath = "icons:Dialogs/dialog-error.ico";
	auto const pItem = static_cast<CItem const* const>(pIImplItem);

	if (pItem != nullptr)
	{
		EItemType const type = pItem->GetType();

		switch (type)
		{
		case EItemType::Event:
			szIconPath = "icons:audio/wwise/event.ico";
			break;
		case EItemType::Parameter:
			szIconPath = "icons:audio/wwise/gameparameter.ico";
			break;
		case EItemType::Switch:
			szIconPath = "icons:audio/wwise/switch.ico";
			break;
		case EItemType::AuxBus:
			szIconPath = "icons:audio/wwise/auxbus.ico";
			break;
		case EItemType::SoundBank:
			szIconPath = "icons:audio/wwise/soundbank.ico";
			break;
		case EItemType::State:
			szIconPath = "icons:audio/wwise/state.ico";
			break;
		case EItemType::SwitchGroup:
			szIconPath = "icons:audio/wwise/switchgroup.ico";
			break;
		case EItemType::StateGroup:
			szIconPath = "icons:audio/wwise/stategroup.ico";
			break;
		case EItemType::WorkUnit:
			szIconPath = "icons:audio/wwise/workunit.ico";
			break;
		case EItemType::VirtualFolder:
			szIconPath = "icons:audio/wwise/virtualfolder.ico";
			break;
		case EItemType::PhysicalFolder:
			szIconPath = "icons:audio/wwise/physicalfolder.ico";
			break;
		default:
			szIconPath = "icons:Dialogs/dialog-error.ico";
			break;
		}
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
bool CEditorImpl::IsTypeCompatible(EAssetType const assetType, IImplItem const* const pIImplItem) const
{
	bool isCompatible = false;
	auto const pItem = static_cast<CItem const* const>(pIImplItem);

	if (pItem != nullptr)
	{
		EItemType const implType = pItem->GetType();

		switch (assetType)
		{
		case EAssetType::Trigger:
			isCompatible = (implType == EItemType::Event);
			break;
		case EAssetType::Parameter:
			isCompatible = (implType == EItemType::Parameter);
			break;
		case EAssetType::State:
			isCompatible = (implType == EItemType::Switch) || (implType == EItemType::State) || (implType == EItemType::Parameter);
			break;
		case EAssetType::Environment:
			isCompatible = (implType == EItemType::AuxBus) || (implType == EItemType::Parameter);
			break;
		case EAssetType::Preload:
			isCompatible = (implType == EItemType::SoundBank);
			break;
		}
	}

	return isCompatible;
}

//////////////////////////////////////////////////////////////////////////
EAssetType CEditorImpl::ImplTypeToAssetType(IImplItem const* const pIImplItem) const
{
	EAssetType assetType = EAssetType::None;
	auto const pItem = static_cast<CItem const* const>(pIImplItem);

	if (pItem != nullptr)
	{
		EItemType const implType = pItem->GetType();

		switch (implType)
		{
		case EItemType::Event:
			assetType = EAssetType::Trigger;
			break;
		case EItemType::Parameter:
			assetType = EAssetType::Parameter;
			break;
		case EItemType::Switch:
		case EItemType::State:
			assetType = EAssetType::State;
			break;
		case EItemType::AuxBus:
			assetType = EAssetType::Environment;
			break;
		case EItemType::SoundBank:
			assetType = EAssetType::Preload;
			break;
		case EItemType::StateGroup:
		case EItemType::SwitchGroup:
			assetType = EAssetType::Switch;
			break;
		default:
			assetType = EAssetType::None;
			break;
		}
	}

	return assetType;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CEditorImpl::CreateConnectionToControl(EAssetType const assetType, IImplItem const* const pIImplItem)
{
	ConnectionPtr pConnection = nullptr;
	auto const pItem = static_cast<CItem const* const>(pIImplItem);

	if (pItem != nullptr)
	{
		EItemType itemType = pItem->GetType();

		if (itemType == EItemType::Parameter)
		{
			switch (assetType)
			{
			case EAssetType::Parameter:
			case EAssetType::Environment:
				pConnection = std::make_shared<CParameterConnection>(pItem->GetId());
				break;
			case EAssetType::State:
				pConnection = std::make_shared<CParameterToStateConnection>(pItem->GetId());
				break;
			default:
				pConnection = std::make_shared<CBaseConnection>(pItem->GetId());
				break;
			}
		}
		else if (itemType == EItemType::SoundBank)
		{
			pConnection = std::make_shared<CSoundBankConnection>(pItem->GetId());
		}
		else
		{
			pConnection = std::make_shared<CBaseConnection>(pItem->GetId());
		}

	}

	return pConnection;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CEditorImpl::CreateConnectionFromXMLNode(XmlNodeRef pNode, EAssetType const assetType)
{
	ConnectionPtr pConnectionPtr = nullptr;

	if (pNode != nullptr)
	{
		EItemType const type = TagToType(pNode->getTag());

		if (type != EItemType::None)
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

			CItem* pItem = SearchForItem(&m_rootItem, name, type);

			// If item not found, create a placeholder.
			// We want to keep that connection even if it's not in the middleware.
			// The user could be using the engine without the wwise project
			if (pItem == nullptr)
			{
				ControlId const id = GenerateID(name, isLocalized, &m_rootItem);
				EItemFlags const flags = isLocalized ? (EItemFlags::IsPlaceHolder | EItemFlags::IsLocalized) : EItemFlags::IsPlaceHolder;

				pItem = new CItem(name, id, type, flags);

				m_itemCache[id] = pItem;
			}

			// If it's a switch we actually connect to one of the states within the switch
			if ((type == EItemType::SwitchGroup) || (type == EItemType::StateGroup))
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

						CItem* pStateItem = nullptr;
						size_t const count = pItem->GetNumChildren();

						for (size_t i = 0; i < count; ++i)
						{
							auto const pChild = static_cast<CItem* const>(pItem->GetChildAt(i));

							if ((pChild != nullptr) && (pChild->GetName().compareNoCase(childName) == 0))
							{
								pStateItem = pChild;
							}
						}

						if (pStateItem == nullptr)
						{
							ControlId const id = GenerateID(childName);
							pStateItem = new CItem(childName, id, type == EItemType::SwitchGroup ? EItemType::Switch : EItemType::State, EItemFlags::IsPlaceHolder);
							pItem->AddChild(pStateItem);

							m_itemCache[id] = pStateItem;
						}

						pItem = pStateItem;
					}
				}
				else
				{
					CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[Audio Controls Editor] [Wwise] Error reading connection to Wwise control %s", name);
				}
			}

			if (pItem != nullptr)
			{
				if (type == EItemType::Parameter)
				{
					switch (assetType)
					{
					case EAssetType::Parameter:
					case EAssetType::Environment:
						{

							float mult = CryAudio::Impl::Wwise::s_defaultParamMultiplier;
							float shift = CryAudio::Impl::Wwise::s_defaultParamShift;

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

							auto const pConnection = std::make_shared<CParameterConnection>(pItem->GetId(), mult, shift);
							pConnectionPtr = pConnection;
						}
						break;
					case EAssetType::State:
						{
							float value = CryAudio::Impl::Wwise::s_defaultStateValue;

							pNode->getAttr(CryAudio::Impl::Wwise::s_szValueAttribute, value);
#if defined (USE_BACKWARDS_COMPATIBILITY)
							if (pNode->haveAttr("wwise_value"))
							{
								pNode->getAttr("wwise_value", value);
							}
#endif            // USE_BACKWARDS_COMPATIBILITY

							auto const pConnection = std::make_shared<CParameterToStateConnection>(pItem->GetId(), value);
							pConnectionPtr = pConnection;
						}
						break;
					default:
						pConnectionPtr = std::make_shared<CBaseConnection>(pItem->GetId());
						break;
					}
				}
				else if (type == EItemType::SoundBank)
				{
					pConnectionPtr = std::make_shared<CSoundBankConnection>(pItem->GetId());
				}
				else
				{
					pConnectionPtr = std::make_shared<CBaseConnection>(pItem->GetId());
				}
			}
		}
	}

	return pConnectionPtr;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CEditorImpl::CreateXMLNodeFromConnection(ConnectionPtr const pConnection, EAssetType const assetType)
{
	XmlNodeRef pNode = nullptr;

	auto const pItem = static_cast<CItem const* const>(GetItem(pConnection->GetID()));

	if (pItem != nullptr)
	{
		auto const itemType = static_cast<EItemType>(pItem->GetType());

		switch (itemType)
		{
		case EItemType::Switch:
		case EItemType::SwitchGroup:
		case EItemType::State:
		case EItemType::StateGroup:
			{
				auto const pParent = static_cast<CItem const* const>(pItem->GetParent());

				if (pParent != nullptr)
				{
					XmlNodeRef const pSwitchNode = GetISystem()->CreateXmlNode(TypeToTag(pParent->GetType()));
					pSwitchNode->setAttr(CryAudio::s_szNameAttribute, pParent->GetName());

					XmlNodeRef const pStateNode = pSwitchNode->createNode(CryAudio::Impl::Wwise::s_szValueTag);
					pStateNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());
					pSwitchNode->addChild(pStateNode);

					pNode = pSwitchNode;
				}
			}
			break;
		case EItemType::Parameter:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(itemType));
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

				if ((assetType == EAssetType::Parameter) || (assetType == EAssetType::Environment))
				{
					std::shared_ptr<const CParameterConnection> pParameterConnection = std::static_pointer_cast<const CParameterConnection>(pConnection);

					float const mult = pParameterConnection->GetMultiplier();

					if (mult != CryAudio::Impl::Wwise::s_defaultParamMultiplier)
					{
						pConnectionNode->setAttr(CryAudio::Impl::Wwise::s_szMutiplierAttribute, mult);
					}

					float const shift = pParameterConnection->GetShift();

					if (shift != CryAudio::Impl::Wwise::s_defaultParamShift)
					{
						pConnectionNode->setAttr(CryAudio::Impl::Wwise::s_szShiftAttribute, shift);
					}

				}
				else if (assetType == EAssetType::State)
				{
					std::shared_ptr<const CParameterToStateConnection> pStateConnection = std::static_pointer_cast<const CParameterToStateConnection>(pConnection);
					pConnectionNode->setAttr(CryAudio::Impl::Wwise::s_szValueAttribute, pStateConnection->GetValue());
				}

				pNode = pConnectionNode;
			}
			break;
		case EItemType::Event:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(itemType));
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());
				pNode = pConnectionNode;
			}
			break;
		case EItemType::AuxBus:
			{
				XmlNodeRef pConnectionNode;
				pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(itemType));
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());
				pNode = pConnectionNode;
			}
			break;
		case EItemType::SoundBank:
			{
				XmlNodeRef pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(itemType));
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

				if (pItem->IsLocalized())
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
	auto const pItem = static_cast<CItem* const>(GetItem(pConnection->GetID()));

	if (pItem != nullptr)
	{
		++m_connectionsByID[pItem->GetId()];
		pItem->SetConnected(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::DisableConnection(ConnectionPtr const pConnection)
{
	auto const pItem = static_cast<CItem* const>(GetItem(pConnection->GetID()));

	if (pItem != nullptr)
	{
		int connectionCount = m_connectionsByID[pItem->GetId()] - 1;

		if (connectionCount < 1)
		{
			CRY_ASSERT_MESSAGE(connectionCount >= 0, "Connection count is < 0");
			connectionCount = 0;
			pItem->SetConnected(false);
		}

		m_connectionsByID[pItem->GetId()] = connectionCount;
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::Clear()
{
	for (auto const& itemPair : m_itemCache)
	{
		delete itemPair.second;
	}

	m_itemCache.clear();
	m_rootItem.Clear();
}

//////////////////////////////////////////////////////////////////////////
ControlId CEditorImpl::GenerateID(string const& fullPathName) const
{
	return CryAudio::StringToId(fullPathName);
}

//////////////////////////////////////////////////////////////////////////
ControlId CEditorImpl::GenerateID(string const& name, bool isLocalized, CItem* pParent) const
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
