// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <utility>
#include <CryCore/SmallFunction.h>


//! A simple scope guard implementation that stores a void() callable on construction and calls it upon destruction in a RAII fashion.
//! Intended primarily for executing a user-supplied cleanup routine when exiting the enclosing scope.
//!
//! Usage example:
//! void DrawThing(SRendParams& renderParams)
//! {
//!     const auto flagsBackup = renderParams.dwFObjFlags;
//!     ScopeGuard restoreFlags{ [&]() { renderParams.dwFObjFlags = flagsBackup; } };
//!
//!     renderParams.dwFObjFlags &= ~FOB_SOME_FLAG;
//!     renderParams.dwFObjFlags |= FOB_ANOTHER_FLAG;
//!     DrawSubThing(renderParams);
//!
//!     // Original renderParams.dwFObjFlags state gets restored.
//! }
//!
//! Usage example with a Dismiss() call:
//! CObject* BuildComplexObject()
//! {
//!     Object* pObj = AllocateObject();
//!     ScopeGuard guard{ [pObj]() { ReleaseObject(pObj); } };
//!
//!     // Some initialization logic, pObj gets released in case of an early return or exception.
//!
//!     guard.Dismiss(); // Do not release the object if everything went fine.
//!     return pObj;
//! }
class ScopeGuard
{
public:

	//! Constructs a new ScopeGuard instance.
	//! \param function A callable to be stored. The signature needs to be void().
	template<typename T>
	ScopeGuard(T&& function) noexcept;

	//! Dismisses the stored callable. After calling this method, no action will be taken by this ScopeGuard instance upon destruction.
	void Dismiss() noexcept;

	~ScopeGuard();

	ScopeGuard(ScopeGuard&&) = default;
	ScopeGuard& operator=(ScopeGuard&& other) = default;

	ScopeGuard(const ScopeGuard&) = delete;
	ScopeGuard& operator=(const ScopeGuard&) = delete;

private:

	SmallFunction<void()> m_function;
};


template<typename T>
inline ScopeGuard::ScopeGuard(T&& function) noexcept
	: m_function(std::forward<T>(function))
{
}

inline void ScopeGuard::Dismiss() noexcept
{
	m_function = nullptr;
}

inline ScopeGuard::~ScopeGuard()
{
	if (m_function)
	{
		m_function();
	}
}
