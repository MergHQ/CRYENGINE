// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EditorImpl.h"

#include "Connection.h"
#include "ProjectLoader.h"
#include "Utils.h"

#include <CrySystem/ISystem.h>
#include <CryCore/StlUtils.h>
#include <CrySerialization/IArchiveHost.h>

namespace ACE
{
namespace Fmod
{
const string g_userSettingsFile = "%USER%/audiocontrolseditor_fmod.user";

//////////////////////////////////////////////////////////////////////////
EItemType TagToType(char const* const szTag)
{
	EItemType type = EItemType::None;

	if (_stricmp(szTag, CryAudio::s_szEventTag) == 0)
	{
		type = EItemType::Event;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Fmod::s_szParameterTag) == 0)
	{
		type = EItemType::Parameter;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Fmod::s_szSnapshotTag) == 0)
	{
		type = EItemType::Snapshot;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Fmod::s_szFileTag) == 0)
	{
		type = EItemType::Bank;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Fmod::s_szBusTag) == 0)
	{
		type = EItemType::Return;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Fmod::s_szVcaTag) == 0)
	{
		type = EItemType::VCA;
	}

	// Backwards compatibility will be removed before March 2019.
#if defined (USE_BACKWARDS_COMPATIBILITY)
	else if (_stricmp(szTag, "FmodEvent") == 0)
	{
		type = EItemType::Event;
	}
	else if (_stricmp(szTag, "FmodEventParameter") == 0)
	{
		type = EItemType::Parameter;
	}
	else if (_stricmp(szTag, "FmodSnapshot") == 0)
	{
		type = EItemType::Snapshot;
	}
	else if (_stricmp(szTag, "FmodSnapshotParameter") == 0)
	{
		type = EItemType::Parameter;
	}
	else if (_stricmp(szTag, "FmodFile") == 0)
	{
		type = EItemType::Bank;
	}
	else if (_stricmp(szTag, "FmodBus") == 0)
	{
		type = EItemType::Return;
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
		szTag = CryAudio::Impl::Fmod::s_szParameterTag;
		break;
	case EItemType::Snapshot:
		szTag = CryAudio::Impl::Fmod::s_szSnapshotTag;
		break;
	case EItemType::Bank:
		szTag = CryAudio::Impl::Fmod::s_szFileTag;
		break;
	case EItemType::Return:
		szTag = CryAudio::Impl::Fmod::s_szBusTag;
		break;
	case EItemType::VCA:
		szTag = CryAudio::Impl::Fmod::s_szVcaTag;
		break;
	default:
		szTag = nullptr;
		break;
	}

	return szTag;
}

//////////////////////////////////////////////////////////////////////////
string TypeToEditorFolderName(EItemType const type)
{
	string folderName = "";

	switch (type)
	{
	case EItemType::Event:
		folderName = s_eventsFolderName + "/";
		break;
	case EItemType::Parameter:
		folderName = s_parametersFolderName + "/";
		break;
	case EItemType::Snapshot:
		folderName = s_snapshotsFolderName + "/";
		break;
	case EItemType::Bank:
		folderName = s_soundBanksFolderName + "/";
		break;
	case EItemType::Return:
		folderName = s_returnsFolderName + "/";
		break;
	case EItemType::VCA:
		folderName = s_vcasFolderName + "/";
		break;
	default:
		folderName = "";
		break;
	}

	return folderName;
}

//////////////////////////////////////////////////////////////////////////
CItem* SearchForItem(CItem* const pItem, string const& name, EItemType const type)
{
	CItem* pSearchedItem = nullptr;

	if ((pItem->GetName() == name) && (pItem->GetType() == type))
	{
		pSearchedItem = pItem;
	}
	else
	{
		size_t const count = pItem->GetNumChildren();

		for (size_t i = 0; i < count; ++i)
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
	: m_projectPath(AUDIO_SYSTEM_DATA_ROOT "/fmod_project")
	, m_assetsPath(AUDIO_SYSTEM_DATA_ROOT "/" + string(CryAudio::Impl::Fmod::s_szImplFolderName) + "/" + string(CryAudio::s_szAssetsFolderName))
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
	m_implFolderName = CryAudio::Impl::Fmod::s_szImplFolderName;
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

	CProjectLoader(GetSettings()->GetProjectPath(), GetSettings()->GetAssetsPath(), m_rootItem, m_itemCache, *this);

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
		case EItemType::Folder:
			szIconPath = "icons:audio/fmod/folder_closed.ico";
			break;
		case EItemType::Event:
			szIconPath = "icons:audio/fmod/event.ico";
			break;
		case EItemType::Parameter:
			szIconPath = "icons:audio/fmod/tag.ico";
			break;
		case EItemType::Snapshot:
			szIconPath = "icons:audio/fmod/snapshot.ico";
			break;
		case EItemType::Bank:
			szIconPath = "icons:audio/fmod/bank.ico";
			break;
		case EItemType::Return:
			szIconPath = "icons:audio/fmod/return.ico";
			break;
		case EItemType::VCA:
			szIconPath = "icons:audio/fmod/vca.ico";
			break;
		case EItemType::MixerGroup:
			szIconPath = "icons:audio/fmod/group.ico";
			break;
		case EItemType::EditorFolder:
			szIconPath = "icons:General/Folder.ico";
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
			isCompatible = (implType == EItemType::Event) || (implType == EItemType::Snapshot);
			break;
		case EAssetType::Parameter:
			isCompatible = (implType == EItemType::Parameter) || (implType == EItemType::VCA);
			break;
		case EAssetType::State:
			isCompatible = (implType == EItemType::Parameter) || (implType == EItemType::VCA);
			break;
		case EAssetType::Environment:
			isCompatible = (implType == EItemType::Return) || (implType == EItemType::Parameter);
			break;
		case EAssetType::Preload:
			isCompatible = (implType == EItemType::Bank);
			break;
		default:
			isCompatible = false;
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
		case EItemType::VCA:
			assetType = EAssetType::Parameter;
			break;
		case EItemType::Snapshot:
			assetType = EAssetType::Trigger;
			break;
		case EItemType::Bank:
			assetType = EAssetType::Preload;
			break;
		case EItemType::Return:
			assetType = EAssetType::Environment;
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
		EItemType const type = pItem->GetType();

		if (type == EItemType::Event)
		{
			pConnection = std::make_shared<CEventConnection>(pItem->GetId());
		}
		else if (type == EItemType::Snapshot)
		{
			pConnection = std::make_shared<CSnapshotConnection>(pItem->GetId());
		}
		else if ((type == EItemType::Parameter) || (type == EItemType::VCA))
		{
			if ((assetType == EAssetType::Parameter) || (assetType == EAssetType::Environment))
			{
				pConnection = std::make_shared<CParameterConnection>(pItem->GetId());
			}
			else if (assetType == EAssetType::State)
			{
				pConnection = std::make_shared<CParameterToStateConnection>(pItem->GetId(), type);
			}
			else
			{
				pConnection = std::make_shared<CBaseConnection>(pItem->GetId());
			}
		}
		else if (type == EItemType::Bank)
		{
			pConnection = std::make_shared<CBankConnection>(pItem->GetId());
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
		auto const type = TagToType(pNode->getTag());

		if (type != EItemType::None)
		{
			string name = pNode->getAttr(CryAudio::s_szNameAttribute);
			string localizedAttribute = pNode->getAttr(CryAudio::Impl::Fmod::s_szLocalizedAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
			if (name.IsEmpty() && pNode->haveAttr("fmod_name"))
			{
				name = pNode->getAttr("fmod_name");
			}

			if (localizedAttribute.IsEmpty() && pNode->haveAttr("fmod_localized"))
			{
				localizedAttribute = pNode->getAttr("fmod_localized");
			}
#endif    // USE_BACKWARDS_COMPATIBILITY
			bool const isLocalized = (localizedAttribute.compareNoCase(CryAudio::Impl::Fmod::s_szTrueValue) == 0);

			CItem* pItem = nullptr;

			if (type != EItemType::Parameter)
			{
				pItem = GetItemFromPath(TypeToEditorFolderName(type) + name);
			}
			else
			{
				pItem = SearchForItem(&m_rootItem, name, type);
			}

			if ((pItem == nullptr) || (type != pItem->GetType()))
			{
				// If item not found, create a placeholder.
				// We want to keep that connection even if it's not in the middleware as the user could
				// be using the engine without the fmod project

				string path = "";
				int const pos = name.find_last_of("/");

				if (pos != string::npos)
				{
					path = name.substr(0, pos);
					name = name.substr(pos + 1, name.length() - pos);
				}

				pItem = CreateItem(name, type, CreatePlaceholderFolderPath(path));

				if (pItem != nullptr)
				{
					pItem->SetPlaceholder(true);
				}
			}

			switch (type)
			{
			case EItemType::Event:
				{
					string actionType = pNode->getAttr(CryAudio::s_szTypeAttribute);

#if defined (USE_BACKWARDS_COMPATIBILITY)
					if (actionType.IsEmpty() && pNode->haveAttr("fmod_event_type"))
					{
						actionType = pNode->getAttr("fmod_event_type");
					}
#endif        // USE_BACKWARDS_COMPATIBILITY

					EEventActionType eventActionType = EEventActionType::Start;

					if (actionType.compareNoCase(CryAudio::Impl::Fmod::s_szStopValue) == 0)
					{
						eventActionType = EEventActionType::Stop;
					}
					else if (actionType.compareNoCase(CryAudio::Impl::Fmod::s_szPauseValue) == 0)
					{
						eventActionType = EEventActionType::Pause;
					}
					else if (actionType.compareNoCase(CryAudio::Impl::Fmod::s_szResumeValue) == 0)
					{
						eventActionType = EEventActionType::Resume;
					}

					auto const pConnection = std::make_shared<CEventConnection>(pItem->GetId(), eventActionType);
					pConnectionPtr = pConnection;
				}
				break;
			case EItemType::Snapshot:
				{
					string actionType = pNode->getAttr(CryAudio::s_szTypeAttribute);

#if defined (USE_BACKWARDS_COMPATIBILITY)
					if (actionType.IsEmpty() && pNode->haveAttr("fmod_event_type"))
					{
						actionType = pNode->getAttr("fmod_event_type");
					}
#endif        // USE_BACKWARDS_COMPATIBILITY

					ESnapshotActionType const snapshotActionType = (actionType.compareNoCase(CryAudio::Impl::Fmod::s_szStopValue) == 0) ? ESnapshotActionType::Stop : ESnapshotActionType::Start;
					auto const pConnection = std::make_shared<CSnapshotConnection>(pItem->GetId(), snapshotActionType);
					pConnectionPtr = pConnection;
				}
				break;
			case EItemType::Parameter:
			case EItemType::VCA:
				{
					if ((assetType == EAssetType::Parameter) || (assetType == EAssetType::Environment))
					{
						float mult = CryAudio::Impl::Fmod::s_defaultParamMultiplier;
						float shift = CryAudio::Impl::Fmod::s_defaultParamShift;

						if (pNode->haveAttr(CryAudio::Impl::Fmod::s_szMutiplierAttribute))
						{
							string const value = pNode->getAttr(CryAudio::Impl::Fmod::s_szMutiplierAttribute);
							mult = (float)std::atof(value.c_str());
						}
#if defined (USE_BACKWARDS_COMPATIBILITY)
						else if (pNode->haveAttr("fmod_value_multiplier"))
						{
							string const value = pNode->getAttr("fmod_value_multiplier");
							mult = (float)std::atof(value.c_str());
						}
#endif          // USE_BACKWARDS_COMPATIBILITY
						if (pNode->haveAttr(CryAudio::Impl::Fmod::s_szShiftAttribute))
						{
							string const value = pNode->getAttr(CryAudio::Impl::Fmod::s_szShiftAttribute);
							shift = (float)std::atof(value.c_str());
						}
#if defined (USE_BACKWARDS_COMPATIBILITY)
						else if (pNode->haveAttr("fmod_value_shift"))
						{
							string const value = pNode->getAttr("fmod_value_shift");
							shift = (float)std::atof(value.c_str());
						}
#endif          // USE_BACKWARDS_COMPATIBILITY

						auto const pConnection = std::make_shared<CParameterConnection>(pItem->GetId(), mult, shift);
						pConnectionPtr = pConnection;
					}
					else if (assetType == EAssetType::State)
					{
						string valueString = pNode->getAttr(CryAudio::Impl::Fmod::s_szValueAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
						if (valueString.IsEmpty() && pNode->haveAttr("fmod_value"))
						{
							valueString = pNode->getAttr("fmod_value");
						}
#endif          // USE_BACKWARDS_COMPATIBILITY

						auto const valueFloat = static_cast<float>(std::atof(valueString.c_str()));
						auto const pConnection = std::make_shared<CParameterToStateConnection>(pItem->GetId(), type, valueFloat);
						pConnectionPtr = pConnection;
					}
				}
				break;
			case EItemType::Bank:
				{
					pConnectionPtr = std::make_shared<CBankConnection>(pItem->GetId());
				}
				break;
			case EItemType::Return:
				{
					pConnectionPtr = std::make_shared<CBaseConnection>(pItem->GetId());
				}
				break;
			}
		}
	}

	return pConnectionPtr;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CEditorImpl::CreateXMLNodeFromConnection(ConnectionPtr const pConnection, EAssetType const assetType)
{
	XmlNodeRef pNode = nullptr;

	auto const pItem = static_cast<CItem* const>(GetItem(pConnection->GetID()));

	if (pItem != nullptr)
	{
		auto const type = pItem->GetType();
		pNode = GetISystem()->CreateXmlNode(TypeToTag(type));

		switch (type)
		{
		case EItemType::Event:
			{
				pNode->setAttr(CryAudio::s_szNameAttribute, Utils::GetPathName(pItem, m_rootItem));
				auto const pEventConnection = static_cast<const CEventConnection*>(pConnection.get());

				if (pEventConnection != nullptr)
				{
					if (pEventConnection->GetActionType() == EEventActionType::Stop)
					{
						pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::Fmod::s_szStopValue);
					}
					else if (pEventConnection->GetActionType() == EEventActionType::Pause)
					{
						pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::Fmod::s_szPauseValue);
					}
					else if (pEventConnection->GetActionType() == EEventActionType::Resume)
					{
						pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::Fmod::s_szResumeValue);
					}
				}
			}
			break;
		case EItemType::Snapshot:
			{
				pNode->setAttr(CryAudio::s_szNameAttribute, Utils::GetPathName(pItem, m_rootItem));
				auto const pEventConnection = static_cast<const CSnapshotConnection*>(pConnection.get());

				if ((pEventConnection != nullptr) && (pEventConnection->GetActionType() == ESnapshotActionType::Stop))
				{
					pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::Fmod::s_szStopValue);
				}
			}
			break;
		case EItemType::Return:
			{
				pNode->setAttr(CryAudio::s_szNameAttribute, Utils::GetPathName(pItem, m_rootItem));
			}
			break;
		case EItemType::Parameter:
		case EItemType::VCA:
			{
				pNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

				if (assetType == EAssetType::State)
				{
					auto const pStateConnection = static_cast<const CParameterToStateConnection*>(pConnection.get());

					if (pStateConnection != nullptr)
					{
						pNode->setAttr(CryAudio::Impl::Fmod::s_szValueAttribute, pStateConnection->GetValue());
					}
				}
				else if ((assetType == EAssetType::Parameter) || (assetType == EAssetType::Environment))
				{
					auto const pParamConnection = static_cast<const CParameterConnection*>(pConnection.get());

					if (pParamConnection->GetMultiplier() != CryAudio::Impl::Fmod::s_defaultParamMultiplier)
					{
						pNode->setAttr(CryAudio::Impl::Fmod::s_szMutiplierAttribute, pParamConnection->GetMultiplier());
					}

					if (pParamConnection->GetShift() != CryAudio::Impl::Fmod::s_defaultParamShift)
					{
						pNode->setAttr(CryAudio::Impl::Fmod::s_szShiftAttribute, pParamConnection->GetShift());
					}
				}
			}
			break;
		case EItemType::Bank:
			{
				pNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

				if (pItem->IsLocalized())
				{
					pNode->setAttr(CryAudio::Impl::Fmod::s_szLocalizedAttribute, CryAudio::Impl::Fmod::s_szTrueValue);
				}
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
CItem* CEditorImpl::CreateItem(string const& name, EItemType const type, CItem* const pParent)
{
	ControlId const id = Utils::GetId(type, name, pParent, m_rootItem);

	auto pItem = static_cast<CItem*>(GetItem(id));

	if (pItem != nullptr)
	{
		if (pItem->IsPlaceholder())
		{
			pItem->SetPlaceholder(false);
			CItem* pParentItem = pParent;

			while (pParentItem != nullptr)
			{
				pParentItem->SetPlaceholder(false);
				pParentItem = static_cast<CItem*>(pParentItem->GetParent());
			}
		}
	}
	else
	{
		if (type == EItemType::EditorFolder)
		{
			pItem = new CItem(name, id, type, EItemFlags::IsContainer);
		}
		else
		{
			pItem = new CItem(name, id, type);
		}

		if (pParent != nullptr)
		{
			pParent->AddChild(pItem);
		}
		else
		{
			m_rootItem.AddChild(pItem);
		}

		m_itemCache[id] = pItem;

	}

	return pItem;
}

//////////////////////////////////////////////////////////////////////////
CItem* CEditorImpl::GetItemFromPath(string const& fullpath)
{
	CItem* pItem = &m_rootItem;
	int start = 0;
	string token = fullpath.Tokenize("/", start);

	while (!token.empty())
	{
		token.Trim();
		CItem* pChild = nullptr;
		int const size = pItem->GetNumChildren();

		for (int i = 0; i < size; ++i)
		{
			auto const pNextChild = static_cast<CItem* const>(pItem->GetChildAt(i));

			if ((pNextChild != nullptr) && (pNextChild->GetName() == token))
			{
				pChild = pNextChild;
				break;
			}
		}

		if (pChild != nullptr)
		{
			pItem = pChild;
			token = fullpath.Tokenize("/", start);
		}
		else
		{
			pItem = nullptr;
			break;
		}
	}

	return pItem;
}

//////////////////////////////////////////////////////////////////////////
CItem* CEditorImpl::CreatePlaceholderFolderPath(string const& path)
{
	CItem* pItem = &m_rootItem;
	int start = 0;
	string token = path.Tokenize("/", start);

	while (!token.empty())
	{
		token.Trim();
		CItem* pFoundChild = nullptr;
		int const size = pItem->GetNumChildren();

		for (int i = 0; i < size; ++i)
		{
			auto const pChild = static_cast<CItem* const>(pItem->GetChildAt(i));

			if ((pChild != nullptr) && (pChild->GetName() == token))
			{
				pFoundChild = pChild;
				break;
			}
		}

		if (pFoundChild == nullptr)
		{
			pFoundChild = CreateItem(token, EItemType::Folder, pItem);
			pFoundChild->SetPlaceholder(true);
		}

		pItem = pFoundChild;
		token = path.Tokenize("/", start);
	}

	return pItem;
}
} // namespace Fmod
} // namespace ACE
