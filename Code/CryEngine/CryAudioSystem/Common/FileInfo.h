// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryAudio/IAudioInterfacesCommonData.h>

namespace CryAudio
{
namespace Impl
{
struct IFile;

/**
 * This is a POD structure used to pass the information about a file preloaded into memory between
 * the CryAudioSystem and an implementation
 * Note: This struct cannot define a constructor, it needs to be a POD!
 */
struct SFileInfo
{
	void*  pFileData = nullptr;                       // pointer to the memory location of the file's contents
	size_t memoryBlockAlignment = 0;                  // memory alignment to be used for storing this file's contents in memory
	size_t size = 0;                                  // file size
	char   fileName[MaxFileNameLength] = { '\0' };    // file name
	char   filePath[MaxFilePathLength] = { '\0' };    // file path
	bool   isLocalized = false;                       // is the file localized
	IFile* pImplData = nullptr;                       // pointer to the implementation-specific data needed for this AudioFileEntry
};
} // namespace Impl
} // namespace CryAudio
