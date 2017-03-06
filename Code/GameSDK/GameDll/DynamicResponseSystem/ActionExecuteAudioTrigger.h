// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/************************************************************************

   This action will execute the specified audio trigger.

   /************************************************************************/

#pragma once

#include <CryDynamicResponseSystem/IDynamicResponseAction.h>

class CActionExecuteAudioTrigger final : public DRS::IResponseAction
{
public:
	CActionExecuteAudioTrigger() : m_bWaitToBeFinished(true) {}
	virtual ~CActionExecuteAudioTrigger() = default;

	//////////////////////////////////////////////////////////
	// IResponseAction implementation
	virtual DRS::IResponseActionInstanceUniquePtr Execute(DRS::IResponseInstance* pResponseInstance) override;
	virtual string                                GetVerboseInfo() const override;
	virtual void                                  Serialize(Serialization::IArchive& ar) override;
	virtual const char*                           GetType() const override { return "Execute Audio Trigger"; }
	//////////////////////////////////////////////////////////

private:
	string m_AudioTriggerName;
	bool   m_bWaitToBeFinished;
};

//////////////////////////////////////////////////////////////////////////

class CActionExecuteAudioTriggerInstance final : public DRS::IResponseActionInstance
{
public:
	CActionExecuteAudioTriggerInstance(IEntityAudioComponent* pAudioProxy, CryAudio::ControlId audioStartTriggerID);
	virtual ~CActionExecuteAudioTriggerInstance();

	//////////////////////////////////////////////////////////
	// IResponseActionInstance implementation
	virtual eCurrentState Update() override;
	virtual void          Cancel() override;
	//////////////////////////////////////////////////////////

	void        SetFinished();
	static void OnAudioTriggerFinished(const CryAudio::SRequestInfo* const pAudioRequestInfo);
private:
	CryAudio::ControlId    m_audioStartTriggerID;
	IEntityAudioComponent* m_pEntityAudioProxy;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CActionSetAudioSwitch final : public DRS::IResponseAction
{
public:
	virtual ~CActionSetAudioSwitch() = default;

	//////////////////////////////////////////////////////////
	// IResponseAction implementation
	virtual DRS::IResponseActionInstanceUniquePtr Execute(DRS::IResponseInstance* pResponseInstance) override;
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

class CActionSetAudioParameter final : public DRS::IResponseAction
{
public:
	virtual ~CActionSetAudioParameter() = default;

	//////////////////////////////////////////////////////////
	// IResponseAction implementation
	virtual DRS::IResponseActionInstanceUniquePtr Execute(DRS::IResponseInstance* pResponseInstance) override;
	virtual string                                GetVerboseInfo() const override;
	virtual void                                  Serialize(Serialization::IArchive& ar) override;
	virtual const char*                           GetType() const override { return "Set Audio Parameter"; }
	//////////////////////////////////////////////////////////

private:
	string m_audioParameter;
	float m_valueToSet = 1.0f;
};
