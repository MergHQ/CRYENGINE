// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Script/IScript.h>
#include <Schematyc/Utils/StackString.h>

namespace Schematyc
{
	class CScript : public IScript
	{
	private:

		typedef std::vector<IScriptElement*> Elements;

	public:

		CScript(const SGUID& guid, const char* szName);

		// IScript
		virtual SGUID GetGUID() const override;
		virtual void SetName(const char* szName) override;
		virtual const char* SetNameFromRoot() override;
		virtual const char* GetName() const override;
		virtual const CTimeValue& GetTimeStamp() const override;
		virtual void SetRoot(IScriptElement* pRoot) override;
		virtual IScriptElement* GetRoot() override;
		virtual EVisitStatus VisitElements(const ScriptElementVisitor& visitor) override;
		// ~IScript

	private:

		EVisitStatus VisitElementsRecursive(const ScriptElementVisitor& visitor, IScriptElement& element);
		void SetNameFromRootRecursive(CStackString& name, IScriptElement& element);

		SGUID           m_guid;
		string          m_name;
		CTimeValue      m_timeStamp;
		IScriptElement* m_pRoot;
	};

	DECLARE_SHARED_POINTERS(CScript)
}