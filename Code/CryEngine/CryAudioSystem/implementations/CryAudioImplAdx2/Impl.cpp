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
#include "IoInterface.h"

#include <FileInfo.h>
#include <Logger.h>
#include <CrySystem/IStreamEngine.h>
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
std::vector<CBaseObject*> g_constructedObjects;
SPoolSizes g_poolSizes;
SPoolSizes g_poolSizesLevelSpecific;

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
SPoolSizes g_debugPoolSizes;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CountPoolSizes(XmlNodeRef const pNode, SPoolSizes& poolSizes)
{
	uint16 numTriggers = 0;
	pNode->getAttr(s_szTriggersAttribute, numTriggers);
	poolSizes.triggers += numTriggers;

	uint16 numParameters = 0;
	pNode->getAttr(s_szParametersAttribute, numParameters);
	poolSizes.parameters += numParameters;

	uint16 numSwitchStates = 0;
	pNode->getAttr(s_szSwitchStatesAttribute, numSwitchStates);
	poolSizes.switchStates += numSwitchStates;

	uint16 numEnvironments = 0;
	pNode->getAttr(s_szEnvironmentsAttribute, numEnvironments);
	poolSizes.environments += numEnvironments;

	uint16 numSettings = 0;
	pNode->getAttr(s_szSettingsAttribute, numSettings);
	poolSizes.settings += numSettings;

	uint16 numFiles = 0;
	pNode->getAttr(s_szFilesAttribute, numFiles);
	poolSizes.files += numFiles;
}

//////////////////////////////////////////////////////////////////////////
void AllocateMemoryPools(uint16 const objectPoolSize, uint16 const eventPoolSize)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 Object Pool");
	CObject::CreateAllocator(objectPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 Event Pool");
	CEvent::CreateAllocator(eventPoolSize);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 Trigger Pool");
	CTrigger::CreateAllocator(g_poolSizes.triggers);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 Parameter Pool");
	CParameter::CreateAllocator(g_poolSizes.parameters);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 Switch State Pool");
	CSwitchState::CreateAllocator(g_poolSizes.switchStates);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 Environment Pool");
	CEnvironment::CreateAllocator(g_poolSizes.environments);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 Setting Pool");
	CSetting::CreateAllocator(g_poolSizes.settings);

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "Adx2 File Pool");
	CFile::CreateAllocator(g_poolSizes.files);
}

//////////////////////////////////////////////////////////////////////////
void FreeMemoryPools()
{
	CObject::FreeMemoryPool();
	CEvent::FreeMemoryPool();
	CTrigger::FreeMemoryPool();
	CParameter::FreeMemoryPool();
	CSwitchState::FreeMemoryPool();
	CEnvironment::FreeMemoryPool();
	CSetting::FreeMemoryPool();
	CFile::FreeMemoryPool();
}

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
	CRY_ASSERT_MESSAGE(szPath != nullptr, "szPath is null pointer during %s", __FUNCTION__);
	CRY_ASSERT_MESSAGE(pDeviceId != nullptr, "pDeviceId is null pointer during %s", __FUNCTION__);
	CRY_ASSERT_MESSAGE(pIoInterface != nullptr, "pIoInterface is null pointer during %s", __FUNCTION__);

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
	: m_pAcfBuffer(nullptr)
	, m_dbasId(CRIATOMEXDBAS_ILLEGAL_ID)
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	, m_name("Adx2 (" CRI_ATOM_VER_NUM ")")
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
{
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::Init(uint16 const objectPoolSize, uint16 const eventPoolSize)
{
	ERequestStatus result = ERequestStatus::Success;

	g_constructedObjects.reserve(static_cast<size_t>(objectPoolSize));
	AllocateMemoryPools(objectPoolSize, eventPoolSize);

	m_regularSoundBankFolder = AUDIO_SYSTEM_DATA_ROOT;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += s_szImplFolderName;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += s_szAssetsFolderName;
	m_localizedSoundBankFolder = m_regularSoundBankFolder;

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
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

	FreeMemoryPools();
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetLibraryData(XmlNodeRef const pNode, bool const isLevelSpecific)
{
	if (isLevelSpecific)
	{
		SPoolSizes levelPoolSizes;
		CountPoolSizes(pNode, levelPoolSizes);

		g_poolSizesLevelSpecific.triggers = std::max(g_poolSizesLevelSpecific.triggers, levelPoolSizes.triggers);
		g_poolSizesLevelSpecific.parameters = std::max(g_poolSizesLevelSpecific.parameters, levelPoolSizes.parameters);
		g_poolSizesLevelSpecific.switchStates = std::max(g_poolSizesLevelSpecific.switchStates, levelPoolSizes.switchStates);
		g_poolSizesLevelSpecific.environments = std::max(g_poolSizesLevelSpecific.environments, levelPoolSizes.environments);
		g_poolSizesLevelSpecific.settings = std::max(g_poolSizesLevelSpecific.settings, levelPoolSizes.settings);
		g_poolSizesLevelSpecific.files = std::max(g_poolSizesLevelSpecific.files, levelPoolSizes.files);
	}
	else
	{
		CountPoolSizes(pNode, g_poolSizes);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnBeforeLibraryDataChanged()
{
	ZeroStruct(g_poolSizes);
	ZeroStruct(g_poolSizesLevelSpecific);

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	ZeroStruct(g_debugPoolSizes);
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnAfterLibraryDataChanged()
{
	g_poolSizes.triggers += g_poolSizesLevelSpecific.triggers;
	g_poolSizes.parameters += g_poolSizesLevelSpecific.parameters;
	g_poolSizes.switchStates += g_poolSizesLevelSpecific.switchStates;
	g_poolSizes.environments += g_poolSizesLevelSpecific.environments;
	g_poolSizes.settings += g_poolSizesLevelSpecific.settings;
	g_poolSizes.files += g_poolSizesLevelSpecific.files;

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	// Used to hide pools without allocations in debug draw.
	g_debugPoolSizes = g_poolSizes;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

	g_poolSizes.triggers = std::max<uint16>(1, g_poolSizes.triggers);
	g_poolSizes.parameters = std::max<uint16>(1, g_poolSizes.parameters);
	g_poolSizes.switchStates = std::max<uint16>(1, g_poolSizes.switchStates);
	g_poolSizes.environments = std::max<uint16>(1, g_poolSizes.environments);
	g_poolSizes.settings = std::max<uint16>(1, g_poolSizes.settings);
	g_poolSizes.files = std::max<uint16>(1, g_poolSizes.files);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::OnLoseFocus()
{
	MuteAllObjects(CRI_TRUE);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::OnGetFocus()
{
	MuteAllObjects(CRI_FALSE);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::MuteAll()
{
	MuteAllObjects(CRI_TRUE);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::UnmuteAll()
{
	MuteAllObjects(CRI_FALSE);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::PauseAll()
{
	PauseAllObjects(CRI_TRUE);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::ResumeAll()
{
	PauseAllObjects(CRI_FALSE);
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::StopAllSounds()
{
	criAtomExPlayer_StopAllPlayers();
	return ERequestStatus::Success;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetGlobalParameter(IParameterConnection const* const pIParameterConnection, float const value)
{
	auto const pParameter = static_cast<CParameter const*>(pIParameterConnection);

	if (pParameter != nullptr)
	{
		EParameterType const type = pParameter->GetType();

		switch (type)
		{
		case EParameterType::AisacControl:
			{
				for (auto const pObject : g_constructedObjects)
				{

					pObject->SetParameter(pParameter, value);
				}

				break;
			}
		case EParameterType::Category:
			{
				criAtomExCategory_SetVolumeByName(pParameter->GetName(), static_cast<CriFloat32>(pParameter->GetMultiplier() * value + pParameter->GetValueShift()));

				break;
			}
		case EParameterType::GameVariable:
			{
				criAtomEx_SetGameVariableByName(pParameter->GetName(), static_cast<CriFloat32>(pParameter->GetMultiplier() * value + pParameter->GetValueShift()));

				break;
			}
		default:
			{
				Cry::Audio::Log(ELogType::Warning, "Adx2 - Unknown EParameterType: %" PRISIZE_T, type);

				break;
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Adx2 - Invalid Parameter pointer passed to the Adx2 implementation of %s", __FUNCTION__);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::SetGlobalSwitchState(ISwitchStateConnection const* const pISwitchStateConnection)
{
	auto const pSwitchState = static_cast<CSwitchState const*>(pISwitchStateConnection);

	if (pSwitchState != nullptr)
	{
		ESwitchType const type = pSwitchState->GetType();

		switch (type)
		{
		case ESwitchType::Selector:
		case ESwitchType::AisacControl:
			{
				for (auto const pObject : g_constructedObjects)
				{
					pObject->SetSwitchState(pISwitchStateConnection);
				}

				break;
			}
		case ESwitchType::Category:
			{
				criAtomExCategory_SetVolumeByName(pSwitchState->GetName(), pSwitchState->GetValue());

				break;
			}
		case ESwitchType::GameVariable:
			{
				criAtomEx_SetGameVariableByName(pSwitchState->GetName(), pSwitchState->GetValue());

				break;
			}
		default:
			{
				Cry::Audio::Log(ELogType::Warning, "Adx2 - Unknown ESwitchType: %" PRISIZE_T, type);

				break;
			}
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Error, "Invalid switch pointer passed to the Adx2 implementation of %s", __FUNCTION__);
	}
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
	new CGlobalObject;

	if (!stl::push_back_unique(g_constructedObjects, static_cast<CBaseObject*>(g_pObject)))
	{
		Cry::Audio::Log(ELogType::Warning, "Trying to construct an already registered object.");
	}

	return static_cast<IObject*>(g_pObject);
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructObject(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	auto const pObject = new CObject(transformation);

	if (!stl::push_back_unique(g_constructedObjects, pObject))
	{
		Cry::Audio::Log(ELogType::Warning, "Trying to construct an already registered object.");
	}

	return static_cast<IObject*>(pObject);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructObject(IObject const* const pIObject)
{
	auto const pObject = static_cast<CBaseObject const*>(pIObject);

	if (!stl::find_and_erase(g_constructedObjects, pObject))
	{
		Cry::Audio::Log(ELogType::Warning, "Trying to delete a non-existing object.");
	}

	delete pObject;
}

///////////////////////////////////////////////////////////////////////////
IListener* CImpl::ConstructListener(CTransformation const& transformation, char const* const szName /*= nullptr*/)
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
	CRY_ASSERT_MESSAGE(pIListener == g_pListener, "pIListener is not g_pListener during %s", __FUNCTION__);
	criAtomEx3dListener_Destroy(g_pListener->GetHandle());
	delete g_pListener;
	g_pListener = nullptr;
}

//////////////////////////////////////////////////////////////////////////
IEvent* CImpl::ConstructEvent(CryAudio::CEvent& event)
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
IStandaloneFileConnection* CImpl::ConstructStandaloneFileConnection(CryAudio::CStandaloneFile& standaloneFile, char const* const szFile, bool const bLocalized, ITriggerConnection const* pITriggerConnection /*= nullptr*/)
{
	return static_cast<IStandaloneFileConnection*>(new CStandaloneFile);
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructStandaloneFileConnection(IStandaloneFileConnection const* const pIStandaloneFileConnection)
{
	CRY_ASSERT(pIStandaloneFileConnection != nullptr);
	delete pIStandaloneFileConnection;
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
ITriggerConnection const* CImpl::ConstructTriggerConnection(XmlNodeRef const pRootNode, float& radius)
{
	ITriggerConnection const* pITriggerConnection = nullptr;

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

		pITriggerConnection = static_cast<ITriggerConnection const*>(new CTrigger(StringToId(szName), szName, StringToId(szCueSheetName), ETriggerType::Trigger, eventType, szCueSheetName));
#else
		pITriggerConnection = static_cast<ITriggerConnection const*>(new CTrigger(StringToId(szName), szName, StringToId(szCueSheetName), ETriggerType::Trigger, eventType));
#endif    // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
	}
	else if (_stricmp(pRootNode->getTag(), s_szSnapshotTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		int changeoverTime = s_defaultChangeoverTime;
		pRootNode->getAttr(s_szTimeAttribute, changeoverTime);

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
		pITriggerConnection = static_cast<ITriggerConnection const*>(new CTrigger(StringToId(szName), szName, 0, ETriggerType::Snapshot, EEventType::Start, "", static_cast<CriSint32>(changeoverTime)));
#else
		pITriggerConnection = static_cast<ITriggerConnection const*>(new CTrigger(StringToId(szName), szName, 0, ETriggerType::Snapshot, EEventType::Start, static_cast<CriSint32>(changeoverTime)));
#endif    // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", pRootNode->getTag());
	}

	return pITriggerConnection;
}

//////////////////////////////////////////////////////////////////////////
ITriggerConnection const* CImpl::ConstructTriggerConnection(ITriggerInfo const* const pITriggerInfo)
{
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	ITriggerConnection const* pITriggerConnection = nullptr;
	auto const pTriggerInfo = static_cast<STriggerInfo const*>(pITriggerInfo);

	if (pTriggerInfo != nullptr)
	{
		char const* const szName = pTriggerInfo->name.c_str();
		char const* const szCueSheetName = pTriggerInfo->cueSheet.c_str();

		pITriggerConnection = static_cast<ITriggerConnection const*>(new CTrigger(StringToId(szName), szName, StringToId(szCueSheetName), ETriggerType::Trigger, EEventType::Start, szCueSheetName));
	}

	return pITriggerConnection;
#else
	return nullptr;
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructTriggerConnection(ITriggerConnection const* const pITriggerConnection)
{
	delete pITriggerConnection;
}

///////////////////////////////////////////////////////////////////////////
IParameterConnection const* CImpl::ConstructParameterConnection(XmlNodeRef const pRootNode)
{
	IParameterConnection const* pIParameterConnection = nullptr;

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

		pIParameterConnection = static_cast<IParameterConnection const*>(new CParameter(szName, type, multiplier, shift));
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", szTag);
	}

	return pIParameterConnection;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructParameterConnection(IParameterConnection const* const pIParameterConnection)
{
	delete pIParameterConnection;
}

//////////////////////////////////////////////////////////////////////////
bool ParseSwitchOrState(XmlNodeRef const pNode, string& selectorName, string& selectorLabelName)
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
				selectorName = szSelectorName;
				selectorLabelName = szSelctorLabelName;
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
ISwitchStateConnection const* CImpl::ConstructSwitchStateConnection(XmlNodeRef const pRootNode)
{
	ISwitchStateConnection const* pISwitchStateConnection = nullptr;

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
			pISwitchStateConnection = static_cast<ISwitchStateConnection const*>(new CSwitchState(type, szSelectorName, szSelectorLabelName));
		}
	}
	else if (type != ESwitchType::None)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		float value = (type == ESwitchType::Category) ? s_defaultCategoryVolume : s_defaultStateValue;

		if (pRootNode->getAttr(s_szValueAttribute, value))
		{
			pISwitchStateConnection = static_cast<ISwitchStateConnection const*>(new CSwitchState(type, szName, "", static_cast<CriFloat32>(value)));
		}
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", szTag);
	}

	return pISwitchStateConnection;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructSwitchStateConnection(ISwitchStateConnection const* const pISwitchStateConnection)
{
	delete pISwitchStateConnection;
}

///////////////////////////////////////////////////////////////////////////
IEnvironmentConnection const* CImpl::ConstructEnvironmentConnection(XmlNodeRef const pRootNode)
{
	IEnvironmentConnection const* pEnvironmentConnection = nullptr;

	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, s_szBusTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);

		pEnvironmentConnection = static_cast<IEnvironmentConnection const*>(new CEnvironment(szName, EEnvironmentType::Bus));
	}
	else if (_stricmp(szTag, s_szAisacControlTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		float multiplier = s_defaultParamMultiplier;
		float shift = s_defaultParamShift;
		pRootNode->getAttr(s_szMutiplierAttribute, multiplier);
		pRootNode->getAttr(s_szShiftAttribute, shift);

		pEnvironmentConnection = static_cast<IEnvironmentConnection const*>(new CEnvironment(szName, EEnvironmentType::AisacControl, multiplier, shift));
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", szTag);
	}

	return pEnvironmentConnection;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructEnvironmentConnection(IEnvironmentConnection const* const pIEnvironmentConnection)
{
	delete pIEnvironmentConnection;
}

//////////////////////////////////////////////////////////////////////////
ISettingConnection const* CImpl::ConstructSettingConnection(XmlNodeRef const pRootNode)
{
	ISettingConnection const* pISettingConnection = nullptr;

	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, s_szDspBusSettingTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(s_szNameAttribute);
		pISettingConnection = static_cast<ISettingConnection const*>(new CSetting(szName));
	}
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", szTag);
	}

	return pISettingConnection;
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructSettingConnection(ISettingConnection const* const pISettingConnection)
{
	delete pISettingConnection;
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
	for (auto const pObject : g_constructedObjects)
	{
		pObject->MutePlayer(shouldMute);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::PauseAllObjects(CriBool const shouldPause)
{
	for (auto const pObject : g_constructedObjects)
	{
		pObject->PausePlayer(shouldPause);
	}
}

#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
void DrawMemoryPoolInfo(
	IRenderAuxGeom& auxGeom,
	float const posX,
	float& posY,
	stl::SPoolMemoryUsage const& mem,
	stl::SMemoryUsage const& pool,
	char const* const szType)
{
	CryFixedStringT<MaxMiscStringLength> memUsedString;

	if (mem.nUsed < 1024)
	{
		memUsedString.Format("%" PRISIZE_T " Byte", mem.nUsed);
	}
	else
	{
		memUsedString.Format("%" PRISIZE_T " KiB", mem.nUsed >> 10);
	}

	CryFixedStringT<MaxMiscStringLength> memAllocString;

	if (mem.nAlloc < 1024)
	{
		memAllocString.Format("%" PRISIZE_T " Byte", mem.nAlloc);
	}
	else
	{
		memAllocString.Format("%" PRISIZE_T " KiB", mem.nAlloc >> 10);
	}

	posY += g_debugSystemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, g_debugSystemFontSize, g_debugSystemColorTextPrimary.data(), false,
	                    "[%s] In Use: %" PRISIZE_T " | Constructed: %" PRISIZE_T " (%s) | Memory Pool: %s",
	                    szType, pool.nUsed, pool.nAlloc, memUsedString.c_str(), memAllocString.c_str());
}
#endif  // INCLUDE_ADX2_IMPL_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugInfo(IRenderAuxGeom& auxGeom, float const posX, float& posY, bool const showDetailedInfo)
{
#if defined(INCLUDE_ADX2_IMPL_PRODUCTION_CODE)
	CryModuleMemoryInfo memInfo;
	ZeroStruct(memInfo);
	CryGetMemoryInfoForModule(&memInfo);

	CryFixedStringT<MaxMiscStringLength> memInfoString;
	auto const memAlloc = static_cast<uint32>(memInfo.allocated - memInfo.freed);

	if (memAlloc < 1024)
	{
		memInfoString.Format("%s (Total Memory: %u Byte)", m_name.c_str(), memAlloc);
	}
	else
	{
		memInfoString.Format("%s (Total Memory: %u KiB)", m_name.c_str(), memAlloc >> 10);
	}

	auxGeom.Draw2dLabel(posX, posY, g_debugSystemHeaderFontSize, g_debugSystemColorHeader.data(), false, memInfoString.c_str());

	if (showDetailedInfo)
	{
		{
			auto& allocator = CObject::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Objects");
		}

		{
			auto& allocator = CEvent::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Events");
		}

		if (g_debugPoolSizes.triggers > 0)
		{
			auto& allocator = CTrigger::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Triggers");
		}

		if (g_debugPoolSizes.parameters > 0)
		{
			auto& allocator = CParameter::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Parameters");
		}

		if (g_debugPoolSizes.switchStates > 0)
		{
			auto& allocator = CSwitchState::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "SwitchStates");
		}

		if (g_debugPoolSizes.environments > 0)
		{
			auto& allocator = CEnvironment::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Environments");
		}

		if (g_debugPoolSizes.settings > 0)
		{
			auto& allocator = CSetting::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Settings");
		}

		if (g_debugPoolSizes.files > 0)
		{
			auto& allocator = CFile::GetAllocator();
			DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Files");
		}
	}

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
