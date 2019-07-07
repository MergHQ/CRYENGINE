// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <limits> // std::numeric_limits

template<typename STORAGE>
class CCryFlags
{
	// the reason for the following assert: AreMultipleFlagsActive() works incorrect if STORAGE is signed
	static_assert(!std::numeric_limits<STORAGE>::is_signed, "'STORAGE' must not be a signed type!");
public:
	CCryFlags() : m_bitStorage(0) {}
	ILINE void    AddFlags(STORAGE flags)                  { m_bitStorage |= flags; }
	ILINE void    ClearFlags(STORAGE flags)                { m_bitStorage &= ~flags; }
	ILINE bool    AreAllFlagsActive(STORAGE flags) const   { return((m_bitStorage & flags) == flags); }
	ILINE bool    AreAnyFlagsActive(STORAGE flags) const   { return((m_bitStorage & flags) != 0); }
	ILINE bool    AreMultipleFlagsActive() const           { return (m_bitStorage & (m_bitStorage - 1)) != 0; }
	ILINE bool    IsOneFlagActive() const                  { return m_bitStorage != 0 && !AreMultipleFlagsActive(); }
	ILINE void    ClearAllFlags()                          { m_bitStorage = 0; }
	ILINE void    SetFlags(STORAGE flags, const bool bSet) { if (bSet) m_bitStorage |= flags; else  m_bitStorage &= ~flags; }
	ILINE STORAGE GetRawFlags() const                      { return m_bitStorage; }
	ILINE bool    operator==(const CCryFlags& other) const { return m_bitStorage == other.m_bitStorage; }
	ILINE bool    operator!=(const CCryFlags& other) const { return !(*this == other); }

private:
	STORAGE m_bitStorage;
};
