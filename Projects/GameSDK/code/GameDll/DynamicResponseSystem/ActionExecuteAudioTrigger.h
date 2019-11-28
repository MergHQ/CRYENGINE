// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

/************************************************************************

   This action will execute the specified audio trigger.

   /************************************************************************/

#pragma once

#include <CryDynamicResponseSystem/IDynamicResponseAction.h>

class CActionExecuteAudioTrigger final : public Cry::DRS::IResponseAction
{
public:
	CActionExecuteAudioTrigger() : m_bWaitToBeFinished(true) {}

	//////////////////////////////////////////////////////////
	// IResponseAction implementation
	virtual Cry::DRS::IResponseActionInstanceUniquePtr Execute(Cry::DRS::IResponseInstance* pResponseInstance) override;
	virtual string                                GetVerboseInfo() const override;
	virtual void                                  Serialize(Serialization::IArchive& ar) override;
	virtual const char*                           GetType() const override { return "Execute Audio Trigger"; }
	//////////////////////////////////////////////////////////

private:
	string m_AudioTriggerName;
	bool   m_bWaitToBeFinished;
};

//////////////////////////////////////////////////////////////////////////

class CActionExecuteAudioTriggerInstance final : public Cry::DRS::IResponseActionInstance
{
public:
	CActionExecuteAudioTriggerInstance(Cry::DRS::IResponseActor* pActor, Cry::Audio::ControlId audioStartTriggerID);
	virtual ~CActionExecuteAudioTriggerInstance() override;

	//////////////////////////////////////////////////////////
	// IResponseActionInstance implementation
	virtual eCurrentState Update() override;
	virtual void          Cancel() override;
	//////////////////////////////////////////////////////////

	void        SetFinished();
	static void OnAudioTriggerFinished(const Cry::Audio::SRequestInfo* const pAudioRequestInfo);
private:
	Cry::Audio::ControlId  m_audioStartTriggerID;
	Cry::DRS::IResponseActor* m_pActor;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CActionSetAudioSwitch final : public Cry::DRS::IResponseAction
{
public:
	//////////////////////////////////////////////////////////
	// IResponseAction implementation
	virtual Cry::DRS::IResponseActionInstanceUniquePtr Execute(Cry::DRS::IResponseInstance* pResponseInstance) override;
	virtual string                                GetVerboseInfo() const override;
	virtual void                                  Serialize(Serialization::IArchive& ar) override;
	virtual const char*                           GetType() const override { return "Set Audio Switch"; }
	//////////////////////////////////////////////////////////

private:
	string m_switchName;
	string m_stateName;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CActionSetAudioParameter final : public Cry::DRS::IResponseAction
{
public:
	//////////////////////////////////////////////////////////
	// IResponseAction implementation
	virtual Cry::DRS::IResponseActionInstanceUniquePtr Execute(Cry::DRS::IResponseInstance* pResponseInstance) override;
	virtual string                                GetVerboseInfo() const override;
	virtual void                                  Serialize(Serialization::IArchive& ar) override;
	virtual const char*                           GetType() const override { return "Set Audio Parameter"; }
	//////////////////////////////////////////////////////////

private:
	string m_audioParameter;
	float m_valueToSet = 1.0f;
};
