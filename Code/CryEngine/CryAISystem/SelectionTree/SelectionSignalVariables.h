// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __SelectionSignalVariables_h__
#define __SelectionSignalVariables_h__

#pragma once

/*
   A simple mechanism to flip variable values based on signal fire and conditions.
   A signal is mapped to an expression which will evaluate to the value of the variable,
   shall the signal be fired.
 */

#include "BlockyXml.h"
#include "SelectionVariables.h"
#include "SelectionCondition.h"

class SelectionSignalVariables
{
public:
	bool LoadFromXML(const BlockyXmlBlocks::Ptr& blocks, const SelectionVariableDeclarations& variableDecls,
	                 const XmlNodeRef& rootNode, const char* scopeName, const char* fileName);

	bool ProcessSignal(const char* signalName, uint32 signalCRC, SelectionVariables& variables) const;

private:
	struct SignalVariable
	{
		SelectionCondition  valueExpr;
		SelectionVariableID variableID;
	};

	typedef std::multimap<uint32, SignalVariable> SignalVariables;
	SignalVariables m_signalVariables;
};

#endif
