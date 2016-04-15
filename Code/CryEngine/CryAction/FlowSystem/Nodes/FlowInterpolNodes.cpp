// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

// -------------------------------------------------------------------------
//  File name:   FlowInterpolNodes.cpp
//  Version:     v1.00
//  Created:     27/4/2006 by AlexL.
//  Compilers:   Visual Studio.NET 2005
//  Description:
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "FlowBaseNode.h"

template<typename T> struct Limits
{
	static const char* min_name;
	static const char* max_name;
	static const T     min_val;
	static const T     max_val;
};

template<typename T> struct LimitsColor
{
	static const char* min_name;
	static const char* max_name;
	static const T     min_val;
	static const T     max_val;
};

template<typename T> const char* Limits<T>::min_name = "StartValue";
template<typename T> const char* Limits<T>::max_name = "EndValue";

template<> const float Limits<float>::min_val = 0.0f;
template<> const float Limits<float>::max_val = 1.0f;
template<> const int Limits<int>::min_val = 0;
template<> const int Limits<int>::max_val = 255;
template<> const Vec3 Limits<Vec3>::min_val = Vec3(0.0f, 0.0f, 0.0f);
template<> const Vec3 Limits<Vec3>::max_val = Vec3(1.0f, 1.0f, 1.0f);
template<> const Vec3 LimitsColor<Vec3>::min_val = Vec3(0.0f, 0.0f, 0.0f);
template<> const Vec3 LimitsColor<Vec3>::max_val = Vec3(1.0f, 1.0f, 1.0f);
template<> const char* LimitsColor<Vec3>::min_name = "color_StartValue";
template<> const char* LimitsColor<Vec3>::max_name = "color_EndValue";

//////////////////////////////////////////////////////////////////////////
template<class T, class L = Limits<T>>
class CFlowInterpolNode : public CFlowBaseNode<eNCT_Instanced>
{
	typedef CFlowInterpolNode<T, L> Self;
public:
	enum EInputs
	{
		EIP_Start,
		EIP_Stop,
		EIP_StartValue,
		EIP_EndValue,
		EIP_Time,
		EIP_UpdateFrequency
	};
	enum EOutputs
	{
		EOP_Value,
		EOP_Done
	};

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

	CFlowInterpolNode(SActivationInfo* pActInfo)
		: m_startTime(0.0f),
		m_endTime(0.0f),
		m_startValue(L::min_val),
		m_endValue(L::min_val),
		m_updateFrequency(0.0f),
		m_lastUpdateTime(0.0f)
	{
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new Self(pActInfo);
	}

	virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		ser.BeginGroup("Local");
		ser.Value("m_startTime", m_startTime);
		ser.Value("m_endTime", m_endTime);
		ser.Value("m_startValue", m_startValue);
		ser.Value("m_endValue", m_endValue);
		ser.Value("m_updateFrequency", m_updateFrequency);
		ser.Value("m_lastUpdateTime", m_lastUpdateTime);
		ser.EndGroup();
		// regular update is handled by FlowGraph serialization
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig_Void("Start",             _HELP("StartInterpol")),
			InputPortConfig_Void("Stop",              _HELP("StopInterpol")),
			InputPortConfig<T>(L::min_name,           L::min_val,             _HELP("Start value")),
			InputPortConfig<T>(L::max_name,           L::max_val,             _HELP("End value")),
			InputPortConfig<float>("Time",            1.0f,                   _HELP("Time in seconds")),
			InputPortConfig<float>("UpdateFrequency", 0.0f,                   _HELP("Update frequency in seconds (0 = every frame)")),
			{ 0 }
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<T>("Value",  _HELP("Current Value")),
			OutputPortConfig_Void("Done", _HELP("Triggered when finished")),
			{ 0 }
		};

		config.sDescription = _HELP("Interpol nodes can be used to linearly interpolate from an initial value to an end value within a given time frame.");
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}

	bool GetValue(SActivationInfo* pActInfo, int nPort, T& value)
	{
		T* pVal = (pActInfo->pInputPorts[nPort].GetPtr<T>());
		if (pVal)
		{
			value = *pVal;
			return true;
		}
		return false;
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
			break;
		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, EIP_Stop))
				{
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
					ActivateOutput(pActInfo, EOP_Done, true);
				}
				if (IsPortActive(pActInfo, EIP_Start))
				{
					m_startTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
					m_endTime = m_startTime + GetPortFloat(pActInfo, EIP_Time) * 1000.0f;
					GetValue(pActInfo, EIP_StartValue, m_startValue);
					GetValue(pActInfo, EIP_EndValue, m_endValue);
					m_updateFrequency = GetPortFloat(pActInfo, EIP_UpdateFrequency) * 1000.0f;
					m_lastUpdateTime = m_startTime;
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
					Interpol(pActInfo, 0.0f);
				}
			}
			break;
		case eFE_Update:
			{
				const float fTime = gEnv->pTimer->GetFrameStartTime().GetMilliSeconds();
				if (fTime >= m_endTime)
				{
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
					ActivateOutput(pActInfo, EOP_Done, true);
					Interpol(pActInfo, 1.0f);
					m_lastUpdateTime = fTime;
				}
				else if ((fTime - m_lastUpdateTime) >= m_updateFrequency)
				{
					const float fDuration = m_endTime - m_startTime;
					float fPosition;
					if (fDuration <= 0.0f)
						fPosition = 1.0f;
					else
					{
						fPosition = (fTime - m_startTime) / fDuration;
						fPosition = clamp_tpl(fPosition, 0.0f, 1.0f);
					}
					Interpol(pActInfo, fPosition);
					m_lastUpdateTime = fTime;
				}
			}
			break;
		}
	}

protected:
	void Interpol(SActivationInfo* pActInfo, const float fPosition)
	{
		T value = ::Lerp(m_startValue, m_endValue, fPosition);
		ActivateOutput(pActInfo, EOP_Value, value);
	}

	float m_startTime;
	float m_endTime;
	T     m_startValue;
	T     m_endValue;
	float m_updateFrequency, m_lastUpdateTime;
};

namespace FlowInterpolNodes
{
template<class T>
bool valueReached(const T& currVal, const T& toVal, const T& minDistToVal)
{
	return currVal == toVal;
}

template<>
bool valueReached(const float& currVal, const float& toVal, const float& minDistToVal)
{
	return fabsf(currVal - toVal) <= minDistToVal;
}

template<>
bool valueReached(const Vec3& currVal, const Vec3& toVal, const Vec3& minDistToVal)
{
	bool bdone = fabsf(currVal.x - toVal.x) <= minDistToVal.x &&
	             fabsf(currVal.y - toVal.y) <= minDistToVal.y &&
	             fabsf(currVal.z - toVal.z) <= minDistToVal.z;
	return bdone;
}

template<class T>
T CalcMinDistToVal(const T& startVal, const T& toVal)
{
	return 0;
}

template<>
float CalcMinDistToVal(const float& startVal, const float& toVal)
{
	const float FACTOR = (1 / 100.f);
	const float MIN_DIST = 0.01f;
	return max(fabsf(toVal - startVal) * FACTOR, MIN_DIST);
}

template<>
Vec3 CalcMinDistToVal(const Vec3& startVal, const Vec3& toVal)
{
	const float FACTOR = (1 / 100.f);
	const float MIN_DIST = 0.01f;
	Vec3 calc(max(fabsf(toVal.x - startVal.x) * FACTOR, MIN_DIST),
	          max(fabsf(toVal.y - startVal.y) * FACTOR, MIN_DIST),
	          max(fabsf(toVal.z - startVal.z) * FACTOR, MIN_DIST)
	          );
	return calc;
}
}

//////////////////////////////////////////////////////////////////////////
template<class T, class L = Limits<T>>
class CFlowSmoothNode : public CFlowBaseNode<eNCT_Instanced>
{
	typedef CFlowSmoothNode<T, L> Self;
public:
	enum EInputs
	{
		EIP_InitialValue,
		EIP_Value,
		EIP_Time
	};
	enum EOutputs
	{
		EOP_Value,
		EOP_Done
	};

	virtual void GetMemoryUsage(ICrySizer* s) const
	{
		s->Add(*this);
	}

	CFlowSmoothNode(SActivationInfo* pActInfo)
		: m_val(L::min_val),
		m_valRate(L::min_val),
		m_minDistToVal(L::min_val)
	{
	}

	IFlowNodePtr Clone(SActivationInfo* pActInfo)
	{
		return new Self(pActInfo);
	}

	virtual void Serialize(SActivationInfo* pActInfo, TSerialize ser)
	{
		ser.BeginGroup("Local");
		ser.Value("m_val", m_val);
		ser.Value("m_valRate", m_valRate);
		ser.Value("m_minDistToVal", m_minDistToVal);
		ser.EndGroup();
		// regular update is handled by FlowGraph serialization
	}

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig in_config[] = {
			InputPortConfig<T>("InitialValue", L::min_val, _HELP("Initial value")),
			InputPortConfig<T>("Value",        L::min_val, _HELP("Target value")),
			InputPortConfig<float>("Time",     1.0f,       _HELP("Smoothing time (seconds)")),
			{ 0 }
		};
		static const SOutputPortConfig out_config[] = {
			OutputPortConfig<T>("Value",  _HELP("Current Value")),
			OutputPortConfig_Void("Done", _HELP("Triggered when finished")),
			{ 0 }
		};

		config.sDescription = _HELP("Smooth nodes can be used to interpolate in a non-linear way (damped spring system) from an initial value to an end value within a given time frame. Calculation will slow as it reaches the end value.");
		config.pInputPorts = in_config;
		config.pOutputPorts = out_config;
		config.SetCategory(EFLN_APPROVED);
	}

	bool GetValue(SActivationInfo* pActInfo, int nPort, T& value)
	{
		T* pVal = (pActInfo->pInputPorts[nPort].GetPtr<T>());
		if (pVal)
		{
			value = *pVal;
			return true;
		}
		return false;
	}

	virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				GetValue(pActInfo, EIP_InitialValue, m_val);
				m_valRate = L::min_val;
				pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
				break;
			}

		case eFE_Activate:
			{
				T toValue;
				GetValue(pActInfo, EIP_Value, toValue);
				if (IsPortActive(pActInfo, EIP_Value))
				{
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
					m_minDistToVal = FlowInterpolNodes::CalcMinDistToVal(m_val, toValue);
				}
				if (IsPortActive(pActInfo, EIP_InitialValue))
				{
					GetValue(pActInfo, EIP_InitialValue, m_val);
					m_minDistToVal = FlowInterpolNodes::CalcMinDistToVal(m_val, toValue);
				}
				break;
			}

		case eFE_Update:
			{
				T toValue;
				GetValue(pActInfo, EIP_Value, toValue);
				SmoothCD(m_val, m_valRate, gEnv->pTimer->GetFrameTime(), toValue, GetPortFloat(pActInfo, EIP_Time));
				if (FlowInterpolNodes::valueReached(m_val, toValue, m_minDistToVal))
				{
					pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, false);
					ActivateOutput(pActInfo, EOP_Done, true);
					m_val = toValue;
					m_valRate = L::min_val;
				}
				ActivateOutput(pActInfo, EOP_Value, m_val);
				break;
			}
		}
	}

	T m_val;
	T m_valRate;
	T m_minDistToVal;  // used to check when the value is so near that it must be considered reached
};

typedef CFlowInterpolNode<int>                     IntInterpol;
typedef CFlowInterpolNode<float>                   FloatInterpol;
typedef CFlowInterpolNode<Vec3>                    Vec3Interpol;
typedef CFlowInterpolNode<Vec3, LimitsColor<Vec3>> ColorInterpol;

REGISTER_FLOW_NODE("Interpol:Int", IntInterpol);
REGISTER_FLOW_NODE("Interpol:Float", FloatInterpol);
REGISTER_FLOW_NODE("Interpol:Vec3", Vec3Interpol);
REGISTER_FLOW_NODE("Interpol:Color", ColorInterpol);

typedef CFlowSmoothNode<int>   SmoothInt;
typedef CFlowSmoothNode<float> SmoothFloat;
typedef CFlowSmoothNode<Vec3>  SmoothVec3;
typedef CFlowSmoothNode<Vec3>  SmoothColor;

REGISTER_FLOW_NODE("Interpol:SmoothInt", SmoothInt);
REGISTER_FLOW_NODE("Interpol:SmoothFloat", SmoothFloat);
REGISTER_FLOW_NODE("Interpol:SmoothVec3", SmoothVec3);
REGISTER_FLOW_NODE("Interpol:SmoothColor", SmoothColor);
