// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

//
// -------------------------------------------------------------------------
//  File name:   FlowAudioTriggerNode.cpp
//  Version:     v1.00
//  Created:     14/05/2014 by Mikhail Korotyaev
//  Description: FlowGraph Node that sends an output upon leaving the game (either in Editor or in Pure Game mode).
//
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include <CryFlowGraph/IFlowBaseNode.h>

class CFlowNode_Stop : public CFlowBaseNode<eNCT_Instanced>
{
public:

	CFlowNode_Stop(SActivationInfo* pActInfo)
		: m_bActive(false)
	{}

	//////////////////////////////////////////////////////////////////////////
	~CFlowNode_Stop() {}

	//////////////////////////////////////////////////////////////////////////
	virtual IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new CFlowNode_Stop(pActInfo);
	}

	//////////////////////////////////////////////////////////////////////////
	enum INPUTS
	{
		eIn_TriggerInGame,
		eIn_TriggerInEditor,
	};

	enum OUTPUTS
	{
		eOut_Output,
	};

	//////////////////////////////////////////////////////////////////////////
	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] =
		{
			InputPortConfig<bool>("TriggerInGame",   true, _HELP("Determines if the output should be triggered in the PureGameMode"),   "InGame"),
			InputPortConfig<bool>("TriggerInEditor", true, _HELP("Determines if the output should be triggered in the EditorGameMode"), "InEditor"),
			{ 0 }
		};

		static const SOutputPortConfig outputs[] =
		{
			OutputPortConfig_Void("Output", _HELP("Produces output on game stop"), "output"),
			{ 0 }
		};

		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("This node sends output when leaving the game.");
		config.SetCategory(EFLN_APPROVED);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		bool bActive = m_bActive;
		ser.Value("active", bActive);

		if (ser.IsReading())
		{
			m_bActive = false;
			SetState(pActInfo, bActive);
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		SFlowAddress addr(pActInfo->myID, eOut_Output, true);

		switch (event)
		{
		case eFE_Update:
			{
				if (gEnv->IsEditor())
				{
					if (gEnv->IsEditing())
					{
						pActInfo->pGraph->ActivatePort(addr, true);
						SetState(pActInfo, false);
					}
				}
				else
				{
					// when we're in pure game mode
					if (gEnv->pSystem->IsQuitting()) //Is this too late?
					{
						pActInfo->pGraph->ActivatePort(addr, true);
						SetState(pActInfo, false);
					}
				}

				break;
			}

		case eFE_Initialize:
			{
				SetState(pActInfo, true);
				break;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

private:

	//////////////////////////////////////////////////////////////////////////
	bool CanTriggerInGame(SActivationInfo* const pActInfo) const
	{
		return *(pActInfo->GetInputPort(eIn_TriggerInGame).GetPtr<bool>());
	}

	//////////////////////////////////////////////////////////////////////////
	bool CanTriggerInEditor(SActivationInfo* const pActInfo) const
	{
		return *(pActInfo->GetInputPort(eIn_TriggerInEditor).GetPtr<bool>());
	}

	//////////////////////////////////////////////////////////////////////////
	void SetState(SActivationInfo* const pActInfo, bool const bActive)
	{
		if (m_bActive != bActive)
		{
			m_bActive = bActive;
			pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, m_bActive);
		}
	}

	bool m_bActive;
};

REGISTER_FLOW_NODE("Game:Stop", CFlowNode_Stop);
