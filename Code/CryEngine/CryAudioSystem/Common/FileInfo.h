// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

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
	void*       pFileData;            // pointer to the memory location of the file's contents
	size_t      memoryBlockAlignment; // memory alignment to be used for storing this file's contents in memory
	size_t      size;                 // file size
	char const* szFileName;           // file name
	char const* szFilePath;           // file path
	bool        bLocalized;           // is the file localized
	IFile*      pImplData;            // pointer to the implementation-specific data needed for this AudioFileEntry
};
} // namespace Impl
} // namespace CryAudio
