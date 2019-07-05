/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2013 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#pragma once 
#include <CrySerialization/yasli/Config.h>

#ifndef YASLI_ASSERT_DEFINED
#include <stdio.h>

#ifndef NDEBUG
#	define YASLI_ASSERT CRY_ASSERT
#	define YASLI_ASSERT_STR(expr, msg) CRY_ASSERT(expr, msg)
#	define YASLI_CHECK CRY_VERIFY
#else
#	define YASLI_ASSERT(expr, ...) (1, true)
#	define YASLI_CHECK(expr, ...) (expr)
#endif

// use YASLI_CHECK instead
#define YASLI_ESCAPE(x, action) if(!(x)) { YASLI_ASSERT(0 && #x); action; } 

#pragma warning(disable:4127) // conditional expression is constant
#endif
