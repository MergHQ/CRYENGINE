// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef __SelectionVariables_h__
#define __SelectionVariables_h__

#pragma once

/*
   This file implements variable value and variable declaration collections.
 */

class CDebugDrawContext;

typedef uint32 SelectionVariableID;

class BlockyXmlBlocks;
class SelectionVariableDeclarations;

class SelectionVariables
{
public:
	SelectionVariables();

	bool   GetVariable(const SelectionVariableID& variableID, bool* value) const;
	bool   SetVariable(const SelectionVariableID& variableID, bool value);

	void   ResetChanged(bool state = false);
	bool   Changed() const;

	void   Swap(SelectionVariables& other);

	void   Serialize(TSerialize ser);

	size_t GetVariablesAmount() const { return m_variables.size(); }

#if defined(CRYAISYSTEM_DEBUG)

	void DebugTrackSignalHistory(const char* signalName);

	void DebugDraw(bool history, const SelectionVariableDeclarations& declarations) const;

#endif

private:

#if defined(CRYAISYSTEM_DEBUG)

	void DebugDrawSignalHistory(CDebugDrawContext& dc, float bottomLeftX, float bottomLeftY, float lineHeight, float fontSize) const;

#endif

private:
	struct Variable
	{
		Variable()
			: value(false)
		{
		}

		bool value;

		void Serialize(TSerialize ser)
		{
			ser.Value("value", value);
		}
	};

	typedef std::unordered_map<SelectionVariableID, Variable, stl::hash_uint32> Variables;
	Variables m_variables;

	bool      m_changed;

	struct VariableChangeEvent
	{
		VariableChangeEvent(const CTimeValue& _when, const SelectionVariableID _variableID, bool _value)
			: when(_when)
			, variableID(_variableID)
			, value(_value)
		{
		}

		CTimeValue          when;
		SelectionVariableID variableID;
		bool                value;
	};

	typedef std::deque<VariableChangeEvent> History;
	History m_history;

#if defined(CRYAISYSTEM_DEBUG)

	// Debug: we track the last few received signals here.
	struct DebugSignalHistoryEntry
	{
		DebugSignalHistoryEntry();
		DebugSignalHistoryEntry(const char* signalName, const CTimeValue timeStamp);

		// The name of the signal.
		string m_Name;

		// The time index at which the signal was delivered.
		CTimeValue m_TimeStamp;
	};

	typedef std::list<DebugSignalHistoryEntry> DebugSignalHistory;
	DebugSignalHistory m_DebugSignalHistory;

#endif
};

class SelectionVariableDeclarations
{
public:
	struct VariableDescription
	{
		VariableDescription(const char* _name)
			: name(_name)
		{
		}

		string name;
	};
	typedef std::map<SelectionVariableID, VariableDescription> VariableDescriptions;

	SelectionVariableDeclarations();
	bool                        LoadFromXML(const _smart_ptr<BlockyXmlBlocks>& blocks, const XmlNodeRef& rootNode, const char* scopeName, const char* fileName);

	bool                        IsDeclared(const SelectionVariableID& variableID) const;

	SelectionVariableID         GetVariableID(const char* name) const;
	const VariableDescription&  GetVariableDescription(const SelectionVariableID& variableID) const;
	void                        GetVariablesNames(const char** variableNamesBuffer, const size_t maxSize, size_t& actualSize) const;

	const SelectionVariables&   GetDefaults() const;
	const VariableDescriptions& GetDescriptions() const;

private:
	VariableDescriptions m_descriptions;
	SelectionVariables   m_defaults;
};

#endif
