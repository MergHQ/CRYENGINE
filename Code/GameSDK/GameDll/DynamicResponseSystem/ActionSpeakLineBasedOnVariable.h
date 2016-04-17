// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/************************************************************************

This action will speak a dialog line based on the current value of a specified variable.

Example: Say you have a variable in which you count how often the player has visited a trader,
then you could set the action up like: 
m_lineIDToSpeak1 = "hi there, new customer" (+ variations)
m_lineIDToSpeak2 = "so you liked what you saw the first time?" (+ variations)
m_lineIDToSpeak3 = "old friend, how are you?" (+ variations)
m_variableValueForLine1 = 1 (assuming the variable was incremented before this action was executed)
m_variableValueForLine2 = 2
With this setup its up to the game to decide when to reset the times-visited-counter.

Remark: Its just a more convenient way to setup dialog lines that depend on counter-variables.
Its an example to show how you can create custom-actions for your game to make the life for your writer easier.
You could create the same behavior by using 3 responses (with two of them having conditions to check the variables) 
to execute separate SpeakLine Actions for each of the lines. 
This one is a bit faster (because it only needs to fetch the variable value once) and less memory consuming and mostly it`s easier to setup.

/************************************************************************/

#pragma once

#include <CryDynamicResponseSystem/IDynamicResponseAction.h>

	class CActionSpeakLineBasedOnVariable final : public DRS::IResponseAction
	{
	public:
		CActionSpeakLineBasedOnVariable() : m_collectionName("Local"), m_variableValueForLine1(1), m_variableValueForLine2(2) {}

		//////////////////////////////////////////////////////////
		// IResponseAction implementation
		virtual DRS::IResponseActionInstanceUniquePtr Execute(DRS::IResponseInstance* pResponseInstance) override;
		virtual string GetVerboseInfo() const override;
		virtual void Serialize(Serialization::IArchive& ar) override;
		virtual const char* GetType() const override { return "Speak Line Based On Variable"; }
		//////////////////////////////////////////////////////////

	private:
		CHashedString m_speakerOverrideName;
		CHashedString m_collectionName;
		CHashedString m_variableName;
		int m_variableValueForLine1;
		CHashedString m_lineIDToSpeak1;
		int m_variableValueForLine2;
		CHashedString m_lineIDToSpeak2;
		CHashedString m_lineIDToSpeak3;
	};

//////////////////////////////////////////////////////////////////////////

	class CActionSpeakLineBasedOnVariableInstance final : public DRS::IResponseActionInstance
	{
	public:
		CActionSpeakLineBasedOnVariableInstance(const DRS::IResponseActor* pSpeaker, const CHashedString& lineID) : m_pSpeaker(pSpeaker), m_LineID(lineID) {}

		//////////////////////////////////////////////////////////
		// IResponseAction implementation
		virtual DRS::IResponseActionInstance::eCurrentState Update() override;
		virtual void Cancel() override;
		//////////////////////////////////////////////////////////
	private:
		const DRS::IResponseActor* m_pSpeaker;
		const CHashedString m_LineID;
	};
