// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryString/HashedString.h>
#include <CryDynamicResponseSystem/IDynamicResponseAction.h>
#include <CryDynamicResponseSystem/IDynamicResponseSystem.h>

namespace CryDRS
{
class CResponseActor;

class CActionSpeakLine final : public DRS::IResponseAction
{
public:
	CActionSpeakLine() {}
	CActionSpeakLine(const CHashedString& lineToSpeak) : m_lineIDToSpeak(lineToSpeak) {}

	//////////////////////////////////////////////////////////
	// IResponseAction implementation
	virtual DRS::IResponseActionInstanceUniquePtr Execute(DRS::IResponseInstance* pResponseInstance) override;
	virtual string                                GetVerboseInfo() const override { return "'" + m_lineIDToSpeak.GetText() + "'"; }
	virtual void                                  Serialize(Serialization::IArchive& ar) override;
	virtual const char*                           GetType() const override        { return "Speak Line"; }
	//////////////////////////////////////////////////////////

private:
	CHashedString m_speakerOverrideName;
	CHashedString m_lineIDToSpeak;
};

//////////////////////////////////////////////////////////

class CActionSpeakLineInstance final : public DRS::IResponseActionInstance
{
public:
	CActionSpeakLineInstance(const CResponseActor* pSpeaker, const CHashedString& lineID) : m_pSpeaker(pSpeaker), m_lineID(lineID) {}

	//////////////////////////////////////////////////////////
	// IResponseActionInstance implementation
	virtual eCurrentState Update() override;
	virtual void          Cancel() override;
	//////////////////////////////////////////////////////////

private:
	const CResponseActor* m_pSpeaker;
	const CHashedString   m_lineID;
};

//////////////////////////////////////////////////////////

class CActionCancelSpeaking final : public DRS::IResponseAction
{
public:
	CActionCancelSpeaking() : m_maxPrioToCancel(-1) {}
	CActionCancelSpeaking(const CHashedString& speakerName) : m_maxPrioToCancel(-1) {}

	//////////////////////////////////////////////////////////
	// IResponseAction implementation
	virtual DRS::IResponseActionInstanceUniquePtr Execute(DRS::IResponseInstance* pResponseInstance) override;
	virtual string                                GetVerboseInfo() const override { return " for speaker '" + m_speakerOverrideName.GetText() + "'"; }
	virtual void                                  Serialize(Serialization::IArchive& ar) override;
	virtual const char*                           GetType() const override        { return "Cancel Line"; }
	//////////////////////////////////////////////////////////

private:
	CHashedString m_speakerOverrideName;
	int           m_maxPrioToCancel;
};

//////////////////////////////////////////////////////////

class CActionCancelSpeakingInstance final : public DRS::IResponseActionInstance
{
public:
	CActionCancelSpeakingInstance(const CResponseActor* pSpeaker) : m_pSpeaker(pSpeaker) {}

	//////////////////////////////////////////////////////////
	// IResponseActionInstance implementation
	virtual eCurrentState Update() override;
	virtual void          Cancel() override;
	//////////////////////////////////////////////////////////

private:
	const CResponseActor* m_pSpeaker;
};
}  //namespace CryDRS
