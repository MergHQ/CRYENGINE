// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

////////////////////////////////////////////////////////////////////////////
// check that we are allowed to be included
#if !defined(CRYPLATFROM_ALLOW_DETAIL_INCLUDES)
	#error Please include CryPlatfrom.h instead of this private implementation header
#endif

////////////////////////////////////////////////////////////////////////////
// Create a string from an Preprocessor macro or a literal text
#define CRY_DETAIL_CREATE_STRING(string) # string
#define CRY_CREATE_STRING(string)        CRY_DETAIL_CREATE_STRING(string)
#define RESOLVE_MACRO(x)                 x

////////////////////////////////////////////////////////////////////////////
#define __DETAIL__LINK_THIRD_PARTY_LIBRARY(name)                                                \
  __pragma(message(__FILE__ "(" CRY_CREATE_STRING(__LINE__) "): Including SDK Library: " name)) \
  __pragma(comment(lib, RESOLVE_MACRO(CODE_BASE_FOLDER) name))

////////////////////////////////////////////////////////////////////////////
#define __DETAIL__LINK_SYSTEM_PARTY_LIBRARY(name)                                                  \
  __pragma(message(__FILE__ "(" CRY_CREATE_STRING(__LINE__) "): Including System Library: " name)) \
  __pragma(comment(lib, name))
