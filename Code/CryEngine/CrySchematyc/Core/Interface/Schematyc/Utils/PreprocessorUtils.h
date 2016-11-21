// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryCore/CryCustomTypes.h>

#ifndef SCHEMATYC_NOP
#define SCHEMATYC_NOP ((void)0)
#endif

#ifndef SCHEMATYC_DEBUG_BREAK
#ifdef _RELEASE
#define SCHEMATYC_DEBUG_BREAK
#else
#define SCHEMATYC_DEBUG_BREAK CryDebugBreak();
#endif
#endif

#ifndef SCHEMATYC_FILE_NAME
#define SCHEMATYC_FILE_NAME __FILE__
#endif

#ifndef SCHEMATYC_LINE_NUMBER
#define SCHEMATYC_LINE_NUMBER __LINE__
#endif

#ifndef SCHEMATYC_FUNCTION_NAME
#ifdef _MSC_VER
#define SCHEMATYC_FUNCTION_NAME __FUNCSIG__
#else
#define SCHEMATYC_FUNCTION_NAME __PRETTY_FUNCTION__
#endif
#endif

#define SCHEMATYC_PP_EMPTY
#define SCHEMATYC_PP_COMMA                        ,
#define SCHEMATYC_PP_LEFT_BRACKET                 (
#define SCHEMATYC_PP_RIGHT_BRACKET                )
#define SCHEMATYC_PP_SEMI_COLON                   ;
#define SCHEMATYC_PP_COLON                        :

#define SCHEMATYC_PP_NUMBER(x)                    x

#define SCHEMATYC_PP_TO_STRING_(x)                #x
#define SCHEMATYC_PP_TO_STRING(x)                 SCHEMATYC_PP_TO_STRING_(x)
#define SCHEMATYC_PP_JOIN_XY_(x, y)               x##y
#define SCHEMATYC_PP_JOIN_XY(x, y)                SCHEMATYC_PP_JOIN_XY_(x, y)
#define SCHEMATYC_PP_JOIN_XYZ_(x, y, z)           x##y##z
#define SCHEMATYC_PP_JOIN_XYZ(x, y, z)            SCHEMATYC_PP_JOIN_XYZ_(x, y, z)

#define SCHEMATYC_PP_BOOL_0                       0
#define SCHEMATYC_PP_BOOL_1                       1
#define SCHEMATYC_PP_BOOL_2                       1
#define SCHEMATYC_PP_BOOL_3                       1
#define SCHEMATYC_PP_BOOL_4                       1
#define SCHEMATYC_PP_BOOL_5                       1
#define SCHEMATYC_PP_BOOL_6                       1
#define SCHEMATYC_PP_BOOL_7                       1
#define SCHEMATYC_PP_BOOL_8                       1
#define SCHEMATYC_PP_BOOL_9                       1
#define SCHEMATYC_PP_BOOL_10                      1
#define SCHEMATYC_PP_BOOL(x)                      SCHEMATYC_PP_JOIN_XY(SCHEMATYC_PP_BOOL_, x)

#define SCHEMATYC_PP_NOT_0                        1
#define SCHEMATYC_PP_NOT_1                        0
#define SCHEMATYC_PP_NOT(x)                       SCHEMATYC_PP_JOIN_XY(SCHEMATYC_PP_NOT_, x)

#define SCHEMATYC_PP_AND_0_0                      0
#define SCHEMATYC_PP_AND_0_1                      0
#define SCHEMATYC_PP_AND_1_0                      0
#define SCHEMATYC_PP_AND_1_1                      1
#define SCHEMATYC_PP_AND_Y(x, y)                  SCHEMATYC_PP_AND_##x##_##y
#define SCHEMATYC_PP_AND_X(x, y)                  SCHEMATYC_PP_AND_Y(x, y)
#define SCHEMATYC_PP_AND(x, y)                    SCHEMATYC_PP_AND_X(SCHEMATYC_PP_BOOL(x), SCHEMATYC_PP_BOOL(y))

#define SCHEMATYC_PP_OR_0_0                       0
#define SCHEMATYC_PP_OR_0_1                       1
#define SCHEMATYC_PP_OR_1_0                       1
#define SCHEMATYC_PP_OR_1_1                       1
#define SCHEMATYC_PP_OR_Y(x, y)                   SCHEMATYC_PP_OR_##x##_##y
#define SCHEMATYC_PP_OR_X(x, y)                   SCHEMATYC_PP_OR_Y(x, y)
#define SCHEMATYC_PP_OR(x, y)                     SCHEMATYC_PP_OR_X(SCHEMATYC_PP_BOOL(x), SCHEMATYC_PP_BOOL(y))

#define SCHEMATYC_PP_IF_0(on_true)
#define SCHEMATYC_PP_IF_1(on_true)                on_true
#define SCHEMATYC_PP_IF(x, on_true)               SCHEMATYC_PP_JOIN_XY(SCHEMATYC_PP_IF_, SCHEMATYC_PP_BOOL(x))(on_true)

#define SCHEMATYC_PP_COMMA_IF_0                   SCHEMATYC_PP_EMPTY
#define SCHEMATYC_PP_COMMA_IF_1                   SCHEMATYC_PP_COMMA
#define SCHEMATYC_PP_COMMA_IF(x)                  SCHEMATYC_PP_JOIN_XY(SCHEMATYC_PP_COMMA_IF_, SCHEMATYC_PP_BOOL(x))

#define SCHEMATYC_PP_COLON_IF_0                   SCHEMATYC_PP_EMPTY
#define SCHEMATYC_PP_COLON_IF_1                   SCHEMATYC_PP_COLON
#define SCHEMATYC_PP_COLON_IF(x)                  SCHEMATYC_PP_JOIN_XY(SCHEMATYC_PP_COLON_IF_, SCHEMATYC_PP_BOOL(x))

#define SCHEMATYC_PP_SELECT_0(on_true, on_false)  on_false
#define SCHEMATYC_PP_SELECT_1(on_true, on_false)  on_true
#define SCHEMATYC_PP_SELECT(x, on_true, on_false) SCHEMATYC_PP_JOIN_XY(SCHEMATYC_PP_SELECT_, SCHEMATYC_PP_BOOL(x))(on_true, on_false)

#define SCHEMATYC_PP_ENUM_0(element, separator, user_data)
#define SCHEMATYC_PP_ENUM_1(element, separator, user_data)  element(0, user_data)
#define SCHEMATYC_PP_ENUM_2(element, separator, user_data)  element(0, user_data) separator element(1, user_data)
#define SCHEMATYC_PP_ENUM_3(element, separator, user_data)  element(0, user_data) separator element(1, user_data) separator element(2, user_data)
#define SCHEMATYC_PP_ENUM_4(element, separator, user_data)  element(0, user_data) separator element(1, user_data) separator element(2, user_data) separator element(3, user_data)
#define SCHEMATYC_PP_ENUM_5(element, separator, user_data)  element(0, user_data) separator element(1, user_data) separator element(2, user_data) separator element(3, user_data) separator element(4, user_data)
#define SCHEMATYC_PP_ENUM_6(element, separator, user_data)  element(0, user_data) separator element(1, user_data) separator element(2, user_data) separator element(3, user_data) separator element(4, user_data) separator element(5, user_data)
#define SCHEMATYC_PP_ENUM_7(element, separator, user_data)  element(0, user_data) separator element(1, user_data) separator element(2, user_data) separator element(3, user_data) separator element(4, user_data) separator element(5, user_data) separator element(6, user_data)
#define SCHEMATYC_PP_ENUM_8(element, separator, user_data)  element(0, user_data) separator element(1, user_data) separator element(2, user_data) separator element(3, user_data) separator element(4, user_data) separator element(5, user_data) separator element(6, user_data) separator element(7, user_data)
#define SCHEMATYC_PP_ENUM_9(element, separator, user_data)  element(0, user_data) separator element(1, user_data) separator element(2, user_data) separator element(3, user_data) separator element(4, user_data) separator element(5, user_data) separator element(6, user_data) separator element(7, user_data) separator element(8, user_data)
#define SCHEMATYC_PP_ENUM_10(element, separator, user_data) element(0, user_data) separator element(1, user_data) separator element(2, user_data) separator element(3, user_data) separator element(4, user_data) separator element(5, user_data) separator element(6, user_data) separator element(7, user_data) separator element(8, user_data) separator element(9, user_data)

#define SCHEMATYC_PP_ENUM(count, element, separator, user_data) SCHEMATYC_PP_JOIN_XY(SCHEMATYC_PP_ENUM_, count)(element, separator, user_data)

#define SCHEMATYC_PP_COMMA_ENUM_0(element, user_data)
#define SCHEMATYC_PP_COMMA_ENUM_1(element, user_data)  element(0, user_data)
#define SCHEMATYC_PP_COMMA_ENUM_2(element, user_data)  element(0, user_data) SCHEMATYC_PP_COMMA element(1, user_data)
#define SCHEMATYC_PP_COMMA_ENUM_3(element, user_data)  element(0, user_data) SCHEMATYC_PP_COMMA element(1, user_data) SCHEMATYC_PP_COMMA element(2, user_data)
#define SCHEMATYC_PP_COMMA_ENUM_4(element, user_data)  element(0, user_data) SCHEMATYC_PP_COMMA element(1, user_data) SCHEMATYC_PP_COMMA element(2, user_data) SCHEMATYC_PP_COMMA element(3, user_data)
#define SCHEMATYC_PP_COMMA_ENUM_5(element, user_data)  element(0, user_data) SCHEMATYC_PP_COMMA element(1, user_data) SCHEMATYC_PP_COMMA element(2, user_data) SCHEMATYC_PP_COMMA element(3, user_data) SCHEMATYC_PP_COMMA element(4, user_data)
#define SCHEMATYC_PP_COMMA_ENUM_6(element, user_data)  element(0, user_data) SCHEMATYC_PP_COMMA element(1, user_data) SCHEMATYC_PP_COMMA element(2, user_data) SCHEMATYC_PP_COMMA element(3, user_data) SCHEMATYC_PP_COMMA element(4, user_data) SCHEMATYC_PP_COMMA element(5, user_data)
#define SCHEMATYC_PP_COMMA_ENUM_7(element, user_data)  element(0, user_data) SCHEMATYC_PP_COMMA element(1, user_data) SCHEMATYC_PP_COMMA element(2, user_data) SCHEMATYC_PP_COMMA element(3, user_data) SCHEMATYC_PP_COMMA element(4, user_data) SCHEMATYC_PP_COMMA element(5, user_data) SCHEMATYC_PP_COMMA element(6, user_data)
#define SCHEMATYC_PP_COMMA_ENUM_8(element, user_data)  element(0, user_data) SCHEMATYC_PP_COMMA element(1, user_data) SCHEMATYC_PP_COMMA element(2, user_data) SCHEMATYC_PP_COMMA element(3, user_data) SCHEMATYC_PP_COMMA element(4, user_data) SCHEMATYC_PP_COMMA element(5, user_data) SCHEMATYC_PP_COMMA element(6, user_data) SCHEMATYC_PP_COMMA element(7, user_data)
#define SCHEMATYC_PP_COMMA_ENUM_9(element, user_data)  element(0, user_data) SCHEMATYC_PP_COMMA element(1, user_data) SCHEMATYC_PP_COMMA element(2, user_data) SCHEMATYC_PP_COMMA element(3, user_data) SCHEMATYC_PP_COMMA element(4, user_data) SCHEMATYC_PP_COMMA element(5, user_data) SCHEMATYC_PP_COMMA element(6, user_data) SCHEMATYC_PP_COMMA element(7, user_data) SCHEMATYC_PP_COMMA element(8, user_data)
#define SCHEMATYC_PP_COMMA_ENUM_10(element, user_data) element(0, user_data) SCHEMATYC_PP_COMMA element(1, user_data) SCHEMATYC_PP_COMMA element(2, user_data) SCHEMATYC_PP_COMMA element(3, user_data) SCHEMATYC_PP_COMMA element(4, user_data) SCHEMATYC_PP_COMMA element(5, user_data) SCHEMATYC_PP_COMMA element(6, user_data) SCHEMATYC_PP_COMMA element(7, user_data) SCHEMATYC_PP_COMMA element(8, user_data) SCHEMATYC_PP_COMMA element(9, user_data)

#define SCHEMATYC_PP_COMMA_ENUM(count, element, user_data) SCHEMATYC_PP_JOIN_XY(SCHEMATYC_PP_COMMA_ENUM_, count)(element, user_data)

#define SCHEMATYC_PP_ENUM_PARAMS_0(param)
#define SCHEMATYC_PP_ENUM_PARAMS_1(param)  SCHEMATYC_PP_JOIN_XY(param, 0)
#define SCHEMATYC_PP_ENUM_PARAMS_2(param)  SCHEMATYC_PP_JOIN_XY(param, 0), SCHEMATYC_PP_JOIN_XY(param, 1)
#define SCHEMATYC_PP_ENUM_PARAMS_3(param)  SCHEMATYC_PP_JOIN_XY(param, 0), SCHEMATYC_PP_JOIN_XY(param, 1), SCHEMATYC_PP_JOIN_XY(param, 2)
#define SCHEMATYC_PP_ENUM_PARAMS_4(param)  SCHEMATYC_PP_JOIN_XY(param, 0), SCHEMATYC_PP_JOIN_XY(param, 1), SCHEMATYC_PP_JOIN_XY(param, 2), SCHEMATYC_PP_JOIN_XY(param, 3)
#define SCHEMATYC_PP_ENUM_PARAMS_5(param)  SCHEMATYC_PP_JOIN_XY(param, 0), SCHEMATYC_PP_JOIN_XY(param, 1), SCHEMATYC_PP_JOIN_XY(param, 2), SCHEMATYC_PP_JOIN_XY(param, 3), SCHEMATYC_PP_JOIN_XY(param, 4)
#define SCHEMATYC_PP_ENUM_PARAMS_6(param)  SCHEMATYC_PP_JOIN_XY(param, 0), SCHEMATYC_PP_JOIN_XY(param, 1), SCHEMATYC_PP_JOIN_XY(param, 2), SCHEMATYC_PP_JOIN_XY(param, 3), SCHEMATYC_PP_JOIN_XY(param, 4), SCHEMATYC_PP_JOIN_XY(param, 5)
#define SCHEMATYC_PP_ENUM_PARAMS_7(param)  SCHEMATYC_PP_JOIN_XY(param, 0), SCHEMATYC_PP_JOIN_XY(param, 1), SCHEMATYC_PP_JOIN_XY(param, 2), SCHEMATYC_PP_JOIN_XY(param, 3), SCHEMATYC_PP_JOIN_XY(param, 4), SCHEMATYC_PP_JOIN_XY(param, 5), SCHEMATYC_PP_JOIN_XY(param, 6)
#define SCHEMATYC_PP_ENUM_PARAMS_8(param)  SCHEMATYC_PP_JOIN_XY(param, 0), SCHEMATYC_PP_JOIN_XY(param, 1), SCHEMATYC_PP_JOIN_XY(param, 2), SCHEMATYC_PP_JOIN_XY(param, 3), SCHEMATYC_PP_JOIN_XY(param, 4), SCHEMATYC_PP_JOIN_XY(param, 5), SCHEMATYC_PP_JOIN_XY(param, 6), SCHEMATYC_PP_JOIN_XY(param, 7)
#define SCHEMATYC_PP_ENUM_PARAMS_9(param)  SCHEMATYC_PP_JOIN_XY(param, 0), SCHEMATYC_PP_JOIN_XY(param, 1), SCHEMATYC_PP_JOIN_XY(param, 2), SCHEMATYC_PP_JOIN_XY(param, 3), SCHEMATYC_PP_JOIN_XY(param, 4), SCHEMATYC_PP_JOIN_XY(param, 5), SCHEMATYC_PP_JOIN_XY(param, 6), SCHEMATYC_PP_JOIN_XY(param, 7), SCHEMATYC_PP_JOIN_XY(param, 8)
#define SCHEMATYC_PP_ENUM_PARAMS_10(param) SCHEMATYC_PP_JOIN_XY(param, 0), SCHEMATYC_PP_JOIN_XY(param, 1), SCHEMATYC_PP_JOIN_XY(param, 2), SCHEMATYC_PP_JOIN_XY(param, 3), SCHEMATYC_PP_JOIN_XY(param, 4), SCHEMATYC_PP_JOIN_XY(param, 5), SCHEMATYC_PP_JOIN_XY(param, 6), SCHEMATYC_PP_JOIN_XY(param, 7), SCHEMATYC_PP_JOIN_XY(param, 8), SCHEMATYC_PP_JOIN_XY(param, 9)

#define SCHEMATYC_PP_ENUM_PARAMS(count, param) SCHEMATYC_PP_JOIN_XY(SCHEMATYC_PP_ENUM_PARAMS_, count)(param)