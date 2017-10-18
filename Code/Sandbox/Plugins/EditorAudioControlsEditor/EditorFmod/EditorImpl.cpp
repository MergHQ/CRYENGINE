// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EditorImpl.h"

#include "ProjectLoader.h"

#include <ImplConnection.h>

#include <CryAudio/IAudioSystem.h>
#include <CryAudio/IProfileData.h>
#include <CrySystem/File/CryFile.h>
#include <CrySystem/ISystem.h>
#include <CryString/CryPath.h>
#include <CryCore/StlUtils.h>
#include <CrySerialization/IArchiveHost.h>
#include <CrySerialization/Enum.h>

namespace ACE
{
namespace Fmod
{
const string g_userSettingsFile = "%USER%/audiocontrolseditor_fmod.user";

// Paths
string const g_foldersPath = "/metadata/eventfolder/";
string const g_groupsPath = "/metadata/group/";
string const g_eventsPath = "/metadata/event/";
string const g_snapshotsPath = "/metadata/snapshot/";
string const g_returnsPath = "/metadata/return/";

// XML tags
string const g_eventTag = "FmodEvent";
string const g_eventParameterTag = "FmodEventParameter";
string const g_snapshotTag = "FmodSnapshot";
string const g_snapshotParameterTag = "FmodSnapshotParameter";
string const g_bankTag = "FmodFile";
string const g_returnTag = "FmodBus";
string const g_nameAttribute = "fmod_name";
string const g_pathAttribute = "fmod_path";
string const g_valueAttribute = "fmod_value";
string const g_multAttribute = "fmod_value_multiplier";
string const g_shiftAttribute = "fmod_value_shift";
string const g_eventTypeAttribute = "fmod_event_type";

//////////////////////////////////////////////////////////////////////////
EImpltemType TagToType(string const& tag)
{
	if (tag == g_eventTag)
	{
		return EImpltemType::Event;
	}
	else if (tag == g_eventParameterTag)
	{
		return EImpltemType::EventParameter;
	}
	else if (tag == g_snapshotTag)
	{
		return EImpltemType::Snapshot;
	}
	else if (tag == g_snapshotParameterTag)
	{
		return EImpltemType::SnapshotParameter;
	}
	else if (tag == g_bankTag)
	{
		return EImpltemType::Bank;
	}
	else if (tag == g_returnTag)
	{
		return EImpltemType::Return;
	}

	return EImpltemType::Invalid;
}

//////////////////////////////////////////////////////////////////////////
string TypeToTag(EImpltemType const type)
{
	switch (type)
	{
	case EImpltemType::Event:
		return g_eventTag;
	case EImpltemType::EventParameter:
		return g_eventParameterTag;
	case EImpltemType::Snapshot:
		return g_snapshotTag;
	case EImpltemType::SnapshotParameter:
		return g_snapshotParameterTag;
	case EImpltemType::Bank:
		return g_bankTag;
	case EImpltemType::Return:
		return g_returnTag;
	}
	return "";
}

//////////////////////////////////////////////////////////////////////////
enum class EEventType
{
	Start = 0,
	Stop,
};

//////////////////////////////////////////////////////////////////////////
class CEventConnection : public CImplConnection
{
public:

	explicit CEventConnection(CID const nID)
		: CImplConnection(nID)
	{}

	virtual bool HasProperties() const override { return true; }

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		ar(type, "action", "Action");
		if (ar.isInput())
		{
			signalConnectionChanged();
		}
	}

	EEventType type = EEventType::Start;
};

//////////////////////////////////////////////////////////////////////////
class CParamConnection : public CImplConnection
{
public:

	explicit CParamConnection(CID const nID)
		: CImplConnection(nID)
	{}

	virtual bool HasProperties() const override { return true; }

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		ar(mult, "mult", "Multiply");
		ar(shift, "shift", "Shift");

		if (ar.isInput())
		{
			signalConnectionChanged();
		}
	}

	float mult = 1.0f;
	float shift = 0.0f;
};

//////////////////////////////////////////////////////////////////////////
class CParamToStateConnection : public CImplConnection
{
public:

	explicit CParamToStateConnection(CID const id)
		: CImplConnection(id)
	{}

	virtual bool HasProperties() const override { return true; }

	virtual void Serialize(Serialization::IArchive& ar) override
	{
		ar(m_value, "value", "Value");

		if (ar.isInput())
		{
			signalConnectionChanged();
		}
	}

	float m_value = 1.0f;
};

//////////////////////////////////////////////////////////////////////////
void CImplSettings::SetProjectPath(char const* szPath)
{
	m_projectPath = szPath;
	Serialization::SaveJsonFile(g_userSettingsFile.c_str(), *this);
}

//////////////////////////////////////////////////////////////////////////
CEditorImpl::CEditorImpl()
	: m_root("", ACE_INVALID_ID, AUDIO_SYSTEM_INVALID_TYPE)
{
	Serialization::LoadJsonFile(m_implSettings, g_userSettingsFile.c_str());
	Reload();
}

//////////////////////////////////////////////////////////////////////////
CEditorImpl::~CEditorImpl()
{
	m_containerIdMap.clear();
	std::for_each(m_items.begin(), m_items.end(), stl::container_object_deleter());
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::Reload(bool const preserveConnectionStatus)
{
	// set all the controls as placeholder as we don't know if
	// any of them have been removed but still have connections to them
	for (CImplItem* const pImplItem : m_items)
	{
		if (pImplItem != nullptr)
		{
			pImplItem->SetPlaceholder(true);
			pImplItem->SetConnected(false);
		}
	}

	// Load banks
	{
		_finddata_t fd;
		ICryPak* const pCryPak = gEnv->pCryPak;
		string const path = m_implSettings.GetSoundBanksPath();
		intptr_t const handle = pCryPak->FindFirst(path + "/*.bank", &fd);

		if (handle != -1)
		{
			// We have to exclude the Master Bank, for this we look
			// for the file that ends with "strings.bank" as it is guaranteed
			// to have the same name as the Master Bank and there should be unique
			std::vector<string> banks;
			string masterBankName = "";

			do
			{
				string const filename = fd.name;

				if ((filename != ".") && (filename != "..") && !filename.empty())
				{
					int const pos = filename.rfind(".strings.bank");

					if (pos != string::npos)
					{
						masterBankName = filename.substr(0, pos);
					}
					else
					{
						banks.push_back(filename);
					}
				}
			}
			while (pCryPak->FindNext(handle, &fd) >= 0);

			for (string const& filename : banks)
			{
				if (filename.compare(0, masterBankName.length(), masterBankName) != 0)
				{
					CreateItem(EImpltemType::Bank, nullptr, filename);
				}
			}

			pCryPak->FindClose(handle);
		}
	}

	ParseFolder(m_implSettings.GetProjectPath() + g_foldersPath);    // folders
	ParseFolder(m_implSettings.GetProjectPath() + g_groupsPath);     // groups
	ParseFolder(m_implSettings.GetProjectPath() + g_eventsPath);     // events
	ParseFolder(m_implSettings.GetProjectPath() + g_snapshotsPath);  // snapshots
	ParseFolder(m_implSettings.GetProjectPath() + g_returnsPath);    // returns

	if (preserveConnectionStatus)
	{
		for (auto const connection : m_connectionsByID)
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
	for (CImplItem* const pImplItem : m_items)
	{

		if (pImplItem->GetId() == id)
		{
			return pImplItem;
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
TImplControlTypeMask CEditorImpl::GetCompatibleTypes(ESystemItemType const systemType) const
{
	switch (systemType)
	{
	case ESystemItemType::Trigger:
		return static_cast<TImplControlTypeMask>(EImpltemType::Event) | static_cast<TImplControlTypeMask>(EImpltemType::Snapshot);
		break;
	case ESystemItemType::Parameter:
		return static_cast<TImplControlTypeMask>(EImpltemType::EventParameter) | static_cast<TImplControlTypeMask>(EImpltemType::SnapshotParameter);
		break;
	case ESystemItemType::Preload:
		return static_cast<TImplControlTypeMask>(EImpltemType::Bank);
		break;
	case ESystemItemType::State:
		return static_cast<TImplControlTypeMask>(EImpltemType::EventParameter) | static_cast<TImplControlTypeMask>(EImpltemType::SnapshotParameter);
		break;
	case ESystemItemType::Environment:
		return static_cast<int>(static_cast<TImplControlTypeMask>(EImpltemType::Return));
		break;
	}
	return static_cast<TImplControlTypeMask>(EImpltemType::Invalid);
}

//////////////////////////////////////////////////////////////////////////
char const* CEditorImpl::GetTypeIcon(CImplItem const* const pImplItem) const
{
	auto const type = static_cast<EImpltemType>(pImplItem->GetType());

	switch (type)
	{
	case EImpltemType::Folder:
		return ":Icons/fmod/folder_closed.ico";
	case EImpltemType::Event:
		return ":Icons/fmod/event.ico";
	case EImpltemType::EventParameter:
		return ":Icons/fmod/tag.ico";
	case EImpltemType::Snapshot:
		return ":Icons/fmod/snapshot.ico";
	case EImpltemType::SnapshotParameter:
		return ":Icons/fmod/tag.ico";
	case EImpltemType::Bank:
		return ":Icons/fmod/bank.ico";
	case EImpltemType::Return:
		return ":Icons/fmod/return.ico";
	case EImpltemType::Group:
		return ":Icons/fmod/group.ico";
	}
	return "icons:Dialogs/dialog-error.ico";
}

//////////////////////////////////////////////////////////////////////////
string CEditorImpl::GetName() const
{
	return gEnv->pAudioSystem->GetProfileData()->GetImplName();
}

//////////////////////////////////////////////////////////////////////////
ESystemItemType CEditorImpl::ImplTypeToSystemType(CImplItem const* const pImplItem) const
{
	auto const type = static_cast<EImpltemType>(pImplItem->GetType());

	switch (type)
	{
	case EImpltemType::Event:
		return ESystemItemType::Trigger;
	case EImpltemType::EventParameter:
		return ESystemItemType::Parameter;
	case EImpltemType::Snapshot:
		return ESystemItemType::Trigger;
	case EImpltemType::SnapshotParameter:
		return ESystemItemType::Parameter;
	case EImpltemType::Bank:
		return ESystemItemType::Preload;
	case EImpltemType::Return:
		return ESystemItemType::Environment;
	}
	return ESystemItemType::Invalid;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CEditorImpl::CreateConnectionToControl(ESystemItemType const controlType, CImplItem* const pImplItem)
{
	if (pImplItem != nullptr)
	{
		auto const type = static_cast<EImpltemType>(pImplItem->GetType());

		if ((type == EImpltemType::Event) || (type == EImpltemType::Snapshot))
		{
			return std::make_shared<CEventConnection>(pImplItem->GetId());
		}
		else if ((type == EImpltemType::EventParameter) || (type == EImpltemType::SnapshotParameter))
		{
			if (controlType == ESystemItemType::Parameter)
			{
				return std::make_shared<CParamConnection>(pImplItem->GetId());
			}
			else if (controlType == ESystemItemType::State)
			{
				return std::make_shared<CParamToStateConnection>(pImplItem->GetId());
			}
		}

		return std::make_shared<CImplConnection>(pImplItem->GetId());
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CEditorImpl::CreateConnectionFromXMLNode(XmlNodeRef pNode, ESystemItemType const controlType)
{
	if (pNode != nullptr)
	{
		auto const type = TagToType(pNode->getTag());

		if (type != EImpltemType::Invalid)
		{

			string name = pNode->getAttr(g_nameAttribute);
			string path = pNode->getAttr(g_pathAttribute);

			if (!path.empty())
			{
				name = path + "/" + name;
			}

			CImplItem* pImplItem = GetItemFromPath(name);

			if ((pImplItem == nullptr) || (type != static_cast<EImpltemType>(pImplItem->GetType())))
			{
				// If control not found, create a placeholder.
				// We want to keep that connection even if it's not in the middleware as the user could
				// be using the engine without the fmod project

				path = "";
				int const pos = name.find_last_of("/");

				if (pos != string::npos)
				{
					path = name.substr(0, pos);
					name = name.substr(pos + 1, name.length() - pos);
				}

				pImplItem = CreateItem(type, CreatePlaceholderFolderPath(path), name);

				if (pImplItem != nullptr)
				{
					pImplItem->SetPlaceholder(true);
				}
			}

			switch (type)
			{
			case EImpltemType::Event:
			case EImpltemType::Snapshot:
				{
					string const eventType = pNode->getAttr(g_eventTypeAttribute);
					auto const pConnection = std::make_shared<CEventConnection>(pImplItem->GetId());
					pConnection->type = (eventType == "stop") ? EEventType::Stop : EEventType::Start;
					return pConnection;
				}
				break;
			case EImpltemType::EventParameter:
			case EImpltemType::SnapshotParameter:
				{
					if (controlType == ESystemItemType::Parameter)
					{
						std::shared_ptr<CParamConnection> const pConnection = std::make_shared<CParamConnection>(pImplItem->GetId());
						float mult = 1.0f;
						float shift = 0.0f;

						if (pNode->haveAttr(g_multAttribute))
						{
							string const value = pNode->getAttr(g_multAttribute);
							mult = (float)std::atof(value.c_str());
						}

						if (pNode->haveAttr(g_shiftAttribute))
						{
							string const value = pNode->getAttr(g_shiftAttribute);
							shift = (float)std::atof(value.c_str());
						}

						pConnection->mult = mult;
						pConnection->shift = shift;
						return pConnection;
					}
					else if (controlType == ESystemItemType::State)
					{
						std::shared_ptr<CParamToStateConnection> const pConnection = std::make_shared<CParamToStateConnection>(pImplItem->GetId());
						string const value = pNode->getAttr(g_valueAttribute);
						pConnection->m_value = (float)std::atof(value.c_str());
						return pConnection;
					}
				}
				break;
			case EImpltemType::Bank:
			case EImpltemType::Return:
				{
					return std::make_shared<CImplConnection>(pImplItem->GetId());
				}
				break;
			}
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CEditorImpl::CreateXMLNodeFromConnection(ConnectionPtr const pConnection, ESystemItemType const controlType)
{
	CImplItem* const pImplControl = GetControl(pConnection->GetID());

	if (pImplControl != nullptr)
	{
		auto const type = static_cast<EImpltemType>(pImplControl->GetType());
		XmlNodeRef const pNode = GetISystem()->CreateXmlNode(TypeToTag(type));

		switch (type)
		{
		case EImpltemType::Event:
		case EImpltemType::Snapshot:
			{
				pNode->setAttr(g_nameAttribute, GetFullPathName(pImplControl));
				auto const pEventConnection = static_cast<const CEventConnection*>(pConnection.get());

				if ((pEventConnection != nullptr) && (pEventConnection->type == EEventType::Stop))
				{
					pNode->setAttr(g_eventTypeAttribute, "stop");
				}
			}
			break;
		case EImpltemType::Return:
			{
				pNode->setAttr(g_nameAttribute, GetFullPathName(pImplControl));
			}
			break;
		case EImpltemType::EventParameter:
		case EImpltemType::SnapshotParameter:
			{
				pNode->setAttr(g_nameAttribute, pImplControl->GetName());
				pNode->setAttr(g_pathAttribute, GetFullPathName(pImplControl->GetParent()));

				if (controlType == ESystemItemType::State)
				{
					auto const pStateConnection = static_cast<const CParamToStateConnection*>(pConnection.get());

					if (pStateConnection != nullptr)
					{
						pNode->setAttr(g_valueAttribute, pStateConnection->m_value);
					}
				}
				else if (controlType == ESystemItemType::Parameter)
				{
					auto const pParamConnection = static_cast<const CParamConnection*>(pConnection.get());

					if (pParamConnection->mult != 1.0f)
					{
						pNode->setAttr(g_multAttribute, pParamConnection->mult);
					}

					if (pParamConnection->shift != 0.0f)
					{
						pNode->setAttr(g_shiftAttribute, pParamConnection->shift);
					}
				}
			}
			break;
		case EImpltemType::Bank:
			{
				pNode->setAttr(g_nameAttribute, GetFullPathName(pImplControl));
			}
			break;
		}

		return pNode;
	}

	return nullptr;
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
CImplItem* CEditorImpl::CreateItem(EImpltemType const type, CImplItem* const pParent, const string& name)
{
	CID const id = GetId(type, name, pParent);

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
		if (type == EImpltemType::Folder)
		{
			pImplItem = new CImplFolder(name, id);
		}
		else if (type == EImpltemType::Group)
		{
			pImplItem = new CImplGroup(name, id);
		}
		else
		{
			pImplItem = new CImplItem(name, id, static_cast<ItemType>(type));
		}
		if (pParent != nullptr)
		{
			pParent->AddChild(pImplItem);
			pImplItem->SetParent(pParent);
		}
		else
		{
			m_root.AddChild(pImplItem);
			pImplItem->SetParent(&m_root);
		}
		m_items.push_back(pImplItem);

	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
CID CEditorImpl::GetId(EImpltemType const type, string const& name, CImplItem* const pParent) const
{
	string const fullname = GetTypeName(type) + GetFullPathName(pParent) + CRY_NATIVE_PATH_SEPSTR + name;
	return CCrc32::Compute(fullname);
}

//////////////////////////////////////////////////////////////////////////
string CEditorImpl::GetFullPathName(CImplItem const* const pImplItem) const
{
	if (pImplItem != nullptr)
	{
		string fullname = pImplItem->GetName();
		CImplItem const* pParent = pImplItem->GetParent();
		while ((pParent != nullptr) && (pParent != &m_root))
		{
			// The id needs to represent the full path, as we can have items with the same name in different folders
			fullname = pParent->GetName() + "/" + fullname;
			pParent = pParent->GetParent();
		}

		return fullname;
	}

	return "";
}

//////////////////////////////////////////////////////////////////////////
string CEditorImpl::GetTypeName(EImpltemType const type) const
{
	switch (type)
	{
	case EImpltemType::Folder:
		return "folder:";
	case EImpltemType::Event:
		return "event:";
	case EImpltemType::EventParameter:
		return "eventparameter:";
	case EImpltemType::Snapshot:
		return "snapshot:";
	case EImpltemType::SnapshotParameter:
		return "snapshotparameter:";
	case EImpltemType::Bank:
		return "bank:";
	case EImpltemType::Return:
		return "return:";
	case EImpltemType::Group:
		return "group:";
	}
	return "";
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::ParseFolder(string const& folderPath)
{
	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(folderPath + "*.xml", &fd);

	if (handle != -1)
	{
		do
		{
			string const filename = fd.name;

			if ((filename != ".") && (filename != "..") && !filename.empty())
			{
				ParseFile(folderPath + filename);
			}
		} while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
void CEditorImpl::ParseFile(string const& filepath)
{
	if (GetISystem()->GetIPak()->IsFileExist(filepath))
	{
		XmlNodeRef const pRoot = GetISystem()->LoadXmlFromFile(filepath);
		if (pRoot != nullptr)
		{
			CImplItem* pImplItem = nullptr;
			int const size = pRoot->getChildCount();

			for (int i = 0; i < size; ++i)
			{
				XmlNodeRef const pChild = pRoot->getChild(i);

				if (pChild != nullptr)
				{
					string const className = pChild->getAttr("class");

					if (className == "EventFolder")
					{
						pImplItem = LoadFolder(pChild);
					}
					else if (className == "Event")
					{
						pImplItem = LoadEvent(pChild);
					}
					else if (className == "Snapshot")
					{
						pImplItem = LoadSnapshot(pChild);
					}
					else if (className == "GameParameter")
					{
						if (pImplItem != nullptr)
						{
							if (static_cast<EImpltemType>(pImplItem->GetType()) == EImpltemType::Event)
							{
								LoadParameter(pChild, EImpltemType::EventParameter, *pImplItem);
							}
							else
							{
								LoadParameter(pChild, EImpltemType::SnapshotParameter, *pImplItem);
							}
						}
						else
						{
							CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "[Audio Controls Editor] [Fmod] Found GameParameter tag before Event in file %s", filepath.c_str());
						}
					}
					else if (className == "MixerReturn")
					{
						LoadReturn(pChild);
					}
					else if (className == "MixerGroup")
					{
						LoadGroup(pChild);
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CEditorImpl::GetContainer(string const& id, EImpltemType const type)
{
	auto folder = m_containerIdMap.find(id);

	if (folder != m_containerIdMap.end())
	{
		return (*folder).second;
	}

	// If folder not found parse the file corresponding to it and try looking for it again
	if (type == EImpltemType::Folder)
	{
		ParseFile(m_implSettings.GetProjectPath() + g_foldersPath + id + ".xml");
	}
	else if (type == EImpltemType::Group)
	{
		ParseFile(m_implSettings.GetProjectPath() + g_groupsPath + id + ".xml");
	}

	folder = m_containerIdMap.find(id);

	if (folder != m_containerIdMap.end())
	{
		return (*folder).second;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CEditorImpl::LoadContainer(XmlNodeRef const pNode, EImpltemType const type, string const& relationshipParamName)
{
	if (pNode != nullptr)
	{
		string containerName = "";
		CImplItem* pParent = nullptr;

		int const size = pNode->getChildCount();

		for (int i = 0; i < size; ++i)
		{
			XmlNodeRef const pChild = pNode->getChild(i);
			string const name = pChild->getAttr("name");

			if (name == "name")
			{
				// Get the container name
				XmlNodeRef const pContainerNameNode = pChild->getChild(0);

				if (pContainerNameNode != nullptr)
				{
					containerName = pContainerNameNode->getContent();
				}
			}
			else if (name == relationshipParamName)
			{
				// Get the container parent
				XmlNodeRef const pParentContainerNode = pChild->getChild(0);
				if (pParentContainerNode != nullptr)
				{
					string const parentContainerId = pParentContainerNode->getContent();
					pParent = GetContainer(parentContainerId, type);
				}
			}
		}

		CImplItem* const pContainer = CreateItem(type, pParent, containerName);
		m_containerIdMap[pNode->getAttr("id")] = pContainer;
		return pContainer;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CEditorImpl::LoadFolder(XmlNodeRef const pNode)
{
	return LoadContainer(pNode, EImpltemType::Folder, "folder");
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CEditorImpl::LoadGroup(XmlNodeRef const pNode)
{
	return LoadContainer(pNode, EImpltemType::Group, "output");
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CEditorImpl::LoadItem(XmlNodeRef const pNode, EImpltemType const type)
{
	if (pNode != nullptr)
	{
		string name = "";
		CImplItem* pParent = nullptr;
		int const size = pNode->getChildCount();

		for (int i = 0; i < size; ++i)
		{
			XmlNodeRef const pChild = pNode->getChild(i);

			if (pChild != nullptr)
			{
				string const tag = pChild->getTag();

				if (tag == "property")
				{
					string const paramName = pChild->getAttr("name");

					if (paramName == "name")
					{
						XmlNodeRef const pValue = pChild->getChild(0);

						if (pValue != nullptr)
						{
							name = pValue->getContent();
						}
					}
				}
				else if (tag == "relationship")
				{
					string const name = pChild->getAttr("name");

					if (name == "folder" || name == "output")
					{
						XmlNodeRef const pValue = pChild->getChild(0);

						if (pValue != nullptr)
						{
							string const parentContainerId = pValue->getContent();
							auto const folder = m_containerIdMap.find(parentContainerId);

							if (folder != m_containerIdMap.end())
							{
								pParent = (*folder).second;
							}
						}
					}
				}
			}
		}

		return CreateItem(type, pParent, name);
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CEditorImpl::LoadEvent(XmlNodeRef const pNode)
{
	return LoadItem(pNode, EImpltemType::Event);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CEditorImpl::LoadSnapshot(XmlNodeRef const pNode)
{
	return LoadItem(pNode, EImpltemType::Snapshot);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CEditorImpl::LoadReturn(XmlNodeRef const pNode)
{
	return LoadItem(pNode, EImpltemType::Return);
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CEditorImpl::LoadParameter(XmlNodeRef const pNode, EImpltemType const type, CImplItem& parentEvent)
{
	if (pNode != nullptr)
	{
		int const size = pNode->getChildCount();

		for (int i = 0; i < size; ++i)
		{
			XmlNodeRef const pChild = pNode->getChild(i);

			if (pChild != nullptr)
			{
				string const name = pChild->getAttr("name");

				if (name == "name")
				{
					XmlNodeRef const pValue = pChild->getChild(0);

					if (pValue != nullptr)
					{
						string const value = pValue->getContent();
						CImplItem* const pImplItem = CreateItem(type, &parentEvent, value);
						return pImplItem;
					}
				}
			}
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CEditorImpl::GetItemFromPath(string const& fullpath)
{
	CImplItem* pImplItem = &m_root;
	int start = 0;
	string token = fullpath.Tokenize("/", start);

	while (!token.empty())
	{
		token.Trim();
		CImplItem* pChild = nullptr;
		int const size = pImplItem->ChildCount();

		for (int i = 0; i < size; ++i)
		{
			CImplItem* const pNextChild = pImplItem->GetChildAt(i);

			if ((pNextChild != nullptr) && (pNextChild->GetName() == token))
			{
				pChild = pNextChild;
				break;
			}
		}

		if (pChild == nullptr)
		{
			return nullptr;
		}

		pImplItem = pChild;
		token = fullpath.Tokenize("/", start);
	}

	return pImplItem;
}

//////////////////////////////////////////////////////////////////////////
CImplItem* CEditorImpl::CreatePlaceholderFolderPath(string const& path)
{
	CImplItem* pImplItem = &m_root;
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
			pFoundChild = CreateItem(EImpltemType::Folder, pImplItem, token);
			pFoundChild->SetPlaceholder(true);
		}
		pImplItem = pFoundChild;
		token = path.Tokenize("/", start);
	}

	return pImplItem;
}

SERIALIZATION_ENUM_BEGIN(EEventType, "Event Type")
SERIALIZATION_ENUM(EEventType::Start, "start", "Start")
SERIALIZATION_ENUM(EEventType::Stop, "stop", "Stop")
SERIALIZATION_ENUM_END()
} // namespace Fmod
} // namespace ACE
