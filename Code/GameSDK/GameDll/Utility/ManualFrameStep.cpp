// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Player ability manager.
-------------------------------------------------------------------------
History:
- 14:04:2014: Created by Jean Geffroy
*************************************************************************/

#include "StdAfx.h"

#include "ManualFrameStep.h"

#include "GameCVars.h"
#include "GameRules.h"
#include <CryCore/Platform/CryWindows.h>

#if CRY_PLATFORM_WINDOWS && ENABLE_MANUAL_FRAME_STEP
#define ENABLE_MANUAL_FRAME_STEP_BACKWARDS 1
#endif

namespace
{
	stack_string GetFramesFolder()
	{
		stack_string framesFolder = "ManualStepFrames_";
		if (IActor* const pLocalPlayer = g_pGame->GetIGameFramework()->GetClientActor())
		{
			framesFolder.append(pLocalPlayer->GetEntity()->GetName());
		}
		else
		{
			framesFolder.append("Default");
		}

		return framesFolder;
	}

	static const uint8 k_disableMask = ~0;
}

CManualFrameStepManager::CManualFrameStepManager()
	: m_rmiListenerIdx(-1) 
	, m_framesLeft(-1)
	, m_previousFixedStep(0.0f)
{
	if (gEnv->pInput)
	{
		gEnv->pInput->AddEventListener(this);
	}
}

CManualFrameStepManager::~CManualFrameStepManager()
{
	if (gEnv->pInput)
	{
		gEnv->pInput->RemoveEventListener(this);
	}

	if (m_rmiListenerIdx > 0 && g_pGame)
	{
		if (CGameRules* const pGameRules = g_pGame->GetGameRules())
		{
			pGameRules->UnRegisterModuleRMIListener(m_rmiListenerIdx);
		}
	}
}

bool CManualFrameStepManager::Update()
{
	if (m_rmiListenerIdx < 0 && g_pGame->GetGameRules())
	{
		if (CGameRules* const pGameRules = g_pGame->GetGameRules())
		{
			m_rmiListenerIdx = pGameRules->RegisterModuleRMIListener(this);
		}
	}

	const bool shouldBlock = m_framesLeft == 0;
	if (m_framesLeft > 0)
	{
		--m_framesLeft;
	}

	if (shouldBlock)
	{
		// Manually update core systems which have to be ticked even when blocking the main loop
		if (gEnv->pInput)
		{
			gEnv->pInput->Update(true);
		}

		if (gEnv->pNetwork)
		{
			gEnv->pNetwork->SyncWithGame(eNGS_FrameStart);
			gEnv->pNetwork->SyncWithGame(eNGS_FrameEnd);
		}
	}

	return shouldBlock;
}

bool CManualFrameStepManager::IsEnabled() const
{
	return m_framesLeft >= 0;
}

void CManualFrameStepManager::Enable(const bool enable)
{
	if (IsEnabled() && enable)
	{
		// Already enabled: early out
		return;
	}

	m_framesLeft = enable ? 0 : -1;

	// Set fixed step
	if (ICVar* const pFixedStep = gEnv->pConsole->GetCVar("t_FixedStep"))
	{
		if (enable)
		{
			m_previousFixedStep = pFixedStep->GetFVal();
			pFixedStep->Set(1.0f / g_pGameCVars->g_manualFrameStepFrequency);
		}
		else
		{
			pFixedStep->Set(m_previousFixedStep);
		}
	}

	// And capture output folder
	const stack_string& framesFolder = GetFramesFolder();

	if (!gEnv->IsDedicated())
	{
		if (ICVar* const pCaptureFrames = gEnv->pConsole->GetCVar("capture_frames"))
		{
			if (ICVar* const pCaptureFramesFolder = gEnv->pConsole->GetCVar("capture_folder"))
			{
				if (enable)
				{
					pCaptureFramesFolder->Set(framesFolder.c_str());
					pCaptureFrames->Set(1);
				}
				else
				{
					pCaptureFramesFolder->Set("CaptureOutput");
					pCaptureFrames->Set(0);
				}
			}
		}
	}
}

void CManualFrameStepManager::Toggle()
{
	Enable(!IsEnabled());
}

void CManualFrameStepManager::RequestFrames(const uint8 numFrames)
{
	m_framesLeft += numFrames;
}

bool CManualFrameStepManager::OnInputEvent(const SInputEvent& inputEvent)
{
	if (g_pGameCVars->g_manualFrameStepFrequency != 0.0f)
	{
		if (eIDT_Keyboard == inputEvent.deviceType)
		{
			bool processEvent = false;
			CGameRules::SModuleRMIEntityParams params;
			params.m_listenerIndex = m_rmiListenerIdx;

			switch(inputEvent.keyId)
			{
			case eKI_Pause:
				if (eIS_Pressed == inputEvent.state)
				{
					// Toggle
					params.m_data = IsEnabled() ? k_disableMask : 0;
					processEvent  = true;
				}
				break;
			case eKI_Right:
				if (IsEnabled() && eIS_Pressed == inputEvent.state)
				{
					// Generate one frame
					params.m_data = 1;
					processEvent  = true;
				}
				break;
#if ENABLE_MANUAL_FRAME_STEP_BACKWARDS
			case eKI_Left:
				{
					// Open last generated frames
					if (IsEnabled() && eIS_Pressed == inputEvent.state)
					{
						char szWorkingDirectory[MAX_PATH];
						GetCurrentDirectory(sizeof(szWorkingDirectory) - 1, szWorkingDirectory);

						const stack_string& framesFolder = GetFramesFolder();

						stack_string framesPath;
						framesPath.Format("%s\\%s\\*", szWorkingDirectory, framesFolder.c_str());

						WIN32_FIND_DATA ffd;
						HANDLE hFind = FindFirstFile(framesPath.c_str(), &ffd);

						string bestPath;
						FILETIME bestTime;
						if (INVALID_HANDLE_VALUE != hFind) 
						{
							do
							{
								if (0 == (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
								{
									const FILETIME lastWrite = ffd.ftLastWriteTime;
									bool newBest = false;
									if (bestPath.empty())
									{
										newBest = true;
									}
									else
									{
										if (ffd.ftLastWriteTime.dwHighDateTime > bestTime.dwHighDateTime)
										{
											newBest = true;
										}
										else if (ffd.ftLastWriteTime.dwHighDateTime == bestTime.dwHighDateTime)
										{
											newBest = ffd.ftLastWriteTime.dwLowDateTime > bestTime.dwLowDateTime;
										}
										else
										{
											newBest = false;
										}
									}

									if (newBest)
									{
										bestTime = ffd.ftLastWriteTime;
										bestPath = ffd.cFileName;
									}
								}
							} while (FindNextFile(hFind, &ffd) != 0);
						}

						if (false == bestPath.empty())
						{
							char szBuffer[MAX_PATH + 256];
							cry_sprintf(szBuffer, "explorer.exe %s\\%s\\%s", szWorkingDirectory, framesFolder.c_str(), bestPath.c_str());

							STARTUPINFO startupInfo;
							ZeroMemory(&startupInfo, sizeof(startupInfo));
							startupInfo.cb = sizeof(startupInfo);

							PROCESS_INFORMATION processInformation;
							ZeroMemory(&processInformation, sizeof(processInformation));

							CreateProcess(NULL, szBuffer, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &processInformation);
						}
					}
					break;
				}
#endif
			}

			if (processEvent)
			{
				if (!gEnv->bServer)
				{
					// Client: send RMI to server, which will then take care of broadcasting it to everybody
					g_pGame->GetGameRules()->GetGameObject()->InvokeRMI(CGameRules::SvModuleRMISingleEntity(), params, eRMI_ToServer);
				}
				else
				{
					// Server: Process immediately. Broadcasting handled internally
					OnSingleEntityRMI(params);
				}
			}
		}
	}
	return false;
}

void CManualFrameStepManager::OnSingleEntityRMI(CGameRules::SModuleRMIEntityParams params)
{
	switch (params.m_data)
	{
	case k_disableMask:
		Enable(false);
		break;
	case 0:
		Enable(true);
		break;
	default:
		Enable(true);
		RequestFrames(params.m_data);
		break;
	}

	if (gEnv->bServer && gEnv->bMultiplayer)
	{
		g_pGame->GetGameRules()->GetGameObject()->InvokeRMI(CGameRules::ClModuleRMISingleEntity(), params, eRMI_ToAllClients);
	}
}
