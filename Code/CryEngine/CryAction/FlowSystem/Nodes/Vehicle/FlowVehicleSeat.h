// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "FlowVehicleBase.h"

class CFlowVehicleSeat
	: public CFlowVehicleBase
{
public:

	CFlowVehicleSeat(SActivationInfo* pActivationInfo) {}
	~CFlowVehicleSeat() { Delete(); }

	// CFlowBaseNode
	virtual IFlowNodePtr Clone(SActivationInfo* pActivationInfo);
	virtual void         GetConfiguration(SFlowNodeConfig& nodeConfig);
	virtual void         ProcessEvent(EFlowEvent flowEvent, SActivationInfo* pActivationInfo);
	// ~CFlowBaseNode

	// IVehicleEventListener
	virtual void OnVehicleEvent(EVehicleEvent event, const SVehicleEventParams& params) {}
	// ~IVehicleEventListener

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

protected:

	void ActivateOutputPorts(SActivationInfo* pActivationInfo);

	enum EInputs
	{
		eIn_Trigger,
		eIn_Seat,
		eIn_Lock,
		eIn_LockType,
	};

	enum EOutputs
	{
		eOut_Success,
	};
};
