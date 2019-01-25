// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Impl.h"

#include "Common.h"
#include "PlatformConnection.h"
#include "CueConnection.h"
#include "GenericConnection.h"
#include "ParameterConnection.h"
#include "ParameterToStateConnection.h"
#include "SnapshotConnection.h"
#include "ProjectLoader.h"
#include "DataPanel.h"
#include "Utils.h"

#include <CryAudioImplAdx2/GlobalData.h>
#include <CrySystem/ISystem.h>
#include <CryCore/StlUtils.h>
#include <CrySerialization/IArchiveHost.h>
#include <CrySystem/ILocalizationManager.h>
#include <CrySystem/XML/IXml.h>

namespace ACE
{
namespace Impl
{
namespace Adx2
{
constexpr uint32 g_itemPoolSize = 8192;
constexpr uint32 g_cueConnectionPoolSize = 8192;
constexpr uint32 g_parameterConnectionPoolSize = 512;
constexpr uint32 g_parameterToStateConnectionPoolSize = 256;
constexpr uint32 g_platformConnectionPoolSize = 256;
constexpr uint32 g_snapshotConnectionPoolSize = 128;
constexpr uint32 g_genericConnectionPoolSize = 512;

//////////////////////////////////////////////////////////////////////////
EItemType TagToType(char const* const szTag)
{
	EItemType type = EItemType::None;

	if (_stricmp(szTag, CryAudio::Impl::Adx2::g_szCueTag) == 0)
	{
		type = EItemType::Cue;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Adx2::g_szAisacControlTag) == 0)
	{
		type = EItemType::AisacControl;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Adx2::g_szGameVariableTag) == 0)
	{
		type = EItemType::GameVariable;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Adx2::g_szSelectorTag) == 0)
	{
		type = EItemType::Selector;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Adx2::g_szSCategoryTag) == 0)
	{
		type = EItemType::Category;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Adx2::g_szDspBusSettingTag) == 0)
	{
		type = EItemType::DspBusSetting;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Adx2::g_szDspBusTag) == 0)
	{
		type = EItemType::Bus;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Adx2::g_szSnapshotTag) == 0)
	{
		type = EItemType::Snapshot;
	}
	else if (_stricmp(szTag, CryAudio::Impl::Adx2::g_szBinaryTag) == 0)
	{
		type = EItemType::Binary;
	}
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
	case EItemType::Cue:
		{
			szTag = CryAudio::Impl::Adx2::g_szCueTag;
			break;
		}
	case EItemType::AisacControl:
		{
			szTag = CryAudio::Impl::Adx2::g_szAisacControlTag;
			break;
		}
	case EItemType::GameVariable:
		{
			szTag = CryAudio::Impl::Adx2::g_szGameVariableTag;
			break;
		}
	case EItemType::Selector:
		{
			szTag = CryAudio::Impl::Adx2::g_szSelectorTag;
			break;
		}
	case EItemType::SelectorLabel:
		{
			szTag = CryAudio::Impl::Adx2::g_szSelectorLabelTag;
			break;
		}
	case EItemType::Category:
		{
			szTag = CryAudio::Impl::Adx2::g_szSCategoryTag;
			break;
		}
	case EItemType::DspBusSetting:
		{
			szTag = CryAudio::Impl::Adx2::g_szDspBusSettingTag;
			break;
		}
	case EItemType::Bus:
		{
			szTag = CryAudio::Impl::Adx2::g_szDspBusTag;
			break;
		}
	case EItemType::Snapshot:
		{
			szTag = CryAudio::Impl::Adx2::g_szSnapshotTag;
			break;
		}
	case EItemType::Binary:
		{
			szTag = CryAudio::Impl::Adx2::g_szBinaryTag;
			break;
		}
	default:
		{
			szTag = nullptr;
			break;
		}
	}

	return szTag;
}

//////////////////////////////////////////////////////////////////////////
void CountConnections(EAssetType const assetType, EItemType const itemType)
{
	switch (itemType)
	{
	case EItemType::Cue:
		{
			++g_connections.cues;
			break;
		}
	case EItemType::AisacControl:
		{
			switch (assetType)
			{
			case EAssetType::Parameter:
				{
					++g_connections.aisacControls;
					break;
				}
			case EAssetType::State:
				{
					++g_connections.aisacStates;
					break;
				}
			case EAssetType::Environment:
				{
					++g_connections.aisacEnvironments;
					break;
				}
			default:
				{
					break;
				}
			}

			break;
		}
	case EItemType::Category:
		{
			switch (assetType)
			{
			case EAssetType::Parameter:
				{
					++g_connections.categories;
					break;
				}
			case EAssetType::State:
				{
					++g_connections.categoryStates;
					break;
				}
			default:
				{
					break;
				}
			}

			break;
		}
	case EItemType::GameVariable:
		{
			switch (assetType)
			{
			case EAssetType::Parameter:
				{
					++g_connections.gameVariables;
					break;
				}
			case EAssetType::State:
				{
					++g_connections.gameVariableStates;
					break;
				}
			default:
				{
					break;
				}
			}

			break;
		}
	case EItemType::SelectorLabel:
		{
			++g_connections.selectorLabels;
			break;
		}
	case EItemType::Bus:
		{
			++g_connections.dspBuses;
			break;
		}
	case EItemType::Snapshot:
		{
			++g_connections.snapshots;
			break;
		}
	case EItemType::DspBusSetting:
		{
			++g_connections.dspBusSettings;
			break;
		}
	case EItemType::Binary:
		{
			++g_connections.binaries;
			break;
		}
	default:
		{
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CItem* SearchForItem(CItem* const pItem, string const& name, EItemType const type, string const& cuesheetName)
{
	CItem* pSearchedItem = nullptr;
	bool const nameAndTypeMatches = (pItem->GetName().compareNoCase(name) == 0) && (pItem->GetType() == type);

	if (nameAndTypeMatches && (type == EItemType::Cue) && (pItem->GetCueSheetName().compareNoCase(cuesheetName) == 0))
	{
		pSearchedItem = pItem;
	}
	else if (nameAndTypeMatches && (type != EItemType::Cue))
	{
		pSearchedItem = pItem;
	}
	else
	{
		size_t const numChildren = pItem->GetNumChildren();

		for (size_t i = 0; i < numChildren; ++i)
		{
			CItem* const pFoundItem = SearchForItem(static_cast<CItem* const>(pItem->GetChildAt(i)), name, type, cuesheetName);

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
	, m_projectPath(AUDIO_SYSTEM_DATA_ROOT "/adx2_project")
	, m_assetsPath(AUDIO_SYSTEM_DATA_ROOT "/" + string(CryAudio::Impl::Adx2::g_szImplFolderName) + "/" + string(CryAudio::s_szAssetsFolderName))
	, m_localizedAssetsPath(m_assetsPath)
	, m_szUserSettingsFile("%USER%/audiocontrolseditor_adx2.user")
{
}

//////////////////////////////////////////////////////////////////////////
CImpl::~CImpl()
{
	Clear();
	DestroyDataPanel();

	CItem::FreeMemoryPool();
	CCueConnection::FreeMemoryPool();
	CParameterConnection::FreeMemoryPool();
	CParameterToStateConnection::FreeMemoryPool();
	CPlatformConnection::FreeMemoryPool();
	CSnapshotConnection::FreeMemoryPool();
	CGenericConnection::FreeMemoryPool();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::Initialize(SImplInfo& implInfo, Platforms const& platforms)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 ACE Item Pool");
	CItem::CreateAllocator(g_itemPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 ACE Cue Connection Pool");
	CCueConnection::CreateAllocator(g_cueConnectionPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 ACE Parameter Connection Pool");
	CParameterConnection::CreateAllocator(g_parameterConnectionPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 ACE Parameter To State Connection Pool");
	CParameterToStateConnection::CreateAllocator(g_parameterToStateConnectionPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 ACE Platform Connection Pool");
	CPlatformConnection::CreateAllocator(g_platformConnectionPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 ACE Snapshot Connection Pool");
	CSnapshotConnection::CreateAllocator(g_snapshotConnectionPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 ACE Generic Connection Pool");
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
			{
				isCompatible = (implType == EItemType::Cue) || (implType == EItemType::Snapshot);
				break;
			}
		case EAssetType::Parameter:
			{
				isCompatible = (implType == EItemType::AisacControl) || (implType == EItemType::Category) || (implType == EItemType::GameVariable);
				break;
			}
		case EAssetType::State:
			{
				isCompatible = (implType == EItemType::SelectorLabel) || (implType == EItemType::AisacControl) || (implType == EItemType::Category) || (implType == EItemType::GameVariable);
				break;
			}
		case EAssetType::Environment:
			{
				isCompatible = (implType == EItemType::Bus) || (implType == EItemType::AisacControl);
				break;
			}
		case EAssetType::Preload:
			{
				isCompatible = (implType == EItemType::Binary);
				break;
			}
		case EAssetType::Setting:
			{
				isCompatible = (implType == EItemType::DspBusSetting);
				break;
			}
		default:
			{
				isCompatible = false;
				break;
			}
		}
	}

	return isCompatible;
}

//////////////////////////////////////////////////////////////////////////
EAssetType CImpl::ImplTypeToAssetType(IItem const* const pIItem) const
{
	EAssetType assetType = EAssetType::None;

	if (pIItem != nullptr)
	{
		auto const pItem = static_cast<CItem const* const>(pIItem);
		EItemType const implType = pItem->GetType();

		switch (implType)
		{
		case EItemType::Cue: // Intentional fall-through.
		case EItemType::Snapshot:
			{
				assetType = EAssetType::Trigger;
				break;
			}
		case EItemType::AisacControl: // Intentional fall-through.
		case EItemType::Category:     // Intentional fall-through.
		case EItemType::GameVariable:
			{
				assetType = EAssetType::Parameter;
				break;
			}
		case EItemType::Selector:
			{
				assetType = EAssetType::Switch;
				break;
			}
		case EItemType::SelectorLabel:
			{
				assetType = EAssetType::State;
				break;
			}
		case EItemType::Binary:
			{
				assetType = EAssetType::Preload;
				break;
			}
		case EItemType::DspBusSetting:
			{
				assetType = EAssetType::Setting;
				break;
			}
		case EItemType::Bus:
			{
				assetType = EAssetType::Environment;
				break;
			}
		default:
			{
				assetType = EAssetType::None;
				break;
			}
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

		if (type == EItemType::Cue)
		{
			pIConnection = static_cast<IConnection*>(new CCueConnection(pItem->GetId()));
		}
		else if ((type == EItemType::AisacControl) || (type == EItemType::Category) || (type == EItemType::GameVariable))
		{
			if ((assetType == EAssetType::Parameter) || (assetType == EAssetType::Environment))
			{
				pIConnection = static_cast<IConnection*>(new CParameterConnection(pItem->GetId()));
			}
			else if (assetType == EAssetType::State)
			{
				float const value = (type == EItemType::Category) ? CryAudio::Impl::Adx2::g_defaultCategoryVolume : CryAudio::Impl::Adx2::g_defaultStateValue;
				pIConnection = static_cast<IConnection*>(new CParameterToStateConnection(pItem->GetId(), type, value));
			}
			else
			{
				pIConnection = static_cast<IConnection*>(new CGenericConnection(pItem->GetId()));
			}
		}
		else if ((type == EItemType::Binary) || (type == EItemType::DspBusSetting))
		{
			pIConnection = static_cast<IConnection*>(new CPlatformConnection(pItem->GetId()));
		}
		else if (type == EItemType::Snapshot)
		{
			pIConnection = static_cast<IConnection*>(new CSnapshotConnection(pItem->GetId()));
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
			string name = pNode->getAttr(CryAudio::s_szNameAttribute);
			string const cueSheetName = pNode->getAttr(CryAudio::Impl::Adx2::g_szCueSheetAttribute);

			CItem* pItem = SearchForItem(&m_rootItem, name, type, cueSheetName);

			if ((pItem == nullptr) || (pItem->GetType() != type))
			{
				string const localizedAttribute = pNode->getAttr(CryAudio::Impl::Adx2::g_szLocalizedAttribute);
				bool const isLocalized = (localizedAttribute.compareNoCase(CryAudio::Impl::Adx2::g_szTrueValue) == 0);

				pItem = CreatePlaceholderItem(name, type, isLocalized, &m_rootItem);
			}

			switch (type)
			{
			case EItemType::Cue:
				{
					string const cueSheetName = pNode->getAttr(CryAudio::Impl::Adx2::g_szCueSheetAttribute);
					string const actionType = pNode->getAttr(CryAudio::s_szTypeAttribute);
					CCueConnection::EActionType cueActionType = CCueConnection::EActionType::Start;

					if (actionType.compareNoCase(CryAudio::Impl::Adx2::g_szStopValue) == 0)
					{
						cueActionType = CCueConnection::EActionType::Stop;
					}
					else if (actionType.compareNoCase(CryAudio::Impl::Adx2::g_szPauseValue) == 0)
					{
						cueActionType = CCueConnection::EActionType::Pause;
					}
					else if (actionType.compareNoCase(CryAudio::Impl::Adx2::g_szResumeValue) == 0)
					{
						cueActionType = CCueConnection::EActionType::Resume;
					}

					pIConnection = static_cast<IConnection*>(new CCueConnection(pItem->GetId(), cueSheetName, cueActionType));

					break;
				}
			case EItemType::AisacControl: // Intentional fall-through.
			case EItemType::Category:     // Intentional fall-through.
			case EItemType::GameVariable:
				{
					if ((assetType == EAssetType::Parameter) || (assetType == EAssetType::Environment))
					{
						float mult = CryAudio::Impl::Adx2::g_defaultParamMultiplier;
						float shift = CryAudio::Impl::Adx2::g_defaultParamShift;
						pNode->getAttr(CryAudio::Impl::Adx2::g_szMutiplierAttribute, mult);
						pNode->getAttr(CryAudio::Impl::Adx2::g_szShiftAttribute, shift);

						auto const pParameterConnection = new CParameterConnection(pItem->GetId(), mult, shift);
						pIConnection = static_cast<IConnection*>(pParameterConnection);
					}
					else if (assetType == EAssetType::State)
					{
						float value = (type == EItemType::Category) ? CryAudio::Impl::Adx2::g_defaultCategoryVolume : CryAudio::Impl::Adx2::g_defaultStateValue;
						pNode->getAttr(CryAudio::Impl::Adx2::g_szValueAttribute, value);

						pIConnection = static_cast<IConnection*>(new CParameterToStateConnection(pItem->GetId(), type, value));
					}

					break;
				}
			case EItemType::Selector:
				{
					if (pNode->getChildCount() == 1)
					{
						pNode = pNode->getChild(0);

						if (pNode != nullptr)
						{
							CItem* pSelectorLabelItem = nullptr;
							string const childName = pNode->getAttr(CryAudio::s_szNameAttribute);
							size_t const numChildren = pItem->GetNumChildren();

							for (size_t i = 0; i < numChildren; ++i)
							{
								auto const pChild = static_cast<CItem* const>(pItem->GetChildAt(i));

								if ((pChild != nullptr) && (pChild->GetName().compareNoCase(childName) == 0))
								{
									pSelectorLabelItem = pChild;
									break;
								}
							}

							if (pSelectorLabelItem == nullptr)
							{
								pSelectorLabelItem = CreatePlaceholderItem(childName, EItemType::SelectorLabel, false, pItem);
							}

							pIConnection = static_cast<IConnection*>(new CGenericConnection(pSelectorLabelItem->GetId()));
						}
					}
					else
					{
						CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "[Audio Controls Editor] [Adx2] Error reading connection to Adx2 control %s", name);
					}

					break;
				}
			case EItemType::Bus:
				{
					pIConnection = static_cast<IConnection*>(new CGenericConnection(pItem->GetId()));

					break;
				}
			case EItemType::Snapshot:
				{
					string const actionType = pNode->getAttr(CryAudio::s_szTypeAttribute);

					CSnapshotConnection::EActionType const snapshotActionType =
						(actionType.compareNoCase(CryAudio::Impl::Adx2::g_szStopValue) == 0) ? CSnapshotConnection::EActionType::Stop : CSnapshotConnection::EActionType::Start;

					int changeoverTime = CryAudio::Impl::Adx2::g_defaultChangeoverTime;
					pNode->getAttr(CryAudio::Impl::Adx2::g_szTimeAttribute, changeoverTime);

					pIConnection = static_cast<IConnection*>(new CSnapshotConnection(pItem->GetId(), snapshotActionType, changeoverTime));

					break;
				}
			case EItemType::Binary: // Intentional fall-through.
			case EItemType::DspBusSetting:
				{
					pIConnection = static_cast<IConnection*>(new CPlatformConnection(pItem->GetId()));

					break;
				}
			default:
				{
					break;
				}
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
		case EItemType::Cue:
			{
				pNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());
				auto const pCueConnection = static_cast<CCueConnection const*>(pIConnection);

				string cueSheetName = Utils::FindCueSheetName(pItem, m_rootItem);

				if (cueSheetName.empty())
				{
					cueSheetName = pCueConnection->GetCueSheetName();
				}

				pNode->setAttr(CryAudio::Impl::Adx2::g_szCueSheetAttribute, cueSheetName);

				if (pCueConnection->GetActionType() == CCueConnection::EActionType::Stop)
				{
					pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::Adx2::g_szStopValue);
				}
				else if (pCueConnection->GetActionType() == CCueConnection::EActionType::Pause)
				{
					pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::Adx2::g_szPauseValue);
				}
				else if (pCueConnection->GetActionType() == CCueConnection::EActionType::Resume)
				{
					pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::Adx2::g_szResumeValue);
				}

				break;
			}
		case EItemType::AisacControl: // Intentional fall-through.
		case EItemType::Category:     // Intentional fall-through.
		case EItemType::GameVariable:
			{
				pNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

				if ((assetType == EAssetType::Parameter) || (assetType == EAssetType::Environment))
				{
					auto const pParamConnection = static_cast<CParameterConnection const*>(pIConnection);

					if (pParamConnection->GetMultiplier() != CryAudio::Impl::Adx2::g_defaultParamMultiplier)
					{
						pNode->setAttr(CryAudio::Impl::Adx2::g_szMutiplierAttribute, pParamConnection->GetMultiplier());
					}

					if (pParamConnection->GetShift() != CryAudio::Impl::Adx2::g_defaultParamShift)
					{
						pNode->setAttr(CryAudio::Impl::Adx2::g_szShiftAttribute, pParamConnection->GetShift());
					}
				}
				else if (assetType == EAssetType::State)
				{
					auto const pStateConnection = static_cast<CParameterToStateConnection const*>(pIConnection);

					pNode->setAttr(CryAudio::Impl::Adx2::g_szValueAttribute, pStateConnection->GetValue());
				}

				break;
			}
		case EItemType::SelectorLabel:
			{
				auto const pParent = static_cast<CItem const* const>(pItem->GetParent());

				if (pParent != nullptr)
				{
					XmlNodeRef const pSwitchNode = GetISystem()->CreateXmlNode(TypeToTag(pParent->GetType()));
					pSwitchNode->setAttr(CryAudio::s_szNameAttribute, pParent->GetName());

					XmlNodeRef const pStateNode = pSwitchNode->createNode(CryAudio::Impl::Adx2::g_szSelectorLabelTag);
					pStateNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());
					pSwitchNode->addChild(pStateNode);

					pNode = pSwitchNode;
				}

				break;
			}
		case EItemType::DspBusSetting: // Intentional fall-through.
		case EItemType::Bus:
			{
				pNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

				break;
			}
		case EItemType::Snapshot:
			{
				pNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

				auto const pSnapshotConnection = static_cast<CSnapshotConnection const*>(pIConnection);

				if (pSnapshotConnection->GetActionType() == CSnapshotConnection::EActionType::Stop)
				{
					pNode->setAttr(CryAudio::s_szTypeAttribute, CryAudio::Impl::Adx2::g_szStopValue);
				}

				if (pSnapshotConnection->GetChangeoverTime() != CryAudio::Impl::Adx2::g_defaultChangeoverTime)
				{
					pNode->setAttr(CryAudio::Impl::Adx2::g_szTimeAttribute, pSnapshotConnection->GetChangeoverTime());
				}

				break;
			}
		case EItemType::Binary:
			{
				pNode->setAttr(CryAudio::s_szNameAttribute, pItem->GetName());

				if ((pItem->GetFlags() & EItemFlags::IsLocalized) != 0)
				{
					pNode->setAttr(CryAudio::Impl::Adx2::g_szLocalizedAttribute, CryAudio::Impl::Adx2::g_szTrueValue);
				}

				break;
			}
		default:
			{
				break;
			}
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

	if (g_connections.cues > 0)
	{
		pNode->setAttr(CryAudio::Impl::Adx2::g_szCuesAttribute, g_connections.cues);
		hasConnections = true;
	}

	if (g_connections.aisacControls > 0)
	{
		pNode->setAttr(CryAudio::Impl::Adx2::g_szAisacControlsAttribute, g_connections.aisacControls);
		hasConnections = true;
	}

	if (g_connections.aisacEnvironments > 0)
	{
		pNode->setAttr(CryAudio::Impl::Adx2::g_szAisacEnvironmentsAttribute, g_connections.aisacEnvironments);
		hasConnections = true;
	}

	if (g_connections.aisacStates > 0)
	{
		pNode->setAttr(CryAudio::Impl::Adx2::g_szAisacStatesAttribute, g_connections.aisacStates);
		hasConnections = true;
	}

	if (g_connections.categories > 0)
	{
		pNode->setAttr(CryAudio::Impl::Adx2::g_szCategoriesAttribute, g_connections.categories);
		hasConnections = true;
	}

	if (g_connections.categoryStates > 0)
	{
		pNode->setAttr(CryAudio::Impl::Adx2::g_szCategoryStatesAttribute, g_connections.categoryStates);
		hasConnections = true;
	}

	if (g_connections.gameVariables > 0)
	{
		pNode->setAttr(CryAudio::Impl::Adx2::g_szGameVariablesAttribute, g_connections.gameVariables);
		hasConnections = true;
	}

	if (g_connections.gameVariableStates > 0)
	{
		pNode->setAttr(CryAudio::Impl::Adx2::g_szGameVariableStatesAttribute, g_connections.gameVariableStates);
		hasConnections = true;
	}

	if (g_connections.selectorLabels > 0)
	{
		pNode->setAttr(CryAudio::Impl::Adx2::g_szSelectorLabelsAttribute, g_connections.selectorLabels);
		hasConnections = true;
	}

	if (g_connections.dspBuses > 0)
	{
		pNode->setAttr(CryAudio::Impl::Adx2::g_szDspBusesAttribute, g_connections.dspBuses);
		hasConnections = true;
	}

	if (g_connections.snapshots > 0)
	{
		pNode->setAttr(CryAudio::Impl::Adx2::g_szSnapshotsAttribute, g_connections.snapshots);
		hasConnections = true;
	}

	if (g_connections.dspBusSettings > 0)
	{
		pNode->setAttr(CryAudio::Impl::Adx2::g_szDspBusSettingsAttribute, g_connections.dspBusSettings);
		hasConnections = true;
	}

	if (g_connections.binaries > 0)
	{
		pNode->setAttr(CryAudio::Impl::Adx2::g_szBinariesAttribute, g_connections.binaries);
		hasConnections = true;
	}

	if (hasConnections)
	{
		// Reset connection count for next library.
		ZeroStruct(g_connections);
	}
	else
	{
		pNode = nullptr;
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

	m_itemCache.clear();
	m_rootItem.Clear();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetImplInfo(SImplInfo& implInfo)
{
	SetLocalizedAssetsPath();

	implInfo.name = m_implName.c_str();
	implInfo.folderName = CryAudio::Impl::Adx2::g_szImplFolderName;
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
		EImplInfoFlags::SupportsPreloads |
		EImplInfoFlags::SupportsSettings);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetLocalizedAssetsPath()
{
	if (ICVar* pCVar = gEnv->pConsole->GetCVar("g_languageAudio"))
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
			m_localizedAssetsPath += CryAudio::Impl::Adx2::g_szImplFolderName;
			m_localizedAssetsPath += "/";
			m_localizedAssetsPath += CryAudio::s_szAssetsFolderName;
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
} // namespace Adx2
} // namespace Impl
} // namespace ACE
