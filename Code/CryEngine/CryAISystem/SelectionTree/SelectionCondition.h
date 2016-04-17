// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __SelectionCondition_h__
#define __SelectionCondition_h__

#pragma once

/*
   Simple logical expression for use as node conditions in the selection tree.
   The expression is pre-compiled and stored as a vector of byte-code ops.
 */

class SimpleLexer;
class SelectionVariables;
class SelectionVariableDeclarations;

class SelectionCondition
{
	struct ConditionOp;
public:
	SelectionCondition();
	SelectionCondition(const char* condition, const SelectionVariableDeclarations& variables);

	bool Evaluate(const SelectionVariables& variables) const;
	bool Valid() const;

private:
	int  AddOp(const ConditionOp& op);
	int  ParseLogical(SimpleLexer& lex, int tok, const SelectionVariableDeclarations& variables);
	int  ParseCompOp(SimpleLexer& lex, int tok, const SelectionVariableDeclarations& variables);
	int  ParseUnary(SimpleLexer& lex, int tok, const SelectionVariableDeclarations& variables);
	int  ParseValue(SimpleLexer& lex, int tok, const SelectionVariableDeclarations& variables);
	int  Parse(const char* condition, const SelectionVariableDeclarations& variables);
	void Optimise();

	bool EvaluateOp(const SelectionVariables& variables, const struct ConditionOp& op) const;

	struct ConditionOp
	{
		enum Type
		{
			Not = 1,
			And,
			Or,
			Xor,
			Equal,
			NotEqual,
			Constant,
			Variable,
		};

		ConditionOp()
			: value(false)
		{
		}

		ConditionOp(int type, int left, int right)
			: value(false)
			, opType((Type)type)
			, operandLeft((uint8)left)
			, operandRight((uint8)right)
		{
		}

		ConditionOp(int type, uint32 varID)
			: variableID(varID)
			, value(false)
			, opType((Type)type)
		{
		}

		ConditionOp(int type, bool val)
			: value(val)
			, opType((Type)type)
		{
		}

		uint32 variableID;
		bool   value;

		uint8  opType;
		uint8  operandLeft;
		uint8  operandRight;
	};

	typedef std::vector<ConditionOp> ConditionOps;
	ConditionOps m_conditionOps;
	int          m_rootID;
};

DECLARE_SHARED_POINTERS(SelectionCondition)

#endif
