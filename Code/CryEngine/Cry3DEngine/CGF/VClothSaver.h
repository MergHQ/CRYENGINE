// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

//#include <CryString/CryPath.h>
#include <CryCore/CryTypeInfo.h>
//#include "../CryEngine/Cry3DEngine/CGF/ChunkFile.h"
#include <Cry3DEngine/CGF/CGFContent.h>
//#include "../CGA/Controller.h"
//#include "../CGA/ControllerPQ.h"
//#include "../CGA/ControllerPQLog.h"
//#include "LoaderCAF.h"

class CChunkData;
struct SVClothInfoCGF;

class CSaverVCloth
{
public:
	CSaverVCloth(CChunkData& chunkData, const SVClothInfoCGF* pVClothInfo, bool swapEndian);

	void WriteChunkHeader();
	void WriteChunkVertices();
	void WriteTriangleData();
	void WriteLraNotAttachedOrdered();
	void WriteLinks();

private:

	CChunkData* m_pChunkData;
	const SVClothInfoCGF* m_pVClothInfo;
	bool m_bSwapEndian;

};

