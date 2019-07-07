// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#pragma once

#include "VersionControlFileStatus.h"
#include "CryString/CryString.h"

//! This class is responsible for applying change to a CVerisonControlFileStatus object
//! \see CVerisonControlFileStatus.
class CVersionControlFileStatusUpdate
{
public:
	//! This constructor is equivalent to calling SetState()
	//! \see SetState().
	CVersionControlFileStatusUpdate(const string& fileName, int state)
		: m_fileName(fileName)
		, m_setBits(state)
		, m_clearBits(-1)
	{}

	explicit CVersionControlFileStatusUpdate(const string& fileName)
		: m_fileName(fileName)
	{}

	//! Applies changes to the given CVersionControlFileStatus object.
	void Apply(CVersionControlFileStatus& fs) const
	{
		fs.RemoveState(m_clearBits);
		fs.AddState(m_setBits);
	}

	//! Instructs to reset the current state of CVersionControlFileStatus object and 
	//! and override it with the given state
	void SetState(int state)
	{
		m_setBits = state;
		m_clearBits = -1;
	}

	const string& GetFileName() const { return m_fileName; }

	//! Instructs to add the given state to the current one.
	void AddState(int state) { m_setBits |= state; }

	//! Instructs to remove the given state from the current one.
	void RemoveState(int state)
	{
		m_clearBits |= state;
		// since removal of state is applied first, we use this way of canceling of unnecessary addition.
		m_setBits &= ~state;
	}

	int GetStatesToAdd() const { return m_setBits; }

	int GetStatesToRemote() const { return m_clearBits; }

private:
	string m_fileName;
	int m_setBits{ 0 };
	int m_clearBits{ 0 };
};
