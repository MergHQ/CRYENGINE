// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "CgfUtil.h"
#include "CGF/ChunkFile.h"
#include "FileUtil.h"

namespace CgfUtil
{

bool WriteTempRename(CChunkFile& chunkFile, const string& targetFilename)
{
	string tmpFilename = targetFilename;
	tmpFilename += ".$tmp$";

	if (!chunkFile.Write(tmpFilename.c_str()))
	{
		RCLogError("%s", chunkFile.GetLastError());
		return false;
	}

	// Force remove of the read only flag.
	FileUtil::MakeWritable(targetFilename.c_str());
	remove(targetFilename.c_str());

	return rename(tmpFilename.c_str(), targetFilename.c_str()) == 0;
}

};