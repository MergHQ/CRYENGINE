/*
	image factory declaration
*/
#if defined(OFFLINE_COMPUTATION)

#pragma once

#include "ISHMaterial.h"

class CSimpleIndexedMesh;

namespace NSH
{
	namespace NTransfer
	{
		struct STransferParameters;
		interface ITransferConfigurator;
	}

	namespace NMaterial
	{
		struct SSharedMaterialInitData
		{
			CSimpleIndexedMesh *pMesh;					//!< mesh associated to material
			union
			{
				TRGBCoeffF** pRGBImageData;
				NSH::NMaterial::SAlphaImageValue** pARGBImageData;
			};																	//!< optional image data associated to material
			uint32 imageWidth;									//!< optional image data width associated to material
			uint32 imageHeight;									//!< optional image data height associated to material
			Vec3	 backlightingColour;					//!< optional backlighting colour associated to material
			float transparencyShadowingFactor;	//!< factor 0..1 shadowing the transparency of materials to fake density(0: complete non transparent, 1: unchanged)
			SAlphaImageValue diffuseIntensity;	//!< additional diffuse intensity for materials
			SSharedMaterialInitData() 
				: pRGBImageData(NULL), pMesh(NULL), imageWidth(0), imageHeight(0), backlightingColour(0,0,0), transparencyShadowingFactor(1.f), diffuseIntensity(1.f,1.f,1.f,0.f){}
		};

		//!< singleton to abstract image factory
		class CMaterialFactory
		{
		public:
			//!< singleton stuff
			static CMaterialFactory* Instance();

			const NSH::CSmartPtr<NSH::NMaterial::ISHMaterial, CSHAllocator<> > GetMaterial(const EMaterialType cMaterialType, const SSharedMaterialInitData& rInitData);
			const char* GetObjExportShaderName
			(
				const NSH::NTransfer::STransferParameters& crParameters, 
				const EMaterialType cMaterialType
			);

		private:
			//!< singleton stuff
			CMaterialFactory(){}
			CMaterialFactory(const CMaterialFactory&);
			CMaterialFactory& operator= (const CMaterialFactory&);
		};
	}
}

#endif
