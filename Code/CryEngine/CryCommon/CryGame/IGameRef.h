// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryGame/IGame.h>

struct IGameRef
{
	CRY_DEPRECATED_GAME_DLL IGameRef() : m_ptr(nullptr) {}
	IGameRef(IGame** ptr) : m_ptr(ptr) {}

	IGame*    operator->() const     { return m_ptr ? *m_ptr : nullptr; }
	operator IGame*() const { return m_ptr ? *m_ptr : nullptr; }
	IGameRef& operator=(IGame** ptr) { m_ptr = ptr; return *this; }

private:
	IGame** m_ptr;
};