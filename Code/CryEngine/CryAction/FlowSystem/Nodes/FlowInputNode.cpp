// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   FlowInputNode.cpp
//  Version:     v1.00
//  Created:     13/10/2006 by AlexL.
//  Compilers:   Visual Studio.NET 2003
//  Description:
// -------------------------------------------------------------------------
//  History:
//  24.09.2012 - Dario Sancho
//               * Added new node ForceFeedbackTriggerTweaker (for Durango)
//
//  28.08.2015 - Marco Hopp
//               * Deprecated Input:ActionMapManager, Input:ActionListener, Input:ActionFilter
//               * new versions of those nodes can be found in FlowActionMapNodes.cpp
//               * TODO: Remove this code at next major release
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FlowBaseNode.h"
#include <CryAction.h>
#include <CryActionCVars.h>
#include <IGameObjectSystem.h>
#include <CryString/StringUtils.h>
#include <CryInput/IInput.h>
#include "ForceFeedbackSystem/ForceFeedbackSystem.h"
#include "ActionMap.h"

class CFlowNode_InputKey : public CFlowBaseNode<eNCT_Instanced>, public IInputEventListener
{
	enum INPUTS
	{
		EIP_ENABLE = 0,
		EIP_DISABLE,
		EIP_KEY,
		EIP_NONDEVMODE,
	};

	enum OUTPUTS
	{
		EOP_PRESSED,
		EOP_RELEASED,
	};

public:
	CFlowNode_InputKey(SActivationInfo* pActInfo) : m_bActive(false), m_bAllowedInNonDevMode(false)
	{
		m_eventKeyName.resize(64); // avoid delete/re-newing during execution - assuming no action keys are longer than 64 char
	}

	~CFlowNode_InputKey()
	{
		Register(false, 0);
	}

	void GetConfiguration(SFlowNodeConfig& config)
	{
		// declare input ports
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig_Void("Enable",      _HELP("Trigger to enable. Default: Already enabled.")),
			InputPortConfig_Void("Disable",     _HELP("Trigger to disable")),
			InputPortConfig<string>("Key",      _HELP("Key name. Leave empty for any.")),
			InputPortConfig<bool>("NonDevMode", false,                                                 _HELP("If set to true, can be used in Non-Devmode as well [Debugging backdoor]")),
			{ 0 }
		};
		static const SOutputPortConfig outputs[] =
		{
			OutputPortConfig<string>("Pressed",  _HELP("Key pressed.")),
			OutputPortConfig<string>("Released", _HELP("Key released.")),
			{ 0 }
		};

		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("FlowNode to catch key inputs. Use only for debugging. It is enabled by default. Entity Input need to be used in multiplayer.");
		config.SetCategory(EFLN_DEBUG);
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_InputKey(pActInfo);
	}

	void Register(bool bRegister, SActivationInfo* pActInfo)
	{
		IInput* pInput = gEnv->pInput;
		if (pInput)
		{
			if (bRegister)
			{
				assert(pActInfo != 0);
				m_bAllowedInNonDevMode = GetPortBool(pActInfo, EIP_NONDEVMODE);
				if (gEnv->pSystem->IsDevMode() || m_bAllowedInNonDevMode)
					pInput->AddEventListener(this);
			}
			else
				pInput->RemoveEventListener(this);
		}
		m_bActive = bRegister;
	}

	void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		ser.Value("m_bActive", m_bActive);
		if (ser.IsReading())
		{
			Register(m_bActive, pActInfo);
		}
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		if (InputEntityIsLocalPlayer(pActInfo))
		{
			switch (event)
			{
			case eFE_Initialize:
				m_actInfo = *pActInfo;
				Register(true, pActInfo);
				break;
			case eFE_Activate:
				m_actInfo = *pActInfo;
				if (IsPortActive(pActInfo, EIP_DISABLE))
					Register(false, pActInfo);
				if (IsPortActive(pActInfo, EIP_ENABLE))
					Register(true, pActInfo);
				break;
			}
		}
	}

	virtual bool OnInputEvent(const SInputEvent& event)
	{
		// Kevin - Ignore if console is up
		if (gEnv->pConsole->IsOpened())
			return false;

		m_eventKeyName = event.keyName.c_str();

		const string& keyName = GetPortString(&m_actInfo, EIP_KEY);

		if (keyName.empty() == false)
		{
			if (stricmp(keyName.c_str(), m_eventKeyName.c_str()) != 0)
				return false;
		}

		if (gEnv->pSystem->IsDevMode() && !gEnv->IsEditor() && CCryActionCVars::Get().g_disableInputKeyFlowNodeInDevMode != 0 && !m_bAllowedInNonDevMode)
			return false;

		const bool nullInputValue = (fabs_tpl(event.value) < 0.02f);
		if ((event.state == eIS_Pressed) || ((event.state == eIS_Changed) && !nullInputValue))
			ActivateOutput(&m_actInfo, EOP_PRESSED, m_eventKeyName);
		else if ((event.state == eIS_Released) || ((event.state == eIS_Changed) && !nullInputValue))
			ActivateOutput(&m_actInfo, EOP_RELEASED, m_eventKeyName);

		// return false, so other listeners get notification as well
		return false;
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

	SActivationInfo m_actInfo;
	bool            m_bActive;
	bool            m_bAllowedInNonDevMode;
	string          m_eventKeyName; // string is necessary for ActivateOutput, and it allocates in heap - so we use a member var for it
};

/////////////////////////////////////////////////////////////////////////////////

class CFlowNode_InputActionListenerOLD : public CFlowBaseNode<eNCT_Instanced>, public IInputEventListener
{
	enum INPUTS
	{
		EIP_ENABLE = 0,
		EIP_DISABLE,
		EIP_ACTION,
		EIP_ACTIONMAP,
		EIP_NONDEVMODE,
	};

	enum OUTPUTS
	{
		EOP_PRESSED,
		EOP_RELEASED,
	};

public:
	CFlowNode_InputActionListenerOLD(SActivationInfo* pActInfo) : m_bActive(false), m_bAnyAction(false)
	{
		m_eventKeyName.resize(64); // avoid delete/re-newing during execution - assuming no action keys are longer than 64 char
	}

	~CFlowNode_InputActionListenerOLD()
	{
		Register(false, 0);
	}

	void GetConfiguration(SFlowNodeConfig& config)
	{
		// declare input ports
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig_Void("Enable",       _HELP("Trigger to enable (default: disabled).")),
			InputPortConfig_Void("Disable",      _HELP("Trigger to disable")),
			InputPortConfig<string>("Action",    _HELP("Action input to trigger"),                _HELP("Action"),                                                                    _UICONFIG("enum_global:input_actions")),
			InputPortConfig<string>("ActionMap", _HELP("Action Map to use"),                      _HELP("Action Map"),                                                                _UICONFIG("enum_global:action_maps")),
			InputPortConfig<bool>("NonDevMode",  false,                                           _HELP("If set to true, can be used in Non-Devmode as well [Debugging backdoor]")),
			{ 0 }
		};
		static const SOutputPortConfig outputs[] =
		{
			OutputPortConfig<string>("Pressed",  _HELP("Action trigger pressed.")),
			OutputPortConfig<string>("Released", _HELP("Action trigger released.")),
			{ 0 }
		};

		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("FlowNode to catch input events via their action name. It is enabled by default. Entity Input need to be used in multiplayer.");
		config.SetCategory(EFLN_OBSOLETE);
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_InputActionListenerOLD(pActInfo);
	}

	void Register(bool bRegister, SActivationInfo* pActInfo)
	{
		IInput* pInput = gEnv->pInput;
		if (pInput)
		{
			if (bRegister)
			{
				assert(pActInfo != 0);
				//const bool bAllowedInNonDevMode = GetPortBool(pActInfo, EIP_NONDEVMODE );
				//if (gEnv->pSystem->IsDevMode() || bAllowedInNonDevMode)
				pInput->AddEventListener(this);
			}
			else
				pInput->RemoveEventListener(this);
		}
		m_bActive = bRegister;
	}

	void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		ser.Value("m_bActive", m_bActive);
		ser.Value("m_bAnyAction", m_bAnyAction);
		if (ser.IsReading())
		{
			Register(m_bActive, pActInfo);
		}
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		if (InputEntityIsLocalPlayer(pActInfo))
		{
			switch (event)
			{
			case eFE_Initialize:
				m_actInfo = *pActInfo;
				Register(false, pActInfo);
				break;

			case eFE_Activate:
				{
					m_actInfo = *pActInfo;
					if (IsPortActive(pActInfo, EIP_DISABLE))
						Register(false, pActInfo);
					if (IsPortActive(pActInfo, EIP_ENABLE))
					{
						Register(true, pActInfo);
					}
					break;
				}
			}
		}
	}

	virtual bool OnInputEvent(const SInputEvent& event)
	{
		// Kevin - Ignore if console is up
		if (gEnv->pConsole->IsOpened())
			return false;

		m_eventKeyName = event.keyName.c_str();

		// Removed caching of actions to avoid problems when rebinding keys if node is already active
		// Only a very slight performance hit without caching
		// TODO: Should be refactored to handle onAction instead of directly comparing inputs, caching won't be needed
		const string& actionName = GetPortString(&m_actInfo, EIP_ACTION);
		const string& actionMapName = GetPortString(&m_actInfo, EIP_ACTIONMAP);
		bool bActionFound = actionName.empty();
		m_bAnyAction = bActionFound;
		if (!bActionFound)
		{
			IActionMapManager* pAMMgr = gEnv->pGame->GetIGameFramework()->GetIActionMapManager();
			if (pAMMgr)
			{
				IActionMap* pAM = pAMMgr->GetActionMap(actionMapName);
				if (pAM)
				{
					const IActionMapAction* pAction = pAM->GetAction(ActionId(actionName));
					if (pAction)
					{
						const int iNumInputData = pAction->GetNumActionInputs();
						for (int i = 0; i < iNumInputData; ++i)
						{
							const SActionInput* pActionInput = pAction->GetActionInput(i);
							CRY_ASSERT(pActionInput != NULL);
							bActionFound = stricmp(pActionInput->input, m_eventKeyName.c_str()) == 0;

							if (bActionFound)
							{
								break;
							}
						}
					}
				}
			}
		}

		if (bActionFound)
		{
			bool nullInputValue = (fabs_tpl(event.value) < 0.02f);
			if ((event.state == eIS_Pressed) || ((event.state == eIS_Changed) && !nullInputValue))
				ActivateOutput(&m_actInfo, EOP_PRESSED, m_eventKeyName);
			else if ((event.state == eIS_Released) || ((event.state == eIS_Changed) && nullInputValue))
				ActivateOutput(&m_actInfo, EOP_RELEASED, m_eventKeyName);
		}

		// return false, so other listeners get notification as well
		return false;
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

	SActivationInfo m_actInfo;
	bool            m_bActive;
	bool            m_bAnyAction;
	string          m_eventKeyName; // string is necessary for ActivateOutput, and it allocates in heap - so we use a member var for it

};
//////////////////////////////////////////////////////////////////////
class CFlowNode_InputActionFilterOLD : public CFlowBaseNode<eNCT_Instanced>
{
	enum INPUTS
	{
		EIP_ENABLE = 0,
		EIP_DISABLE,
		EIP_FILTER
	};

	enum OUTPUTS
	{
		EOP_ENABLED,
		EOP_DISABLED,
	};

	void EnableFilter(bool bEnable, bool bActivateOutput = true)
	{
		if (m_filterName.empty())
			return;

		m_bEnabled = bEnable;

		IGameFramework* pGameFramework = gEnv->pGame ? gEnv->pGame->GetIGameFramework() : NULL;
		IActionMapManager* pAMMgr = pGameFramework ? pGameFramework->GetIActionMapManager() : NULL;
		if (pAMMgr)
		{
			pAMMgr->EnableFilter(m_filterName, bEnable);
			if (bActivateOutput && m_pActInfo && m_pActInfo->pGraph)
			{
				ActivateOutput(m_pActInfo, bEnable ? EOP_ENABLED : EOP_DISABLED, 1);
			}
		}
	}

public:
	CFlowNode_InputActionFilterOLD(SActivationInfo* pActInfo)
		: m_bEnabled(false)
		, m_pActInfo(pActInfo)
	{
	}

	~CFlowNode_InputActionFilterOLD()
	{
		EnableFilter(false, false);
	}

	void GetConfiguration(SFlowNodeConfig& config)
	{
		// declare input ports
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig_Void("Enable",    _HELP("Trigger to enable ActionFilter")),
			InputPortConfig_Void("Disable",   _HELP("Trigger to disable ActionFilter")),
			InputPortConfig<string>("Filter", _HELP("Action Filter"),                   0,_UICONFIG("enum_global:action_filter")),
			{ 0 }
		};
		static const SOutputPortConfig outputs[] =
		{
			OutputPortConfig_Void("Enabled",  _HELP("Triggered when enabled.")),
			OutputPortConfig_Void("Disabled", _HELP("Triggered when disabled.")),
			{ 0 }
		};

		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("FlowNode to enable/disable ActionFilters\rIf a player is not proved as input entity, the filter will affect all players");
		config.SetCategory(EFLN_OBSOLETE);
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_InputActionFilterOLD(pActInfo);
	}

	void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		m_pActInfo = pActInfo;
		// disable on reading
		if (ser.IsReading())
			EnableFilter(false);
		ser.Value("m_filterName", m_filterName);
		// on saving we ask the ActionMapManager if the filter is enabled
		// so, all basically all nodes referring to the same filter will write the correct (current) value
		// on quickload, all of them or none of them will enable/disable the filter again
		// maybe use a more central location for this
		if (ser.IsWriting())
		{
			bool bWroteIt = false;
			IActionMapManager* pAMMgr = gEnv->pGame->GetIGameFramework()->GetIActionMapManager();
			if (pAMMgr && m_filterName.empty() == false)
			{
				IActionFilter* pFilter = pAMMgr->GetActionFilter(m_filterName.c_str());
				if (pFilter)
				{
					bool bIsEnabled = pAMMgr->IsFilterEnabled(m_filterName.c_str());
					ser.Value("m_bEnabled", bIsEnabled);
					bWroteIt = true;
				}
			}
			if (!bWroteIt)
				ser.Value("m_bEnabled", m_bEnabled);
		}
		else
		{
			ser.Value("m_bEnabled", m_bEnabled);
		}
		// re-enable
		if (ser.IsReading())
			EnableFilter(m_bEnabled);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		m_pActInfo = pActInfo;
		switch (event)
		{
		case eFE_Initialize:
			EnableFilter(false);
			m_filterName = GetPortString(pActInfo, EIP_FILTER);
			break;
		case eFE_Activate:
			if (InputEntityIsLocalPlayer(pActInfo))
			{
				if (IsPortActive(pActInfo, EIP_FILTER))
				{
					m_filterName = GetPortString(pActInfo, EIP_FILTER);
				}
				else
				{
					EnableFilter(IsPortActive(pActInfo, EIP_ENABLE));
				}
			}
			break;
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

protected:
	bool             m_bEnabled;
	string           m_filterName; // we need to store this name, as it the d'tor we need to disable the filter, but don't have access to port names
	SActivationInfo* m_pActInfo;
};

//////////////////////////////////////////////////////////////////////
class CFlowNode_ActionMapManagerOLD : public CFlowBaseNode<eNCT_Instanced>
{
	enum INPUTS
	{
		EIP_ENABLE = 0,
		EIP_DISABLE,
		EIP_ACTIONMAP,
	};

public:
	CFlowNode_ActionMapManagerOLD(SActivationInfo* pActInfo)
		: m_pActInfo(pActInfo)
	{
	}

	~CFlowNode_ActionMapManagerOLD()
	{
	}

	void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig_Void("Enable",       _HELP("Trigger to enable (default: disabled).")),
			InputPortConfig_Void("Disable",      _HELP("Trigger to disable")),
			InputPortConfig<string>("ActionMap", _HELP("Action Map to use"),                      _HELP("Action Map"),_UICONFIG("enum_global:action_maps")),
			{ 0 }
		};

		config.pInputPorts = inputs;
		config.sDescription = _HELP("FlowNode to enable/disable actionmaps");
		config.SetCategory(EFLN_OBSOLETE);
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_ActionMapManagerOLD(pActInfo);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		m_pActInfo = pActInfo;
		switch (event)
		{
		case eFE_Activate:
			if (IsPortActive(pActInfo, EIP_ENABLE) || IsPortActive(pActInfo, EIP_DISABLE))
			{
				const bool bEnable = IsPortActive(pActInfo, EIP_ENABLE);
				const string actionMapName = GetPortString(pActInfo, EIP_ACTIONMAP);

				IGameFramework* pGameFramework = gEnv->pGame ? gEnv->pGame->GetIGameFramework() : NULL;
				IActionMapManager* pAMMgr = pGameFramework ? pGameFramework->GetIActionMapManager() : NULL;
				if (pAMMgr && !actionMapName.empty())
				{
					pAMMgr->EnableActionMap(actionMapName, bEnable);
				}
			}
			break;
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

protected:
	SActivationInfo* m_pActInfo;
};

//////////////////////////////////////////////////////////////////////////

class CFlowNode_ForceFeedback : public CFlowBaseNode<eNCT_Singleton>
{
	enum INPUTS
	{
		eIP_EffectName = 0,
		eIP_Play,
		eIP_Intensity,
		eIP_Delay,
		eIP_Stop,
		eIP_StopAll
	};

public:
	CFlowNode_ForceFeedback(SActivationInfo* pActInfo){}

	void GetConfiguration(SFlowNodeConfig& config)
	{
		// declare input ports
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig<string>("EffectName", _HELP("Name of the force feedback effect to be played/stopped"), _HELP("Effect Name"),                                          _UICONFIG("enum_global:forceFeedback")),
			InputPortConfig_Void("Play",          _HELP("Play the specified effect")),
			InputPortConfig<float>("Intensity",   1.f,                                                             _HELP("Intensity factor applied to the effect being played")),
			InputPortConfig<float>("Delay",       0.f,                                                             _HELP("Time delay to start the effect")),
			InputPortConfig_Void("Stop",          _HELP("Stop the specified effect")),
			InputPortConfig_Void("StopAll",       _HELP("Stop all currently playing force feedback effects")),
			{ 0 }
		};
		static const SOutputPortConfig outputs[] =
		{
			{ 0 }
		};

		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("FlowNode to play/stop force feedback effects");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Activate:
			{
				if (InputEntityIsLocalPlayer(pActInfo))
				{
					IForceFeedbackSystem* pForceFeedback = gEnv->pGame->GetIGameFramework()->GetIForceFeedbackSystem();

					if (IsPortActive(pActInfo, eIP_Play))
					{
						const char* effectName = GetPortString(pActInfo, eIP_EffectName).c_str();
						ForceFeedbackFxId fxId = pForceFeedback->GetEffectIdByName(effectName);
						const float intensity = GetPortFloat(pActInfo, eIP_Intensity);
						const float delay = GetPortFloat(pActInfo, eIP_Delay);
						pForceFeedback->PlayForceFeedbackEffect(fxId, SForceFeedbackRuntimeParams(intensity, delay));
					}
					if (IsPortActive(pActInfo, eIP_Stop))
					{
						const char* effectName = GetPortString(pActInfo, eIP_EffectName).c_str();
						ForceFeedbackFxId fxId = pForceFeedback->GetEffectIdByName(effectName);
						pForceFeedback->StopForceFeedbackEffect(fxId);
					}
					if (IsPortActive(pActInfo, eIP_StopAll))
					{
						pForceFeedback->StopAllEffects();
					}
				}
				break;
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_ForceFeedbackTweaker : public CFlowBaseNode<eNCT_Singleton>
{
	enum INPUTS
	{
		eIP_LowPassMultiplier,
		eIP_HighPassMultiplier,
		eIP_Start,
		eIP_Stop,
	};

public:
	CFlowNode_ForceFeedbackTweaker(SActivationInfo* pActInfo){}

	void GetConfiguration(SFlowNodeConfig& config)
	{
		// declare input ports
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig<float>("LowPass",     1.f,                                             _HELP("Multiplier applied to the low frequencey effect being played")),
			InputPortConfig<float>("HighPass",    1.f,                                             _HELP("Multiplier applied to the high frequencey effect being played")),
			InputPortConfig_AnyType("Activate",   _HELP("activate the forcefeedback tweaker")),
			InputPortConfig_AnyType("Deactivate", _HELP("deactivate the force feedback tweaker")),
			{ 0 }
		};

		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.sDescription = _HELP("FlowNode to control individual high and low frequency force feedback effect");
		config.SetCategory(EFLN_APPROVED);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, eIP_Start))
				{
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
				}
				if (IsPortActive(pActInfo, eIP_Stop))
				{
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
				}
				break;
			}

		case eFE_Update:
			{
				IForceFeedbackSystem* pForceFeedback = gEnv->pGame->GetIGameFramework()->GetIForceFeedbackSystem();
				if (pForceFeedback)
				{
					pForceFeedback->AddFrameCustomForceFeedback(GetPortFloat(pActInfo, eIP_HighPassMultiplier), GetPortFloat(pActInfo, eIP_LowPassMultiplier));
				}
				break;
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

//////////////////////////////////////////////////////////////////////////
class CFlowNode_ForceFeedbackTriggerTweaker : public CFlowBaseNode<eNCT_Singleton>
{
	enum INPUTS
	{
		eIP_LeftTouchToActivate,
		eIP_LeftGain,
		eIP_LeftEnvelope,

		eIP_RightTouchToActivate,
		eIP_RightGain,
		eIP_RightEnvelope,

		eIP_Activate,
		eIP_Deactivate,
	};

public:
	CFlowNode_ForceFeedbackTriggerTweaker(SActivationInfo* pActInfo){}

	void GetConfiguration(SFlowNodeConfig& config)
	{
		// declare input ports
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig<bool>("LeftTouchToActivate",  true,                                            _HELP("If true the left trigger's gain will be modulated by how much the trigger is pressed")),
			InputPortConfig<float>("LeftGain",            1.f,                                             _HELP("Gain sent to the left trigger's motor")),
			InputPortConfig<int>("LeftEnvelope",          1.f,                                             _HELP("Envelope sent to the left trigger's motor (multiple of 4, from 0 to 2000)")),
			InputPortConfig<bool>("RightTouchToActivate", true,                                            _HELP("If true the right trigger's gain will be modulated by how much the trigger is pressed")),
			InputPortConfig<float>("RightGain",           1.f,                                             _HELP("Gain sent to the right trigger's motor")),
			InputPortConfig<int>("RightEnvelope",         1.f,                                             _HELP("Envelope sent to the right trigger's motor (multiple of 4, from 0 to 2000)")),
			InputPortConfig_AnyType("Activate",           _HELP("activate the forcefeedback tweaker")),
			InputPortConfig_AnyType("Deactivate",         _HELP("deactivate the force feedback tweaker")),
			{ 0 }
		};

		config.nFlags |= EFLN_TARGET_ENTITY;
		config.pInputPorts = inputs;
		config.sDescription = _HELP("FlowNode to control force feedback effect on left and right triggers");
		config.SetCategory(EFLN_OBSOLETE);
	}

	void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, eIP_Activate))
				{
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
				}
				if (IsPortActive(pActInfo, eIP_Deactivate))
				{
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
				}
				break;
			}

		case eFE_Update:
			{
				IForceFeedbackSystem* pForceFeedback = gEnv->pGame->GetIGameFramework()->GetIForceFeedbackSystem();
				if (pForceFeedback)
				{
					pForceFeedback->AddCustomTriggerForceFeedback(SFFTriggerOutputData(
					                                                GetPortBool(pActInfo, eIP_LeftTouchToActivate),
					                                                GetPortBool(pActInfo, eIP_RightTouchToActivate),
					                                                GetPortFloat(pActInfo, eIP_LeftGain),
					                                                GetPortFloat(pActInfo, eIP_RightGain),
					                                                0.0f,
					                                                0.0f,
					                                                GetPortInt(pActInfo, eIP_LeftEnvelope),
					                                                GetPortInt(pActInfo, eIP_RightEnvelope)
					                                                )
					                                              );
				}
				break;
			}
		}
	}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}
};

REGISTER_FLOW_NODE("Input:Key", CFlowNode_InputKey);
REGISTER_FLOW_NODE("Input:ActionMapManager", CFlowNode_ActionMapManagerOLD);
REGISTER_FLOW_NODE("Input:ActionListener", CFlowNode_InputActionListenerOLD);
REGISTER_FLOW_NODE("Input:ActionFilter", CFlowNode_InputActionFilterOLD);
REGISTER_FLOW_NODE("Game:ForceFeedback", CFlowNode_ForceFeedback);
REGISTER_FLOW_NODE("Game:ForceFeedbackTweaker", CFlowNode_ForceFeedbackTweaker);
REGISTER_FLOW_NODE("Game:ForceFeedbackTriggerTweaker", CFlowNode_ForceFeedbackTriggerTweaker);
