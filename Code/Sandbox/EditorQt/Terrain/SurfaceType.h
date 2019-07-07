// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once
#include "SandboxAPI.h"
#include <CryCore/smartptr.h>

/** Defines axises of projection for detail texture.
 */
enum ESurfaceTypeProjectionAxis
{
	ESFT_X,
	ESFT_Y,
	ESFT_Z,
};

/** CSurfaceType describe parameters of terrain surface.
   Surface types are applied to the layers, total of ms_maxSurfaceTypeIdCount surface types are currently supported.
 */
class SANDBOX_API CSurfaceType : public _i_reference_target_t
{
public:
	CSurfaceType();

	void          SetSurfaceTypeID(int nID) { m_surfaceTypeID = nID; }
	int           GetSurfaceTypeID() const  { return m_surfaceTypeID; }
	void          AssignUnusedSurfaceTypeID();

	void          SetName(const string& name)              { m_name = name; }
	const string& GetName() const                          { return m_name; }

	void          SetDetailTexture(const string& tex)      { m_detailTexture = tex; }
	const string& GetDetailTexture() const                 { return m_detailTexture; }

	void          SetBumpmap(const string& tex)            { m_bumpmap = tex; }
	const string& GetBumpmap() const                       { return m_bumpmap; }

	void          SetDetailTextureScale(const Vec3& scale) { m_detailScale[0] = scale.x; m_detailScale[1] = scale.y; }
	Vec3          GetDetailTextureScale() const            { return Vec3(m_detailScale[0], m_detailScale[1], 0); }

	void          SetMaterial(const string& mtl)           { m_material = mtl; }
	const string& GetMaterial() const                      { return m_material; }

	//! Set detail texture projection axis
	void SetProjAxis(int axis) { m_projAxis = axis; }

	//! Get detail texture projection axis
	int GetProjAxis() const { return m_projAxis; }

	void Serialize(class CXmlArchive& xmlAr);
	void Serialize(XmlNodeRef xmlRootNode, bool boLoading);

	//! Maximum SurfaceTypeId count (unique id from 0)
	static const int ms_maxSurfaceTypeIdCount;

private:
	void SaveVegetationIds(XmlNodeRef& node);

	//! Name of surface type.
	string m_name;
	//! Detail texture applied to this surface type.
	string m_detailTexture;
	//! Bump map for this surface type.
	string m_bumpmap;
	//! Detail texture tiling.
	float  m_detailScale[2];
	string m_material;

	//! Detail texture projection axis.
	int m_projAxis;

	//! Surface type in in 3dengine [0; CSurfaceType::ms_maxSurfaceTypeIdCount)
	int m_surfaceTypeID;
};
