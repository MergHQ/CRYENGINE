// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   FlowLogInput.h
//  Version:     v1.00
//  Created:     9/5/2005 by Timur.
//  Compilers:   Visual Studio.NET 2003
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FlowBaseNode.h"

class CFlowLogInput : public CFlowBaseNode<eNCT_Singleton>
{
public:
	CFlowLogInput(SActivationInfo* pActInfo) {}

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

	//////////////////////////////////////////////////////////////////////////
	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_AnyType("in", "Any value to be logged"),
			{ 0 }
		};
		static const SOutputPortConfig out_config[] = {
			{ 0 }
		};

		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_ADVANCED);
	}
	//////////////////////////////////////////////////////////////////////////
	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		if (event == eFE_Activate && IsPortActive(pActInfo, 0))
		{
			//			const TFlowInputData &in = pActInfo->pInputPorts[0];
			string strValue;
			pActInfo->pInputPorts[0].GetValueWithConversion(strValue);
			const char* sName = pActInfo->pGraph->GetNodeName(pActInfo->myID);
			CryLogAlways("[flow-log] Node%s: Value=%s", sName, strValue.c_str());
			/*
			   switch (in.GetType())
			   {
			   case eFDT_Bool:
			   port.pVar->Set( *pPort->defaultData.GetPtr<bool>() );
			   break;
			   case eFDT_Int:
			   port.pVar->Set( *pPort->defaultData.GetPtr<int>() );
			   break;
			   case eFDT_Float:
			   port.pVar->Set( *pPort->defaultData.GetPtr<float>() );
			   break;
			   case eFDT_EntityId:
			   port.pVar->Set( *pPort->defaultData.GetPtr<int>() );
			   break;
			   case eFDT_Vec3:
			   port.pVar->Set( *pPort->defaultData.GetPtr<Vec3>() );
			   break;
			   case eFDT_String:
			   port.pVar->Set( (pPort->defaultData.GetPtr<string>())->c_str() );
			   break;
			   default:
			   CryLogAlways( "Unknown Type" );
			   }
			 */
		}
	}
	//////////////////////////////////////////////////////////////////////////
};

REGISTER_FLOW_NODE("Log:LogInput", CFlowLogInput)
