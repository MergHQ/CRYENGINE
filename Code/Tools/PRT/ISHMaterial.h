/*
	material interface definition
*/
#if defined(OFFLINE_COMPUTATION)

#pragma once

#include "PRTTypes.h"

namespace NSH
{
	namespace NMaterial
	{
		/************************************************************************************************************************************************/

		//!< argb image struct, always this format if there is a alpha material
		struct SAlphaImageValue
		{
			float r;
			float g;
			float b;
			float a;
			SAlphaImageValue() : r(0.f), g(0.f), b(0.f), a(0.f){}
			SAlphaImageValue(const SAlphaImageValue& rCopyFrom) : r(rCopyFrom.r), g(rCopyFrom.g), b(rCopyFrom.b), a(rCopyFrom.a){}
			SAlphaImageValue(const float cR, const float cG, const float cB, const float cA) : r(cR), g(cG), b(cB), a(cA){}
		};

		inline NSH::NMaterial::SAlphaImageValue operator+(const NSH::NMaterial::SAlphaImageValue& rAdd0, const NSH::NMaterial::SAlphaImageValue& rAdd1)
		{
			return NSH::NMaterial::SAlphaImageValue(rAdd0.r + rAdd1.r, rAdd0.g + rAdd1.g, rAdd0.b + rAdd1.b, rAdd0.a + rAdd1.a);
		}

		inline NSH::NMaterial::SAlphaImageValue operator *(const NSH::NMaterial::SAlphaImageValue& rAlphaImageValue, const float cScalarFactor)
		{
			return NSH::NMaterial::SAlphaImageValue(rAlphaImageValue.r * cScalarFactor, rAlphaImageValue.g * cScalarFactor, rAlphaImageValue.b * cScalarFactor, rAlphaImageValue.a * cScalarFactor);
		}

		/************************************************************************************************************************************************/

		typedef enum EMaterialType
		{
			MATERIAL_TYPE_DEFAULT,				//!< default material without texture access
			MATERIAL_TYPE_ALPHA_DEFAULT,		//!< default material without texture access but transparency
			MATERIAL_TYPE_BASETEXTURE,		//!< base material with base texture access
			MATERIAL_TYPE_ALPHATEXTURE,		//!< alpha material with alpha texture access
			MATERIAL_TYPE_BACKLIGHTING,		//!< alpha material with alpha texture access and back lighting colour
			MATERIAL_TYPE_BACKLIGHTING_DEFAULT,		//!< alpha material without alpha texture but a back lighting colour
		}EMaterialType;

		/************************************************************************************************************************************************/

		//!< simple sh-related material interface used at preprocessing time
		interface ISHMaterial
		{
			//first the functions which don't necessarily needs to be shared
			//!< indicates transparency support
			virtual const bool HasTransparencyTransfer() const{return false;}

			//!< sets the diffuse material intensity
			virtual void SetDiffuseIntensity(const float cRedIntensity, const float cGreenIntensity, const float cBlueIntensity, const float cAlphaIntensity = 0.f) = 0;
			//!< returns diffuse intensity at a given angle, coloured incident intensity and surface orientation(expressed in barycentric coordinates and triangle index)
			virtual const TRGBCoeffD DiffuseIntensity(const TRGBCoeffD& crIncidentIntensity, const uint32 cTriangleIndex, const TVec& rBaryCoord, const TCartesianCoord& crIncidentAngle, const bool cApplyCosTerm = true, const bool cApplyExitanceTerm = false, const bool cAbsCosTerm = false, const bool cUseTransparency = false) const = 0;
			//!< returns diffuse intensity at a given angle, coloured incident intensity and surface pos(important for subsurface shader) and orientation
			virtual const TRGBCoeffD DiffuseIntensity(const TVec& crVertexPos, const TVec& crVertexNormal, const Vec2& rUVCoords, const TRGBCoeffD& crIncidentIntensity, const TCartesianCoord& crIncidentAngle, const bool cApplyCosTerm = true, const bool cApplyExitanceTerm = false, const bool cAbsCosTerm = false, const bool cUseTransparency = false) const = 0;
			//!< returns material type
			virtual const EMaterialType Type()const  = 0;
			//!< need virtual destructor due to the inheriting materials are using a different allocator
			virtual ~ISHMaterial(){}
		};
	}
}

#endif
