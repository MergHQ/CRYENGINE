// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Impl.h"

#include "Common.h"
#include "BankConnection.h"
#include "EventConnection.h"
#include "GenericConnection.h"
#include "KeyConnection.h"
#include "ParameterConnection.h"
#include "ParameterToStateConnection.h"
#include "SnapshotConnection.h"
#include "ProjectLoader.h"
#include "DataPanel.h"
#include "Utils.h"

#include <CrySystem/ISystem.h>
#include <CryCore/StlUtils.h>
#include <CrySerialization/IArchiveHost.h>
#include <CrySystem/XML/IXml.h>

namespace ACE
{
namespace Impl
{
namespace Fmod
{
std::vector<string> CImpl::s_programmerSoundEvents;

constexpr uint32 g_itemPoolSize = 8192;
constexpr uint32 g_eventConnectionPoolSize = 8192;
constexpr uint32 g_keyConnectionPoolSize = 4096;
constexpr uint32 g_parameterConnectionPoolSize = 512;
constexpr uint32 g_parameterToStateConnectionPoolSize = 256;
constexpr uint32 g_bankConnectionPoolSize = 256;
constexpr uint32 g_snapshotConnectionPoolSize = 128;
constexpr uint32 g_genericConnectionPoolSize = 128;

//////////////////////////////////////////////////////////////////////////
EItemType TagToType(char const* const szTag)
{
	EItemType type = EItemType::None;

	if (_stricmp(szTag, CryAudio::Impl::Fmod::g_szEventTag) == 0)
	{
		type = EItemType::Event;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Fmod::g_szParameterTag) == 0)
	{
		type = EItemType::Parameter;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Fmod::g_szKeyTag) == 0)
	{
		type = EItemType::Key;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Fmod::g_szSnapshotTag) == 0)
	{
		type = EItemType::Snapshot;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Fmod::g_szFileTag) == 0)
	{
		type = EItemType::Bank;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Fmod::g_szBusTag) == 0)
	{
		type = EItemType::Return;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Fmod::g_szVcaTag) == 0)
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
#endif  // USE_BACKWARDS_COMPATIBILITY

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
		szTag = CryAudio::Impl::Fmod::g_szEventTag;
		break;
	case EItemType::Key:
		szTag = CryAudio::Impl::Fmod::g_szKeyTag;
		break;
	case EItemType::Parameter:
		szTag = CryAudio::Impl::Fmod::g_szParameterTag;
		break;
	case EItemType::Snapshot:
		szTag = CryAudio::Impl::Fmod::g_szSnapshotTag;
		break;
	case EItemType::Bank:
		szTag = CryAudio::Impl::Fmod::g_szFileTag;
		break;
	case EItemType::Return:
		szTag = CryAudio::Impl::Fmod::g_szBusTag;
		break;
	case EItemType::VCA:
		szTag = CryAudio::Impl::Fmod::g_szVcaTag;
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
	case EItemType::Key:
		folderName = s_keysFolderName + "/";
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
void CountConnections(EAssetType const assetType, EItemType const itemType)
{
	switch (itemType)
	{
	case EItemType::Event: // Intentional fall-through.
	case EItemType::Key:
		{
			++g_connections.events;
			break;
		}

	case EItemType::Snapshot:
		{
			++g_connections.snapshots;
			break;
		}
	case EItemType::Parameter:
		{
			switch (assetType)
			{
			case EAssetType::Parameter:
				{
					++g_connections.parameters;
					break;
				}
			case EAssetType::State:
				{
					++g_connections.parameterStates;
					break;
				}
			case EAssetType::Environment:
				{
					++g_connections.parameterEnvironments;
					break;
				}
			default:
				break;
			}

			break;
		}
	case EItemType::VCA:
		{
			switch (assetType)
			{
			case EAssetType::Parameter:
				{
					++g_connections.vcas;
					break;
				}
			case EAssetType::State:
				{
					++g_connections.vcaStates;
					break;
				}
			default:
				break;
			}

			break;
		}
	case EItemType::Return:
		{
			++g_connections.returns;
			break;
		}
	case EItemType::Bank:
		{
			++g_connections.banks;
			break;
		}
	default:
		break;
	}
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
		size_t const numChildren = pItem->GetNumChildren();

		for (size_t i = 0; i < numChildren; ++i)
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
CImpl::CImpl()
	: m_pDataPanel(nullptr)
	, m_projectPath(CRY_AUDIO_DATA_ROOT "/fmod_project")
	, m_assetsPath(CRY_AUDIO_DATA_ROOT "/" + string(CryAudio::Impl::Fmod::g_szImplFolderName) + "/" + string(CryAudio::g_szAssetsFolderName))
	, m_localizedAssetsPath(m_assetsPath)
	, m_szUserSettingsFile("%USER%/audiocontrolseditor_fmod.user")
{
}

//////////////////////////////////////////////////////////////////////////
CImpl::~CImpl()
{
	Clear();
	DestroyDataPanel();

	CItem::FreeMemoryPool();
	CEventConnection::FreeMemoryPool();
	CKeyConnection::FreeMemoryPool();
	CParameterConnection::FreeMemoryPool();
	CParameterToStateConnection::FreeMemoryPool();
	CBankConnection::FreeMemoryPool();
	CSnapshotConnection::FreeMemoryPool();
	CGenericConnection::FreeMemoryPool();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::Initialize(
	SImplInfo& implInfo,
	Platforms const& platforms,
	ExtensionFilterVector& extensionFilters,
	QStringList& supportedFileTypes)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Fmod Studio ACE Item Pool");
	CItem::CreateAllocator(g_itemPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Fmod Studio ACE Event Connection Pool");
	CEventConnection::CreateAllocator(g_eventConnectionPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Fmod Studio ACE Key Connection Pool");
	CKeyConnection::CreateAllocator(g_keyConnectionPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Fmod Studio ACE Parameter Connection Pool");
	CParameterConnection::CreateAllocator(g_parameterConnectionPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Fmod Studio ACE Parameter To State Connection Pool");
	CParameterToStateConnection::CreateAllocator(g_parameterToStateConnectionPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Fmod Studio ACE Bank Connection Pool");
	CBankConnection::CreateAllocator(g_bankConnectionPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Fmod Studio ACE Snapshot Connection Pool");
	CSnapshotConnection::CreateAllocator(g_snapshotConnectionPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Fmod Studio ACE Generic Connection Pool");
	CGenericConnection::CreateAllocator(g_genericConnectionPoolSize);

	CryAudio::SImplInfo systemImplInfo;
	gEnv->pAudioSystem->GetImplInfo(systemImplInfo);
	m_implName = systemImplInfo.name.c_str();

	SetImplInfo(implInfo);
	g_platforms = platforms;

	Serialization::LoadJsonFile(*this, m_szUserSettingsFile);
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

	CProjectLoader(m_projectPath, m_assetsPath, m_localizedAssetsPath, m_rootItem, m_itemCache, *this);

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
void CImpl::SetProjectPath(char const* const szPath)
{
	m_projectPath = szPath;
	Serialization::SaveJsonFile(m_szUserSettingsFile, *this);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::Serialize(Serialization::IArchive& ar)
{
	ar(m_projectPath, "projectPath", "Project Path");
}

//////////////////////////////////////////////////////////////////////////
bool CImpl::IsTypeCompatible(EAssetType const assetType, IItem const* const pIItem) const
{
	bool isCompatible = false;
	auto const pItem = static_cast<CItem const* const>(pIItem);

	if (pItem != nullptr)
	{
		EItemType const implType = pItem->GetType();

		switch (assetType)
		{
		case EAssetType::Trigger:
			isCompatible = (implType == EItemType::Event) || (implType == EItemType::Key) || (implType == EItemType::Snapshot);
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
EAssetType CImpl::ImplTypeToAssetType(IItem const* const pIItem) const
{
	EAssetType assetType = EAssetType::None;
	auto const pItem = static_cast<CItem const* const>(pIItem);

	if (pItem != nullptr)
	{
		EItemType const implType = pItem->GetType();

		switch (implType)
		{
		case EItemType::Event: // Intentional fall-through.
		case EItemType::Key:   // Intentional fall-through.
		case EItemType::Snapshot:
			assetType = EAssetType::Trigger;
			break;
		case EItemType::Parameter: // Intentional fall-through.
		case EItemType::VCA:
			assetType = EAssetType::Parameter;
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
IConnection* CImpl::CreateConnectionToControl(EAssetType const assetType, IItem const* const pIItem)
{
	IConnection* pIConnection = nullptr;
	auto const pItem = static_cast<CItem const* const>(pIItem);

	if (pItem != nullptr)
	{
		EItemType const type = pItem->GetType();

		if (type == EItemType::Event)
		{
			pIConnection = static_cast<IConnection*>(new CEventConnection(pItem->GetId()));
		}
		else if (type == EItemType::Key)
		{
			pIConnection = static_cast<IConnection*>(new CKeyConnection(pItem->GetId()));
		}
		else if (type == EItemType::Snapshot)
		{
			pIConnection = static_cast<IConnection*>(new CSnapshotConnection(pItem->GetId()));
		}
		else if ((type == EItemType::Parameter) || (type == EItemType::VCA))
		{
			if ((assetType == EAssetType::Parameter) || (assetType == EAssetType::Environment))
			{
				pIConnection = static_cast<IConnection*>(new CParameterConnection(pItem->GetId()));
			}
			else if (assetType == EAssetType::State)
			{
				pIConnection = static_cast<IConnection*>(new CParameterToStateConnection(pItem->GetId(), type));
			}
			else
			{
				pIConnection = static_cast<IConnection*>(new CGenericConnection(pItem->GetId()));
			}
		}
		else if (type == EItemType::Bank)
		{
			pIConnection = static_cast<IConnection*>(new CBankConnection(pItem->GetId()));
		}
		else
		{
			pIConnection = static_cast<IConnection*>(new CGenericConnection(pItem->GetId()));
		}
	}

	return pIConnection;
}

//////////////////////////////////////////////////////////////////////////
IConnection* CImpl::CreateConnectionFromXMLNode(XmlNodeRef pNode, EAssetType const assetType)
{
	IConnection* pIConnection = nullptr;

	if (pNode != nullptr)
	{
		auto const type = TagToType(pNode->getTag());

		if (type != EItemType::None)
		{
			string name = pNode->getAttr(CryAudio::g_szNameAttribute);

#if defined (USE_BACKWARDS_COMPATIBILITY)
			if (name.IsEmpty() && pNode->haveAttr("fmod_name"))
			{
				name = pNode->getAttr("fmod_name");
			}
#endif      // USE_BACKWARDS_COMPATIBILITY

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

				string localizedAttribute = pNode->getAttr(CryAudio::Impl::Fmod::g_szLocalizedAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
				if (localizedAttribute.IsEmpty() && pNode->haveAttr("fmod_localized"))
				{
					localizedAttribute = pNode->getAttr("fmod_localized");
				}
#endif        // USE_BACKWARDS_COMPATIBILITY
				bool const isLocalized = (localizedAttribute.compareNoCase(CryAudio::Impl::Fmod::g_szTrueValue) == 0);

				string path = "";
				int const pos = name.find_last_of("/");

				if (pos != string::npos)
				{
					path = name.substr(0, pos);
					name = name.substr(pos + 1, name.length() - pos);
				}

				pItem = CreatePlaceholderItem(name, type, isLocalized, CreatePlaceholderFolderPath(path));
			}

			switch (type)
			{
			case EItemType::Event:
				{
					string actionType = pNode->getAttr(CryAudio::g_szTypeAttribute);

#if defined (USE_BACKWARDS_COMPATIBILITY)
					if (actionType.IsEmpty() && pNode->haveAttr("fmod_event_type"))
					{
						actionType = pNode->getAttr("fmod_event_type");
					}
#endif          // USE_BACKWARDS_COMPATIBILITY

					CEventConnection::EActionType eventActionType = CEventConnection::EActionType::Start;

					if (actionType.compareNoCase(CryAudio::Impl::Fmod::g_szStopValue) == 0)
					{
						eventActionType = CEventConnection::EActionType::Stop;
					}
					else if (actionType.compareNoCase(CryAudio::Impl::Fmod::g_szPauseValue) == 0)
					{
						eventActionType = CEventConnection::EActionType::Pause;
					}
					else if (actionType.compareNoCase(CryAudio::Impl::Fmod::g_szResumeValue) == 0)
					{
						eventActionType = CEventConnection::EActionType::Resume;
					}

					pIConnection = static_cast<IConnection*>(new CEventConnection(pItem->GetId(), eventActionType));
				}
				break;
			case EItemType::Key:
				{
					string const eventName = pNode->getAttr(CryAudio::Impl::Fmod::g_szEventAttribute);

					pIConnection = static_cast<IConnection*>(new CKeyConnection(pItem->GetId(), eventName));
				}
				break;
			case EItemType::Snapshot:
				{
					string actionType = pNode->getAttr(CryAudio::g_szTypeAttribute);

#if defined (USE_BACKWARDS_COMPATIBILITY)
					if (actionType.IsEmpty() && pNode->haveAttr("fmod_event_type"))
					{
						actionType = pNode->getAttr("fmod_event_type");
					}
#endif          // USE_BACKWARDS_COMPATIBILITY

					CSnapshotConnection::EActionType const snapshotActionType = (actionType.compareNoCase(CryAudio::Impl::Fmod::g_szStopValue) == 0) ? CSnapshotConnection::EActionType::Stop : CSnapshotConnection::EActionType::Start;
					pIConnection = static_cast<IConnection*>(new CSnapshotConnection(pItem->GetId(), snapshotActionType));
				}
				break;
			case EItemType::Parameter: // Intentional fall-through.
			case EItemType::VCA:
				{
					if ((assetType == EAssetType::Parameter) || (assetType == EAssetType::Environment))
					{
						float mult = CryAudio::Impl::Fmod::g_defaultParamMultiplier;
						float shift = CryAudio::Impl::Fmod::g_defaultParamShift;

						if (pNode->haveAttr(CryAudio::Impl::Fmod::g_szMutiplierAttribute))
						{
							string const value = pNode->getAttr(CryAudio::Impl::Fmod::g_szMutiplierAttribute);
							mult = (float)std::atof(value.c_str());
						}
#if defined (USE_BACKWARDS_COMPATIBILITY)
						else if (pNode->haveAttr("fmod_value_multiplier"))
						{
							string const value = pNode->getAttr("fmod_value_multiplier");
							mult = (float)std::atof(value.c_str());
						}
#endif            // USE_BACKWARDS_COMPATIBILITY
						if (pNode->haveAttr(CryAudio::Impl::Fmod::g_szShiftAttribute))
						{
							string const value = pNode->getAttr(CryAudio::Impl::Fmod::g_szShiftAttribute);
							shift = (float)std::atof(value.c_str());
						}
#if defined (USE_BACKWARDS_COMPATIBILITY)
						else if (pNode->haveAttr("fmod_value_shift"))
						{
							string const value = pNode->getAttr("fmod_value_shift");
							shift = (float)std::atof(value.c_str());
						}
#endif            // USE_BACKWARDS_COMPATIBILITY

						pIConnection = static_cast<IConnection*>(new CParameterConnection(pItem->GetId(), mult, shift));
					}
					else if (assetType == EAssetType::State)
					{
						string valueString = pNode->getAttr(CryAudio::Impl::Fmod::g_szValueAttribute);
#if defined (USE_BACKWARDS_COMPATIBILITY)
						if (valueString.IsEmpty() && pNode->haveAttr("fmod_value"))
						{
							valueString = pNode->getAttr("fmod_value");
						}
#endif            // USE_BACKWARDS_COMPATIBILITY

						auto const valueFloat = static_cast<float>(std::atof(valueString.c_str()));
						pIConnection = static_cast<IConnection*>(new CParameterToStateConnection(pItem->GetId(), type, valueFloat));
					}
				}
				break;
			case EItemType::Bank:
				{
					pIConnection = static_cast<IConnection*>(new CBankConnection(pItem->GetId()));
				}
				break;
			case EItemType::Return:
				{
					pIConnection = static_cast<IConnection*>(new CGenericConnection(pItem->GetId()));
				}
				break;
			}
		}
	}

	return pIConnection;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CImpl::CreateXMLNodeFromConnection(IConnection const* const pIConnection, EAssetType const assetType)
{
	XmlNodeRef pNode = nullptr;

	auto const pItem = static_cast<CItem const*>(GetItem(pIConnection->GetID()));

	if (pItem != nullptr)
	{
		auto const type = pItem->GetType();
		pNode = GetISystem()->CreateXmlNode(TypeToTag(type));

		switch (type)
		{
		case EItemType::Event:
			{
				pNode->setAttr(CryAudio::g_szNameAttribute, Utils::GetPathName(pItem, m_rootItem));
				auto const pEventConnection = static_cast<CEventConnection const*>(pIConnection);

				if (pEventConnection != nullptr)
				{
					CEventConnection::EActionType const actionType = pEventConnection->GetActionType();

					if (actionType == CEventConnection::EActionType::Stop)
					{
						pNode->setAttr(CryAudio::g_szTypeAttribute, CryAudio::Impl::Fmod::g_szStopValue);
					}
					else if (actionType == CEventConnection::EActionType::Pause)
					{
						pNode->setAttr(CryAudio::g_szTypeAttribute, CryAudio::Impl::Fmod::g_szPauseValue);
					}
					else if (actionType == CEventConnection::EActionType::Resume)
					{
						pNode->setAttr(CryAudio::g_szTypeAttribute, CryAudio::Impl::Fmod::g_szResumeValue);
					}
				}
			}
			break;
		case EItemType::Key:
			{
				pNode->setAttr(CryAudio::g_szNameAttribute, pItem->GetName());
				auto const pKeyConnection = static_cast<CKeyConnection const*>(pIConnection);

				if (pKeyConnection != nullptr)
				{
					pNode->setAttr(CryAudio::Impl::Fmod::g_szEventAttribute, pKeyConnection->GetEvent());
				}
			}
			break;
		case EItemType::Snapshot:
			{
				pNode->setAttr(CryAudio::g_szNameAttribute, Utils::GetPathName(pItem, m_rootItem));
				auto const pEventConnection = static_cast<CSnapshotConnection const*>(pIConnection);

				if ((pEventConnection != nullptr) && (pEventConnection->GetActionType() == CSnapshotConnection::EActionType::Stop))
				{
					pNode->setAttr(CryAudio::g_szTypeAttribute, CryAudio::Impl::Fmod::g_szStopValue);
				}
			}
			break;
		case EItemType::Return:
			{
				pNode->setAttr(CryAudio::g_szNameAttribute, Utils::GetPathName(pItem, m_rootItem));
			}
			break;
		case EItemType::Parameter: // Intentional fall-through.
		case EItemType::VCA:
			{
				pNode->setAttr(CryAudio::g_szNameAttribute, pItem->GetName());

				if (assetType == EAssetType::State)
				{
					auto const pStateConnection = static_cast<CParameterToStateConnection const*>(pIConnection);

					if (pStateConnection != nullptr)
					{
						pNode->setAttr(CryAudio::Impl::Fmod::g_szValueAttribute, pStateConnection->GetValue());
					}
				}
				else if ((assetType == EAssetType::Parameter) || (assetType == EAssetType::Environment))
				{
					auto const pParamConnection = static_cast<CParameterConnection const*>(pIConnection);

					if (pParamConnection->GetMultiplier() != CryAudio::Impl::Fmod::g_defaultParamMultiplier)
					{
						pNode->setAttr(CryAudio::Impl::Fmod::g_szMutiplierAttribute, pParamConnection->GetMultiplier());
					}

					if (pParamConnection->GetShift() != CryAudio::Impl::Fmod::g_defaultParamShift)
					{
						pNode->setAttr(CryAudio::Impl::Fmod::g_szShiftAttribute, pParamConnection->GetShift());
					}
				}
			}
			break;
		case EItemType::Bank:
			{
				pNode->setAttr(CryAudio::g_szNameAttribute, pItem->GetName());

				if ((pItem->GetFlags() & EItemFlags::IsLocalized) != 0)
				{
					pNode->setAttr(CryAudio::Impl::Fmod::g_szLocalizedAttribute, CryAudio::Impl::Fmod::g_szTrueValue);
				}
			}
			break;
		}

		CountConnections(assetType, type);
	}

	return pNode;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef CImpl::SetDataNode(char const* const szTag)
{
	XmlNodeRef pNode = GetISystem()->CreateXmlNode(szTag);
	bool hasConnections = false;

	if (g_connections.events > 0)
	{
		pNode->setAttr(CryAudio::Impl::Fmod::g_szEventsAttribute, g_connections.events);
		hasConnections = true;
	}

	if (g_connections.parameters > 0)
	{
		pNode->setAttr(CryAudio::Impl::Fmod::g_szParametersAttribute, g_connections.parameters);
		hasConnections = true;
	}

	if (g_connections.parameterEnvironments > 0)
	{
		pNode->setAttr(CryAudio::Impl::Fmod::g_szParameterEnvironmentsAttribute, g_connections.parameterEnvironments);
		hasConnections = true;
	}

	if (g_connections.parameterStates > 0)
	{
		pNode->setAttr(CryAudio::Impl::Fmod::g_szParameterStatesAttribute, g_connections.parameterStates);
		hasConnections = true;
	}

	if (g_connections.snapshots > 0)
	{
		pNode->setAttr(CryAudio::Impl::Fmod::g_szSnapshotsAttribute, g_connections.snapshots);
		hasConnections = true;
	}

	if (g_connections.returns > 0)
	{
		pNode->setAttr(CryAudio::Impl::Fmod::g_szReturnsAttribute, g_connections.returns);
		hasConnections = true;
	}

	if (g_connections.vcas > 0)
	{
		pNode->setAttr(CryAudio::Impl::Fmod::g_szVcasAttribute, g_connections.vcas);
		hasConnections = true;
	}

	if (g_connections.vcaStates > 0)
	{
		pNode->setAttr(CryAudio::Impl::Fmod::g_szVcaStatesAttribute, g_connections.vcaStates);
		hasConnections = true;
	}

	if (g_connections.banks > 0)
	{
		pNode->setAttr(CryAudio::Impl::Fmod::g_szBanksAttribute, g_connections.banks);
		hasConnections = true;
	}

	if (!hasConnections)
	{
		pNode = nullptr;
	}
	else
	{
		// Reset connection count for next library.
		ZeroStruct(g_connections);
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
void CImpl::Clear()
{
	for (auto const& itemPair : m_itemCache)
	{
		delete itemPair.second;
	}

	s_programmerSoundEvents.clear();
	m_itemCache.clear();
	m_rootItem.Clear();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetImplInfo(SImplInfo& implInfo)
{
	SetLocalizedAssetsPath();

	implInfo.name = m_implName.c_str();
	implInfo.folderName = CryAudio::Impl::Fmod::g_szImplFolderName;
	implInfo.projectPath = m_projectPath.c_str();
	implInfo.assetsPath = m_assetsPath.c_str();
	implInfo.localizedAssetsPath = m_localizedAssetsPath.c_str();
	implInfo.flags = (
		EImplInfoFlags::SupportsProjects |
		EImplInfoFlags::SupportsTriggers |
		EImplInfoFlags::SupportsParameters |
		EImplInfoFlags::SupportsSwitches |
		EImplInfoFlags::SupportsStates |
		EImplInfoFlags::SupportsEnvironments |
		EImplInfoFlags::SupportsPreloads);
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
			m_localizedAssetsPath += CRY_AUDIO_DATA_ROOT;
			m_localizedAssetsPath += "/";
			m_localizedAssetsPath += CryAudio::Impl::Fmod::g_szImplFolderName;
			m_localizedAssetsPath += "/";
			m_localizedAssetsPath += CryAudio::g_szAssetsFolderName;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CItem* CImpl::CreatePlaceholderItem(string const& name, EItemType const type, bool const isLocalized, CItem* const pParent)
{
	ControlId const id = Utils::GetId(type, name, pParent, m_rootItem);

	auto pItem = static_cast<CItem*>(GetItem(id));

	if (pItem == nullptr)
	{
		EItemFlags const flags = isLocalized ? (EItemFlags::IsPlaceHolder | EItemFlags::IsLocalized) : EItemFlags::IsPlaceHolder;
		pItem = new CItem(name, id, type, flags);

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
CItem* CImpl::GetItemFromPath(string const& fullpath)
{
	CItem* pItem = &m_rootItem;
	int start = 0;
	string token = fullpath.Tokenize("/", start);

	while (!token.empty())
	{
		token.Trim();
		CItem* pChild = nullptr;
		size_t const numChildren = pItem->GetNumChildren();

		for (size_t i = 0; i < numChildren; ++i)
		{
			auto const pNextChild = static_cast<CItem* const>(pItem->GetChildAt(i));

			if ((pNextChild != nullptr) && (pNextChild->GetName().compareNoCase(token) == 0))
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
CItem* CImpl::CreatePlaceholderFolderPath(string const& path)
{
	CItem* pItem = &m_rootItem;
	int start = 0;
	string token = path.Tokenize("/", start);

	while (!token.empty())
	{
		token.Trim();
		CItem* pFoundChild = nullptr;
		size_t const numChildren = pItem->GetNumChildren();

		for (size_t i = 0; i < numChildren; ++i)
		{
			auto const pChild = static_cast<CItem* const>(pItem->GetChildAt(i));

			if ((pChild != nullptr) && (pChild->GetName().compareNoCase(token) == 0))
			{
				pFoundChild = pChild;
				break;
			}
		}

		if (pFoundChild == nullptr)
		{
			pFoundChild = CreatePlaceholderItem(token, EItemType::Folder, false, pItem);
		}

		pItem = pFoundChild;
		token = path.Tokenize("/", start);
	}

	return pItem;
}
} // namespace Fmod
} // namespace Impl
} // namespace ACE
