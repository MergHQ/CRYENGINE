// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
-------------------------------------------------------------------------
$Id$
$DateTime$
Description:  Flow nodes for working with XBox 360 controller
-------------------------------------------------------------------------
History:
- 31:3:2008        : Created by Kevin

*************************************************************************/

#include "StdAfx.h"
#include "Nodes/G2FlowBaseNode.h"

class CFlowYesNoGesture : public CFlowBaseNode<eNCT_Instanced>, public IInputEventListener
{
public:
	CFlowYesNoGesture(SActivationInfo * pActInfo)
	{
		m_bActive = false;
		m_bFinished = false;
		m_activationTime.SetValue(0);
		Reset();
	}

	~CFlowYesNoGesture()
	{
		Register(false);
	}

	IFlowNodePtr Clone( SActivationInfo * pActInfo )
	{
		return new CFlowYesNoGesture(pActInfo);
	}

	virtual void GetMemoryUsage(ICrySizer * s) const
	{
		s->Add(*this);
	}

	virtual void Serialize(SActivationInfo *pActInfo, TSerialize ser)
	{
		ser.Value("m_bActive", m_bActive);
		if (ser.IsReading())
		{
			m_actInfo = *pActInfo;
			Register(m_bActive);
		}
	}

	enum EInputPorts
	{
		EIP_Enable = 0,
		EIP_Disable,
		EIP_Timeout,
		EIP_StickDeadThreshold,
		EIP_StickExtremeThreshold,
		EIP_MinExtremeDuration,
		EIP_MaxExtremeDuration,
		EIP_MinTotalDuration,
		EIP_MaxTotalDuration,
	};

	enum EOutputPorts
	{
		EOP_Yes = 0,
		EOP_No,
		EOP_TimedOut,
	};

	virtual void GetConfiguration(SFlowNodeConfig& config)
	{
		static const SInputPortConfig inputs[] = {
			InputPortConfig_Void("Enable", _HELP("Enable reporting")),
			InputPortConfig_Void("Disable", _HELP("Disable reporting")),
			InputPortConfig<float>("Timeout", 5.0f, _HELP("Maximum time gesture is waited for after node is activated.")),
			InputPortConfig<float>("StickDeadThreshold", 0.2f,_HELP("Stick deadzone tilt threshold.")),
			InputPortConfig<float>("StickExtremeThreshold", 0.7f,_HELP("Stick extreme tilt threshold.")),
			InputPortConfig<float>("MinExtremeDuration", 0.2f, _HELP("Minimum duration of stick being at extreme tilt")),
			InputPortConfig<float>("MaxExtremeDuration", 1.0f, _HELP("Maximum duration of stick being at extreme tilt")),
			InputPortConfig<float>("MinTotalDuration", 0.5f, _HELP("Minimum duration of whole gesture sequence.")),
			InputPortConfig<float>("MaxTotalDuration", 2.0f, _HELP("Maximum duration of whole gesture sequence.")),
			{0}
		};
		static const SOutputPortConfig outputs[] = {
			OutputPortConfig<bool>("Yes", _HELP("A yes gesture was identified.")),
			OutputPortConfig<bool>("No", _HELP("A no gesture was identified.")),
			OutputPortConfig<bool>("TimedOut", _HELP("No gesture was identified between activation and timeout.")),
			{0}
		};
		config.pInputPorts = inputs;
		config.pOutputPorts = outputs;
		config.sDescription = _HELP("Identifies Yes/No gestures from right analogue stick (nod/shake).");
		config.SetCategory(EFLN_DEBUG);
	}

	virtual void ProcessEvent( EFlowEvent event, SActivationInfo *pActInfo )
	{
		switch (event)
		{
		case eFE_Initialize:
			{
				m_bActive = false;
				m_actInfo = *pActInfo;
				Register(true);
			}
			break;

		case eFE_Activate:
			{
				if (IsPortActive(pActInfo, EIP_Enable))
					Register(true);
				if (IsPortActive(pActInfo, EIP_Disable))
					Register(false);
			}
			break;

		case eFE_Update:
			{
				if (!m_bFinished)
				{
					float fTimeout = GetPortFloat(&m_actInfo, EIP_Timeout);
					CTimeValue curTime = gEnv->pTimer->GetFrameStartTime();
					float elapsedTime = (curTime - m_activationTime).GetSeconds();
					if (elapsedTime > fTimeout)
					{
						Finish();
						ActivateOutput(&m_actInfo, EOP_TimedOut, true);
					}
				}
			}
			break;
		}

	}

	void Finish()
	{
		m_bFinished = true;
		NeedUpdate(false);
	}

	void NeedUpdate(bool update)
	{
		if (m_actInfo.pGraph != NULL)
			m_actInfo.pGraph->SetRegularlyUpdated(m_actInfo.myID, update);
	}

	void Register(bool bRegister)
	{
		m_activationTime = gEnv->pTimer->GetFrameStartTime();
		m_bFinished = false;

		if (IInput *pInput = gEnv->pSystem->GetIInput())
		{
			Reset();
			if (bRegister)
			{
				if (!m_bActive)
				{
					pInput->AddEventListener(this);
					NeedUpdate(true);
				}
			}
			else
			{
				pInput->RemoveEventListener(this);
				NeedUpdate(false);
			}
			m_bActive = bRegister;
		}
		else
			m_bActive = false;
	}

	// ~IInputEventListener
	virtual bool OnInputEvent( const SInputEvent &event )
	{
		if (!m_bActive)
			return false;

		if (m_bFinished)
			return false;

		if (event.state != eIS_Changed)
			return false;

		if (event.keyId == eKI_XI_ThumbRX)
		{
			m_stick.x = event.value;
			Update();
			return true;
		}
		else if (event.keyId == eKI_XI_ThumbRY)
		{
			m_stick.y = event.value;
			Update();
			return true;
		}

		// Let other listeners handle it
		return false;
	}

	void ResetX()
	{
		m_iLastStartDeadX = -1;
		m_iFirstEndDeadX = -1;
		m_iMinStartX = -1;
		m_iMinEndX = -1;
		m_iMaxStartX = -1;
		m_iMaxEndX = -1;
	}

	void ResetY()
	{
		m_iLastStartDeadY = -1;
		m_iFirstEndDeadY = -1;
		m_iMinStartY = -1;
		m_iMinEndY = -1;
		m_iMaxStartY = -1;
		m_iMaxEndY = -1;
	}

	void Reset()
	{
		m_stick.zero();
		m_stickHistory.clear();
		m_stickHistoryTime.clear();
		ResetX();
		ResetY();
	}

	float GetDuration(int iA, int iB)
	{
		CTimeValue dt = m_stickHistoryTime[iB] - m_stickHistoryTime[iA];
		return fabs_tpl(dt.GetSeconds());
	}

	void Update()
	{
		if (m_bFinished)
			return;

		CTimeValue curTime = gEnv->pTimer->GetFrameStartTime();
		m_stickHistory.push_back(m_stick);
		m_stickHistoryTime.push_back(curTime);

		float fStickDeadThreshold = GetPortFloat(&m_actInfo, EIP_StickDeadThreshold);
		float fStickExtremeThreshold = GetPortFloat(&m_actInfo, EIP_StickExtremeThreshold);
		float fMinExtremeDuration = GetPortFloat(&m_actInfo, EIP_MinExtremeDuration);
		float fMaxExtremeDuration = GetPortFloat(&m_actInfo, EIP_MaxExtremeDuration);
		float fMinTotalDuration = GetPortFloat(&m_actInfo, EIP_MinTotalDuration);
		float fMaxTotalDuration = GetPortFloat(&m_actInfo, EIP_MaxTotalDuration);

		float apexDurationX = 0.0f;
		float apexDurationY = 0.0f;

		ResetX();
		ResetY();

		for (int i = 0; i < (int)m_stickHistory.size(); ++i)
		{
			if (m_iMinStartX >= 0)
			{
				if ((m_stickHistory[i].x > -fStickExtremeThreshold) && (m_iMinEndX < 0))
					m_iMinEndX = i;
			}
			else
			{
				if (m_stickHistory[i].x < -fStickExtremeThreshold)
					m_iMinStartX = i;
			}
			if (m_iMaxStartX >= 0)
			{
				if ((m_stickHistory[i].x < fStickExtremeThreshold) && (m_iMaxEndX < 0))
					m_iMaxEndX = i;
			}
			else
			{
				if (m_stickHistory[i].x > fStickExtremeThreshold)
					m_iMaxStartX = i;
			}

			if (m_iMinStartY >= 0)
			{
				if ((m_stickHistory[i].y > -fStickExtremeThreshold) && (m_iMinEndY < 0))
					m_iMinEndY = i;
			}
			else
			{
				if (m_stickHistory[i].y < -fStickExtremeThreshold)
					m_iMinStartY = i;
			}
			if (m_iMaxStartY >= 0)
			{
				if ((m_stickHistory[i].y < fStickExtremeThreshold) && (m_iMaxEndY < 0))
					m_iMaxEndY = i;
			}
			else
			{
				if (m_stickHistory[i].y > fStickExtremeThreshold)
					m_iMaxStartY = i;
			}

			if (m_iMinEndX >= 0)
			{
				float duration = GetDuration(m_iMinStartX, m_iMinEndX);
				if ((duration < fMinExtremeDuration) || (duration > fMaxExtremeDuration))
					ResetX();
			}

			if (m_iMaxEndX >= 0)
			{
				float duration = GetDuration(m_iMaxStartX, m_iMaxEndX);
				if ((duration < fMinExtremeDuration) || (duration > fMaxExtremeDuration))
					ResetX();
			}
			if (m_iMinEndY >= 0)
			{
				float duration = GetDuration(m_iMinStartY, m_iMinEndY);
				if ((duration < fMinExtremeDuration) || (duration > fMaxExtremeDuration))
					ResetY();
			}

			if (m_iMaxEndY >= 0)
			{
				float duration = GetDuration(m_iMaxStartY, m_iMaxEndY);
				if ((duration < fMinExtremeDuration) || (duration > fMaxExtremeDuration))
					ResetY();
			}

			if ((m_iMinStartX < 0) && (m_iMaxStartX < 0))
			{
				if ((m_stickHistory[i].x > -fStickDeadThreshold) && (m_stickHistory[i].x < fStickDeadThreshold))
					m_iLastStartDeadX = i;
			}
			if ((m_iMinStartY < 0) && (m_iMaxStartY < 0))
			{
				if ((m_stickHistory[i].y > -fStickDeadThreshold) && (m_stickHistory[i].y < fStickDeadThreshold))
					m_iLastStartDeadY = i;
			}

			if (m_iFirstEndDeadX < 0)
				if ((m_iMinEndX >= 0) && (m_iMaxEndX >= 0))
					if ((m_stickHistory[i].x > -fStickDeadThreshold) && (m_stickHistory[i].x < fStickDeadThreshold))
						m_iFirstEndDeadX = i;

			if (m_iFirstEndDeadY < 0)
				if ((m_iMinEndY >= 0) && (m_iMaxEndY >= 0))
					if ((m_stickHistory[i].y > -fStickDeadThreshold) && (m_stickHistory[i].y < fStickDeadThreshold))
						m_iFirstEndDeadY = i;

			if ((m_iLastStartDeadX >= 0) && (m_iFirstEndDeadX >= 0))
			{
				float duration = GetDuration(m_iLastStartDeadX, m_iFirstEndDeadX);
				if ((duration < fMinTotalDuration) || (duration > fMaxTotalDuration))
					ResetX();
			}

			if ((m_iLastStartDeadY >= 0) && (m_iFirstEndDeadY >= 0))
			{
				float duration = GetDuration(m_iLastStartDeadY, m_iFirstEndDeadY);
				if ((duration < fMinTotalDuration) || (duration > fMaxTotalDuration))
					ResetY();
			}
		}

		int iYes = m_iFirstEndDeadY;
		int iNo = m_iFirstEndDeadX;
		bool bYes = (iYes >= 0);
		bool bNo = (iNo >= 0);

		if (bYes && !bNo)
		{
			Reset();
			Finish();
			ActivateOutput(&m_actInfo, EOP_Yes, true);
		}
		if (bNo && !bYes)
		{
			Reset();
			Finish();
			ActivateOutput(&m_actInfo, EOP_No, true);
		}
		if (bYes && bNo)
		{
			Reset();
		}
	}

private:
	CTimeValue m_activationTime;
	int m_iLastStartDeadX;
	int m_iLastStartDeadY;
	int m_iFirstEndDeadX;
	int m_iFirstEndDeadY;
	int m_iMinStartX;
	int m_iMinEndX;
	int m_iMaxStartX;
	int m_iMaxEndX;
	int m_iMinStartY;
	int m_iMinEndY;
	int m_iMaxStartY;
	int m_iMaxEndY;
	std::vector<Vec2> m_stickHistory;
	std::vector<CTimeValue> m_stickHistoryTime;
	Vec2 m_stick;
	bool m_bActive; // TRUE when node is enabled
	bool m_bFinished;
	SActivationInfo m_actInfo; // Activation info instance
};

REGISTER_FLOW_NODE("Input:YesNoGesture", CFlowYesNoGesture);
