// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Impl.h"

#include "Common.h"
#include "EventConnection.h"
#include "ProjectLoader.h"
#include "DataPanel.h"
#include "Utils.h"

#include <CryAudioImplPortAudio/GlobalData.h>
#include <CrySystem/ISystem.h>
#include <CryCore/StlUtils.h>
#include <CrySystem/XML/IXml.h>

namespace ACE
{
namespace Impl
{
namespace PortAudio
{
constexpr uint32 g_itemPoolSize = 2048;
constexpr uint32 g_eventConnectionPoolSize = 2048;

//////////////////////////////////////////////////////////////////////////
CImpl::CImpl()
	: m_pDataPanel(nullptr)
	, m_assetAndProjectPath(AUDIO_SYSTEM_DATA_ROOT "/" +
	                        string(CryAudio::Impl::PortAudio::s_szImplFolderName) +
	                        "/" +
	                        string(CryAudio::s_szAssetsFolderName))
	, m_localizedAssetsPath(m_assetAndProjectPath)
{
}

//////////////////////////////////////////////////////////////////////////
CImpl::~CImpl()
{
	Clear();
	DestroyDataPanel();

	CItem::FreeMemoryPool();
	CEventConnection::FreeMemoryPool();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::Initialize(SImplInfo& implInfo, Platforms const& platforms)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Port Audio ACE Item Pool");
	CItem::CreateAllocator(g_itemPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Port Audio ACE Event Connection Pool");
	CEventConnection::CreateAllocator(g_eventConnectionPoolSize);

	CryAudio::SImplInfo systemImplInfo;
	gEnv->pAudioSystem->GetImplInfo(systemImplInfo);
	m_implName = systemImplInfo.name.c_str();

	SetImplInfo(implInfo);
}

//////////////////////////////////////////////////////////////////////////
QWidget* CImpl::CreateDataPanel()
{
	m_pDataPanel = new CDataPanel(*this);
	return m_pDataPanel;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestroyDataPanel()
{
	if (m_pDataPanel != nullptr)
	{
		delete m_pDataPanel;
		m_pDataPanel = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::Reload(SImplInfo& implInfo)
{
	Clear();
	SetImplInfo(implInfo);

	CProjectLoader(m_assetAndProjectPath, m_localizedAssetsPath, m_rootItem, m_itemCache, *this);

	for (auto const& connection : m_connectionsByID)
	{
		if (connection.second > 0)
		{
			auto const pItem = static_cast<CItem* const>(GetItem(connection.first));

			if (pItem != nullptr)
			{
				pItem->SetFlags(pItem->GetFlags() | EItemFlags::IsConnected);
			}
		}
	}

	if (m_pDataPanel != nullptr)
	{
		m_pDataPanel->Reset();
	}
}

//////////////////////////////////////////////////////////////////////////
IItem* CImpl::GetItem(ControlId const id) const
{
	IItem* pIItem = nullptr;

	if (id >= 0)
	{
		pIItem = stl::find_in_map(m_itemCache, id, nullptr);
	}

	return pIItem;
}

//////////////////////////////////////////////////////////////////////////
CryIcon const& CImpl::GetItemIcon(IItem const* const pIItem) const
{
	auto const pItem = static_cast<CItem const* const>(pIItem);
	CRY_ASSERT_MESSAGE(pItem != nullptr, "Impl item is null pointer during %s", __FUNCTION__);
	return GetTypeIcon(pItem->GetType());
}

//////////////////////////////////////////////////////////////////////////
QString const& CImpl::GetItemTypeName(IItem const* const pIItem) const
{
	auto const pItem = static_cast<CItem const* const>(pIItem);
	CRY_ASSERT_MESSAGE(pItem != nullptr, "Impl item is null pointer during %s", __FUNCTION__);
	return TypeToString(pItem->GetType());
}

//////////////////////////////////////////////////////////////////////////
bool CImpl::IsTypeCompatible(EAssetType const assetType, IItem const* const pIItem) const
{
	bool isCompatible = false;
	auto const pItem = static_cast<CItem const* const>(pIItem);

	if (pItem != nullptr)
	{
		if (assetType == EAssetType::Trigger)
		{
			isCompatible = (pItem->GetType() == EItemType::Event);
		}
	}

	return isCompatible;
}

//////////////////////////////////////////////////////////////////////////
EAssetType CImpl::ImplTypeToAssetType(IItem const* const pIItem) const
{
	EAssetType assetType = EAssetType::None;
	auto const pItem = static_cast<CItem const* const>(pIItem);

	if (pItem != nullptr)
	{
		EItemType const implType = pItem->GetType();

		switch (implType)
		{
		case EItemType::Event:
			assetType = EAssetType::Trigger;
			break;
		default:
			assetType = EAssetType::None;
			break;
		}
	}

	return assetType;
}

//////////////////////////////////////////////////////////////////////////
IConnection* CImpl::CreateConnectionToControl(EAssetType const assetType, IItem const* const pIItem)
{
	IConnection* pIConnection = nullptr;

	if (pIItem != nullptr)
	{
		pIConnection = static_cast<IConnection*>(new CEventConnection(pIItem->GetId()));
	}

	return pIConnection;
}

//////////////////////////////////////////////////////////////////////////
IConnection* CImpl::CreateConnectionFromXMLNode(XmlNodeRef pNode, EAssetType const assetType)
{
	IConnection* pIConnection = nullptr;

	if (pNode != nullptr)
	{
		char const* const szTag = pNode->getTag();

		if ((_stricmp(szTag, CryAudio::Impl::PortAudio::s_szEventTag) == 0) ||
		    (_stricmp(szTag, CryAudio::Impl::PortAudio::s_szFileTag) == 0) ||
		    (_stricmp(szTag, "PortAudioEvent") == 0) || // Backwards compatibility.
		    (_stricmp(szTag, "PortAudioSample") == 0))  // Backwards compatibility.
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
#endif      // USE_BACKWARDS_COMPATIBILITY
			string const localizedAttribute = pNode->getAttr(CryAudio::Impl::PortAudio::s_szLocalizedAttribute);
			bool const isLocalized = (localizedAttribute.compareNoCase(CryAudio::Impl::PortAudio::s_szTrueValue) == 0);
			ControlId const id = Utils::GetId(EItemType::Event, name, path, isLocalized);

			auto pItem = static_cast<CItem*>(GetItem(id));

			if (pItem == nullptr)
			{
				EItemFlags const flags = (isLocalized ? (EItemFlags::IsPlaceHolder | EItemFlags::IsLocalized) : EItemFlags::IsPlaceHolder);
				pItem = new CItem(name, id, EItemType::Event, path, flags);
				m_itemCache[id] = pItem;
			}

			if (pItem != nullptr)
			{
				auto const pEventConnection = new CEventConnection(pItem->GetId());
				string actionType = pNode->getAttr(CryAudio::s_szTypeAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
				if (actionType.IsEmpty() && pNode->haveAttr("event_type"))
				{
					actionType = pNode->getAttr("event_type");
				}
#endif        // USE_BACKWARDS_COMPATIBILITY
				pEventConnection->SetActionType(actionType.compareNoCase(CryAudio::Impl::PortAudio::s_szStopValue) == 0 ? CEventConnection::EActionType::Stop : CEventConnection::EActionType::Start);

				int loopCount = 0;
				pNode->getAttr(CryAudio::Impl::PortAudio::s_szLoopCountAttribute, loopCount);
				loopCount = std::max(0, loopCount);
				pEventConnection->SetLoopCount(static_cast<uint32>(loopCount));

				if (pEventConnection->GetLoopCount() == 0)
				{
					pEventConnection->SetInfiniteLoop(true);
				}

				pIConnection = static_cast<IConnection*>(pEventConnection);
			}
		}
	}

	return pIConnection;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CImpl::CreateXMLNodeFromConnection(IConnection const* const pIConnection, EAssetType const assetType)
{
	XmlNodeRef pNode = nullptr;

	auto const pEventConnection = static_cast<CEventConnection const*>(pIConnection);
	auto const pItem = static_cast<CItem const*>(GetItem(pIConnection->GetID()));

	if ((pItem != nullptr) && (pEventConnection != nullptr) && (assetType == EAssetType::Trigger))
	{
		pNode = GetISystem()->CreateXmlNode(CryAudio::Impl::PortAudio::s_szEventTag);
		pNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

		string const& path = pItem->GetPath();

		if (!path.IsEmpty())
		{
			pNode->setAttr(CryAudio::Impl::PortAudio::s_szPathAttribute, path.c_str());
		}

		if (pEventConnection->GetActionType() == CEventConnection::EActionType::Start)
		{
			pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::PortAudio::s_szStartValue);

			if (pEventConnection->IsInfiniteLoop())
			{
				pNode->setAttr(CryAudio::Impl::PortAudio::s_szLoopCountAttribute, 0);
			}
			else
			{
				pNode->setAttr(CryAudio::Impl::PortAudio::s_szLoopCountAttribute, pEventConnection->GetLoopCount());
			}
		}
		else
		{
			pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::PortAudio::s_szStopValue);
		}

		if ((pItem->GetFlags() & EItemFlags::IsLocalized) != 0)
		{
			pNode->setAttr(CryAudio::Impl::PortAudio::s_szLocalizedAttribute, CryAudio::Impl::PortAudio::s_szTrueValue);
		}

		++g_triggerConnections;
	}

	return pNode;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CImpl::SetDataNode(char const* const szTag)
{
	XmlNodeRef pNode = nullptr;

	if (g_triggerConnections > 0)
	{
		pNode = GetISystem()->CreateXmlNode(szTag);
		pNode->setAttr(CryAudio::Impl::PortAudio::s_szTriggersAttribute, g_triggerConnections);

		// Reset connection count for next library.
		g_triggerConnections = 0;
	}

	return pNode;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::EnableConnection(IConnection const* const pIConnection, bool const isLoading)
{
	auto const pItem = static_cast<CItem*>(GetItem(pIConnection->GetID()));

	if (pItem != nullptr)
	{
		++m_connectionsByID[pItem->GetId()];
		pItem->SetFlags(pItem->GetFlags() | EItemFlags::IsConnected);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DisableConnection(IConnection const* const pIConnection, bool const isLoading)
{
	auto const pItem = static_cast<CItem*>(GetItem(pIConnection->GetID()));

	if (pItem != nullptr)
	{
		int connectionCount = m_connectionsByID[pItem->GetId()] - 1;

		if (connectionCount < 1)
		{
			CRY_ASSERT_MESSAGE(connectionCount >= 0, "Connection count is < 0 during %s", __FUNCTION__);
			connectionCount = 0;
			pItem->SetFlags(pItem->GetFlags() & ~EItemFlags::IsConnected);
		}

		m_connectionsByID[pItem->GetId()] = connectionCount;
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructConnection(IConnection const* const pIConnection)
{
	delete pIConnection;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnBeforeReload()
{
	if (m_pDataPanel != nullptr)
	{
		m_pDataPanel->OnBeforeReload();
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnAfterReload()
{
	if (m_pDataPanel != nullptr)
	{
		m_pDataPanel->OnAfterReload();
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnSelectConnectedItem(ControlId const id) const
{
	if (m_pDataPanel != nullptr)
	{
		m_pDataPanel->OnSelectConnectedItem(id);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnFileImporterOpened()
{
	if (m_pDataPanel != nullptr)
	{
		m_pDataPanel->OnFileImporterOpened();
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnFileImporterClosed()
{
	if (m_pDataPanel != nullptr)
	{
		m_pDataPanel->OnFileImporterClosed();
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::Clear()
{
	for (auto const& itemPair : m_itemCache)
	{
		delete itemPair.second;
	}

	m_itemCache.clear();
	m_rootItem.Clear();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetImplInfo(SImplInfo& implInfo)
{
	SetLocalizedAssetsPath();

	implInfo.name = m_implName.c_str();
	implInfo.folderName = CryAudio::Impl::PortAudio::s_szImplFolderName;
	implInfo.projectPath = m_assetAndProjectPath.c_str();
	implInfo.assetsPath = m_assetAndProjectPath.c_str();
	implInfo.localizedAssetsPath = m_localizedAssetsPath.c_str();
	implInfo.flags = (
		EImplInfoFlags::SupportsTriggers);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetLocalizedAssetsPath()
{
	if (ICVar const* const pCVar = gEnv->pConsole->GetCVar("g_languageAudio"))
	{
		char const* const szLanguage = pCVar->GetString();

		if (szLanguage != nullptr)
		{
			m_localizedAssetsPath = PathUtil::GetLocalizationFolder().c_str();
			m_localizedAssetsPath += "/";
			m_localizedAssetsPath += szLanguage;
			m_localizedAssetsPath += "/";
			m_localizedAssetsPath += AUDIO_SYSTEM_DATA_ROOT;
			m_localizedAssetsPath += "/";
			m_localizedAssetsPath += CryAudio::Impl::PortAudio::s_szImplFolderName;
			m_localizedAssetsPath += "/";
			m_localizedAssetsPath += CryAudio::s_szAssetsFolderName;
		}
	}
}
} // namespace PortAudio
} // namespace Impl
} // namespace ACE
