// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Impl.h"

#include "CVars.h"
#include "Listener.h"
#include "Object.h"
#include "GlobalObject.h"
#include "File.h"
#include "Event.h"
#include "Trigger.h"
#include "Parameter.h"
#include "SwitchState.h"
#include "Environment.h"
#include "Setting.h"
#include "StandaloneFile.h"
#include "GlobalData.h"
#include "IoInterface.h"
#include <CrySystem/IStreamEngine.h>

#include <Logger.h>
#include <CrySystem/File/ICryPak.h>
#include <CrySystem/IProjectManager.h>
#include <CryAudio/IAudioSystem.h>

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	#include "Debug.h"
	#include <CryRenderer/IRenderAuxGeom.h>
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

#if defined(CRY_PLATFORM_WINDOWS)
	#include <cri_atom_wasapi.h>
#endif // CRY_PLATFORM_WINDOWS

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
std::map<string, std::vector<std::pair<string, float>>> g_cueRadiusInfo;

//////////////////////////////////////////////////////////////////////////
void ParseAcbInfoFile(XmlNodeRef const pRoot, string const& acbName)
{
	int const numChildren = pRoot->getChildCount();

	for (int i = 0; i < numChildren; ++i)
	{
		XmlNodeRef const pChild = pRoot->getChild(i);

		if (pChild != nullptr)
		{
			char const* const typeAttrib = pChild->getAttr("OrcaType");

			if (_stricmp(typeAttrib, "CriMw.CriAtomCraft.AcCore.AcOoCueSynthCue") == 0)
			{
				if (pChild->haveAttr("Pos3dDistanceMax"))
				{
					float distanceMax = 0.0f;
					pChild->getAttr("Pos3dDistanceMax", distanceMax);

					g_cueRadiusInfo[pChild->getAttr("OrcaName")].emplace_back(acbName, distanceMax);
				}
			}
			else if ((_stricmp(typeAttrib, "CriMw.CriAtomCraft.AcCore.AcOoCueFolder") == 0) ||
			         (_stricmp(typeAttrib, "CriMw.CriAtomCraft.AcCore.AcOoCueFolderPrivate") == 0))
			{
				ParseAcbInfoFile(pChild, acbName);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void LoadAcbInfos(string const& folderPath)
{
	_finddata_t fd;
	ICryPak* const pCryPak = gEnv->pCryPak;
	intptr_t const handle = pCryPak->FindFirst(folderPath + "/*_acb_info.xml", &fd);

	if (handle != -1)
	{
		do
		{
			string fileName = fd.name;
			XmlNodeRef const pRoot = GetISystem()->LoadXmlFromFile(folderPath + "/" + fileName);

			if (pRoot != nullptr)
			{
				XmlNodeRef const pInfoNode = pRoot->getChild(0);

				if (pInfoNode != nullptr)
				{
					XmlNodeRef const pInfoRootNode = pInfoNode->getChild(0);

					if (pInfoRootNode != nullptr)
					{
						XmlNodeRef const pAcbNode = pInfoRootNode->getChild(0);

						if ((pAcbNode != nullptr) && pAcbNode->haveAttr("AwbHash"))
						{
							ParseAcbInfoFile(pAcbNode, pAcbNode->getAttr("OrcaName"));
						}
					}
				}
			}
		}
		while (pCryPak->FindNext(handle, &fd) >= 0);

		pCryPak->FindClose(handle);
	}
}

//////////////////////////////////////////////////////////////////////////
static void errorCallback(Char8 const* const errid, Uint32 const p1, Uint32 const p2, Uint32* const parray)
{
	Char8 const* errorMessage;
	errorMessage = criErr_ConvertIdToMessage(errid, p1, p2);
	Cry::Audio::Log(ELogType::Error, static_cast<char const*>(errorMessage));
}
#endif // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
CriError selectIoFunc(CriChar8 const* szPath, CriFsDeviceId* pDeviceId, CriFsIoInterfacePtr* pIoInterface)
{
	CRY_ASSERT_MESSAGE(szPath != nullptr, "szPath is null pointer");
	CRY_ASSERT_MESSAGE(pDeviceId != nullptr, "pDeviceId is null pointer");
	CRY_ASSERT_MESSAGE(pIoInterface != nullptr, "pIoInterface is null pointer");

	*pDeviceId = CRIFS_DEFAULT_DEVICE;
	*pIoInterface = &g_IoInterface;

	return CRIERR_OK;
}
//////////////////////////////////////////////////////////////////////////
void* userMalloc(void* const pObj, CriUint32 const size)
{
	void* const pMem = CryModuleMalloc(static_cast<size_t>(size));
	return pMem;
}

//////////////////////////////////////////////////////////////////////////
void userFree(void* const pObj, void* const pMem)
{
	CryModuleFree(pMem);
}

///////////////////////////////////////////////////////////////////////////
CImpl::CImpl()
	: m_isMuted(false)
	, m_pAcfBuffer(nullptr)
	, m_dbasId(CRIATOMEXDBAS_ILLEGAL_ID)
{
	m_constructedObjects.reserve(256);
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::Init(uint32 const objectPoolSize, uint32 const eventPoolSize)
{
	ERequestStatus result = ERequestStatus::Success;

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Adx2 Object Pool");
	CObject::CreateAllocator(objectPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_Other, 0, "Adx2 Event Pool");
	CEvent::CreateAllocator(eventPoolSize);

	m_regularSoundBankFolder = AUDIO_SYSTEM_DATA_ROOT;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += s_szImplFolderName;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += s_szAssetsFolderName;
	m_localizedSoundBankFolder = m_regularSoundBankFolder;

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	m_name = "Adx2 (" CRI_ATOM_VER_NUM ")";

	LoadAcbInfos(m_regularSoundBankFolder);

	if (ICVar* pCVar = gEnv->pConsole->GetCVar("g_languageAudio"))
	{
		SetLanguage(pCVar->GetString());
		LoadAcbInfos(m_localizedSoundBankFolder);
	}

	criErr_SetCallback(errorCallback);
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

	criAtomEx_SetUserAllocator(userMalloc, userFree, nullptr);

	InitializeFileSystem();

	if (InitializeLibrary() && AllocateVoicePool() && CreateDbas() && RegisterAcf())
	{
		SetListenerConfig();
		SetPlayerConfig();
		Set3dSourceConfig();
	}
	else
	{
		ShutDown();
		result = ERequestStatus::Failure;
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::ShutDown()
{
	criAtomEx_UnregisterAcf();
	criAtomExDbas_Destroy(m_dbasId);
	criAtomExVoicePool_FreeAll();

#if defined(CRY_PLATFORM_WINDOWS)
	criAtomEx_Finalize_WASAPI();
#else
	criAtomEx_Finalize();
#endif  // CRY_PLATFORM_WINDOWS

	criFs_FinalizeLibrary();

	delete[] m_pAcfBuffer;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::Release()
{
	delete this;
	g_cvars.UnregisterVariables();

	CObject::FreeMemoryPool();
	CEvent::FreeMemoryPool();
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::OnLoseFocus()
{
	if (!m_isMuted)
	{
		MuteAllObjects(CRI_TRUE);
	}

	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::OnGetFocus()
{
	if (!m_isMuted)
	{
		MuteAllObjects(CRI_FALSE);
	}

	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::MuteAll()
{
	MuteAllObjects(CRI_TRUE);
	m_isMuted = true;
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::UnmuteAll()
{
	MuteAllObjects(CRI_FALSE);
	m_isMuted = false;
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::PauseAll()
{
	PauseAllObjects(CRI_TRUE);
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::ResumeAll()
{
	PauseAllObjects(CRI_FALSE);
	return ERequestStatus::Success;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::StopAllSounds()
{
	criAtomExPlayer_StopAllPlayers();
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::RegisterInMemoryFile(SFileInfo* const pFileInfo)
{
	ERequestStatus result = ERequestStatus::Failure;

	if (pFileInfo != nullptr)
	{
		auto const pFileData = static_cast<CFile*>(pFileInfo->pImplData);

		if (pFileData != nullptr)
		{
			string acbToAwbPath = pFileInfo->szFilePath;
			PathUtil::RemoveExtension(acbToAwbPath);
			acbToAwbPath += ".awb";

			CriChar8 const* const awbPath = (gEnv->pCryPak->IsFileExist(acbToAwbPath.c_str())) ? static_cast<CriChar8 const*>(acbToAwbPath.c_str()) : nullptr;

			pFileData->pAcb = criAtomExAcb_LoadAcbData(pFileInfo->pFileData, static_cast<CriSint32>(pFileInfo->size), nullptr, awbPath, nullptr, 0);

			if (pFileData->pAcb != nullptr)
			{
				string name = pFileInfo->szFileName;
				PathUtil::RemoveExtension(name);
				g_acbHandles[StringToId(name.c_str())] = pFileData->pAcb;

				result = ERequestStatus::Success;
			}
			else
			{
				Cry::Audio::Log(ELogType::Error, "Failed to load ACB %s\n", pFileInfo->szFileName);
			}
		}
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid FileData passed to the Adx2 implementation of RegisterInMemoryFile");
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::UnregisterInMemoryFile(SFileInfo* const pFileInfo)
{
	ERequestStatus result = ERequestStatus::Failure;

	if (pFileInfo != nullptr)
	{
		auto const pFileData = static_cast<CFile*>(pFileInfo->pImplData);

		if ((pFileData != nullptr) && (pFileData->pAcb != nullptr))
		{
			criAtomExAcb_Release(pFileData->pAcb);
			result = ERequestStatus::Success;
		}
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid FileData passed to the Adx2 implementation of UnregisterInMemoryFile");
		}
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::ConstructFile(XmlNodeRef const pRootNode, SFileInfo* const pFileInfo)
{
	ERequestStatus result = ERequestStatus::Failure;

	if ((_stricmp(pRootNode->getTag(), s_szFileTag) == 0) && (pFileInfo != nullptr))
	{
		char const* const szFileName = pRootNode->getAttr(s_szNameAttribute);

		if (szFileName != nullptr && szFileName[0] != '\0')
		{
			char const* const szLocalized = pRootNode->getAttr(s_szLocalizedAttribute);
			pFileInfo->bLocalized = (szLocalized != nullptr) && (_stricmp(szLocalized, s_szTrueValue) == 0);
			pFileInfo->szFileName = szFileName;

			// The Atom library accesses on-memory data with a 32-bit width.
			// The first address of the data must be aligned at a 4-byte boundary.
			pFileInfo->memoryBlockAlignment = 32;

			pFileInfo->pImplData = new CFile();

			result = ERequestStatus::Success;
		}
		else
		{
			pFileInfo->szFileName = nullptr;
			pFileInfo->memoryBlockAlignment = 0;
			pFileInfo->pImplData = nullptr;
		}
	}

	// To do: Handle DSP bus settings.

	return result;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructFile(IFile* const pIFile)
{
	delete pIFile;
}

//////////////////////////////////////////////////////////////////////////
char const* const CImpl::GetFileLocation(SFileInfo* const pFileInfo)
{
	char const* szResult = nullptr;

	if (pFileInfo != nullptr)
	{
		szResult = pFileInfo->bLocalized ? m_localizedSoundBankFolder.c_str() : m_regularSoundBankFolder.c_str();
	}

	return szResult;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GetInfo(SImplInfo& implInfo) const
{
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	implInfo.name = m_name.c_str();
#else
	implInfo.name = "name-not-present-in-release-mode";
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
	implInfo.folderName = s_szImplFolderName;
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructGlobalObject()
{
	auto const pObject = new CGlobalObject(m_constructedObjects);

	if (!stl::push_back_unique(m_constructedObjects, pObject))
	{
		Cry::Audio::Log(ELogType::Warning, "Trying to construct an already registered object.");
	}

	return static_cast<IObject*>(pObject);
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructObject(CObjectTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	auto const pObject = new CObject(transformation);

	if (!stl::push_back_unique(m_constructedObjects, pObject))
	{
		Cry::Audio::Log(ELogType::Warning, "Trying to construct an already registered object.");
	}

	return static_cast<IObject*>(pObject);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructObject(IObject const* const pIObject)
{
	auto const pObject = static_cast<CBaseObject const*>(pIObject);

	if (!stl::find_and_erase(m_constructedObjects, pObject))
	{
		Cry::Audio::Log(ELogType::Warning, "Trying to delete a non-existing object.");
	}

	delete pObject;
}

///////////////////////////////////////////////////////////////////////////
IListener* CImpl::ConstructListener(CObjectTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	IListener* pIListener = nullptr;

	static uint16 id = 0;
	CriAtomEx3dListenerHn const pHandle = criAtomEx3dListener_Create(&m_listenerConfig, nullptr, 0);

	if (pHandle != nullptr)
	{
		g_pListener = new CListener(transformation, id++, pHandle);

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
		if (szName != nullptr)
		{
			g_pListener->SetName(szName);
		}
#endif    // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

		pIListener = static_cast<IListener*>(g_pListener);
	}

	return pIListener;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructListener(IListener* const pIListener)
{
	CRY_ASSERT_MESSAGE(pIListener == g_pListener, "pIListener is not g_pListener");
	criAtomEx3dListener_Destroy(g_pListener->GetHandle());
	delete g_pListener;
	g_pListener = nullptr;
}

//////////////////////////////////////////////////////////////////////////
IEvent* CImpl::ConstructEvent(CATLEvent& event)
{
	return static_cast<IEvent*>(new CEvent(event));
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructEvent(IEvent const* const pIEvent)
{
	CRY_ASSERT(pIEvent != nullptr);
	delete pIEvent;
}

//////////////////////////////////////////////////////////////////////////
IStandaloneFile* CImpl::ConstructStandaloneFile(CATLStandaloneFile& standaloneFile, char const* const szFile, bool const bLocalized, ITrigger const* pITrigger /*= nullptr*/)
{
	return static_cast<IStandaloneFile*>(new CStandaloneFile);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructStandaloneFile(IStandaloneFile const* const pIStandaloneFile)
{
	CRY_ASSERT(pIStandaloneFile != nullptr);
	delete pIStandaloneFile;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GamepadConnected(DeviceId const deviceUniqueID)
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GamepadDisconnected(DeviceId const deviceUniqueID)
{
}

///////////////////////////////////////////////////////////////////////////
ITrigger const* CImpl::ConstructTrigger(XmlNodeRef const pRootNode, float& radius)
{
	ITrigger const* pITrigger = nullptr;

	if (_stricmp(pRootNode->getTag(), s_szCueTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		char const* const szCueSheetName = pRootNode->getAttr(s_szCueSheetAttribute);

		EEventType eventType = EEventType::Start;
		char const* const szEventType = pRootNode->getAttr(s_szTypeAttribute);

		if ((szEventType != nullptr) && (szEventType[0] != '\0'))
		{
			if (_stricmp(szEventType, s_szStopValue) == 0)
			{
				eventType = EEventType::Stop;
			}
			else if (_stricmp(szEventType, s_szPauseValue) == 0)
			{
				eventType = EEventType::Pause;
			}
			else if (_stricmp(szEventType, s_szResumeValue) == 0)
			{
				eventType = EEventType::Resume;
			}
		}

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
		if (eventType == EEventType::Start)
		{
			auto const iter = g_cueRadiusInfo.find(szName);

			if (iter != g_cueRadiusInfo.end())
			{
				for (auto const& pair : iter->second)
				{
					if (_stricmp(szCueSheetName, pair.first) == 0)
					{
						radius = pair.second;
						break;
					}
				}
			}
		}

		pITrigger = static_cast<ITrigger const*>(new CTrigger(StringToId(szName), szName, StringToId(szCueSheetName), ETriggerType::Trigger, eventType, szCueSheetName));
#else
		pITrigger = static_cast<ITrigger const*>(new CTrigger(StringToId(szName), szName, StringToId(szCueSheetName), ETriggerType::Trigger, eventType));
#endif    // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
	}
	else if (_stricmp(pRootNode->getTag(), s_szSnapshotTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		int changeoverTime = s_defaultChangeoverTime;
		pRootNode->getAttr(s_szTimeAttribute, changeoverTime);

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
		pITrigger = static_cast<ITrigger const*>(new CTrigger(StringToId(szName), szName, 0, ETriggerType::Snapshot, EEventType::Start, "", static_cast<CriSint32>(changeoverTime)));
#else
		pITrigger = static_cast<ITrigger const*>(new CTrigger(StringToId(szName), szName, 0, ETriggerType::Snapshot, EEventType::Start, static_cast<CriSint32>(changeoverTime)));
#endif    // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", pRootNode->getTag());
	}

	return pITrigger;
}

//////////////////////////////////////////////////////////////////////////
ITrigger const* CImpl::ConstructTrigger(ITriggerInfo const* const pITriggerInfo)
{
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	ITrigger const* pITrigger = nullptr;
	auto const pTriggerInfo = static_cast<STriggerInfo const*>(pITriggerInfo);

	if (pTriggerInfo != nullptr)
	{
		char const* const szName = pTriggerInfo->name.c_str();
		char const* const szCueSheetName = pTriggerInfo->cueSheet.c_str();

		pITrigger = static_cast<ITrigger const*>(new CTrigger(StringToId(szName), szName, StringToId(szCueSheetName), ETriggerType::Trigger, EEventType::Start, szCueSheetName));
	}

	return pITrigger;
#else
	return nullptr;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructTrigger(ITrigger const* const pITrigger)
{
	delete pITrigger;
}

///////////////////////////////////////////////////////////////////////////
IParameter const* CImpl::ConstructParameter(XmlNodeRef const pRootNode)
{
	IParameter const* pParameter = nullptr;

	char const* const szTag = pRootNode->getTag();
	EParameterType type = EParameterType::None;

	if (_stricmp(szTag, s_szAisacControlTag) == 0)
	{
		type = EParameterType::AisacControl;
	}
	else if (_stricmp(szTag, s_szSCategoryTag) == 0)
	{
		type = EParameterType::Category;
	}
	else if (_stricmp(szTag, s_szGameVariableTag) == 0)
	{
		type = EParameterType::GameVariable;
	}

	if (type != EParameterType::None)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		float multiplier = s_defaultParamMultiplier;
		float shift = s_defaultParamShift;
		pRootNode->getAttr(s_szMutiplierAttribute, multiplier);
		pRootNode->getAttr(s_szShiftAttribute, shift);

		pParameter = static_cast<IParameter const*>(new CParameter(szName, type, multiplier, shift));
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", szTag);
	}

	return pParameter;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructParameter(IParameter const* const pIParameter)
{
	delete pIParameter;
}

//////////////////////////////////////////////////////////////////////////
bool ParseSwitchOrState(XmlNodeRef const pNode, string& outSelectorName, string& outSelectorLabelName)
{
	bool foundAttributes = false;

	char const* const szSelectorName = pNode->getAttr(s_szNameAttribute);

	if ((szSelectorName != nullptr) && (szSelectorName[0] != 0) && (pNode->getChildCount() == 1))
	{
		XmlNodeRef const pLabelNode(pNode->getChild(0));

		if (pLabelNode && _stricmp(pLabelNode->getTag(), s_szSelectorLabelTag) == 0)
		{
			char const* const szSelctorLabelName = pLabelNode->getAttr(s_szNameAttribute);

			if ((szSelctorLabelName != nullptr) && (szSelctorLabelName[0] != 0))
			{
				outSelectorName = szSelectorName;
				outSelectorLabelName = szSelctorLabelName;
				foundAttributes = true;
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "An Adx2 Selector %s inside SwitchState needs to have exactly one Selector Label.", szSelectorName);
	}

	return foundAttributes;
}

///////////////////////////////////////////////////////////////////////////
ISwitchState const* CImpl::ConstructSwitchState(XmlNodeRef const pRootNode)
{
	ISwitchState const* pSwitchState = nullptr;

	char const* const szTag = pRootNode->getTag();
	ESwitchType type = ESwitchType::None;

	if (_stricmp(szTag, s_szSelectorTag) == 0)
	{
		type = ESwitchType::Selector;
	}
	else if (_stricmp(szTag, s_szAisacControlTag) == 0)
	{
		type = ESwitchType::AisacControl;
	}
	else if (_stricmp(szTag, s_szGameVariableTag) == 0)
	{
		type = ESwitchType::GameVariable;
	}
	else if (_stricmp(szTag, s_szSCategoryTag) == 0)
	{
		type = ESwitchType::Category;
	}

	if (type == ESwitchType::Selector)
	{
		string szSelectorName;
		string szSelectorLabelName;

		if (ParseSwitchOrState(pRootNode, szSelectorName, szSelectorLabelName))
		{
			pSwitchState = static_cast<ISwitchState const*>(new CSwitchState(type, szSelectorName, szSelectorLabelName));
		}
	}
	else if (type != ESwitchType::None)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		float value = (type == ESwitchType::Category) ? s_defaultCategoryVolume : s_defaultStateValue;

		if (pRootNode->getAttr(s_szValueAttribute, value))
		{
			pSwitchState = static_cast<ISwitchState const*>(new CSwitchState(type, szName, "", static_cast<CriFloat32>(value)));
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", szTag);
	}

	return pSwitchState;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructSwitchState(ISwitchState const* const pISwitchState)
{
	delete pISwitchState;
}

///////////////////////////////////////////////////////////////////////////
IEnvironment const* CImpl::ConstructEnvironment(XmlNodeRef const pRootNode)
{
	IEnvironment const* pEnvironment = nullptr;

	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, s_szBusTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);

		pEnvironment = static_cast<IEnvironment const*>(new CEnvironment(szName, EEnvironmentType::Bus));
	}
	else if (_stricmp(szTag, s_szAisacControlTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		float multiplier = s_defaultParamMultiplier;
		float shift = s_defaultParamShift;
		pRootNode->getAttr(s_szMutiplierAttribute, multiplier);
		pRootNode->getAttr(s_szShiftAttribute, shift);

		pEnvironment = static_cast<IEnvironment const*>(new CEnvironment(szName, EEnvironmentType::AisacControl, multiplier, shift));
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", szTag);
	}

	return pEnvironment;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructEnvironment(IEnvironment const* const pIEnvironment)
{
	delete pIEnvironment;
}

//////////////////////////////////////////////////////////////////////////
ISetting const* CImpl::ConstructSetting(XmlNodeRef const pRootNode)
{
	ISetting const* pISetting = nullptr;

	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, s_szDspBusSettingTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		pISetting = static_cast<ISetting const*>(new CSetting(szName));
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", szTag);
	}

	return pISetting;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructSetting(ISetting const* const pISetting)
{
	delete pISetting;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnRefresh()
{
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	g_debugCurrentDspBusSettingName = g_debugNoneDspBusSetting;
	m_acfFileSize = 0;
	g_cueRadiusInfo.clear();
	LoadAcbInfos(m_regularSoundBankFolder);
	LoadAcbInfos(m_localizedSoundBankFolder);
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

	criAtomEx_UnregisterAcf();
	RegisterAcf();
	g_acbHandles.clear();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetLanguage(char const* const szLanguage)
{
	if (szLanguage != nullptr)
	{
		m_language = szLanguage;
		m_localizedSoundBankFolder = PathUtil::GetLocalizationFolder().c_str();
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += m_language.c_str();
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += AUDIO_SYSTEM_DATA_ROOT;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += s_szImplFolderName;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += s_szAssetsFolderName;
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GetFileData(char const* const szName, SFileData& fileData) const
{
}

//////////////////////////////////////////////////////////////////////////
bool CImpl::InitializeLibrary()
{
#if defined(CRY_PLATFORM_WINDOWS)
	CriAtomExConfig_WASAPI libraryConfig;
	criAtomEx_SetDefaultConfig_WASAPI(&libraryConfig);
#else
	CriAtomExConfig libraryConfig;
	criAtomEx_SetDefaultConfig(&libraryConfig);
#endif  // CRY_PLATFORM_WINDOWS

	libraryConfig.atom_ex.max_virtual_voices = static_cast<CriSint32>(g_cvars.m_maxVirtualVoices);
	libraryConfig.atom_ex.max_voice_limit_groups = static_cast<CriSint32>(g_cvars.m_maxVoiceLimitGroups);
	libraryConfig.atom_ex.max_categories = static_cast<CriSint32>(g_cvars.m_maxCategories);
	libraryConfig.atom_ex.categories_per_playback = static_cast<CriSint32>(g_cvars.m_categoriesPerPlayback);
	libraryConfig.atom_ex.max_tracks = static_cast<CriUint32>(g_cvars.m_maxTracks);
	libraryConfig.atom_ex.max_track_items = static_cast<CriUint32>(g_cvars.m_maxTrackItems);
	libraryConfig.atom_ex.max_faders = static_cast<CriUint32>(g_cvars.m_maxFaders);
	libraryConfig.atom_ex.max_pitch = static_cast<CriFloat32>(g_cvars.m_maxPitch);
	libraryConfig.asr.num_buses = static_cast<CriSint32>(g_cvars.m_numBuses);
	libraryConfig.asr.output_channels = static_cast<CriSint32>(g_cvars.m_outputChannels);
	libraryConfig.asr.output_sampling_rate = static_cast<CriSint32>(g_cvars.m_outputSamplingRate);

#if defined(CRY_PLATFORM_WINDOWS)
	criAtomEx_Initialize_WASAPI(&libraryConfig, nullptr, 0);
#else
	criAtomEx_Initialize(&libraryConfig, nullptr, 0);
#endif  // CRY_PLATFORM_WINDOWS

	bool const isInitialized = (criAtomEx_IsInitialized() == CRI_TRUE);

	if (!isInitialized)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to initialize CriAtomEx");
	}

	return isInitialized;
}

//////////////////////////////////////////////////////////////////////////
bool CImpl::AllocateVoicePool()
{
	CriAtomExStandardVoicePoolConfig voicePoolConfig;
	criAtomExVoicePool_SetDefaultConfigForStandardVoicePool(&voicePoolConfig);
	voicePoolConfig.num_voices = static_cast<CriSint32>(g_cvars.m_numVoices);
	voicePoolConfig.player_config.max_channels = static_cast<CriSint32>(g_cvars.m_maxChannels);
	voicePoolConfig.player_config.max_sampling_rate = static_cast<CriSint32>(g_cvars.m_maxSamplingRate);
	voicePoolConfig.player_config.streaming_flag = CRI_TRUE;

	bool const isAllocated = (criAtomExVoicePool_AllocateStandardVoicePool(&voicePoolConfig, nullptr, 0) != nullptr);

	if (!isAllocated)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to allocate voice pool");
	}

	return isAllocated;
}

//////////////////////////////////////////////////////////////////////////
bool CImpl::CreateDbas()
{
	CriAtomDbasConfig dbasConfig;
	criAtomDbas_SetDefaultConfig(&dbasConfig);
	dbasConfig.max_streams = static_cast<CriSint32>(g_cvars.m_maxStreams);
	m_dbasId = criAtomExDbas_Create(&dbasConfig, nullptr, 0);

	bool const isCreated = (m_dbasId != CRIATOMEXDBAS_ILLEGAL_ID);

	if (!isCreated)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to create D-BAS");
	}

	return isCreated;
}

//////////////////////////////////////////////////////////////////////////
bool CImpl::RegisterAcf()
{
	bool acfRegistered = false;
	bool acfExists = false;

	CryFixedStringT<MaxFilePathLength> acfPath;
	CryFixedStringT<MaxFilePathLength> search(m_regularSoundBankFolder + "/*.acf");

	_finddata_t fd;
	intptr_t const handle = gEnv->pCryPak->FindFirst(search.c_str(), &fd);

	if (handle != -1)
	{
		do
		{
			char const* const fileName = fd.name;

			if (_stricmp(PathUtil::GetExt(fileName), "acf") == 0)
			{
				acfPath = m_regularSoundBankFolder + "/" + fileName;
				acfExists = true;
				break;
			}
		}
		while (gEnv->pCryPak->FindNext(handle, &fd) >= 0);

		gEnv->pCryPak->FindClose(handle);
	}

	if (acfExists)
	{
		FILE* const pAcfFile = gEnv->pCryPak->FOpen(acfPath.c_str(), "rbx");
		size_t const acfFileSize = gEnv->pCryPak->FGetSize(acfPath.c_str());
		m_pAcfBuffer = new uint8[acfFileSize];
		gEnv->pCryPak->FRead(m_pAcfBuffer, acfFileSize, pAcfFile);
		gEnv->pCryPak->FClose(pAcfFile);

		auto const pAcfData = static_cast<void*>(m_pAcfBuffer);

		criAtomEx_RegisterAcfData(pAcfData, static_cast<CriSint32>(acfFileSize), nullptr, 0);
		CriAtomExAcfInfo acfInfo;

		if (criAtomExAcf_GetAcfInfo(&acfInfo) == CRI_TRUE)
		{
			acfRegistered = true;

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
			m_acfFileSize = acfFileSize;
#endif      // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
		}
		else
		{
			Cry::Audio::Log(ELogType::Error, "Failed to register ACF");
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "ACF not found.");
	}

	return acfRegistered;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::InitializeFileSystem()
{
	criFs_SetUserMallocFunction(userMalloc, nullptr);
	criFs_SetUserFreeFunction(userFree, nullptr);

	CriFsConfig fileSystemConfig;
	criFs_SetDefaultConfig(&fileSystemConfig);
	fileSystemConfig.max_files = static_cast<CriSint32>(g_cvars.m_maxFiles);

	criFs_InitializeLibrary(&fileSystemConfig, nullptr, 0);
	criFs_SetSelectIoCallback(selectIoFunc);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetListenerConfig()
{
	criAtomEx3dListener_SetDefaultConfig(&m_listenerConfig);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetPlayerConfig()
{
	criAtomExPlayer_SetDefaultConfig(&g_playerConfig);
	g_playerConfig.voice_allocation_method = static_cast<CriAtomExVoiceAllocationMethod>(g_cvars.m_voiceAllocationMethod);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::Set3dSourceConfig()
{
	g_3dSourceConfig.enable_voice_priority_decay = CRI_TRUE;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::MuteAllObjects(CriBool const shouldMute)
{
	for (auto const pObject : m_constructedObjects)
	{
		pObject->MutePlayer(shouldMute);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::PauseAllObjects(CriBool const shouldPause)
{
	for (auto const pObject : m_constructedObjects)
	{
		pObject->PausePlayer(shouldPause);
	}
}

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
///////////////////////////////////////////////////////////////////////////
void GetMemoryInfo(SMemoryInfo& memoryInfo)
{
	CryModuleMemoryInfo memInfo;
	ZeroStruct(memInfo);
	CryGetMemoryInfoForModule(&memInfo);

	memoryInfo.totalMemory = static_cast<size_t>(memInfo.allocated - memInfo.freed);

	{
		auto& allocator = CObject::GetAllocator();
		auto mem = allocator.GetTotalMemory();
		auto pool = allocator.GetCounts();
		memoryInfo.poolUsedObjects = pool.nUsed;
		memoryInfo.poolConstructedObjects = pool.nAlloc;
		memoryInfo.poolUsedMemory = mem.nUsed;
		memoryInfo.poolAllocatedMemory = mem.nAlloc;
	}

	{
		auto& allocator = CEvent::GetAllocator();
		auto mem = allocator.GetTotalMemory();
		auto pool = allocator.GetCounts();
		memoryInfo.poolUsedObjects += pool.nUsed;
		memoryInfo.poolConstructedObjects += pool.nAlloc;
		memoryInfo.poolUsedMemory += mem.nUsed;
		memoryInfo.poolAllocatedMemory += mem.nAlloc;
	}
}
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float& posY)
{
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	auxGeom.Draw2dLabel(posX, posY, g_debugSystemHeaderFontSize, g_debugSystemColorHeader.data(), false, m_name.c_str());

	SMemoryInfo memoryInfo;
	GetMemoryInfo(memoryInfo);
	memoryInfo.totalMemory += m_acfFileSize,

	posY += g_debugSystemLineHeightClause;
	auxGeom.Draw2dLabel(posX, posY, g_debugSystemFontSize, g_debugSystemColorTextPrimary.data(), false, "Total Memory Used: %uKiB | ACF: %uKiB",
	                    static_cast<uint32>(memoryInfo.totalMemory / 1024),
	                    static_cast<uint32>(m_acfFileSize / 1024));

	posY += g_debugSystemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, g_debugSystemFontSize, g_debugSystemColorTextPrimary.data(), false, "[Object Pool] In Use: %u | Constructed: %u (%uKiB) | Memory Pool: %uKiB",
	                    memoryInfo.poolUsedObjects, memoryInfo.poolConstructedObjects, memoryInfo.poolUsedMemory, memoryInfo.poolAllocatedMemory);

	posY += g_debugSystemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, g_debugSystemFontSize, g_debugSystemColorTextPrimary.data(), false, "DSP Bus Setting: %s", g_debugCurrentDspBusSettingName.c_str());

	if (g_numObjectsWithDoppler > 0)
	{
		posY += g_debugSystemLineHeight;
		auxGeom.Draw2dLabel(posX, posY, g_debugSystemFontSize, g_debugSystemColorTextPrimary.data(), false, "Objects with Doppler: %u", g_numObjectsWithDoppler);
	}

	Vec3 const& listenerPosition = g_pListener->GetPosition();
	Vec3 const& listenerDirection = g_pListener->GetTransformation().GetForward();
	float const listenerVelocity = g_pListener->GetVelocity().GetLength();
	char const* const szName = g_pListener->GetName();

	posY += g_debugSystemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, g_debugSystemFontSize, g_debugSystemColorListenerActive.data(), false, "Listener: %s | PosXYZ: %.2f %.2f %.2f | FwdXYZ: %.2f %.2f %.2f | Velocity: %.2f m/s",
	                    szName, listenerPosition.x, listenerPosition.y, listenerPosition.z, listenerDirection.x, listenerDirection.y, listenerDirection.z, listenerVelocity);
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
