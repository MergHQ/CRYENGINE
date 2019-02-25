// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#include "stdafx.h"
#include "Impl.h"

#include "CVars.h"
#include "Listener.h"
#include "Object.h"
#include "GlobalObject.h"
#include "AisacControl.h"
#include "AisacEnvironment.h"
#include "AisacState.h"
#include "Binary.h"
#include "Category.h"
#include "CategoryState.h"
#include "Cue.h"
#include "CueInstance.h"
#include "DspBus.h"
#include "GameVariable.h"
#include "GameVariableState.h"
#include "SelectorLabel.h"
#include "Snapshot.h"
#include "DspBusSetting.h"
#include "IoInterface.h"

#include <FileInfo.h>
#include <CrySystem/IStreamEngine.h>
#include <CrySystem/File/ICryPak.h>
#include <CrySystem/IProjectManager.h>
#include <CryAudio/IAudioSystem.h>

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	#include <Logger.h>
	#include <DebugStyle.h>
	#include <CryRenderer/IRenderAuxGeom.h>
	#include <CrySystem/ITimer.h>
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

#if CRY_PLATFORM_WINDOWS
	#include <cri_atom_wasapi.h>
#endif // CRY_PLATFORM_WINDOWS

namespace CryAudio
{
namespace Impl
{
namespace Adx2
{
SPoolSizes g_poolSizes;
SPoolSizes g_poolSizesLevelSpecific;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
SPoolSizes g_debugPoolSizes;
CueInstances g_constructedCueInstances;
uint16 g_objectPoolSize = 0;
uint16 g_cueInstancePoolSize = 0;
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CountPoolSizes(XmlNodeRef const pNode, SPoolSizes& poolSizes)
{
	uint16 numCues = 0;
	pNode->getAttr(g_szCuesAttribute, numCues);
	poolSizes.cues += numCues;

	uint16 numAisacControls = 0;
	pNode->getAttr(g_szAisacControlsAttribute, numAisacControls);
	poolSizes.aisacControls += numAisacControls;

	uint16 numAisacEnvironments = 0;
	pNode->getAttr(g_szAisacEnvironmentsAttribute, numAisacEnvironments);
	poolSizes.aisacEnvironments += numAisacEnvironments;

	uint16 numAisacStates = 0;
	pNode->getAttr(g_szAisacStatesAttribute, numAisacStates);
	poolSizes.aisacStates += numAisacStates;

	uint16 numCategories = 0;
	pNode->getAttr(g_szCategoriesAttribute, numCategories);
	poolSizes.categories += numCategories;

	uint16 numCategoryStates = 0;
	pNode->getAttr(g_szCategoryStatesAttribute, numCategoryStates);
	poolSizes.categoryStates += numCategoryStates;

	uint16 numGameVariables = 0;
	pNode->getAttr(g_szGameVariablesAttribute, numGameVariables);
	poolSizes.gameVariables += numGameVariables;

	uint16 numGameVariableStates = 0;
	pNode->getAttr(g_szGameVariableStatesAttribute, numGameVariableStates);
	poolSizes.gameVariableStates += numGameVariableStates;

	uint16 numSelectorLabels = 0;
	pNode->getAttr(g_szSelectorLabelsAttribute, numSelectorLabels);
	poolSizes.selectorLabels += numSelectorLabels;

	uint16 numBuses = 0;
	pNode->getAttr(g_szDspBusesAttribute, numBuses);
	poolSizes.dspBuses += numBuses;

	uint16 numSnapshots = 0;
	pNode->getAttr(g_szSnapshotsAttribute, numSnapshots);
	poolSizes.snapshots += numSnapshots;

	uint16 numDspBusSettings = 0;
	pNode->getAttr(g_szDspBusSettingsAttribute, numDspBusSettings);
	poolSizes.dspBusSettings += numDspBusSettings;

	uint16 numFiles = 0;
	pNode->getAttr(g_szBinariesAttribute, numFiles);
	poolSizes.binaries += numFiles;
}

//////////////////////////////////////////////////////////////////////////
void AllocateMemoryPools(uint16 const objectPoolSize, uint16 const cueInstancePoolSize)
{
	CObject::CreateAllocator(objectPoolSize);
	CCueInstance::CreateAllocator(cueInstancePoolSize);
	CCue::CreateAllocator(g_poolSizes.cues);
	CAisacControl::CreateAllocator(g_poolSizes.aisacControls);
	CAisacEnvironment::CreateAllocator(g_poolSizes.aisacEnvironments);
	CAisacState::CreateAllocator(g_poolSizes.aisacStates);
	CCategory::CreateAllocator(g_poolSizes.categories);
	CCategoryState::CreateAllocator(g_poolSizes.categoryStates);
	CGameVariable::CreateAllocator(g_poolSizes.gameVariables);
	CGameVariableState::CreateAllocator(g_poolSizes.gameVariableStates);
	CSelectorLabel::CreateAllocator(g_poolSizes.selectorLabels);
	CDspBus::CreateAllocator(g_poolSizes.dspBuses);
	CSnapshot::CreateAllocator(g_poolSizes.snapshots);
	CDspBusSetting::CreateAllocator(g_poolSizes.dspBusSettings);
	CBinary::CreateAllocator(g_poolSizes.binaries);
}

//////////////////////////////////////////////////////////////////////////
void FreeMemoryPools()
{
	CObject::FreeMemoryPool();
	CCueInstance::FreeMemoryPool();
	CCue::FreeMemoryPool();
	CAisacControl::FreeMemoryPool();
	CAisacEnvironment::FreeMemoryPool();
	CAisacState::FreeMemoryPool();
	CCategory::FreeMemoryPool();
	CCategoryState::FreeMemoryPool();
	CGameVariable::FreeMemoryPool();
	CGameVariableState::FreeMemoryPool();
	CSelectorLabel::FreeMemoryPool();
	CDspBus::FreeMemoryPool();
	CSnapshot::FreeMemoryPool();
	CDspBusSetting::FreeMemoryPool();
	CBinary::FreeMemoryPool();
}

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
using CueInfo = std::map<uint32, std::vector<std::pair<uint32, float>>>;
CueInfo g_cueRadiusInfo;
CueInfo g_cueFadeOutTimes;

//////////////////////////////////////////////////////////////////////////
void ParseAcbInfoFile(XmlNodeRef const pRoot, uint32 const acbId)
{
	int const numCueNodes = pRoot->getChildCount();

	for (int i = 0; i < numCueNodes; ++i)
	{
		XmlNodeRef const pCueNode = pRoot->getChild(i);

		if (pCueNode != nullptr)
		{
			char const* const typeAttrib = pCueNode->getAttr("OrcaType");

			if (_stricmp(typeAttrib, "CriMw.CriAtomCraft.AcCore.AcOoCueSynthCue") == 0)
			{
				uint32 const cueId = StringToId(pCueNode->getAttr("OrcaName"));

				if (pCueNode->haveAttr("Pos3dDistanceMax"))
				{
					float distanceMax = 0.0f;
					pCueNode->getAttr("Pos3dDistanceMax", distanceMax);

					g_cueRadiusInfo[cueId].emplace_back(acbId, distanceMax);
				}

				int const numTrackNodes = pCueNode->getChildCount();
				float cueFadeOutTime = 0.0f;

				for (int j = 0; j < numTrackNodes; ++j)
				{
					XmlNodeRef const pTrackNode = pCueNode->getChild(j);

					if (pTrackNode != nullptr)
					{
						int const numWaveNodes = pTrackNode->getChildCount();

						for (int k = 0; k < numWaveNodes; ++k)
						{
							XmlNodeRef const pWaveNode = pTrackNode->getChild(k);

							if (pWaveNode->haveAttr("EgReleaseTimeMs"))
							{
								float trackFadeOutTime = 0.0f;
								pWaveNode->getAttr("EgReleaseTimeMs", trackFadeOutTime);
								cueFadeOutTime = std::max(cueFadeOutTime, trackFadeOutTime);
							}
						}
					}
				}

				if (cueFadeOutTime > 0.0f)
				{
					g_cueFadeOutTimes[cueId].emplace_back(acbId, cueFadeOutTime / 1000.0f);
				}
			}
			else if ((_stricmp(typeAttrib, "CriMw.CriAtomCraft.AcCore.AcOoCueFolder") == 0) ||
			         (_stricmp(typeAttrib, "CriMw.CriAtomCraft.AcCore.AcOoCueFolderPrivate") == 0))
			{
				ParseAcbInfoFile(pCueNode, acbId);
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
							ParseAcbInfoFile(pAcbNode, StringToId(pAcbNode->getAttr("OrcaName")));
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
#endif // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

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
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::userMalloc");
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
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	, m_name("Adx2 - " CRI_ATOM_VER_NUM)
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
{
	g_pImpl = this;
}

///////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::Init(uint16 const objectPoolSize)
{
	ERequestStatus result = ERequestStatus::Success;

	if (g_cvars.m_cuePoolSize < 1)
	{
		g_cvars.m_cuePoolSize = 1;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
		Cry::Audio::Log(ELogType::Warning, R"(Cue pool size must be at least 1. Forcing the cvar "s_Adx2CuePoolSize" to 1!)");
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
	}

	g_constructedObjects.reserve(static_cast<size_t>(objectPoolSize));
	AllocateMemoryPools(objectPoolSize, static_cast<uint16>(g_cvars.m_cuePoolSize));

	m_regularSoundBankFolder = CRY_AUDIO_DATA_ROOT;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += g_szImplFolderName;
	m_regularSoundBankFolder += "/";
	m_regularSoundBankFolder += g_szAssetsFolderName;
	m_localizedSoundBankFolder = m_regularSoundBankFolder;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	g_constructedCueInstances.reserve(static_cast<size_t>(g_cvars.m_cuePoolSize));

	g_objectPoolSize = objectPoolSize;
	g_cueInstancePoolSize = static_cast<uint16>(g_cvars.m_cuePoolSize);

	LoadAcbInfos(m_regularSoundBankFolder);

	if (ICVar* pCVar = gEnv->pConsole->GetCVar("g_languageAudio"))
	{
		SetLanguage(pCVar->GetString());
		LoadAcbInfos(m_localizedSoundBankFolder);
	}

	criErr_SetCallback(errorCallback);
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

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

#if CRY_PLATFORM_WINDOWS
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
	g_pImpl = nullptr;
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

		g_poolSizesLevelSpecific.cues = std::max(g_poolSizesLevelSpecific.cues, levelPoolSizes.cues);
		g_poolSizesLevelSpecific.aisacControls = std::max(g_poolSizesLevelSpecific.aisacControls, levelPoolSizes.aisacControls);
		g_poolSizesLevelSpecific.aisacEnvironments = std::max(g_poolSizesLevelSpecific.aisacEnvironments, levelPoolSizes.aisacEnvironments);
		g_poolSizesLevelSpecific.aisacStates = std::max(g_poolSizesLevelSpecific.aisacStates, levelPoolSizes.aisacStates);
		g_poolSizesLevelSpecific.categories = std::max(g_poolSizesLevelSpecific.categories, levelPoolSizes.categories);
		g_poolSizesLevelSpecific.categoryStates = std::max(g_poolSizesLevelSpecific.categoryStates, levelPoolSizes.categoryStates);
		g_poolSizesLevelSpecific.gameVariables = std::max(g_poolSizesLevelSpecific.gameVariables, levelPoolSizes.gameVariables);
		g_poolSizesLevelSpecific.gameVariableStates = std::max(g_poolSizesLevelSpecific.gameVariableStates, levelPoolSizes.gameVariableStates);
		g_poolSizesLevelSpecific.selectorLabels = std::max(g_poolSizesLevelSpecific.selectorLabels, levelPoolSizes.selectorLabels);
		g_poolSizesLevelSpecific.dspBuses = std::max(g_poolSizesLevelSpecific.dspBuses, levelPoolSizes.dspBuses);
		g_poolSizesLevelSpecific.snapshots = std::max(g_poolSizesLevelSpecific.snapshots, levelPoolSizes.snapshots);
		g_poolSizesLevelSpecific.dspBusSettings = std::max(g_poolSizesLevelSpecific.dspBusSettings, levelPoolSizes.dspBusSettings);
		g_poolSizesLevelSpecific.binaries = std::max(g_poolSizesLevelSpecific.binaries, levelPoolSizes.binaries);
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

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	ZeroStruct(g_debugPoolSizes);
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::OnAfterLibraryDataChanged()
{
	g_poolSizes.cues += g_poolSizesLevelSpecific.cues;
	g_poolSizes.aisacControls += g_poolSizesLevelSpecific.aisacControls;
	g_poolSizes.aisacEnvironments += g_poolSizesLevelSpecific.aisacEnvironments;
	g_poolSizes.aisacStates += g_poolSizesLevelSpecific.aisacStates;
	g_poolSizes.categories += g_poolSizesLevelSpecific.categories;
	g_poolSizes.categoryStates += g_poolSizesLevelSpecific.categoryStates;
	g_poolSizes.gameVariables += g_poolSizesLevelSpecific.gameVariables;
	g_poolSizes.gameVariableStates += g_poolSizesLevelSpecific.gameVariableStates;
	g_poolSizes.selectorLabels += g_poolSizesLevelSpecific.selectorLabels;
	g_poolSizes.dspBuses += g_poolSizesLevelSpecific.dspBuses;
	g_poolSizes.snapshots += g_poolSizesLevelSpecific.snapshots;
	g_poolSizes.dspBusSettings += g_poolSizesLevelSpecific.dspBusSettings;
	g_poolSizes.binaries += g_poolSizesLevelSpecific.binaries;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	// Used to hide pools without allocations in debug draw.
	g_debugPoolSizes = g_poolSizes;
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

	g_poolSizes.cues = std::max<uint16>(1, g_poolSizes.cues);
	g_poolSizes.aisacControls = std::max<uint16>(1, g_poolSizes.aisacControls);
	g_poolSizes.aisacEnvironments = std::max<uint16>(1, g_poolSizes.aisacEnvironments);
	g_poolSizes.aisacStates = std::max<uint16>(1, g_poolSizes.aisacStates);
	g_poolSizes.categories = std::max<uint16>(1, g_poolSizes.categories);
	g_poolSizes.categoryStates = std::max<uint16>(1, g_poolSizes.categoryStates);
	g_poolSizes.gameVariables = std::max<uint16>(1, g_poolSizes.gameVariables);
	g_poolSizes.gameVariableStates = std::max<uint16>(1, g_poolSizes.gameVariableStates);
	g_poolSizes.selectorLabels = std::max<uint16>(1, g_poolSizes.selectorLabels);
	g_poolSizes.dspBuses = std::max<uint16>(1, g_poolSizes.dspBuses);
	g_poolSizes.snapshots = std::max<uint16>(1, g_poolSizes.snapshots);
	g_poolSizes.dspBusSettings = std::max<uint16>(1, g_poolSizes.dspBusSettings);
	g_poolSizes.binaries = std::max<uint16>(1, g_poolSizes.binaries);
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
void CImpl::RegisterInMemoryFile(SFileInfo* const pFileInfo)
{
	if (pFileInfo != nullptr)
	{
		auto const pFileData = static_cast<CBinary*>(pFileInfo->pImplData);

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
			}
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
			else
			{
				Cry::Audio::Log(ELogType::Error, "Failed to load ACB %s\n", pFileInfo->szFileName);
			}
#endif      // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
		}
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid FileData passed to the Adx2 implementation of %s", __FUNCTION__);
		}
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::UnregisterInMemoryFile(SFileInfo* const pFileInfo)
{
	if (pFileInfo != nullptr)
	{
		auto const pFileData = static_cast<CBinary*>(pFileInfo->pImplData);

		if ((pFileData != nullptr) && (pFileData->pAcb != nullptr))
		{
			criAtomExAcb_Release(pFileData->pAcb);
		}
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Error, "Invalid FileData passed to the Adx2 implementation of %s", __FUNCTION__);
		}
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
	}
}

//////////////////////////////////////////////////////////////////////////
ERequestStatus CImpl::ConstructFile(XmlNodeRef const pRootNode, SFileInfo* const pFileInfo)
{
	ERequestStatus result = ERequestStatus::Failure;

	if ((_stricmp(pRootNode->getTag(), g_szBinaryTag) == 0) && (pFileInfo != nullptr))
	{
		char const* const szFileName = pRootNode->getAttr(g_szNameAttribute);

		if (szFileName != nullptr && szFileName[0] != '\0')
		{
			char const* const szLocalized = pRootNode->getAttr(g_szLocalizedAttribute);
			pFileInfo->bLocalized = (szLocalized != nullptr) && (_stricmp(szLocalized, g_szTrueValue) == 0);
			pFileInfo->szFileName = szFileName;

			// The Atom library accesses on-memory data with a 32-bit width.
			// The first address of the data must be aligned at a 4-byte boundary.
			pFileInfo->memoryBlockAlignment = 32;

			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CBinary");
			pFileInfo->pImplData = new CBinary();

			result = ERequestStatus::Success;
		}
		else
		{
			pFileInfo->szFileName = nullptr;
			pFileInfo->memoryBlockAlignment = 0;
			pFileInfo->pImplData = nullptr;
		}
	}

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
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	implInfo.name = m_name.c_str();
#else
	implInfo.name = "name-not-present-in-release-mode";
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
	implInfo.folderName = g_szImplFolderName;
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructGlobalObject()
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CGlobalObject");
	new CGlobalObject;

	if (!stl::push_back_unique(g_constructedObjects, static_cast<CBaseObject*>(g_pObject)))
	{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
		Cry::Audio::Log(ELogType::Warning, "Trying to construct an already registered object.");
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
	}

	return static_cast<IObject*>(g_pObject);
}

///////////////////////////////////////////////////////////////////////////
IObject* CImpl::ConstructObject(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CObject");
	auto const pObject = new CObject(transformation);

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	pObject->SetName(szName);

	if (!stl::push_back_unique(g_constructedObjects, pObject))
	{
		Cry::Audio::Log(ELogType::Warning, "Trying to construct an already registered object.");
	}
#else
	stl::push_back_unique(g_constructedObjects, pObject);
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

	return static_cast<IObject*>(pObject);
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructObject(IObject const* const pIObject)
{
	auto const pBaseObject = static_cast<CBaseObject const*>(pIObject);

	if (!stl::find_and_erase(g_constructedObjects, pBaseObject))
	{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
		Cry::Audio::Log(ELogType::Warning, "Trying to delete a non-existing object.");
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
	}

	delete pBaseObject;
}

///////////////////////////////////////////////////////////////////////////
IListener* CImpl::ConstructListener(CTransformation const& transformation, char const* const szName /*= nullptr*/)
{
	IListener* pIListener = nullptr;

	static uint16 id = 0;
	CriAtomEx3dListenerHn const pHandle = criAtomEx3dListener_Create(&m_listenerConfig, nullptr, 0);

	if (pHandle != nullptr)
	{
		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CListener");
		g_pListener = new CListener(transformation, id++, pHandle);

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
		g_pListener->SetName(szName);
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

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
void CImpl::GamepadConnected(DeviceId const deviceUniqueID)
{
}

//////////////////////////////////////////////////////////////////////////
void CImpl::GamepadDisconnected(DeviceId const deviceUniqueID)
{
}

///////////////////////////////////////////////////////////////////////////
ITriggerConnection* CImpl::ConstructTriggerConnection(XmlNodeRef const pRootNode, float& radius)
{
	ITriggerConnection* pITriggerConnection = nullptr;

	if (_stricmp(pRootNode->getTag(), g_szCueTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		char const* const szCueSheetName = pRootNode->getAttr(g_szCueSheetAttribute);
		uint32 const cueId = StringToId(szName);

		CCue::EActionType type = CCue::EActionType::Start;
		char const* const szType = pRootNode->getAttr(g_szTypeAttribute);

		if ((szType != nullptr) && (szType[0] != '\0'))
		{
			if (_stricmp(szType, g_szStopValue) == 0)
			{
				type = CCue::EActionType::Stop;
			}
			else if (_stricmp(szType, g_szPauseValue) == 0)
			{
				type = CCue::EActionType::Pause;
			}
			else if (_stricmp(szType, g_szResumeValue) == 0)
			{
				type = CCue::EActionType::Resume;
			}
		}

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
		uint32 const cueSheetId = StringToId(szCueSheetName);
		float fadeOutTime = 0.0f;

		if (type == CCue::EActionType::Start)
		{
			auto const iterRadius = g_cueRadiusInfo.find(cueId);

			if (iterRadius != g_cueRadiusInfo.end())
			{
				for (auto const& pair : iterRadius->second)
				{
					if (cueSheetId == pair.first)
					{
						radius = pair.second;
						break;
					}
				}
			}

			auto const iterFadeOutTime = g_cueFadeOutTimes.find(cueId);

			if (iterFadeOutTime != g_cueFadeOutTimes.end())
			{
				for (auto const& pair : iterFadeOutTime->second)
				{
					if (cueSheetId == pair.first)
					{
						fadeOutTime = pair.second;
						break;
					}
				}
			}
		}

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CCue");
		pITriggerConnection = static_cast<ITriggerConnection*>(new CCue(cueId, szName, StringToId(szCueSheetName), type, szCueSheetName, fadeOutTime));
#else
		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CCue");
		pITriggerConnection = static_cast<ITriggerConnection*>(new CCue(cueId, szName, StringToId(szCueSheetName), type));
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
	}
	else if (_stricmp(pRootNode->getTag(), g_szSnapshotTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);

		char const* const szType = pRootNode->getAttr(g_szTypeAttribute);
		CSnapshot::EActionType const type =
			((szType != nullptr) && (szType[0] != '\0') && (_stricmp(szType, g_szStopValue) == 0)) ? CSnapshot::EActionType::Stop : CSnapshot::EActionType::Start;

		int changeoverTime = g_defaultChangeoverTime;
		pRootNode->getAttr(g_szTimeAttribute, changeoverTime);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CSnapshot");
		pITriggerConnection = static_cast<ITriggerConnection*>(new CSnapshot(szName, type, static_cast<CriSint32>(changeoverTime)));
	}
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", pRootNode->getTag());
	}
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

	return pITriggerConnection;
}

//////////////////////////////////////////////////////////////////////////
ITriggerConnection* CImpl::ConstructTriggerConnection(ITriggerInfo const* const pITriggerInfo)
{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	ITriggerConnection* pITriggerConnection = nullptr;
	auto const pTriggerInfo = static_cast<STriggerInfo const*>(pITriggerInfo);

	if (pTriggerInfo != nullptr)
	{
		char const* const szName = pTriggerInfo->name.c_str();
		char const* const szCueSheetName = pTriggerInfo->cueSheet.c_str();
		uint32 const cueId = StringToId(szName);
		uint32 const cueSheetId = StringToId(szCueSheetName);

		float fadeOutTime = 0.0f;

		auto const iterFadeOutTime = g_cueFadeOutTimes.find(cueId);

		if (iterFadeOutTime != g_cueFadeOutTimes.end())
		{
			for (auto const& pair : iterFadeOutTime->second)
			{
				if (cueSheetId == pair.first)
				{
					fadeOutTime = pair.second;
					break;
				}
			}
		}

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CCue");
		pITriggerConnection = static_cast<ITriggerConnection*>(new CCue(cueId, szName, StringToId(szCueSheetName), CCue::EActionType::Start, szCueSheetName, fadeOutTime));
	}

	return pITriggerConnection;
#else
	return nullptr;
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructTriggerConnection(ITriggerConnection const* const pITriggerConnection)
{
	auto const pBaseTriggerConnection = static_cast<CBaseTriggerConnection const*>(pITriggerConnection);

	if (pBaseTriggerConnection->GetType() == CBaseTriggerConnection::EType::Cue)
	{
		auto const pCue = static_cast<CCue const*>(pBaseTriggerConnection);
		pCue->SetToBeDestructed();

		if (pCue->CanBeDestructed())
		{
			delete pCue;
		}
	}
	else
	{
		delete pBaseTriggerConnection;
	}
}

///////////////////////////////////////////////////////////////////////////
IParameterConnection* CImpl::ConstructParameterConnection(XmlNodeRef const pRootNode)
{
	IParameterConnection* pIParameterConnection = nullptr;

	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, g_szAisacControlTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		float multiplier = g_defaultParamMultiplier;
		float shift = g_defaultParamShift;
		pRootNode->getAttr(g_szMutiplierAttribute, multiplier);
		pRootNode->getAttr(g_szShiftAttribute, shift);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CAisacControl");
		pIParameterConnection = static_cast<IParameterConnection*>(new CAisacControl(szName, multiplier, shift));
	}
	else if (_stricmp(szTag, g_szSCategoryTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		float multiplier = g_defaultParamMultiplier;
		float shift = g_defaultParamShift;
		pRootNode->getAttr(g_szMutiplierAttribute, multiplier);
		pRootNode->getAttr(g_szShiftAttribute, shift);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CCategory");
		pIParameterConnection = static_cast<IParameterConnection*>(new CCategory(szName, multiplier, shift));
	}
	else if (_stricmp(szTag, g_szGameVariableTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		float multiplier = g_defaultParamMultiplier;
		float shift = g_defaultParamShift;
		pRootNode->getAttr(g_szMutiplierAttribute, multiplier);
		pRootNode->getAttr(g_szShiftAttribute, shift);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CGameVariable");
		pIParameterConnection = static_cast<IParameterConnection*>(new CGameVariable(szName, multiplier, shift));
	}
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", szTag);
	}
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

	return pIParameterConnection;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructParameterConnection(IParameterConnection const* const pIParameterConnection)
{
	delete pIParameterConnection;
}

//////////////////////////////////////////////////////////////////////////
bool ParseSelectorNode(XmlNodeRef const pNode, string& selectorName, string& selectorLabelName)
{
	bool foundAttributes = false;

	char const* const szSelectorName = pNode->getAttr(g_szNameAttribute);

	if ((szSelectorName != nullptr) && (szSelectorName[0] != 0) && (pNode->getChildCount() == 1))
	{
		XmlNodeRef const pLabelNode(pNode->getChild(0));

		if (pLabelNode && _stricmp(pLabelNode->getTag(), g_szSelectorLabelTag) == 0)
		{
			char const* const szSelectorLabelName = pLabelNode->getAttr(g_szNameAttribute);

			if ((szSelectorLabelName != nullptr) && (szSelectorLabelName[0] != 0))
			{
				selectorName = szSelectorName;
				selectorLabelName = szSelectorLabelName;
				foundAttributes = true;
			}
		}
	}
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "An Adx2 Selector %s inside SwitchState needs to have exactly one Selector Label.", szSelectorName);
	}
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

	return foundAttributes;
}

///////////////////////////////////////////////////////////////////////////
ISwitchStateConnection* CImpl::ConstructSwitchStateConnection(XmlNodeRef const pRootNode)
{
	ISwitchStateConnection* pISwitchStateConnection = nullptr;

	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, g_szSelectorTag) == 0)
	{
		string szSelectorName;
		string szSelectorLabelName;

		if (ParseSelectorNode(pRootNode, szSelectorName, szSelectorLabelName))
		{
			MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CSelectorLabel");
			pISwitchStateConnection = static_cast<ISwitchStateConnection*>(new CSelectorLabel(szSelectorName, szSelectorLabelName));
		}
	}
	else if (_stricmp(szTag, g_szAisacControlTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		float value = g_defaultStateValue;
		pRootNode->getAttr(g_szValueAttribute, value);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CAisacState");
		pISwitchStateConnection = static_cast<ISwitchStateConnection*>(new CAisacState(szName, static_cast<CriFloat32>(value)));

	}
	else if (_stricmp(szTag, g_szGameVariableTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		float value = g_defaultStateValue;
		pRootNode->getAttr(g_szValueAttribute, value);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CGameVariableState");
		pISwitchStateConnection = static_cast<ISwitchStateConnection*>(new CGameVariableState(szName, static_cast<CriFloat32>(value)));
	}
	else if (_stricmp(szTag, g_szSCategoryTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		float value = g_defaultStateValue;
		pRootNode->getAttr(g_szValueAttribute, value);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CCategoryState");
		pISwitchStateConnection = static_cast<ISwitchStateConnection*>(new CCategoryState(szName, static_cast<CriFloat32>(value)));
	}
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", szTag);
	}
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

	return pISwitchStateConnection;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructSwitchStateConnection(ISwitchStateConnection const* const pISwitchStateConnection)
{
	delete pISwitchStateConnection;
}

///////////////////////////////////////////////////////////////////////////
IEnvironmentConnection* CImpl::ConstructEnvironmentConnection(XmlNodeRef const pRootNode)
{
	IEnvironmentConnection* pEnvironmentConnection = nullptr;

	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, g_szDspBusTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CDspBus");
		pEnvironmentConnection = static_cast<IEnvironmentConnection*>(new CDspBus(szName));
	}
	else if (_stricmp(szTag, g_szAisacControlTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);
		float multiplier = g_defaultParamMultiplier;
		float shift = g_defaultParamShift;
		pRootNode->getAttr(g_szMutiplierAttribute, multiplier);
		pRootNode->getAttr(g_szShiftAttribute, shift);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CAisacEnvironment");
		pEnvironmentConnection = static_cast<IEnvironmentConnection*>(new CAisacEnvironment(szName, multiplier, shift));
	}
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", szTag);
	}
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

	return pEnvironmentConnection;
}

///////////////////////////////////////////////////////////////////////////
void CImpl::DestructEnvironmentConnection(IEnvironmentConnection const* const pIEnvironmentConnection)
{
	delete pIEnvironmentConnection;
}

//////////////////////////////////////////////////////////////////////////
ISettingConnection* CImpl::ConstructSettingConnection(XmlNodeRef const pRootNode)
{
	ISettingConnection* pISettingConnection = nullptr;

	char const* const szTag = pRootNode->getTag();

	if (_stricmp(szTag, g_szDspBusSettingTag) == 0)
	{
		char const* const szName = pRootNode->getAttr(g_szNameAttribute);

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CDspBusSetting");
		pISettingConnection = static_cast<ISettingConnection*>(new CDspBusSetting(szName));
	}
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Warning, "Unknown Adx2 tag: %s", szTag);
	}
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

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
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	g_debugCurrentDspBusSettingName = g_debugNoneDspBusSetting;
	m_acfFileSize = 0;
	g_cueRadiusInfo.clear();
	g_cueFadeOutTimes.clear();
	LoadAcbInfos(m_regularSoundBankFolder);
	LoadAcbInfos(m_localizedSoundBankFolder);
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

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
		m_localizedSoundBankFolder += CRY_AUDIO_DATA_ROOT;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += g_szImplFolderName;
		m_localizedSoundBankFolder += "/";
		m_localizedSoundBankFolder += g_szAssetsFolderName;
	}
}

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
CCueInstance* CImpl::ConstructCueInstance(TriggerInstanceId const triggerInstanceId, CriAtomExPlaybackId const playbackId, CCue& cue, CBaseObject const& baseObject)
{
	cue.IncrementNumInstances();

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CCueInstance");
	auto const pCueInstance = new CCueInstance(triggerInstanceId, playbackId, cue, baseObject);
	g_constructedCueInstances.push_back(pCueInstance);

	return pCueInstance;
}
#else
//////////////////////////////////////////////////////////////////////////
CCueInstance* CImpl::ConstructCueInstance(TriggerInstanceId const triggerInstanceId, CriAtomExPlaybackId const playbackId, CCue& cue)
{
	cue.IncrementNumInstances();

	MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CCueInstance");
	return new CCueInstance(triggerInstanceId, playbackId, cue);
}
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CImpl::DestructCueInstance(CCueInstance const* const pCueInstance)
{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	CRY_ASSERT_MESSAGE(pCueInstance != nullptr, "pCueInstance is nullpter during %s", __FUNCTION__);

	auto iter(g_constructedCueInstances.begin());
	auto const iterEnd(g_constructedCueInstances.cend());

	for (; iter != iterEnd; ++iter)
	{
		if ((*iter) == pCueInstance)
		{
			if (iter != (iterEnd - 1))
			{
				(*iter) = g_constructedCueInstances.back();
			}

			g_constructedCueInstances.pop_back();
			break;
		}
	}
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

	CCue* const pCue = &pCueInstance->GetCue();
	delete pCueInstance;

	pCue->DecrementNumInstances();

	if (pCue->CanBeDestructed())
	{
		delete pCue;
	}
}
//////////////////////////////////////////////////////////////////////////
bool CImpl::InitializeLibrary()
{
#if CRY_PLATFORM_WINDOWS
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

#if CRY_PLATFORM_WINDOWS
	criAtomEx_Initialize_WASAPI(&libraryConfig, nullptr, 0);
#else
	criAtomEx_Initialize(&libraryConfig, nullptr, 0);
#endif  // CRY_PLATFORM_WINDOWS

	bool const isInitialized = (criAtomEx_IsInitialized() == CRI_TRUE);

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	if (!isInitialized)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to initialize CriAtomEx");
	}
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

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

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	if (!isAllocated)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to allocate voice pool");
	}
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

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

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	if (!isCreated)
	{
		Cry::Audio::Log(ELogType::Error, "Failed to create D-BAS");
	}
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

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

		MEMSTAT_CONTEXT(EMemStatContextTypes::MSC_AudioImpl, 0, "CryAudio::Impl::Adx2::CImpl::m_pAcfBuffer");
		m_pAcfBuffer = new uint8[acfFileSize];
		gEnv->pCryPak->FRead(m_pAcfBuffer, acfFileSize, pAcfFile);
		gEnv->pCryPak->FClose(pAcfFile);

		auto const pAcfData = static_cast<void*>(m_pAcfBuffer);

		criAtomEx_RegisterAcfData(pAcfData, static_cast<CriSint32>(acfFileSize), nullptr, 0);
		CriAtomExAcfInfo acfInfo;

		if (criAtomExAcf_GetAcfInfo(&acfInfo) == CRI_TRUE)
		{
			acfRegistered = true;

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
			m_acfFileSize = acfFileSize;
#endif      // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
		}
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
		else
		{
			Cry::Audio::Log(ELogType::Error, "Failed to register ACF");
		}
#endif    // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
	}
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	else
	{
		Cry::Audio::Log(ELogType::Error, "ACF not found.");
	}
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

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
	for (auto const pBaseObject : g_constructedObjects)
	{
		pBaseObject->MutePlayer(shouldMute);
	}
}

//////////////////////////////////////////////////////////////////////////
void CImpl::PauseAllObjects(CriBool const shouldPause)
{
	for (auto const pBaseObject : g_constructedObjects)
	{
		pBaseObject->PausePlayer(shouldPause);
	}
}

#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
//////////////////////////////////////////////////////////////////////////
size_t DrawMemoryPoolInfo(
	IRenderAuxGeom& auxGeom,
	float const posX,
	float& posY,
	stl::SPoolMemoryUsage const& mem,
	stl::SMemoryUsage const& pool,
	char const* const szType,
	uint16 const poolSize,
	bool const drawInfo)
{
	if (drawInfo)
	{
		CryFixedStringT<Debug::MaxMemInfoStringLength> memAllocString;
		Debug::FormatMemoryString(memAllocString, mem.nAlloc);

		ColorF const& color = (static_cast<uint16>(pool.nUsed) > poolSize) ? Debug::s_globalColorError : Debug::s_systemColorTextPrimary;

		posY += Debug::g_systemLineHeight;
		auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, color, false,
		                    "[%s] Constructed: %" PRISIZE_T " | Allocated: %" PRISIZE_T " | Preallocated: %u | Pool Size: %s",
		                    szType, pool.nUsed, pool.nAlloc, poolSize, memAllocString.c_str());
	}

	return mem.nAlloc;
}
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugMemoryInfo(IRenderAuxGeom& auxGeom, float const posX, float& posY, bool const showDetailedInfo)
{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)

	float const headerPosY = posY;
	posY += Debug::g_systemHeaderLineSpacerHeight;

	size_t totalPoolSize = 0;

	{
		auto& allocator = CObject::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Objects", g_objectPoolSize, showDetailedInfo);
	}

	{
		auto& allocator = CCueInstance::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Cue Instances", g_cueInstancePoolSize, showDetailedInfo);
	}

	if (g_debugPoolSizes.cues > 0)
	{
		auto& allocator = CCue::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Cues", g_poolSizes.cues, showDetailedInfo);
	}

	if (g_debugPoolSizes.aisacControls > 0)
	{
		auto& allocator = CAisacControl::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Aisac-Controls", g_poolSizes.aisacControls, showDetailedInfo);
	}

	if (g_debugPoolSizes.aisacEnvironments > 0)
	{
		auto& allocator = CAisacEnvironment::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Aisac-Controls on Environments", g_poolSizes.aisacEnvironments, showDetailedInfo);
	}

	if (g_debugPoolSizes.aisacStates > 0)
	{
		auto& allocator = CAisacState::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Aisac-Controls on States", g_poolSizes.aisacStates, showDetailedInfo);
	}

	if (g_debugPoolSizes.categories > 0)
	{
		auto& allocator = CCategory::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Categories", g_poolSizes.categories, showDetailedInfo);
	}

	if (g_debugPoolSizes.categoryStates > 0)
	{
		auto& allocator = CCategoryState::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Categories on States", g_poolSizes.categoryStates, showDetailedInfo);
	}

	if (g_debugPoolSizes.gameVariables > 0)
	{
		auto& allocator = CGameVariable::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "GameVariables", g_poolSizes.gameVariables, showDetailedInfo);
	}

	if (g_debugPoolSizes.gameVariableStates > 0)
	{
		auto& allocator = CGameVariableState::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "GameVariables on States", g_poolSizes.gameVariableStates, showDetailedInfo);
	}

	if (g_debugPoolSizes.selectorLabels > 0)
	{
		auto& allocator = CSelectorLabel::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Selector Labels", g_poolSizes.selectorLabels, showDetailedInfo);
	}

	if (g_debugPoolSizes.dspBuses > 0)
	{
		auto& allocator = CDspBus::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "DSP Buses", g_poolSizes.dspBuses, showDetailedInfo);
	}

	if (g_debugPoolSizes.snapshots > 0)
	{
		auto& allocator = CSnapshot::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Snapshots", g_poolSizes.snapshots, showDetailedInfo);
	}

	if (g_debugPoolSizes.dspBusSettings > 0)
	{
		auto& allocator = CDspBusSetting::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "DSP Bus Settings", g_poolSizes.dspBusSettings, showDetailedInfo);
	}

	if (g_debugPoolSizes.binaries > 0)
	{
		auto& allocator = CBinary::GetAllocator();
		totalPoolSize += DrawMemoryPoolInfo(auxGeom, posX, posY, allocator.GetTotalMemory(), allocator.GetCounts(), "Binaries", g_poolSizes.binaries, showDetailedInfo);
	}

	CryModuleMemoryInfo memInfo;
	ZeroStruct(memInfo);
	CryGetMemoryInfoForModule(&memInfo);

	CryFixedStringT<Debug::MaxMemInfoStringLength> memInfoString;
	auto const memAlloc = static_cast<uint32>(memInfo.allocated - memInfo.freed);
	Debug::FormatMemoryString(memInfoString, memAlloc);

	CryFixedStringT<Debug::MaxMemInfoStringLength> totalPoolSizeString;
	Debug::FormatMemoryString(totalPoolSizeString, totalPoolSize);

	CryFixedStringT<Debug::MaxMemInfoStringLength> acfSizeString;
	Debug::FormatMemoryString(acfSizeString, m_acfFileSize);

	CryFixedStringT<Debug::MaxMemInfoStringLength> totalMemSizeString;
	size_t const totalMemSize = static_cast<size_t>(memAlloc) + totalPoolSize + m_acfFileSize;
	Debug::FormatMemoryString(totalMemSizeString, totalMemSize);

	auxGeom.Draw2dLabel(posX, headerPosY, Debug::g_systemHeaderFontSize, Debug::s_globalColorHeader, false, "%s (System: %s | Pools: %s | ACF: %s | Total: %s)",
	                    m_name.c_str(), memInfoString.c_str(), totalPoolSizeString.c_str(), acfSizeString.c_str(), totalMemSizeString.c_str());

	size_t const numCueInstances = g_constructedCueInstances.size();

	posY += Debug::g_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorTextSecondary, false,
	                    "Active Cues: %3" PRISIZE_T " | Objects with Doppler: %u | DSP Bus Setting: %s | Active Snapshot: %s",
	                    numCueInstances, g_numObjectsWithDoppler, g_debugCurrentDspBusSettingName.c_str(), g_debugActiveSnapShotName.c_str());

	Vec3 const& listenerPosition = g_pListener->GetPosition();
	Vec3 const& listenerDirection = g_pListener->GetTransformation().GetForward();
	float const listenerVelocity = g_pListener->GetVelocity().GetLength();
	char const* const szName = g_pListener->GetName();

	posY += Debug::g_systemLineHeight;
	auxGeom.Draw2dLabel(posX, posY, Debug::g_systemFontSize, Debug::s_systemColorListenerActive, false, "Listener: %s | PosXYZ: %.2f %.2f %.2f | FwdXYZ: %.2f %.2f %.2f | Velocity: %.2f m/s",
	                    szName, listenerPosition.x, listenerPosition.y, listenerPosition.z, listenerDirection.x, listenerDirection.y, listenerDirection.z, listenerVelocity);
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
}

//////////////////////////////////////////////////////////////////////////
void CImpl::DrawDebugInfoList(IRenderAuxGeom& auxGeom, float& posX, float posY, float const debugDistance, char const* const szTextFilter) const
{
#if defined(CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE)
	if ((g_cvars.m_debugListFilter & g_debugListMask) != 0)
	{
		CryFixedStringT<MaxControlNameLength> lowerCaseSearchString(szTextFilter);
		lowerCaseSearchString.MakeLower();

		float const initialPosY = posY;

		if ((g_cvars.m_debugListFilter & EDebugListFilter::CueInstances) != 0)
		{
			auxGeom.Draw2dLabel(posX, posY, Debug::g_listHeaderFontSize, Debug::s_globalColorHeader, false, "Adx2 Cues [%" PRISIZE_T "]", g_constructedCueInstances.size());
			posY += Debug::g_listHeaderLineHeight;

			for (auto const pCueInstance : g_constructedCueInstances)
			{
				Vec3 const& position = pCueInstance->GetObject().GetTransformation().GetPosition();
				float const distance = position.GetDistance(g_pListener->GetPosition());

				if ((debugDistance <= 0.0f) || ((debugDistance > 0.0f) && (distance < debugDistance)))
				{
					char const* const szCueName = pCueInstance->GetCue().GetName();
					CryFixedStringT<MaxControlNameLength> lowerCaseCueName(szCueName);
					lowerCaseCueName.MakeLower();
					bool const draw = ((lowerCaseSearchString.empty() || (lowerCaseSearchString == "0")) || (lowerCaseCueName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos));

					if (draw)
					{
						ECueInstanceFlags const flags = pCueInstance->GetFlags();
						ColorF color = Debug::s_listColorItemActive;

						CryFixedStringT<MaxMiscStringLength> debugText;
						debugText.Format("%s on %s", szCueName, pCueInstance->GetObject().GetName());

						if ((flags& ECueInstanceFlags::IsPending) != 0)
						{
							color = Debug::s_listColorItemLoading;
						}
						else if ((flags& ECueInstanceFlags::IsVirtual) != 0)
						{
							color = Debug::s_globalColorVirtual;
						}
						else if ((flags& ECueInstanceFlags::IsStopping) != 0)
						{
							float const fadeOutTime = pCueInstance->GetCue().GetFadeOutTime();
							float const remainingTime = std::max(0.0f, fadeOutTime - (gEnv->pTimer->GetAsyncTime().GetSeconds() - pCueInstance->GetTimeFadeOutStarted()));

							debugText.Format("%s on %s (%.2f sec)", szCueName, pCueInstance->GetObject().GetName(), remainingTime);
							color = Debug::s_listColorItemStopping;
						}

						auxGeom.Draw2dLabel(posX, posY, Debug::g_listFontSize, color, false, "%s", debugText.c_str());

						posY += Debug::g_listLineHeight;
					}
				}
			}

			posX += 600.0f;
		}

		if ((g_cvars.m_debugListFilter & EDebugListFilter::GameVariables) != 0)
		{
			posY = initialPosY;

			auxGeom.Draw2dLabel(posX, posY, Debug::g_listHeaderFontSize, Debug::s_globalColorHeader, false, "Adx2 GameVariables [%" PRISIZE_T "]", g_gameVariableValues.size());
			posY += Debug::g_listHeaderLineHeight;

			for (auto const& gameVariablePair : g_gameVariableValues)
			{
				char const* const szGameVariableName = gameVariablePair.first.c_str();
				CryFixedStringT<MaxControlNameLength> lowerCaseGameVariableName(szGameVariableName);
				lowerCaseGameVariableName.MakeLower();
				bool const draw = ((lowerCaseSearchString.empty() || (lowerCaseSearchString == "0")) || (lowerCaseGameVariableName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos));

				if (draw)
				{
					auxGeom.Draw2dLabel(posX, posY, Debug::g_listFontSize, Debug::s_listColorItemActive, false, "%s : %3.2f", szGameVariableName, gameVariablePair.second);

					posY += Debug::g_listLineHeight;
				}
			}

			posX += 300.0f;
		}

		if ((g_cvars.m_debugListFilter & EDebugListFilter::Categories) != 0)
		{
			posY = initialPosY;

			auxGeom.Draw2dLabel(posX, posY, Debug::g_listHeaderFontSize, Debug::s_globalColorHeader, false, "Adx2 Categories [%" PRISIZE_T "]", g_categoryValues.size());
			posY += Debug::g_listHeaderLineHeight;

			for (auto const& categoryPair : g_categoryValues)
			{
				char const* const szCategoryName = categoryPair.first.c_str();
				CryFixedStringT<MaxControlNameLength> lowerCaseCategoryName(szCategoryName);
				lowerCaseCategoryName.MakeLower();
				bool const draw = ((lowerCaseSearchString.empty() || (lowerCaseSearchString == "0")) || (lowerCaseCategoryName.find(lowerCaseSearchString) != CryFixedStringT<MaxControlNameLength>::npos));

				if (draw)
				{
					auxGeom.Draw2dLabel(posX, posY, Debug::g_listFontSize, Debug::s_listColorItemActive, false, "%s : %3.2f", szCategoryName, categoryPair.second);

					posY += Debug::g_listLineHeight;
				}
			}

			posX += 300.0f;
		}
	}
#endif  // CRY_AUDIO_IMPL_ADX2_USE_PRODUCTION_CODE
}
} // namespace Adx2
} // namespace Impl
} // namespace CryAudio
