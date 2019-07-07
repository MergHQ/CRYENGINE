// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.
#include "StdAfx.h"
#include "Md5Generator.h"
#include <md5.h>

namespace Md5Generator
{

string Generate(const string& str, bool isLowerCase /*= false*/)
{
	uint8 hash[16];
	uint8* bla = reinterpret_cast<uint8*>(const_cast<char*>(str.c_str()));
	MD5Context context;
	MD5Init(&context);
	MD5Update(&context, bla, str.size());
	MD5Final(hash, &context);
	string result;
	for (uint8 i : hash)
	{
		result += string().Format("%02x", i);
	}
	return isLowerCase ? result : result.MakeUpper();
}

}
