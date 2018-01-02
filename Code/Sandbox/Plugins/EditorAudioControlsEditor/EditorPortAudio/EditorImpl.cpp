// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EditorImpl.h"

#include "ImplConnections.h"
#include "ProjectLoader.h"

#include <CryAudioImplPortAudio/GlobalData.h>
#include <CrySystem/ISystem.h>
#include <CryCore/CryCrc32.h>
#include <CryCore/StlUtils.h>
#include <CrySerialization/IArchiveHost.h>
#include <CrySerialization/ClassFactory.h>

namespace ACE
{
namespace PortAudio
{
//////////////////////////////////////////////////////////////////////////
CImplSettings::CImplSettings()
	: m_assetAndProjectPath(
		PathUtil::GetGameFolder() +
		CRY_NATIVE_PATH_SEPSTR
		AUDIO_SYSTEM_DATA_ROOT
		CRY_NATIVE_PATH_SEPSTR +
		CryAudio::Impl::PortAudio::s_szImplFolderName +
		CRY_NATIVE_PATH_SEPSTR +
		CryAudio::s_szAssetsFolderName)
{
}

//////////////////////////////////////////////////////////////////////////
CEditorImpl::CEditorImpl()
{
	gEnv->pAudioSystem->GetImplInfo(m_implInfo);
	m_implName = m_implInfo.name.c_str();
	m_implFolderName = CryAudio::Impl::PortAudio::s_szImplFolderName;
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

	CProjectLoader(GetSettings()->GetProjectPath(), m_rootControl);

	CreateControlCache(&m_rootControl);

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
		szIconPath = "icons:audio/portaudio/event.ico";
		break;
	case EImpltemType::Folder:
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
	auto const implType = static_cast<EImpltemType>(pImplItem->GetType());

	if (systemType == ESystemItemType::Trigger)
	{
		isCompatible = (implType == EImpltemType::Event);
	}

	return isCompatible;
}

//////////////////////////////////////////////////////////////////////////
ESystemItemType CEditorImpl::ImplTypeToSystemType(CImplItem const* const pImplItem) const
{
	ESystemItemType systemType = ESystemItemType::Invalid;
	auto const implType = static_cast<EImpltemType>(pImplItem->GetType());

	switch (implType)
	{
	case EImpltemType::Event:
		systemType = ESystemItemType::Trigger;
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
		pConnection = std::make_shared<CConnection>(pImplItem->GetId());
	}

	return pConnection;
}

//////////////////////////////////////////////////////////////////////////
ConnectionPtr CEditorImpl::CreateConnectionFromXMLNode(XmlNodeRef pNode, ESystemItemType const controlType)
{
	ConnectionPtr pConnectionPtr = nullptr;

	if (pNode != nullptr)
	{
		string const nodeTag = pNode->getTag();

		if ((nodeTag == CryAudio::s_szEventTag) || (nodeTag == CryAudio::Impl::PortAudio::s_szFileTag) || (nodeTag == "PortAudioEvent") || (nodeTag == "PortAudioSample"))
		{
			string name = pNode->getAttr(CryAudio::s_szNameAttribute);
			string path = pNode->getAttr(CryAudio::Impl::PortAudio::s_szPathAttribute);
// Backwards compatibility will be removed before March 2019.
#if defined (USE_BACKWARDS_COMPATIBILITY)
			if (name.IsEmpty() && pNode->haveAttr("portaudio_name"))
			{
				name = pNode->getAttr("portaudio_name");
			}

			if (path.IsEmpty() && pNode->haveAttr("portaudio_path"))
			{
				path = pNode->getAttr("portaudio_path");
			}
#endif // USE_BACKWARDS_COMPATIBILITY
			CID id;

			if (path.empty())
			{
				id = GetId(name);
			}
			else
			{
				id = GetId(path + CRY_NATIVE_PATH_SEPSTR + name);
			}

			CImplItem* pImplControl = GetControl(id);

			if (pImplControl == nullptr)
			{
				pImplControl = new CImplControl(name, id, static_cast<ItemType>(EImpltemType::Event));
				pImplControl->SetPlaceholder(true);
				m_controlsCache[id] = pImplControl;
			}

			if (pImplControl != nullptr)
			{
				auto const pConnection = std::make_shared<CConnection>(pImplControl->GetId());
				string connectionType = pNode->getAttr(CryAudio::s_szTypeAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
				if (connectionType.IsEmpty() && pNode->haveAttr("event_type"))
				{
					connectionType = pNode->getAttr("event_type");
				}
#endif // USE_BACKWARDS_COMPATIBILITY
				pConnection->m_type = connectionType == CryAudio::Impl::PortAudio::s_szStopValue ? EConnectionType::Stop : EConnectionType::Start;

				pNode->getAttr(CryAudio::Impl::PortAudio::s_szLoopCountAttribute, pConnection->m_loopCount);
#if defined (USE_BACKWARDS_COMPATIBILITY)
				if (pNode->haveAttr("loop_count"))
				{
					pNode->getAttr("loop_count", pConnection->m_loopCount);
				}
#endif // USE_BACKWARDS_COMPATIBILITY

				if (pConnection->m_loopCount == -1)
				{
					pConnection->m_isInfiniteLoop = true;
				}

				pConnectionPtr = pConnection;
			}
		}
	}

	return pConnectionPtr;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CEditorImpl::CreateXMLNodeFromConnection(ConnectionPtr const pConnection, ESystemItemType const controlType)
{
	XmlNodeRef pConnectionNode = nullptr;

	std::shared_ptr<CConnection const> const pImplConnection = std::static_pointer_cast<CConnection const>(pConnection);
	CImplItem const* const pImplControl = GetControl(pConnection->GetID());

	if ((pImplControl != nullptr) && (pImplConnection != nullptr) && (controlType == ESystemItemType::Trigger))
	{
		pConnectionNode = GetISystem()->CreateXmlNode(CryAudio::s_szEventTag);
		pConnectionNode->setAttr(CryAudio::s_szNameAttribute, pImplControl->GetName());

		string path;
		CImplItem const* pParent = pImplControl->GetParent();

		while (pParent != nullptr)
		{
			string const parentName = pParent->GetName();

			if (!parentName.empty())
			{
				if (path.empty())
				{
					path = parentName;
				}
				else
				{
					path = parentName + CRY_NATIVE_PATH_SEPSTR + path;
				}
			}

			pParent = pParent->GetParent();
		}

		pConnectionNode->setAttr(CryAudio::Impl::PortAudio::s_szPathAttribute, path);

		if (pImplConnection->m_type == EConnectionType::Start)
		{
			pConnectionNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::PortAudio::s_szStartValue);

			if (pImplConnection->m_isInfiniteLoop)
			{
				pConnectionNode->setAttr(CryAudio::Impl::PortAudio::s_szLoopCountAttribute, -1);
			}
			else
			{
				pConnectionNode->setAttr(CryAudio::Impl::PortAudio::s_szLoopCountAttribute, pImplConnection->m_loopCount);
			}
		}
		else
		{
			pConnectionNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::PortAudio::s_szStopValue);
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
void CEditorImpl::CreateControlCache(CImplItem const* const pParent)
{
	if (pParent != nullptr)
	{
		size_t const count = pParent->ChildCount();

		for (size_t i = 0; i < count; ++i)
		{
			CImplItem* const pChild = pParent->GetChildAt(i);

			if (pChild != nullptr)
			{
				m_controlsCache[pChild->GetId()] = pChild;
				CreateControlCache(pChild);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CID CEditorImpl::GetId(string const& name) const
{
	return CryAudio::StringToId(name);
}
} // namespace PortAudio
} // namespace ACE
