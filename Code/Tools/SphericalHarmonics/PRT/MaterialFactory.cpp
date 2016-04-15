/*
	image factory implementation
*/
#include "stdafx.h"

#if defined(OFFLINE_COMPUTATION)

#include <PRT/MaterialFactory.h>
#include "DefaultMaterial.h"
#include "DefaultAlphaMaterial.h"
#include "AlphaTextureMaterial.h"
#include "BaseTextureMaterial.h"
#include "BacklightingAlphaTextureMaterial.h"
#include "DefaultBacklightingMaterial.h"
#include <PRT/TransferParameters.h>
#include <PRT/ITransferConfigurator.h>
#include "TransferConfiguratorFactory.h"


NSH::NMaterial::CMaterialFactory* NSH::NMaterial::CMaterialFactory::Instance()
{
	static CMaterialFactory inst;
	return &inst;
}

const NSH::CSmartPtr<NSH::NMaterial::ISHMaterial, CSHAllocator<> > NSH::NMaterial::CMaterialFactory::GetMaterial(const EMaterialType cMaterialType, const SSharedMaterialInitData& rInitData)
{
	NSH::CSmartPtr<NSH::NMaterial::ISHMaterial, CSHAllocator<> > retMat;
	switch(cMaterialType)
	{
	case MATERIAL_TYPE_DEFAULT:
		if(rInitData.pARGBImageData && *rInitData.pARGBImageData)//dont needed, deallocate
		{
			static CSHAllocator<float> sAllocator;
			sAllocator.delete_mem_array(*rInitData.pARGBImageData, sizeof(float) * rInitData.imageWidth * rInitData.imageHeight * 4);
			*rInitData.pARGBImageData = NULL;
		}
		retMat = new CDefaultSHMaterial(rInitData.pMesh);
		break;
	case MATERIAL_TYPE_ALPHA_DEFAULT:
		if(rInitData.pARGBImageData && *rInitData.pARGBImageData)//dont needed, deallocate
		{
			static CSHAllocator<float> sAllocator;
			sAllocator.delete_mem_array(*rInitData.pARGBImageData, sizeof(float) * rInitData.imageWidth * rInitData.imageHeight * 4);
			*rInitData.pARGBImageData = NULL;
		}
		retMat = new CDefaultAlphaSHMaterial(rInitData.pMesh, rInitData.transparencyShadowingFactor);
		break;
	case MATERIAL_TYPE_BACKLIGHTING_DEFAULT:
		if(rInitData.pARGBImageData && *rInitData.pARGBImageData)//dont needed, deallocate
		{
			static CSHAllocator<float> sAllocator;
			sAllocator.delete_mem_array(*rInitData.pARGBImageData, sizeof(float) * rInitData.imageWidth * rInitData.imageHeight * 4);
			*rInitData.pARGBImageData = NULL;
		}
		retMat = new CDefaultBacklightingSHMaterial(rInitData.pMesh, rInitData.backlightingColour, rInitData.transparencyShadowingFactor);
		break;
	case MATERIAL_TYPE_BASETEXTURE:
		retMat = new CBaseTextureSHMaterial(rInitData.pMesh, *rInitData.pARGBImageData, rInitData.imageWidth, rInitData.imageHeight);
		break;
	case MATERIAL_TYPE_ALPHATEXTURE:
		retMat = new CAlphaTextureSHMaterial(rInitData.pMesh, *rInitData.pARGBImageData, rInitData.imageWidth, rInitData.imageHeight, rInitData.transparencyShadowingFactor);
		break;
	case MATERIAL_TYPE_BACKLIGHTING:
		retMat = new CBacklightingAlphaTextureSHMaterial(rInitData.pMesh, *rInitData.pARGBImageData, rInitData.imageWidth, rInitData.imageHeight, rInitData.backlightingColour, rInitData.transparencyShadowingFactor);
		break;
	default:
		retMat = new CDefaultSHMaterial(rInitData.pMesh);
		break;
	}
	retMat->SetDiffuseIntensity(rInitData.diffuseIntensity.r, rInitData.diffuseIntensity.g, rInitData.diffuseIntensity.b, rInitData.diffuseIntensity.a);
	return retMat;
}

#endif