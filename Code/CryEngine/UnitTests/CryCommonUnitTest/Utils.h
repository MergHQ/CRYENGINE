// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

class CCopyMoveInspector
{
public:
	static int copyCount;
	static int moveCount;

	static void Reset() { copyCount = moveCount = 0; }

	CCopyMoveInspector() = default;

	CCopyMoveInspector(const CCopyMoveInspector&)
	{
		++copyCount;
	}

	CCopyMoveInspector(CCopyMoveInspector&&)
	{
		++moveCount;
	}

	friend bool operator==(const CCopyMoveInspector&, const CCopyMoveInspector&) { return true; }
	friend bool operator!=(const CCopyMoveInspector&, const CCopyMoveInspector&) { return false; }
	friend bool operator<=(const CCopyMoveInspector&, const CCopyMoveInspector&) { return true; }
	friend bool operator>=(const CCopyMoveInspector&, const CCopyMoveInspector&) { return true; }
	friend bool operator< (const CCopyMoveInspector&, const CCopyMoveInspector&) { return false; }
	friend bool operator> (const CCopyMoveInspector&, const CCopyMoveInspector&) { return false; }
};
