// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "EditorCommonAPI.h"
#include <CryString/CryString.h>

//! This class holds current VCS status of a file.
class EDITOR_COMMON_API CVersionControlFileStatus
{
public:
	enum State
	{
		eState_AddedLocally =       0x00000001,
		eState_AddedRemotely =      0x00000002,
		eState_CheckedOutLocally =  0x00000004,
		eState_CheckedOutRemotely = 0x00000008,
		eState_UpdatedRemotely =    0x00000010,
		eState_DeletedLocally =     0x00000020,
		eState_DeletedRemotely =    0x00000040,
		eState_ModifiedLocally =    0x00000080,
		eState_Conflicting =        0x00000100,
		eState_NotTracked =         0x00000200,
	};

	static int GetRemoteStates() 
	{
		return eState_AddedRemotely | eState_CheckedOutRemotely | eState_UpdatedRemotely | eState_DeletedRemotely | eState_Conflicting;
	}

	static int GetLocalStates() 
	{
		return eState_AddedLocally | eState_CheckedOutLocally | eState_ModifiedLocally | eState_DeletedLocally | eState_Conflicting;
	}

	CVersionControlFileStatus() = default;

	explicit CVersionControlFileStatus(string fileName, int state = 0)
		: m_fileName(std::move(fileName))
		, m_bits(state)
	{}

	const string& GetFileName() const { return m_fileName; }

	void SetState(int state)       { m_bits = state; }
	void AddState(int state)       { m_bits |= state; }
	void RemoveState(int state)    { m_bits &= ~state; }
	bool HasState(int state) const { return GetState() & state; }
	int  GetState() const          { return m_bits; }

	void ClearStates()      { m_bits = 0; }
	void ClearRemoteState() { RemoveState(GetRemoteStates()); }
	void ClearLocalState()  { RemoveState(GetRemoteStates()); }

	bool IsUntouched() const         { return m_bits == 0; }
	bool IsUntouchedRemotely() const { return !(GetState() & GetRemoteStates()); }
	bool IsUntouchedLocally() const  { return !(GetState() & GetLocalStates()); }

	bool operator==(const CVersionControlFileStatus& other) const                      { return m_fileName == other.m_fileName; }
	friend bool operator==(const CVersionControlFileStatus& left, const string& right) { return left.m_fileName == right; }

private:
	string m_fileName;
	int m_bits{ 0 };
};
