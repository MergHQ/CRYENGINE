// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Schematyc/Utils/IString.h"

namespace Schematyc
{
namespace StringUtils
{
// Is character numeric?
////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool CharIsNumeric(char x)
{
	return ((x >= '0') && (x <= '9'));
}

// Is character alphanumeric?
////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool CharIsAlphanumeric(char x)
{
	return ((x >= 'a') && (x <= 'z')) || ((x >= 'A') && (x <= 'Z')) || ((x >= '0') && (x <= '9'));
}

// Check to see if string contains non-alphanumeric characters.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool ContainsNonAlphaNumericChars(const char* szInput, const char* szExceptions = nullptr)
{
	if (szInput && (szInput[0] != '\0'))
	{
		for (const char* szPos = szInput; *szPos != '\0'; ++szPos)
		{
			if (!CharIsAlphanumeric(*szPos) && (!szExceptions || !strchr(szExceptions, *szPos)))
			{
				return true;
			}
		}
	}
	return false;
}

// Convert hexadecimal character to unsigned char.
////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr unsigned char HexCharToUnisgnedChar(char x)
{
	return x >= '0' && x <= '9' ? x - '0' :
		x >= 'a' && x <= 'f' ? x - 'a' + 10 :
		x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0;
}

// Convert hexadecimal character to unsigned short.
////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr unsigned short HexCharToUnisgnedShort(char x)
{
	return x >= '0' && x <= '9' ? x - '0' :
		x >= 'a' && x <= 'f' ? x - 'a' + 10 :
		x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0;
}

// Convert hexadecimal character to unsigned long.
////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr unsigned long HexCharToUnisgnedLong(char x)
{
	return x >= '0' && x <= '9' ? x - '0' :
		x >= 'a' && x <= 'f' ? x - 'a' + 10 :
		x >= 'A' && x <= 'F' ? x - 'A' + 10 : 0;
}

// Convert hexadecimal string to unsigned char.
////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr unsigned char HexStringToUnsignedChar(const char* szInput)
{
	return (HexCharToUnisgnedChar(szInput[0]) << 4) +
		HexCharToUnisgnedChar(szInput[1]);
}

// Convert hexadecimal string to unsigned short.
////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr unsigned short HexStringToUnsignedShort(const char* szInput)
{
	return (HexCharToUnisgnedShort(szInput[0]) << 12) +
		(HexCharToUnisgnedShort(szInput[1]) << 8) +
		(HexCharToUnisgnedShort(szInput[2]) << 4) +
		HexCharToUnisgnedShort(szInput[3]);
}

// Convert hexadecimal string to unsigned long.
////////////////////////////////////////////////////////////////////////////////////////////////////
constexpr unsigned long HexStringToUnsignedLong(const char* szInput)
{
	return (HexCharToUnisgnedLong(szInput[0]) << 28) +
		(HexCharToUnisgnedLong(szInput[1]) << 24) +
		(HexCharToUnisgnedLong(szInput[2]) << 20) +
		(HexCharToUnisgnedLong(szInput[3]) << 16) +
		(HexCharToUnisgnedLong(szInput[4]) << 12) +
		(HexCharToUnisgnedLong(szInput[5]) << 8) +
		(HexCharToUnisgnedLong(szInput[6]) << 4) +
		HexCharToUnisgnedLong(szInput[7]);
}

// Filter string.
////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool Filter(const char* szInput, const char* szFilter)
{
	// Convert inputs to lower case.
	
	string input = szInput;
	input.MakeLower();
	
	string filter = szFilter;
	filter.MakeLower();
	
	// Tokenize filter and ensure input contains all words in filter.
	
	string::size_type pos = 0;
	string::size_type filterEnd = -1;
	do
	{
		const string::size_type filterStart = filterEnd + 1;
		filterEnd = filter.find(' ', filterStart);
		pos = input.find(filter.substr(filterStart, filterEnd - filterStart), pos);
		if (pos == string::npos)
		{
			return false;
		}
		pos += filterEnd - filterStart;
	}
	while (filterEnd != string::npos);
	return true;
}

// Convert string to 'snake' case (all lower case with underscore word separators).
////////////////////////////////////////////////////////////////////////////////////////////////////
inline void ToSnakeCase(IString& value)
{
	string snakeCase;
	bool bPrevLowercase = false;
	for (const char* szSrc = value.c_str(); szSrc[0] != '\0'; ++szSrc)
	{
		if (szSrc[0] == ' ')
		{
			snakeCase.append("_");
		}
		else if ((szSrc[0] >= 'A') && (szSrc[0] <= 'Z'))
		{
			if (bPrevLowercase)
			{
				snakeCase.append("_");
			}
			snakeCase.append(1, szSrc[0] + ('a' - 'A'));
		}
		else
		{
			snakeCase.append(1, szSrc[0]);
		}
		bPrevLowercase = (szSrc[0] >= 'a') && (szSrc[0] <= 'z');
	}
	value.assign(snakeCase.c_str());
}

inline bool IsValidElementName(const char* szName, const char*& szErrorMessage)
{
	if (!szName || szName[0] == '\0')
	{
		szErrorMessage = "Name cannot be empty.";
		return false;
	}
	else if (CharIsNumeric(szName[0]))
	{
		szErrorMessage = "Name cannot start with a number.";
		return false;
	}
	else if (ContainsNonAlphaNumericChars(szName, "_"))
	{
		szErrorMessage = "Name can only contain alphanumeric characters and underscores.";
		return false;
	}
	return true;
}
} // StringUtils
} // Schematyc
