/*
	default material definition
*/
#if defined(OFFLINE_COMPUTATION)

#pragma once

#include <PRT/ISHMaterial.h>

class CSimpleIndexedMesh;

namespace NSH
{
	namespace NMaterial
	{
		//!< default sh-related material without subsurface transfer, diffuse model based on incident angle (cos)
		class CDefaultBacklightingSHMaterial : public ISHMaterial
		{
		public:
			INSTALL_CLASS_NEW(CDefaultBacklightingSHMaterial)

			CDefaultBacklightingSHMaterial
			(
				CSimpleIndexedMesh *pMesh, 
				const Vec3& crBacklightingColour,
				const float cTransparencyShadowingFactor = 1.f, 
				const float cAlphaIntensity = 0.f);
			//!< sets diffuse material components
			virtual void SetDiffuseIntensity(const float cRedIntensity, const float cGreenIntensity, const float cBlueIntensity, const float cAlphaIntensity = 0.f);
			//!< returns diffuse intensity at a given angle, coloured incident intensity and surface orientation
			virtual const TRGBCoeffD DiffuseIntensity(const TRGBCoeffD& crIncidentIntensity, const uint32 cTriangleIndex, const TVec& rBaryCoord, const TCartesianCoord& rIncidentDir, const bool cApplyCosTerm = true, const bool cApplyExitanceTerm = false, const bool cAbsCosTerm = false, const bool cUseTransparency = false)const;
			virtual const TRGBCoeffD DiffuseIntensity(const TVec&, const TVec& crVertexNormal, const Vec2&, const TRGBCoeffD& crIncidentIntensity, const TCartesianCoord& rIncidentDir, const bool cApplyCosTerm = true, const bool cApplyExitanceTerm = false, const bool cAbsCosTerm = false, const bool cUseTransparency = false)const;
			virtual const EMaterialType Type()const{return MATERIAL_TYPE_BACKLIGHTING_DEFAULT;}
			virtual const bool HasTransparencyTransfer() const{return true;}

		private:
			CSimpleIndexedMesh *m_pMesh;		//!< indexed triangle mesh where surface data get retrieves from, indices are not shared among triangles
			float m_RedIntensity;						//!< diffuse reflectance material property for red
			float m_GreenIntensity;					//!< diffuse reflectance material property for green	
			float m_BlueIntensity;					//!< diffuse reflectance material property for blue
			float	m_AlphaIntensity;					//!< transparency, 0 = opaque, 1 = transparent
			float m_TransparencyShadowingFactor; //!< factor 0..1 shadowing the transparency of materials to fake density(0: complete non transparent, 1: unchanged)
			float m_TransparencyAlphaFactor;//!< product of previous two factors
			Vec3  m_BacklightingColour;			//!< back lighting transparency colour
			bool  m_ApplyTransparencyAlpha;	//!< m_TransparencyAlphaFactor > 0?

			//!< helper function computing the outgoing intensity when cos angle positive
			const TRGBCoeffD ComputeDiffuseIntensity(const TRGBCoeffD& crIncidentIntensity, const bool cApplyExitanceTerm, const bool cApplyCosTerm, const bool cUseTransparency, const double cCosAngle) const;	
		};
	}
}

#endif