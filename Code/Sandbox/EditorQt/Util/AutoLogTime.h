// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CAutoLogTime
{
public:
	CAutoLogTime(const char* what);
	~CAutoLogTime();
private:
	const char* m_what;
	int         m_t0, m_t1;
};
