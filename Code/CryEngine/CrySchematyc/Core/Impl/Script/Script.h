// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <Schematyc/Script/IScript.h>
#include <Schematyc/Utils/StackString.h>

namespace Schematyc
{
class CScript : public IScript
{
	typedef std::vector<IScriptElement*> Elements;

public:
	CScript(const SGUID& guid, const char* szFilePath);
	CScript(const char* szFilePath);

	// IScript
	virtual SGUID             GetGUID() const override;

	virtual const char*       SetFilePath(const char* szFilePath) override;
	virtual const char*       GetFilePath() const override { return m_filePath.c_str(); }

	virtual const CTimeValue& GetTimeStamp() const override;

	virtual void              SetRoot(IScriptElement* pRoot) override;
	virtual IScriptElement*   GetRoot() override;

	virtual EVisitStatus      VisitElements(const ScriptElementVisitor& visitor) override;
	// ~IScript

private:
	EVisitStatus VisitElementsRecursive(const ScriptElementVisitor& visitor, IScriptElement& element);
	void         SetNameFromRootRecursive(CStackString& name, IScriptElement& element);

	SGUID           m_guid;
	string          m_filePath;
	CTimeValue      m_timeStamp;
	IScriptElement* m_pRoot;
};

DECLARE_SHARED_POINTERS(CScript)
}
