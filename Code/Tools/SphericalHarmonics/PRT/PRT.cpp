#include "stdafx.h"
#include "SHFramework.h"

#pragma comment(lib,"Winmm.lib")


void Dump()
{
#if defined(_DEBUG)
	//check allocation and set this value after program start into _crtBreakAlloc
	_CrtDumpMemoryLeaks();
	int hallo = 0; hallo += 1;
#endif 
}

int _tmain(int, _TCHAR*[])
{
	atexit( Dump );

	using namespace NSH;
	try 
	{

/*		const char *pImageName[10] = 
		{
			"E:/Martin/MasterCD/Textures/HDR/beach_probe.hdr", 
			"E:/Martin/MasterCD/Textures/HDR/building_probe.hdr",
			"E:/Martin/MasterCD/Textures/HDR/campus_probe.hdr",
			"E:/Martin/MasterCD/Textures/HDR/galileo_probe.hdr",
			"E:/Martin/MasterCD/Textures/HDR/grace_probe.hdr",
			"E:/Martin/MasterCD/Textures/HDR/kitchen_probe.hdr",
			"E:/Martin/MasterCD/Textures/HDR/rnl_probe.hdr",
			"E:/Martin/MasterCD/Textures/HDR/stpeters_probe.hdr",
			"E:/Martin/MasterCD/Textures/HDR/uffizi0_probe.hdr",
			"E:/Martin/MasterCD/Textures/HDR/uffizi_probe.hdr"
		};

		for(int i=0; i<10; ++i)
			NFramework::ProjectSphericalEnvironmentMap(pImageName[i]);
*/
/*		std::vector<const char*> objNames;	
//		objNames.push_back("E:/Martin/MasterCD/Objects/Quad/Quad.obj");
		objNames.push_back("E:/Martin/MasterCD/Objects/Green/g1.obj");
//		objNames.push_back("E:/Martin/MasterCD/Objects/max_mutant/max_mutant.OBJ");
		NTransfer::STransferParameters transferParams;		
		transferParams.interreflectionDepth = 3;
		transferParams.sampleCountPerVertex = 2000;
		std::vector<Matrix34> matrixVector;
		Matrix34 matrix;	matrix.SetIdentity();
		matrixVector.push_back(matrix);				
//		matrix.SetTranslationMat(Vec3_tpl<float>(10.f,0.f,0.f));
//		matrixVector.push_back(matrix);
		NFramework::ComputeMeshTransfer(objNames, matrixVector, transferParams, NFramework::E_COEFFICIENT_STORE_POLICY_OBJ);
*/
	}catch(const std::exception& rEx)
	{
		printf(rEx.what());
	}
	printf("\n");

	return 0;
}

