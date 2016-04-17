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
		CActionExecuteAudioTrigger(const string& triggerName) : m_AudioTriggerName(triggerName), m_bWaitToBeFinished(true) {}
		virtual ~CActionExecuteAudioTrigger() {}

		//////////////////////////////////////////////////////////
		// IResponseAction implementation
		virtual DRS::IResponseActionInstanceUniquePtr Execute(DRS::IResponseInstance* pResponseInstance) override;
		virtual string GetVerboseInfo() const override;
		virtual void Serialize(Serialization::IArchive& ar) override;
		virtual const char* GetType() const override { return "Execute Audio Trigger"; }
		//////////////////////////////////////////////////////////

	private:
		string m_AudioTriggerName;
		bool m_bWaitToBeFinished;
	};

//////////////////////////////////////////////////////////////////////////

	class CActionExecuteAudioTriggerInstance final : public DRS::IResponseActionInstance
	{
	public:
		CActionExecuteAudioTriggerInstance();
		virtual ~CActionExecuteAudioTriggerInstance();

		//////////////////////////////////////////////////////////
		// IResponseActionInstance implementation
		virtual eCurrentState Update() override;
		virtual void Cancel() override { m_bHasFinished = true; }
		//////////////////////////////////////////////////////////

		void SetFinished(bool bValue) { m_bHasFinished = bValue; }
		static void OnAudioTriggerFinished(const SAudioRequestInfo* const pAudioRequestInfo);
	private:
		bool m_bHasFinished;
	};
