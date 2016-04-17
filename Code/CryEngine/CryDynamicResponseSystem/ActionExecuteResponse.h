// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryString/HashedString.h>
#include <CryDynamicResponseSystem/IDynamicResponseAction.h>
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>

namespace CryDRS
{
class CActionExecuteResponse final : public DRS::IResponseAction
{
public:
	//////////////////////////////////////////////////////////
	// IResponseAction implementation
	virtual DRS::IResponseActionInstanceUniquePtr Execute(DRS::IResponseInstance* pResponseInstance) override;
	virtual string                                GetVerboseInfo() const override { return " '" + m_responseID.GetText() + "'"; }
	virtual void                                  Serialize(Serialization::IArchive& ar) override;
	virtual const char*                           GetType() const override        { return "Execute Response"; }
	//////////////////////////////////////////////////////////

private:
	CHashedString m_responseID;
};

//////////////////////////////////////////////////////////

class CActionExecuteResponseInstance final : public DRS::IResponseActionInstance, public DRS::IResponseManager::IListener
{
public:
	enum eCurrentState
	{
		eCurrentState_WaitingForResponseToFinish,
		eCurrentState_Canceled,
		eCurrentState_Done
	};

	CActionExecuteResponseInstance() : m_state(eCurrentState_WaitingForResponseToFinish) {}
	virtual ~CActionExecuteResponseInstance();

	//////////////////////////////////////////////////////////
	// IResponseActionInstance implementation
	virtual DRS::IResponseActionInstance::eCurrentState Update() override;
	virtual void                                        Cancel() override;
	//////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////
	// DRS::IResponseManager::IListener implementation
	virtual void OnSignalProcessingFinished(DRS::IResponseManager::IListener::SSignalInfos& signal, DRS::IResponseInstance* pFinishedResponse, DRS::IResponseManager::IListener::eProcessingResult outcome) override;
	//////////////////////////////////////////////////////////

private:
	eCurrentState m_state;
};
}  //namespace CryDRS
