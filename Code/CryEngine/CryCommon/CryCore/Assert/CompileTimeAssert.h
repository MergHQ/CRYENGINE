// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if defined(__cplusplus) && !defined(__RECODE__)

	#define COMPILE_TIME_ASSERT(expr) static_assert(expr, # expr)

#else

	#define COMPILE_TIME_ASSERT(expr)

#endif
