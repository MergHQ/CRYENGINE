// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CLevelShaderCache
{
public:
	CLevelShaderCache()
	{
		m_bModified = false;
	}
	bool Load(const char* filename);
	bool LoadBuffer(const string& textBuffer, bool bClearOld = true);
	bool SaveBuffer(string& textBuffer);
	bool Save();
	bool Reload();
	void Clear();

	void Update();
	void ActivateShaders();

private:
	bool    m_bModified;
	string  m_filename;
	typedef std::set<string> Entries;
	Entries m_entries;
};
