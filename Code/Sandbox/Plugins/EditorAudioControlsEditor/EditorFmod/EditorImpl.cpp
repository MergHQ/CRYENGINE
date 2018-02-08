// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EditorImpl.h"

#include "ImplConnections.h"
#include "ProjectLoader.h"
#include "ImplUtils.h"

#include <CryAudioImplFmod/GlobalData.h>
#include <CrySystem/ISystem.h>
#include <CryCore/StlUtils.h>
#include <CrySerialization/IArchiveHost.h>

namespace ACE
{
namespace Fmod
{
const string g_userSettingsFile = "%USER%/audiocontrolseditor_fmod.user";

//////////////////////////////////////////////////////////////////////////
EImplItemType TagToType(string const& tag)
{
	EImplItemType type = EImplItemType::Invalid;

	if (tag == CryAudio::s_szEventTag)
	{
		type = EImplItemType::Event;
	}
	else if (tag == CryAudio::Impl::Fmod::s_szParameterTag)
	{
		type = EImplItemType::Parameter;
	}
	else if (tag == CryAudio::Impl::Fmod::s_szSnapshotTag)
	{
		type = EImplItemType::Snapshot;
	}
	else if (tag == CryAudio::Impl::Fmod::s_szFileTag)
	{
		type = EImplItemType::Bank;
	}
	else if (tag == CryAudio::Impl::Fmod::s_szBusTag)
	{
		type = EImplItemType::Return;
	}
	else if (tag == CryAudio::Impl::Fmod::s_szVcaTag)
	{
		type = EImplItemType::VCA;
	}

	// Backwards compatibility will be removed before March 2019.
#if defined (USE_BACKWARDS_COMPATIBILITY)
	else if (tag == "FmodEvent")
	{
		type = EImplItemType::Event;
	}
	else if (tag == "FmodEventParameter")
	{
		type = EImplItemType::Parameter;
	}
	else if (tag == "FmodSnapshot")
	{
		type = EImplItemType::Snapshot;
	}
	else if (tag == "FmodSnapshotParameter")
	{
		type = EImplItemType::Parameter;
	}
	else if (tag == "FmodFile")
	{
		type = EImplItemType::Bank;
	}
	else if (tag == "FmodBus")
	{
		type = EImplItemType::Return;
	}
#endif // USE_BACKWARDS_COMPATIBILITY

	else
	{
		type = EImplItemType::Invalid;
	}

	return type;
}

//////////////////////////////////////////////////////////////////////////
char const* TypeToTag(EImplItemType const type)
{
	char const* tag = nullptr;

	switch (type)
	{
	case EImplItemType::Event:
		tag = CryAudio::s_szEventTag;
		break;
	case EImplItemType::Parameter:
		tag = CryAudio::Impl::Fmod::s_szParameterTag;
		break;
	case EImplItemType::Snapshot:
		tag = CryAudio::Impl::Fmod::s_szSnapshotTag;
		break;
	case EImplItemType::Bank:
		tag = CryAudio::Impl::Fmod::s_szFileTag;
		break;
	case EImplItemType::Return:
		tag = CryAudio::Impl::Fmod::s_szBusTag;
		break;
	case EImplItemType::VCA:
		tag = CryAudio::Impl::Fmod::s_szVcaTag;
		break;
	default:
		tag = nullptr;
		break;
	}

	return tag;
}

//////////////////////////////////////////////////////////////////////////
string TypeToEditorFolderName(EImplItemType const type)
{
	string folderName = "";

	switch (type)
	{
	case EImplItemType::Event:
		folderName = g_eventsFolderName + "/";
		break;
	case EImplItemType::Parameter:
		folderName = g_parametersFolderName + "/";
		break;
	case EImplItemType::Snapshot:
		folderName = g_snapshotsFolderName + "/";
		break;
	case EImplItemType::Bank:
		folderName = g_soundBanksFolderName + "/";
		break;
	case EImplItemType::Return:
		folderName = g_returnsFolderName + "/";
		break;
	case EImplItemType::VCA:
		folderName = g_vcasFolderName + "/";
		break;
	default:
		folderName = "";
		break;
	}

	return folderName;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* SearchForControl(CImplItem* const pImplItem, string const& name, ItemType const type)
{
	CImplItem* pItem = nullptr;

	if ((pImplItem->GetName() == name) && (pImplItem->GetType() == type))
	{
		pItem = pImplItem;
	}
	else
	{
		int const count = pImplItem->ChildCount();

		for (int i = 0; i < count; ++i)
		{
			CImplItem* const pFoundImplControl = SearchForControl(pImplItem->GetChildAt(i), name, type);

			if (pFoundImplControl != nullptr)
			{
				pItem = pFoundImplControl;
				break;
			}
		}
	}

	return pItem;
}

//////////////////////////////////////////////////////////////////////////
CImplSettings::CImplSettings()
	: m_projectPath(PathUtil::GetGameFolder() + CRY_NATIVE_PATH_SEPSTR AUDIO_SYSTEM_DATA_ROOT CRY_NATIVE_PATH_SEPSTR "fmod_project")
	, m_assetsPath(
	  PathUtil::GetGameFolder() +
	  CRY_NATIVE_PATH_SEPSTR
	  AUDIO_SYSTEM_DATA_ROOT
	  CRY_NATIVE_PATH_SEPSTR +
	  CryAudio::Impl::Fmod::s_szImplFolderName +
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
	auto const type = static_cast<EImplItemType>(pImplItem->GetType());

	switch (type)
	{
	case EImplItemType::Folder:
		szIconPath = "icons:audio/fmod/folder_closed.ico";
		break;
	case EImplItemType::Event:
		szIconPath = "icons:audio/fmod/event.ico";
		break;
	case EImplItemType::Parameter:
		szIconPath = "icons:audio/fmod/tag.ico";
		break;
	case EImplItemType::Snapshot:
		szIconPath = "icons:audio/fmod/snapshot.ico";
		break;
	case EImplItemType::Bank:
		szIconPath = "icons:audio/fmod/bank.ico";
		break;
	case EImplItemType::Return:
		szIconPath = "icons:audio/fmod/return.ico";
		break;
	case EImplItemType::VCA:
		szIconPath = "icons:audio/fmod/vca.ico";
		break;
	case EImplItemType::MixerGroup:
		szIconPath = "icons:audio/fmod/group.ico";
		break;
	case EImplItemType::EditorFolder:
		szIconPath = "icons:General/Folder.ico";
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
	auto const implType = static_cast<EImplItemType>(pImplItem->GetType());

	switch (systemType)
	{
	case ESystemItemType::Trigger:
		isCompatible = (implType == EImplItemType::Event) || (implType == EImplItemType::Snapshot);
		break;
	case ESystemItemType::Parameter:
		isCompatible = (implType == EImplItemType::Parameter) || (implType == EImplItemType::VCA);
		break;
	case ESystemItemType::State:
		isCompatible = (implType == EImplItemType::Parameter) || (implType == EImplItemType::VCA);
		break;
	case ESystemItemType::Environment:
		isCompatible = (implType == EImplItemType::Return) || (implType == EImplItemType::Parameter);
		break;
	case ESystemItemType::Preload:
		isCompatible = (implType == EImplItemType::Bank);
		break;
	default:
		isCompatible = false;
		break;
	}

	return isCompatible;
}

//////////////////////////////////////////////////////////////////////////
ESystemItemType CEditorImpl::ImplTypeToSystemType(CImplItem const* const pImplItem) const
{
	ESystemItemType systemType = ESystemItemType::Invalid;
	auto const implType = static_cast<EImplItemType>(pImplItem->GetType());

	switch (implType)
	{
	case EImplItemType::Event:
		systemType = ESystemItemType::Trigger;
		break;
	case EImplItemType::Parameter:
	case EImplItemType::VCA:
		systemType = ESystemItemType::Parameter;
		break;
	case EImplItemType::Snapshot:
		systemType = ESystemItemType::Trigger;
		break;
	case EImplItemType::Bank:
		systemType = ESystemItemType::Preload;
		break;
	case EImplItemType::Return:
		systemType = ESystemItemType::Environment;
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
		auto const type = static_cast<EImplItemType>(pImplItem->GetType());

		if (type == EImplItemType::Event)
		{
			pConnection = std::make_shared<CEventConnection>(pImplItem->GetId());
		}
		else if (type == EImplItemType::Snapshot)
		{
			pConnection = std::make_shared<CSnapshotConnection>(pImplItem->GetId());
		}
		else if ((type == EImplItemType::Parameter) || (type == EImplItemType::VCA))
		{
			if ((controlType == ESystemItemType::Parameter) || (controlType == ESystemItemType::Environment))
			{
				pConnection = std::make_shared<CParamConnection>(pImplItem->GetId());
			}
			else if (controlType == ESystemItemType::State)
			{
				pConnection = std::make_shared<CParamToStateConnection>(pImplItem->GetId(), type);
			}
			else
			{
				pConnection = std::make_shared<CImplConnection>(pImplItem->GetId());
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
		auto const type = TagToType(pNode->getTag());

		if (type != EImplItemType::Invalid)
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

			CImplItem* pImplItem = nullptr;

			if (type != EImplItemType::Parameter)
			{
				pImplItem = GetItemFromPath(TypeToEditorFolderName(type) + name);
			}
			else
			{
				pImplItem = SearchForControl(&m_rootControl, name, static_cast<ItemType>(type));
			}

			if ((pImplItem == nullptr) || (type != static_cast<EImplItemType>(pImplItem->GetType())))
			{
				// If control not found, create a placeholder.
				// We want to keep that connection even if it's not in the middleware as the user could
				// be using the engine without the fmod project

				string path = "";
				int const pos = name.find_last_of("/");

				if (pos != string::npos)
				{
					path = name.substr(0, pos);
					name = name.substr(pos + 1, name.length() - pos);
				}

				pImplItem = CreateItem(name, type, CreatePlaceholderFolderPath(path));

				if (pImplItem != nullptr)
				{
					pImplItem->SetPlaceholder(true);
				}
			}

			switch (type)
			{
			case EImplItemType::Event:
				{
					string eventType = pNode->getAttr(CryAudio::s_szTypeAttribute);

#if defined (USE_BACKWARDS_COMPATIBILITY)
					if (eventType.IsEmpty() && pNode->haveAttr("fmod_event_type"))
					{
						eventType = pNode->getAttr("fmod_event_type");
					}
#endif        // USE_BACKWARDS_COMPATIBILITY
					auto const pConnection = std::make_shared<CEventConnection>(pImplItem->GetId());

					if (eventType == CryAudio::Impl::Fmod::s_szStopValue)
					{
						pConnection->SetType(EEventType::Stop);
					}
					else if (eventType == CryAudio::Impl::Fmod::s_szPauseValue)
					{
						pConnection->SetType(EEventType::Pause);
					}
					else if (eventType == CryAudio::Impl::Fmod::s_szResumeValue)
					{
						pConnection->SetType(EEventType::Resume);
					}
					else
					{
						pConnection->SetType(EEventType::Start);
					}

					pConnectionPtr = pConnection;
				}
				break;
			case EImplItemType::Snapshot:
				{
					string eventType = pNode->getAttr(CryAudio::s_szTypeAttribute);

#if defined (USE_BACKWARDS_COMPATIBILITY)
					if (eventType.IsEmpty() && pNode->haveAttr("fmod_event_type"))
					{
						eventType = pNode->getAttr("fmod_event_type");
					}
#endif        // USE_BACKWARDS_COMPATIBILITY
					auto const pConnection = std::make_shared<CSnapshotConnection>(pImplItem->GetId());
					pConnection->SetType((eventType == CryAudio::Impl::Fmod::s_szStopValue) ? ESnapshotType::Stop : ESnapshotType::Start);
					pConnectionPtr = pConnection;
				}
				break;
			case EImplItemType::Parameter:
			case EImplItemType::VCA:
				{
					if ((controlType == ESystemItemType::Parameter) || (controlType == ESystemItemType::Environment))
					{
						std::shared_ptr<CParamConnection> const pConnection = std::make_shared<CParamConnection>(pImplItem->GetId());
						float mult = 1.0f;
						float shift = 0.0f;

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
						pConnection->SetMultiplier(mult);
						pConnection->SetShift(shift);
						pConnectionPtr = pConnection;
					}
					else if (controlType == ESystemItemType::State)
					{
						std::shared_ptr<CParamToStateConnection> const pConnection = std::make_shared<CParamToStateConnection>(pImplItem->GetId(), type);
						string value = pNode->getAttr(CryAudio::Impl::Fmod::s_szValueAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
						if (value.IsEmpty() && pNode->haveAttr("fmod_value"))
						{
							value = pNode->getAttr("fmod_value");
						}
#endif
						pConnection->SetValue((float)std::atof(value.c_str()));
						pConnectionPtr = pConnection;
					}
				}
				break;
			case EImplItemType::Bank:
			case EImplItemType::Return:
				{
					pConnectionPtr = std::make_shared<CImplConnection>(pImplItem->GetId());
				}
				break;
			}
		}
	}

	return pConnectionPtr;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CEditorImpl::CreateXMLNodeFromConnection(ConnectionPtr const pConnection, ESystemItemType const controlType)
{
	XmlNodeRef pConnectionNode = nullptr;

	CImplItem* const pImplControl = GetControl(pConnection->GetID());

	if (pImplControl != nullptr)
	{
		auto const type = static_cast<EImplItemType>(pImplControl->GetType());
		pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(type));

		switch (type)
		{
		case EImplItemType::Event:
			{
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, Utils::GetPathName(pImplControl, m_rootControl));
				auto const pEventConnection = static_cast<const CEventConnection*>(pConnection.get());

				if (pEventConnection != nullptr)
				{
					if (pEventConnection->GetType() == EEventType::Stop)
					{
						pConnectionNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::Fmod::s_szStopValue);
					}
					else if (pEventConnection->GetType() == EEventType::Pause)
					{
						pConnectionNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::Fmod::s_szPauseValue);
					}
					else if (pEventConnection->GetType() == EEventType::Resume)
					{
						pConnectionNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::Fmod::s_szResumeValue);
					}
				}
			}
			break;
		case EImplItemType::Snapshot:
			{
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, Utils::GetPathName(pImplControl, m_rootControl));
				auto const pEventConnection = static_cast<const CSnapshotConnection*>(pConnection.get());

				if ((pEventConnection != nullptr) && (pEventConnection->GetType() == ESnapshotType::Stop))
				{
					pConnectionNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::Fmod::s_szStopValue);
				}
			}
			break;
		case EImplItemType::Return:
			{
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, Utils::GetPathName(pImplControl, m_rootControl));
			}
			break;
		case EImplItemType::Parameter:
		case EImplItemType::VCA:
			{
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pImplControl->GetName());

				if (controlType == ESystemItemType::State)
				{
					auto const pStateConnection = static_cast<const CParamToStateConnection*>(pConnection.get());

					if (pStateConnection != nullptr)
					{
						pConnectionNode->setAttr(CryAudio::Impl::Fmod::s_szValueAttribute, pStateConnection->GetValue());
					}
				}
				else if ((controlType == ESystemItemType::Parameter) || (controlType == ESystemItemType::Environment))
				{
					auto const pParamConnection = static_cast<const CParamConnection*>(pConnection.get());

					if (pParamConnection->GetMultiplier() != 1.0f)
					{
						pConnectionNode->setAttr(CryAudio::Impl::Fmod::s_szMutiplierAttribute, pParamConnection->GetMultiplier());
					}

					if (pParamConnection->GetShift() != 0.0f)
					{
						pConnectionNode->setAttr(CryAudio::Impl::Fmod::s_szShiftAttribute, pParamConnection->GetShift());
					}
				}
			}
			break;
		case EImplItemType::Bank:
			{
				pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pImplControl->GetName());

				if (pImplControl->IsLocalised())
				{
					pConnectionNode->setAttr(CryAudio::Impl::Fmod::s_szLocalizedAttribute, CryAudio::Impl::Fmod::s_szTrueValue);
				}
			}
			break;
		}
	}

	return pConnectionNode;
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
CImplItem* CEditorImpl::CreateItem(string const& name, EImplItemType const type, CImplItem* const pParent)
{
	CID const id = Utils::GetId(type, name, pParent, m_rootControl);

	CImplItem* pImplItem = GetControl(id);

	if (pImplItem != nullptr)
	{
		if (pImplItem->IsPlaceholder())
		{
			pImplItem->SetPlaceholder(false);
			CImplItem* pParentItem = pParent;

			while (pParentItem != nullptr)
			{
				pParentItem->SetPlaceholder(false);
				pParentItem = pParentItem->GetParent();
			}
		}
	}
	else
	{
		if (type == EImplItemType::Folder)
		{
			pImplItem = new CImplFolder(name, id);
		}
		else if (type == EImplItemType::MixerGroup)
		{
			pImplItem = new CImplMixerGroup(name, id);
		}
		else
		{
			pImplItem = new CImplItem(name, id, static_cast<ItemType>(type));

			if (type == EImplItemType::EditorFolder)
			{
				pImplItem->SetContainer(true);
			}
		}

		if (pParent != nullptr)
		{
			pParent->AddChild(pImplItem);
		}
		else
		{
			m_rootControl.AddChild(pImplItem);
		}

		m_controlsCache[id] = pImplItem;

	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CEditorImpl::GetItemFromPath(string const& fullpath)
{
	CImplItem* pItem = &m_rootControl;
	int start = 0;
	string token = fullpath.Tokenize("/", start);

	while (!token.empty())
	{
		token.Trim();
		CImplItem* pChild = nullptr;
		int const size = pItem->ChildCount();

		for (int i = 0; i < size; ++i)
		{
			CImplItem* const pNextChild = pItem->GetChildAt(i);

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
CImplItem* CEditorImpl::CreatePlaceholderFolderPath(string const& path)
{
	CImplItem* pImplItem = &m_rootControl;
	int start = 0;
	string token = path.Tokenize("/", start);

	while (!token.empty())
	{
		token.Trim();
		CImplItem* pFoundChild = nullptr;
		int const size = pImplItem->ChildCount();

		for (int i = 0; i < size; ++i)
		{
			CImplItem* const pChild = pImplItem->GetChildAt(i);

			if ((pChild != nullptr) && (pChild->GetName() == token))
			{
				pFoundChild = pChild;
				break;
			}
		}

		if (pFoundChild == nullptr)
		{
			pFoundChild = CreateItem(token, EImplItemType::Folder, pImplItem);
			pFoundChild->SetPlaceholder(true);
		}

		pImplItem = pFoundChild;
		token = path.Tokenize("/", start);
	}

	return pImplItem;
}
} // namespace Fmod
} // namespace ACE
