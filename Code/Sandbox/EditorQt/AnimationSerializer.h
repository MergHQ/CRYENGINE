// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

struct IAnimSequence;
class CPakFile;
class XmlNodeRef;

/** Used by Editor to serialize animation data.
 */
class CAnimationSerializer
{
public:
	/** Save all animation sequences to files in given directory.
	 */
	void SerializeSequences(XmlNodeRef& xmlNode, bool bLoading);

	/** Saves single animation sequence to file in given directory.
	 */
	void SaveSequence(IAnimSequence* seq, const char* szFilePath, bool bSaveEmpty = true);

	/** Load sequence from file.
	 */
	IAnimSequence* LoadSequence(const char* szFilePath);

	/** Save all animation sequences to files in given directory.
	 */
	void SaveAllSequences(const char* szPath, CPakFile& pakFile);

	/** Load all animation sequences from given directory.
	 */
	void LoadAllSequences(const char* szPath);
};
