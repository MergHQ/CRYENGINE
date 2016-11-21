// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DriverD3D.h"
#include "D3DRenderAuxGeom.h"
#include "GraphicsPipeline/StandardGraphicsPipeline.h"
#include <climits>

#if defined(ENABLE_RENDER_AUX_GEOM)

const float c_clipThres(0.1f);

enum EAuxGeomBufferSizes
{
	e_auxGeomVBSize = 0xffff,
	e_auxGeomIBSize = e_auxGeomVBSize * 2 * 3
};

struct SAuxObjVertex
{
	SAuxObjVertex()
	{
	}

	SAuxObjVertex(const Vec3& pos, const Vec3& normal)
		: m_pos(pos)
		, m_normal(normal)
	{
	}

	Vec3 m_pos;
	Vec3 m_normal;
};

typedef std::vector<SAuxObjVertex> AuxObjVertexBuffer;
typedef std::vector<vtx_idx>       AuxObjIndexBuffer;

CRenderAuxGeomD3D::CRenderAuxGeomD3D(CD3D9Renderer& renderer)
	: m_renderer(renderer)
	, m_geomPass(false)
	, m_wndXRes(0)
	, m_wndYRes(0)
	, m_aspect(1.0f)
	, m_aspectInv(1.0f)
	, m_matrices()
	, m_curPrimType(CAuxGeomCB::e_PrimTypeInvalid)
	, m_curPointSize(1)
	, m_curTransMatrixIdx(-1)
	, m_curWorldMatrixIdx(-1)
	, m_pAuxGeomShader(0)
	, m_curDrawInFrontMode(e_DrawInFrontOff)
	, m_auxSortedPushBuffer()
	, m_pCurCBRawData(0)
	, m_auxGeomCBCol()
	, CV_r_auxGeom(1)
{
	REGISTER_CVAR2("r_auxGeom", &CV_r_auxGeom, 1, VF_NULL, "");
}

CRenderAuxGeomD3D::~CRenderAuxGeomD3D()
{
	ReleaseDeviceObjects();

	//SAFE_RELEASE(m_pAuxGeomShader);
	// m_pAuxGeomShader released by CShaderMan ???
	//delete m_pAuxGeomShader;
	//m_pAuxGeomShader = 0;
}

void CRenderAuxGeomD3D::GetMemoryUsage(ICrySizer* pSizer) const
{
	pSizer->AddObject((char*)this - 16, sizeof(*this) + 16);  // adjust for new operator
	pSizer->AddObject(m_auxSortedPushBuffer);
	pSizer->AddObject(m_auxGeomCBCol);
}

void CRenderAuxGeomD3D::ReleaseDeviceObjects()
{
	for (uint32 i(0); i < e_auxObjNumLOD; ++i)
	{
		m_sphereObj[i].Release();
		m_coneObj[i].Release();
		m_cylinderObj[i].Release();
	}
}

int CRenderAuxGeomD3D::GetDeviceDataSize()
{
	int nSize = 0;

	for (uint32 i = 0; i < e_auxObjNumLOD; ++i)
	{
		nSize += m_sphereObj[i].GetDeviceDataSize();
		nSize += m_coneObj[i].GetDeviceDataSize();
		nSize += m_cylinderObj[i].GetDeviceDataSize();
	}
	return nSize;
}

// function to generate a sphere mesh
static void CreateSphere(AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib, float radius, unsigned int rings, unsigned int sections)
{
	// calc required number of vertices/indices/triangles to build a sphere for the given parameters
	uint32 numVertices((rings - 1) * (sections + 1) + 2);
	uint32 numTriangles((rings - 2) * sections * 2 + 2 * sections);
	uint32 numIndices(numTriangles * 3);

	// setup buffers
	vb.clear();
	vb.reserve(numVertices);

	ib.clear();
	ib.reserve(numIndices);

	// 1st pole vertex
	vb.push_back(SAuxObjVertex(Vec3(0.0f, 0.0f, radius), Vec3(0.0f, 0.0f, 1.0f)));

	// calculate "inner" vertices
	float sectionSlice(DEG2RAD(360.0f / (float) sections));
	float ringSlice(DEG2RAD(180.0f / (float) rings));

	for (uint32 a(1); a < rings; ++a)
	{
		float w(sinf(a * ringSlice));
		for (uint32 i(0); i <= sections; ++i)
		{
			Vec3 v;
			v.x = radius * cosf(i * sectionSlice) * w;
			v.y = radius * sinf(i * sectionSlice) * w;
			v.z = radius * cosf(a * ringSlice);
			vb.push_back(SAuxObjVertex(v, v.GetNormalized()));
		}
	}

	// 2nd vertex of pole (for end cap)
	vb.push_back(SAuxObjVertex(Vec3(0.0f, 0.0f, -radius), Vec3(0.0f, 0.0f, 1.0f)));

	// build "inner" faces
	for (uint32 a(0); a < rings - 2; ++a)
	{
		for (uint32 i(0); i < sections; ++i)
		{
			ib.push_back((vtx_idx) (1 + a * (sections + 1) + i + 1));
			ib.push_back((vtx_idx) (1 + a * (sections + 1) + i));
			ib.push_back((vtx_idx) (1 + (a + 1) * (sections + 1) + i + 1));

			ib.push_back((vtx_idx) (1 + (a + 1) * (sections + 1) + i));
			ib.push_back((vtx_idx) (1 + (a + 1) * (sections + 1) + i + 1));
			ib.push_back((vtx_idx) (1 + a * (sections + 1) + i));
		}
	}

	// build faces for end caps (to connect "inner" vertices with poles)
	for (uint32 i(0); i < sections; ++i)
	{
		ib.push_back((vtx_idx) (1 + (0) * (sections + 1) + i));
		ib.push_back((vtx_idx) (1 + (0) * (sections + 1) + i + 1));
		ib.push_back((vtx_idx) 0);
	}

	for (uint32 i(0); i < sections; ++i)
	{
		ib.push_back((vtx_idx) (1 + (rings - 2) * (sections + 1) + i + 1));
		ib.push_back((vtx_idx) (1 + (rings - 2) * (sections + 1) + i));
		ib.push_back((vtx_idx) ((rings - 1) * (sections + 1) + 1));
	}
}

// function to generate a cone mesh
static void CreateCone(AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib, float radius, float height, unsigned int sections)
{
	// calc required number of vertices/indices/triangles to build a cone for the given parameters
	uint32 numVertices(2 * (sections + 1) + 2);
	uint32 numTriangles(2 * sections);
	uint32 numIndices(numTriangles * 3);

	// setup buffers
	vb.clear();
	vb.reserve(numVertices);

	ib.clear();
	ib.reserve(numIndices);

	// center vertex
	vb.push_back(SAuxObjVertex(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f)));

	// create circle around it
	float sectionSlice(DEG2RAD(360.0f / (float) sections));
	for (uint32 i(0); i <= sections; ++i)
	{
		Vec3 v;
		v.x = radius * cosf(i * sectionSlice);
		v.y = 0.0f;
		v.z = radius * sinf(i * sectionSlice);
		vb.push_back(SAuxObjVertex(v, Vec3(0.0f, -1.0f, 0.0f)));
	}

	// build faces for end cap
	for (uint16 i(0); i < sections; ++i)
	{
		ib.push_back((vtx_idx)(0));
		ib.push_back((vtx_idx)(1 + i));
		ib.push_back((vtx_idx)(1 + i + 1));
	}

	// top
	vb.push_back(SAuxObjVertex(Vec3(0.0f, height, 0.0f), Vec3(0.0f, 1.0f, 0.0f)));

	for (uint32 i(0); i <= sections; ++i)
	{
		Vec3 v;
		v.x = radius * cosf(i * sectionSlice);
		v.y = 0.0f;
		v.z = radius * sinf(i * sectionSlice);

		Vec3 v1;
		v1.x = radius * cosf(i * sectionSlice + 0.01f);
		v1.y = 0.0f;
		v1.z = radius * sinf(i * sectionSlice + 0.01f);

		Vec3 d(v1 - v);
		Vec3 d1(Vec3(0.0, height, 0.0f) - v);

		Vec3 n((d1.Cross(d)).normalized());
		vb.push_back(SAuxObjVertex(v, n));
	}

	// build faces
	for (uint16 i(0); i < sections; ++i)
	{
		ib.push_back((vtx_idx)(sections + 2));
		ib.push_back((vtx_idx)(sections + 3 + i + 1));
		ib.push_back((vtx_idx)(sections + 3 + i));
	}
}

// function to generate a cylinder mesh
static void CreateCylinder(AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib, float radius, float height, unsigned int sections)
{
	// calc required number of vertices/indices/triangles to build a cylinder for the given parameters
	uint32 numVertices(4 * (sections + 1) + 2);
	uint32 numTriangles(4 * sections);
	uint32 numIndices(numTriangles * 3);

	// setup buffers
	vb.clear();
	vb.reserve(numVertices);

	ib.clear();
	ib.reserve(numIndices);

	float sectionSlice(DEG2RAD(360.0f / (float) sections));

	// bottom cap
	{
		// center bottom vertex
		vb.push_back(SAuxObjVertex(Vec3(0.0f, -0.5f * height, 0.0f), Vec3(0.0f, -1.0f, 0.0f)));

		// create circle around it
		for (uint32 i(0); i <= sections; ++i)
		{
			Vec3 v;
			v.x = radius * cosf(i * sectionSlice);
			v.y = -0.5f * height;
			v.z = radius * sinf(i * sectionSlice);
			vb.push_back(SAuxObjVertex(v, Vec3(0.0f, -1.0f, 0.0f)));
		}

		// build faces
		for (uint16 i(0); i < sections; ++i)
		{
			ib.push_back((vtx_idx)(0));
			ib.push_back((vtx_idx)(1 + i));
			ib.push_back((vtx_idx)(1 + i + 1));
		}
	}

	// side
	{
		int vIdx(vb.size());

		for (uint32 i(0); i <= sections; ++i)
		{
			Vec3 v;
			v.x = radius * cosf(i * sectionSlice);
			v.y = -0.5f * height;
			v.z = radius * sinf(i * sectionSlice);

			Vec3 n(v.normalized());
			vb.push_back(SAuxObjVertex(v, n));
			vb.push_back(SAuxObjVertex(Vec3(v.x, -v.y, v.z), n));
		}

		// build faces
		for (uint16 i(0); i < sections; ++i, vIdx += 2)
		{
			ib.push_back((vtx_idx)(vIdx));
			ib.push_back((vtx_idx)(vIdx + 1));
			ib.push_back((vtx_idx)(vIdx + 2));

			ib.push_back((vtx_idx)(vIdx + 1));
			ib.push_back((vtx_idx)(vIdx + 3));
			ib.push_back((vtx_idx)(vIdx + 2));

		}
	}

	// top cap
	{
		int vIdx(vb.size());

		// center top vertex
		vb.push_back(SAuxObjVertex(Vec3(0.0f, 0.5f * height, 0.0f), Vec3(0.0f, 1.0f, 0.0f)));

		// create circle around it
		for (uint32 i(0); i <= sections; ++i)
		{
			Vec3 v;
			v.x = radius * cosf(i * sectionSlice);
			v.y = 0.5f * height;
			v.z = radius * sinf(i * sectionSlice);
			vb.push_back(SAuxObjVertex(v, Vec3(0.0f, 1.0f, 0.0f)));
		}

		// build faces
		for (uint16 i(0); i < sections; ++i)
		{
			ib.push_back(vIdx);
			ib.push_back(vIdx + 1 + i + 1);
			ib.push_back(vIdx + 1 + i);
		}
	}
}

// Functor to generate a sphere mesh. To be used with generalized CreateMesh function.
struct SSphereMeshCreateFunc
{
	SSphereMeshCreateFunc(float radius, unsigned int rings, unsigned int sections)
		: m_radius(radius)
		, m_rings(rings)
		, m_sections(sections)
	{
	}

	void CreateMesh(AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib)
	{
		CreateSphere(vb, ib, m_radius, m_rings, m_sections);
	}

	float        m_radius;
	unsigned int m_rings;
	unsigned int m_sections;
};

// Functor to generate a cone mesh. To be used with generalized CreateMesh function.
struct SConeMeshCreateFunc
{
	SConeMeshCreateFunc(float radius, float height, unsigned int sections)
		: m_radius(radius)
		, m_height(height)
		, m_sections(sections)
	{
	}

	void CreateMesh(AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib)
	{
		CreateCone(vb, ib, m_radius, m_height, m_sections);
	}

	float        m_radius;
	float        m_height;
	unsigned int m_sections;
};

// Functor to generate a cylinder mesh. To be used with generalized CreateMesh function.
struct SCylinderMeshCreateFunc
{
	SCylinderMeshCreateFunc(float radius, float height, unsigned int sections)
		: m_radius(radius)
		, m_height(height)
		, m_sections(sections)
	{
	}

	void CreateMesh(AuxObjVertexBuffer& vb, AuxObjIndexBuffer& ib)
	{
		CreateCylinder(vb, ib, m_radius, m_height, m_sections);
	}

	float        m_radius;
	float        m_height;
	unsigned int m_sections;
};

void CRenderAuxGeomD3D::SDrawObjMesh::Release()
{
	if( m_pVB != ~0u ) gcpRendD3D->m_DevBufMan.Destroy(m_pVB);
	if( m_pIB != ~0u ) gcpRendD3D->m_DevBufMan.Destroy(m_pIB);

	m_numVertices = 0;
	m_numFaces = 0;
}

int CRenderAuxGeomD3D::SDrawObjMesh::GetDeviceDataSize() const
{
	int nSize = 0;
	nSize += gcpRendD3D->m_DevBufMan.Size(m_pVB);
	nSize += gcpRendD3D->m_DevBufMan.Size(m_pIB);

	return nSize;
}

// Generalized CreateMesh function to create any mesh via passed-in functor.
// The functor needs to provide a CreateMesh function which accepts an
// AuxObjVertexBuffer and AuxObjIndexBuffer to stored the resulting mesh.
template<typename TMeshFunc>
HRESULT CRenderAuxGeomD3D::CreateMesh(SDrawObjMesh& mesh, TMeshFunc meshFunc)
{
	// create mesh
	AuxObjVertexBuffer vb;
	AuxObjIndexBuffer  ib;
	meshFunc.CreateMesh(vb, ib);

	// create vertex buffer and copy data

	mesh.m_pVB = gcpRendD3D->m_DevBufMan.Create(BBT_VERTEX_BUFFER, BU_STATIC, vb.size() * sizeof(SAuxObjVertex));
	mesh.m_pIB = gcpRendD3D->m_DevBufMan.Create(BBT_INDEX_BUFFER,  BU_STATIC, ib.size() * sizeof(vtx_idx));

	gcpRendD3D->m_DevBufMan.UpdateBuffer(mesh.m_pVB, &vb[0], vb.size() * sizeof(SAuxObjVertex));
	gcpRendD3D->m_DevBufMan.UpdateBuffer(mesh.m_pIB, &ib[0], ib.size() * sizeof(vtx_idx));


#if !defined(RELEASE) && CRY_PLATFORM_WINDOWS
	{
		size_t vbOffset = 0, ibOffset = 0;
		D3DVertexBuffer* vbAux = gcpRendD3D->m_DevBufMan.GetD3DVB(mesh.m_pVB, &vbOffset);
		D3DIndexBuffer * ibAux = gcpRendD3D->m_DevBufMan.GetD3DIB(mesh.m_pIB, &ibOffset);

		vbAux->SetPrivateData(WKPDID_D3DDebugObjectName, strlen("Aux Geom Mesh"), "Aux Geom Mesh");
		ibAux->SetPrivateData(WKPDID_D3DDebugObjectName, strlen("Aux Geom Mesh"), "Aux Geom Mesh");
	}
#endif

	// write mesh info
	mesh.m_numVertices = vb.size();
	mesh.m_numFaces = ib.size() / 3;

	return S_OK;
}

HRESULT CRenderAuxGeomD3D::RestoreDeviceObjects()
{
	HRESULT hr(S_OK);


	// recreate aux objects
	for (uint32 i(0); i < e_auxObjNumLOD; ++i)
	{
		m_sphereObj[i].Release();
		if (FAILED(hr = CreateMesh(m_sphereObj[i], SSphereMeshCreateFunc(1.0f, 9 + 4 * i, 9 + 4 * i))))
		{
			return(hr);
		}

		m_coneObj[i].Release();
		if (FAILED(hr = CreateMesh(m_coneObj[i], SConeMeshCreateFunc(1.0f, 1.0f, 10 + i * 6))))
		{
			return(hr);
		}

		m_cylinderObj[i].Release();
		if (FAILED(hr = CreateMesh(m_cylinderObj[i], SCylinderMeshCreateFunc(1.0f, 1.0f, 10 + i * 6))))
		{
			return(hr);
		}
	}
	return(hr);
}

template<class T = vtx_idx, int size = sizeof(T)*CHAR_BIT> struct indexbuffer_type;

template<class T> struct indexbuffer_type<T, 16> { static const RenderIndexType type = Index16; };
template<class T> struct indexbuffer_type<T, 32> { static const RenderIndexType type = Index32; };


CRenderAuxGeomD3D::CBufferManager::~CBufferManager()
{
	if( vbAux != ~0u ) gcpRendD3D->m_DevBufMan.Destroy(vbAux);
	if( ibAux != ~0u ) gcpRendD3D->m_DevBufMan.Destroy(ibAux);
}

buffer_handle_t CRenderAuxGeomD3D::CBufferManager::update(BUFFER_BIND_TYPE type, const void* data, size_t size)
{
	buffer_handle_t buf = gcpRendD3D->m_DevBufMan.Create(type, BU_TRANSIENT, size);//BU_DYNAMIC

	gcpRendD3D->m_DevBufMan.UpdateBuffer(buf, data, size);

	return buf;
}

buffer_handle_t CRenderAuxGeomD3D::CBufferManager::fill(buffer_handle_t buf, BUFFER_BIND_TYPE type, const void* data, size_t size)
{
	if( buf != ~0u ) gcpRendD3D->m_DevBufMan.Destroy(buf);

	return size ? update(type, data, size) : ~0u;
}


CRenderPrimitive& CRenderAuxGeomD3D::PreparePrimitive(const SAuxGeomRenderFlags& flags, const CCryNameTSCRC& techique, ERenderPrimitiveType topology, EVertexFormat format, size_t stride, buffer_handle_t vb, buffer_handle_t ib, const Matrix44* mViewProj)
{
	CD3D9Renderer* const __restrict renderer = gcpRendD3D;

	CStandardGraphicsPipeline::SViewInfo viewInfo[2];
	const int32 viewInfoCount = renderer->GetGraphicsPipeline().GetViewInfo(viewInfo);

	const bool bReverseDepth = (viewInfo[0].flags & CStandardGraphicsPipeline::SViewInfo::eFlags_ReverseDepth) != 0;
	int32 gsFunc = bReverseDepth ? GS_DEPTHFUNC_GEQUAL : GS_DEPTHFUNC_LEQUAL;


	const SViewport& vp = viewInfo[0].viewport;
	D3DViewPort viewport = { float(vp.nX), float(vp.nY), float(vp.nX) + vp.nWidth, float(vp.nY) + vp.nHeight, vp.fMinZ, vp.fMaxZ };


	m_geomPass.ClearPrimitives();
	m_geomPass.SetViewport(viewport);

	m_geomPass.SetRenderTarget(0, renderer->GetBackBufferTexture());
	m_geomPass.SetDepthTarget(&renderer->m_DepthBufferNative);

	auto& prim = m_geomPrimitiveCache[topology];

	prim.SetFlags(CRenderPrimitive::eFlags_ReflectConstantBuffersFromShader);

	prim.SetTechnique(m_pAuxGeomShader, techique, 0);


	if( flags.GetDepthTestFlag()  == e_DepthTestOff ) gsFunc |= GS_NODEPTHTEST;
	if( flags.GetDepthWriteFlag() == e_DepthWriteOn ) gsFunc |= GS_DEPTHWRITE;

	if( flags.GetFillMode() == e_FillModeWireframe ) gsFunc |= GS_WIREFRAME;


	switch( flags.GetAlphaBlendMode() )
	{
		case e_AlphaBlended : gsFunc |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA; break;
		case e_AlphaAdditive: gsFunc |= GS_BLSRC_ONE | GS_BLDST_ONE;
	}

	switch( flags.GetCullMode() )
	{
		case e_CullModeFront: prim.SetCullMode(eCULL_Front); break;
		case e_CullModeNone : prim.SetCullMode(eCULL_None);  break;
		default:              prim.SetCullMode(eCULL_Back);
	}

	prim.SetRenderState(gsFunc);


	prim.SetCustomVertexStream(vb, format, stride);
	prim.SetCustomIndexStream (ib, indexbuffer_type<vtx_idx>::type);

	//		prim.SetTexture(0, CTexture::GetByID(pfProxy->GetTexture()));
	//		prim.SetSampler(0, CTexture::GetTexState(STexState(FILTER_LINEAR, true)));


	if( mViewProj )
	{
		static CCryNameR matViewProjName("matViewProj");

		auto& constantManager = prim.GetConstantManager();

		constantManager.BeginNamedConstantUpdate();
		constantManager.SetNamedConstantArray(matViewProjName, (Vec4*)mViewProj, 4, eHWSC_Vertex);
		constantManager.EndNamedConstantUpdate();
	}

	prim.m_instances.resize(1);

	return prim;
}


void CRenderAuxGeomD3D::DrawAuxPrimitives(CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd, const Matrix44& mViewProj)
{
	ERenderPrimitiveType topology;

	const SAuxGeomRenderFlags& flags = (*itBegin)->m_renderFlags;
	CAuxGeomCB::EPrimType primType(CAuxGeomCB::GetPrimType(flags));

	if     ( primType == CAuxGeomCB::e_LineList ) topology = eptLineList;
	else if( primType == CAuxGeomCB::e_TriList  ) topology = eptTriangleList;
	else if( primType == CAuxGeomCB::e_PtList   ) topology = eptPointList;
	else
	{
		assert(false);
		return;
	}

	static CCryNameTSCRC tGeom("AuxGeometry");

	CRenderPrimitive& prim = PreparePrimitive(flags, tGeom, topology, eVF_P3F_C4B_T2F, sizeof(SVF_P3F_C4B_T2F), m_bufman.GetVB(), ~0u, &mViewProj);

	prim.SetDrawInfo(topology, 0, (*itBegin)->m_vertexOffs, (*itBegin)->m_numVertices);

	for( CAuxGeomCB::AuxSortedPushBuffer::const_iterator it(itBegin); ++it != itEnd; )
	{
		SCompiledRenderPrimitive::SInstanceInfo instance;

		instance.vertexBaseOffset = 0;
		instance.vertexOrIndexOffset = (*it)->m_vertexOffs;
		instance.vertexOrIndexCount = (*it)->m_numVertices;

		prim.m_instances.push_back(instance);
	}

	if( !m_geomPass.AddPrimitive(&prim) )
	{
		return;
	}

	m_geomPass.Execute();
}

void CRenderAuxGeomD3D::DrawAuxIndexedPrimitives(CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd, const Matrix44& mViewProj)
{
	ERenderPrimitiveType topology;

	const SAuxGeomRenderFlags& flags = (*itBegin)->m_renderFlags;
	CAuxGeomCB::EPrimType primType(CAuxGeomCB::GetPrimType(flags));

	if     ( primType == CAuxGeomCB::e_LineListInd ) topology = eptLineList;
	else if( primType == CAuxGeomCB::e_TriListInd  ) topology = eptTriangleList;
	else
	{
		assert(false);
		return;
	}

	static CCryNameTSCRC tGeom("AuxGeometry");

	CRenderPrimitive& prim = PreparePrimitive(flags, tGeom, topology, eVF_P3F_C4B_T2F, sizeof(SVF_P3F_C4B_T2F), m_bufman.GetVB(), m_bufman.GetIB(), &mViewProj);

	prim.SetDrawInfo(topology, (*itBegin)->m_vertexOffs, (*itBegin)->m_indexOffs, (*itBegin)->m_numIndices);

	for( CAuxGeomCB::AuxSortedPushBuffer::const_iterator it(itBegin); ++it != itEnd; )
	{
		SCompiledRenderPrimitive::SInstanceInfo instance;

		instance.vertexBaseOffset = (*it)->m_vertexOffs;
		instance.vertexOrIndexOffset = (*it)->m_indexOffs;
		instance.vertexOrIndexCount = (*it)->m_numIndices;

		prim.m_instances.push_back(instance);
	}

	if( !m_geomPass.AddPrimitive(&prim) )
	{
		return;
	}

	m_geomPass.Execute();
}

void CRenderAuxGeomD3D::DrawAuxObjects(CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd, const Matrix44& mViewProj)
{
	const SAuxGeomRenderFlags& flags    = (*itBegin)->m_renderFlags;
	CAuxGeomCB::EAuxDrawObjType objType = CAuxGeomCB::GetAuxObjType(flags);

	// get draw params buffer
	const CAuxGeomCB::AuxDrawObjParamBuffer& auxDrawObjParamBuffer(GetAuxDrawObjParamBuffer());

	// process each entry
	for (CAuxGeomCB::AuxSortedPushBuffer::const_iterator it(itBegin); it != itEnd; ++it)
	{
		// get current push buffer entry
		const CAuxGeomCB::SAuxPushBufferEntry* obj(*it);

		// assert than all objects in this batch are of same type
		assert(CAuxGeomCB::GetAuxObjType(obj->m_renderFlags) == objType);

		uint32 drawParamOffs;

		if( !obj->GetDrawParamOffs(drawParamOffs) )
		{
			assert(false);
			continue;
		}

		// get draw params
		const CAuxGeomCB::SAuxDrawObjParams& drawParams(auxDrawObjParamBuffer[drawParamOffs]);

		// Prepare d3d world space matrix in draw param structure
		// Attention: in d3d terms matWorld is actually matWorld^T

		const Matrix44A& matView = GetCurrentView();
		Matrix44A        matWorld;

		matWorld.SetIdentity();
		memcpy(&matWorld, &drawParams.m_matWorld, sizeof(drawParams.m_matWorld));


		// LOD calculation
		Matrix44 matWorldTrans = matWorld.GetTransposed();
		Vec4 objCenterWorld;
		Vec3 nullVec(0.0f, 0.0f, 0.0f);
		mathVec3TransformF(&objCenterWorld, &nullVec, &matWorldTrans);
		Vec4 objOuterRightWorld(objCenterWorld + (Vec4(matView.m00, matView.m10, matView.m20, 0.0f) * drawParams.m_size));

		Vec4 v0, v1;

		Vec3 objCenterWorldVec(objCenterWorld.x, objCenterWorld.y, objCenterWorld.z);
		Vec3 objOuterRightWorldVec(objOuterRightWorld.x, objOuterRightWorld.y, objOuterRightWorld.z);
		mathVec3TransformF(&v0, &objCenterWorldVec, m_matrices.m_pCurTransMat);
		mathVec3TransformF(&v1, &objOuterRightWorldVec, m_matrices.m_pCurTransMat);

		float scale;
		assert(fabs(v0.w - v0.w) < 1e-4);
		if( fabs(v0.w) < 1e-2 )
		{
			scale = 0.5f;
		}
		else
		{
			scale = ((v1.x - v0.x) / v0.w) * (float)max(m_wndXRes, m_wndYRes) / 500.0f;
		}

		// map scale to detail level
		uint32 lodLevel((uint32)((scale / 0.5f) * (e_auxObjNumLOD - 1)));
		if( lodLevel >= e_auxObjNumLOD )
		{
			lodLevel = e_auxObjNumLOD - 1;
		}

		// get appropriate mesh
		assert(lodLevel >= 0 && lodLevel < e_auxObjNumLOD);

		SDrawObjMesh* pMesh = 0;
		switch( objType )
		{
			case CAuxGeomCB::eDOT_Cylinder: pMesh = m_cylinderObj;  break;
			case CAuxGeomCB::eDOT_Cone:     pMesh = m_coneObj;      break;

			default:
			case CAuxGeomCB::eDOT_Sphere:   pMesh = m_sphereObj;
		}

		assert(0 != pMesh);

		pMesh = &pMesh[lodLevel];

		static CCryNameTSCRC techObj("AuxGeometryObj");

		CRenderPrimitive& prim = PreparePrimitive(flags, techObj, eptTriangleList, eVF_P3F_T3F, sizeof(SVF_P3F_T3F), pMesh->m_pVB, pMesh->m_pIB, nullptr);

		prim.SetDrawInfo(eptTriangleList, 0, 0, pMesh->m_numFaces * 3);


		{
			Matrix44A matWorldViewProj;

			// calculate transformation matrices
			if (m_curDrawInFrontMode == e_DrawInFrontOn)
			{
				Matrix44A matScale(Matrix34::CreateScale(Vec3(0.999f, 0.999f, 0.999f)));

				Matrix44A& matWorldViewScaleProjT = matWorldViewProj;
				matWorldViewScaleProjT = matView * matScale;
				matWorldViewScaleProjT = matWorldViewScaleProjT * GetCurrentProj();

				matWorldViewScaleProjT = matWorldViewScaleProjT.GetTransposed();
				matWorldViewScaleProjT = matWorldViewScaleProjT * matWorld;
			}
			else
			{
				Matrix44A& matWorldViewProjT = matWorldViewProj;
				matWorldViewProjT = m_matrices.m_pCurTransMat->GetTransposed();
				matWorldViewProjT = matWorldViewProjT * matWorld;
			}

			// set color
			ColorF col(drawParams.m_color);

			// set light vector (rotate back into local space)
			Matrix33 matWorldInv(drawParams.m_matWorld.GetInverted());
			Vec3 lightLocalSpace(matWorldInv * Vec3(0.5773f, 0.5773f, 0.5773f));

			// normalize light vector (matWorld could contain non-uniform scaling)
			lightLocalSpace.Normalize();


			static CCryNameR matWorldViewProjName ("matWorldViewProj");
			static CCryNameR auxGeomObjColorName  ("auxGeomObjColor");
			static CCryNameR auxGeomObjShadingName("auxGeomObjShading");
			static CCryNameR globalLightLocalName ("globalLightLocal");


			Vec4 auxGeomObjColor  (col.b, col.g, col.r, col.a);                                           // need to flip r/b as drawParams.m_color was originally argb
			Vec4 auxGeomObjShading(drawParams.m_shaded ? 0.4f : 0, drawParams.m_shaded ? 0.6f : 1, 0, 0); // set shading flag
			Vec4 globalLightLocal (lightLocalSpace.x, lightLocalSpace.y, lightLocalSpace.z, 0.0f);        // normalize light vector (matWorld could contain non-uniform scaling)


			auto& constantManager = prim.GetConstantManager();

			constantManager.BeginNamedConstantUpdate();
			constantManager.SetNamedConstantArray(matWorldViewProjName,  alias_cast<Vec4*>(&matWorldViewProj), 4, eHWSC_Vertex);
			constantManager.SetNamedConstant     (auxGeomObjColorName,   auxGeomObjColor,   eHWSC_Vertex);
			constantManager.SetNamedConstant     (auxGeomObjShadingName, auxGeomObjShading, eHWSC_Vertex);
			constantManager.SetNamedConstant     (globalLightLocalName,  globalLightLocal,  eHWSC_Vertex);
			constantManager.EndNamedConstantUpdate();
		}

		if( !m_geomPass.AddPrimitive(&prim) )
		{
			return;
		}

		m_geomPass.Execute();
	}
}

static inline Vec3 IntersectLinePlane(const Vec3& o, const Vec3& d, const Plane& p, float& t)
{
	t = -((p.n | o) + (p.d + c_clipThres)) / (p.n | d);
	return(o + d * t);
}

// maps floating point channels (0.f to 1.f range) to DWORD
	#define DWORD_COLORVALUE(r, g, b, a)        \
	  DWORD(                                    \
	    (((DWORD)((a) * 255.f) & 0xff) << 24) | \
	    (((DWORD)((r) * 255.f) & 0xff) << 16) | \
	    (((DWORD)((g) * 255.f) & 0xff) << 8) |  \
	    (((DWORD)((b) * 255.f) & 0xff) << 0))

static inline DWORD ClipColor(const DWORD& c0, const DWORD& c1, float t)
{
	// convert D3D DWORD color storage (ARGB) to custom ColorF storage (ColorB uses ABGR!)
	const float f = 1.0f / 255.0f;
	ColorF v0(
	  f * (float)(unsigned char)(c0 >> 16),
	  f * (float)(unsigned char)(c0 >> 8),
	  f * (float)(unsigned char)(c0 >> 0),
	  f * (float)(unsigned char)(c0 >> 24));
	ColorF v1(
	  f * (float)(unsigned char)(c1 >> 16),
	  f * (float)(unsigned char)(c1 >> 8),
	  f * (float)(unsigned char)(c1 >> 0),
	  f * (float)(unsigned char)(c1 >> 24));
	ColorF vRes(v0 + (v1 - v0) * t);
	return(DWORD_COLORVALUE(vRes.r, vRes.g, vRes.b, vRes.a));
}

static bool ClipLine(Vec3* v, DWORD* c)
{
	// get near plane to perform clipping
	Plane nearPlane(*gRenDev->GetCamera().GetFrustumPlane(FR_PLANE_NEAR));

	// get clipping flags
	bool bV0Behind(-((nearPlane.n | v[0]) + nearPlane.d) < c_clipThres);
	bool bV1Behind(-((nearPlane.n | v[1]) + nearPlane.d) < c_clipThres);

	// proceed only if both are not behind near clipping plane
	if (false == bV0Behind || false == bV1Behind)
	{
		if (false == bV0Behind && false == bV1Behind)
		{
			// no clipping needed
			return(true);
		}

		// define line to be clipped
		Vec3 p(v[0]);
		Vec3 d(v[1] - v[0]);

		// get clipped position
		float t;
		v[0] = (false == bV0Behind) ? v[0] : IntersectLinePlane(p, d, nearPlane, t);
		v[1] = (false == bV1Behind) ? v[1] : IntersectLinePlane(p, d, nearPlane, t);

		// get clipped colors
		c[0] = (false == bV0Behind) ? c[0] : ClipColor(c[0], c[1], t);
		c[1] = (false == bV1Behind) ? c[1] : ClipColor(c[0], c[1], t);

		return(true);
	}
	else
	{
		return(false);
	}
}

static float ComputeConstantScale(const Vec3& v, const Matrix44A& matView, const Matrix44A& matProj, const uint32 wndXRes)
{
	Vec4 vCam0;
	mathVec3TransformF(&vCam0, &v, &matView);

	Vec4 vCam1(vCam0);
	vCam1.x += 1.0f;

	const float a = vCam0.y * matProj.m10 + vCam0.z * matProj.m20 + matProj.m30;
	const float b = vCam0.y * matProj.m13 + vCam0.z * matProj.m23 + matProj.m33;

	float c0((vCam0.x * matProj.m00 + a) / (vCam0.x * matProj.m03 + b));
	float c1((vCam1.x * matProj.m00 + a) / (vCam1.x * matProj.m03 + b));

	float s = (float)wndXRes * (c1 - c0);

	const float epsilon = 0.001f;
	return (fabsf(s) >= epsilon) ? 1.0f / s : 1.0f / epsilon;
}

void CRenderAuxGeomD3D::PrepareThickLines3D(CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd)
{
	const CAuxGeomCB::AuxVertexBuffer& auxVertexBuffer(GetAuxVertexBuffer());

	// process each entry
	for (CAuxGeomCB::AuxSortedPushBuffer::const_iterator it(itBegin); it != itEnd; ++it)
	{
		// get current push buffer entry
		const CAuxGeomCB::SAuxPushBufferEntry* curPBEntry(*it);

		uint32 offset(curPBEntry->m_vertexOffs);
		for (uint32 i(0); i < curPBEntry->m_numVertices / 6; ++i, offset += 6)
		{
			// get line vertices and thickness parameter
			const float* aTmp0 = (const float*) &(auxVertexBuffer[offset + 0].xyz.x);
			const float* aTmp1 = (const float*) &(auxVertexBuffer[offset + 1].xyz.x);
			const Vec3 v[2] =
			{
				Vec3(aTmp0[0], aTmp0[1], aTmp0[2]),
				Vec3(aTmp1[0], aTmp1[1], aTmp1[2])
			};
			DWORD col[2] =
			{
				auxVertexBuffer[offset + 0].color.dcolor,
				auxVertexBuffer[offset + 1].color.dcolor
			};
			float thickness(auxVertexBuffer[offset + 2].xyz.x);

			bool skipLine(false);
			Vec4 vf[4];

			if (false == IsOrthoMode())  // regular, 3d projected geometry
			{
				skipLine = !ClipLine((Vec3*)v, col);
				if (false == skipLine)
				{
					// compute depth corrected thickness of line end points
					float thicknessV0(0.5f * thickness * ComputeConstantScale(v[0], GetCurrentView(), GetCurrentProj(), m_wndXRes));
					float thicknessV1(0.5f * thickness * ComputeConstantScale(v[1], GetCurrentView(), GetCurrentProj(), m_wndXRes));

					// compute camera space line delta
					Vec4 vt[2];
					mathVec3TransformF(&vt[0], &v[0], &GetCurrentView());
					mathVec3TransformF(&vt[1], &v[1], &GetCurrentView());
					vt[0].z = (float) __fsel(-vt[0].z - c_clipThres, vt[0].z, -c_clipThres);
					vt[1].z = (float) __fsel(-vt[1].z - c_clipThres, vt[1].z, -c_clipThres);
					Vec4 tmp(vt[1] / vt[1].z - vt[0] / vt[0].z);
					Vec2 delta(tmp.x, tmp.y);

					// create screen space normal of line delta
					Vec2 normalVec(-delta.y, delta.x);
					mathVec2NormalizeF(&normalVec, &normalVec);
					Vec2 normal(normalVec.x, normalVec.y);

					Vec2 n[2];
					n[0] = normal * thicknessV0;
					n[1] = normal * thicknessV1;

					// compute final world space vertices of thick line
					Vec4 vertices[4] =
					{
						Vec4(vt[0].x + n[0].x, vt[0].y + n[0].y, vt[0].z, vt[0].w),
						Vec4(vt[1].x + n[1].x, vt[1].y + n[1].y, vt[1].z, vt[1].w),
						Vec4(vt[1].x - n[1].x, vt[1].y - n[1].y, vt[1].z, vt[1].w),
						Vec4(vt[0].x - n[0].x, vt[0].y - n[0].y, vt[0].z, vt[0].w)
					};
					mathVec4TransformF(&vf[0], &vertices[0], &GetCurrentViewInv());
					mathVec4TransformF(&vf[1], &vertices[1], &GetCurrentViewInv());
					mathVec4TransformF(&vf[2], &vertices[2], &GetCurrentViewInv());
					mathVec4TransformF(&vf[3], &vertices[3], &GetCurrentViewInv());
				}
			}
			else // orthogonal projected geometry
			{
				// compute depth corrected thickness of line end points
				float thicknessV0(0.5f * thickness * ComputeConstantScale(v[0], GetCurrentView(), GetCurrentProj(), m_wndXRes));
				float thicknessV1(0.5f * thickness * ComputeConstantScale(v[1], GetCurrentView(), GetCurrentProj(), m_wndXRes));

				// compute line delta
				Vec2 delta(v[1] - v[0]);

				// create normal of line delta
				Vec2 normalVec(-delta.y, delta.x);
				mathVec2NormalizeF(&normalVec, &normalVec);
				Vec2 normal(normalVec.x, normalVec.y);

				Vec2 n[2];
				n[0] = normal * thicknessV0 * 2.0f;
				n[1] = normal * thicknessV1 * 2.0f;

				// compute final world space vertices of thick line
				vf[0] = Vec4(v[0].x + n[0].x, v[0].y + n[0].y, v[0].z, 1.0f);
				vf[1] = Vec4(v[1].x + n[1].x, v[1].y + n[1].y, v[1].z, 1.0f);
				vf[2] = Vec4(v[1].x - n[1].x, v[1].y - n[1].y, v[1].z, 1.0f);
				vf[3] = Vec4(v[0].x - n[0].x, v[0].y - n[0].y, v[0].z, 1.0f);
			}

			SAuxVertex* pVertices(const_cast<SAuxVertex*>(&auxVertexBuffer[offset]));
			if (false == skipLine)
			{
				// copy data to vertex buffer
				pVertices[0].xyz = Vec3(vf[0].x, vf[0].y, vf[0].z);
				pVertices[0].color.dcolor = col[0];
				pVertices[1].xyz = Vec3(vf[1].x, vf[1].y, vf[1].z);
				pVertices[1].color.dcolor = col[1];
				pVertices[2].xyz = Vec3(vf[2].x, vf[2].y, vf[2].z);
				pVertices[2].color.dcolor = col[1];
				pVertices[3].xyz = Vec3(vf[0].x, vf[0].y, vf[0].z);
				pVertices[3].color.dcolor = col[0];
				pVertices[4].xyz = Vec3(vf[2].x, vf[2].y, vf[2].z);
				pVertices[4].color.dcolor = col[1];
				pVertices[5].xyz = Vec3(vf[3].x, vf[3].y, vf[3].z);
				pVertices[5].color.dcolor = col[0];
			}
			else
			{
				// invalidate parameter data of thick line stored in vertex buffer
				// (generates two black degenerated triangles at (0,0,0))
				memset(pVertices, 0, sizeof(SAuxVertex) * 6);
			}
		}
	}
}

void CRenderAuxGeomD3D::PrepareThickLines2D(CAuxGeomCB::AuxSortedPushBuffer::const_iterator itBegin, CAuxGeomCB::AuxSortedPushBuffer::const_iterator itEnd)
{
	const CAuxGeomCB::AuxVertexBuffer& auxVertexBuffer(GetAuxVertexBuffer());

	// process each entry
	for (CAuxGeomCB::AuxSortedPushBuffer::const_iterator it(itBegin); it != itEnd; ++it)
	{
		// get current push buffer entry
		const CAuxGeomCB::SAuxPushBufferEntry* curPBEntry(*it);

		uint32 offset(curPBEntry->m_vertexOffs);
		for (uint32 i(0); i < curPBEntry->m_numVertices / 6; ++i, offset += 6)
		{
			// get line vertices and thickness parameter
			const float* aTmp0 = (const float*) &(auxVertexBuffer[offset + 0].xyz.x);
			const float* aTmp1 = (const float*) &(auxVertexBuffer[offset + 1].xyz.x);
			const Vec3 v[2] =
			{
				Vec3(aTmp0[0], aTmp0[1], aTmp0[2]),
				Vec3(aTmp1[0], aTmp1[1], aTmp1[2])
			};
			const DWORD col[2] =
			{
				auxVertexBuffer[offset + 0].color.dcolor,
				auxVertexBuffer[offset + 1].color.dcolor
			};
			float thickness(auxVertexBuffer[offset + 2].xyz.x);

			// get line delta and aspect ratio corrected normal
			Vec3 delta(v[1] - v[0]);
			Vec3 normalVec(-delta.y * m_aspectInv, delta.x * m_aspect, 0.0f);

			// normalize and scale to line thickness
			mathVec3NormalizeF(&normalVec, &normalVec);
			Vec3 normal(normalVec.x, normalVec.y, normalVec.z);
			normal *= thickness * 0.001f;

			// compute final 2D vertices of thick line in normalized device space
			Vec3 vf[4];
			vf[0] = v[0] + normal;
			vf[1] = v[1] + normal;
			vf[2] = v[1] - normal;
			vf[3] = v[0] - normal;

			// copy data to vertex buffer
			SAuxVertex* pVertices(const_cast<SAuxVertex*>(&auxVertexBuffer[offset]));
			pVertices[0].xyz = Vec3(vf[0].x, vf[0].y, vf[0].z);
			pVertices[0].color.dcolor = col[0];
			pVertices[1].xyz = Vec3(vf[1].x, vf[1].y, vf[1].z);
			pVertices[1].color.dcolor = col[1];
			pVertices[2].xyz = Vec3(vf[2].x, vf[2].y, vf[2].z);
			pVertices[2].color.dcolor = col[1];
			pVertices[3].xyz = Vec3(vf[0].x, vf[0].y, vf[0].z);
			pVertices[3].color.dcolor = col[0];
			pVertices[4].xyz = Vec3(vf[2].x, vf[2].y, vf[2].z);
			pVertices[4].color.dcolor = col[1];
			pVertices[5].xyz = Vec3(vf[3].x, vf[3].y, vf[3].z);
			pVertices[5].color.dcolor = col[0];
		}
	}
}

void CRenderAuxGeomD3D::PrepareRendering()
{
	// update transformation matrices
	m_matrices.UpdateMatrices(m_renderer);

	// get current window resolution and update aspect ratios
	m_wndXRes = m_renderer.GetWidth();
	m_wndYRes = m_renderer.GetHeight();

	m_aspect = 1.0f;
	m_aspectInv = 1.0f;
	if (m_wndXRes > 0 && m_wndYRes > 0)
	{
		m_aspect = (float) m_wndXRes / (float) m_wndYRes;
		m_aspectInv = 1.0f / m_aspect;
	}

	// reset DrawInFront mode
	m_curDrawInFrontMode = e_DrawInFrontOff;

	// reset current prim type
	m_curPrimType = CAuxGeomCB::e_PrimTypeInvalid;
}


void CRenderAuxGeomD3D::Prepare(const SAuxGeomRenderFlags& renderFlags, Matrix44A& mat)
{
	// mode 2D/3D -- set new transformation matrix
	const Matrix44A* pNewTransMat(&GetCurrentTrans3D());
	if( e_Mode2D == renderFlags.GetMode2D3DFlag() )
	{
		pNewTransMat = &GetCurrentTrans2D();
	}
	if( m_matrices.m_pCurTransMat != pNewTransMat )
	{
		m_matrices.m_pCurTransMat = pNewTransMat;

		m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_matView->LoadIdentity();
		m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_matProj->LoadMatrix(pNewTransMat);
		m_renderer.EF_DirtyMatrix();
	}

	m_curPointSize = (CAuxGeomCB::e_PtList == CAuxGeomCB::GetPrimType(renderFlags)) ? CAuxGeomCB::GetPointSize(renderFlags) : 1;

	EAuxGeomPublicRenderflags_DrawInFrontMode newDrawInFrontMode(renderFlags.GetDrawInFrontMode());
	CAuxGeomCB::EPrimType newPrimType(CAuxGeomCB::GetPrimType(renderFlags));

	m_curDrawInFrontMode = (e_DrawInFrontOn == renderFlags.GetDrawInFrontMode() && e_Mode3D == renderFlags.GetMode2D3DFlag()) ? e_DrawInFrontOn : e_DrawInFrontOff;

	if( CAuxGeomCB::e_Obj != newPrimType )
	{
		if( m_curDrawInFrontMode == e_DrawInFrontOn )
		{
			Matrix44A matScale(Matrix34::CreateScale(Vec3(0.999f, 0.999f, 0.999f)));

			mat = GetCurrentView();

			if( HasWorldMatrix() )
			{
				mat = Matrix44A(GetAuxWorldMatrix(m_curWorldMatrixIdx)).GetTransposed() * mat;
			}

			mat = mat * matScale;
			mat = mat * GetCurrentProj();
			mat = mat.GetTransposed();
		}
		else
		{
			mat = *m_matrices.m_pCurTransMat;

			if( HasWorldMatrix() )
			{
				mat = Matrix44(GetAuxWorldMatrix(m_curWorldMatrixIdx)).GetTransposed() *  mat;
			}

			mat = mat.GetTransposed();
		}
	}

	m_curPrimType = newPrimType;

}


void CRenderAuxGeomD3D::RT_Flush(SAuxGeomCBRawDataPackaged& data, size_t begin, size_t end, bool reset)
{
	if (!CV_r_auxGeom)
		return;

	CStandardGraphicsPipeline::SwitchFromLegacyPipeline();

	PROFILE_LABEL_SCOPE("AuxGeom_Flush");

	// should only be called from render thread
	assert(m_renderer.m_pRT->IsRenderThread());

	assert(data.m_pData);

	Matrix44 mViewProj;

	if (begin < end)
	{
		m_pCurCBRawData = data.m_pData;

		m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_matProj->Push();
		m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_matView->Push();

		if (false == m_renderer.IsDeviceLost())
		{
			// prepare rendering
			PrepareRendering();

			// get push buffer to process all submitted auxiliary geometries
			m_pCurCBRawData->GetSortedPushBuffer(begin, end, m_auxSortedPushBuffer);

			for( CAuxGeomCB::AuxSortedPushBuffer::const_iterator it(m_auxSortedPushBuffer.begin()), itEnd(m_auxSortedPushBuffer.end()); it != itEnd; )
			{
				// mark current push buffer position
				CAuxGeomCB::AuxSortedPushBuffer::const_iterator itCur(it);

				// get current render flags
				const SAuxGeomRenderFlags& curRenderFlags((*itCur)->m_renderFlags);
				m_curTransMatrixIdx = (*itCur)->m_transMatrixIdx;
				m_curWorldMatrixIdx = (*itCur)->m_worldMatrixIdx;

				// get prim type
				CAuxGeomCB::EPrimType primType(CAuxGeomCB::GetPrimType(curRenderFlags));

				// find all entries sharing the same render flags
				while( true )
				{
					++it;
					if( (it == itEnd) || ((*it)->m_renderFlags != curRenderFlags) || ((*it)->m_transMatrixIdx != m_curTransMatrixIdx) ||
						((*it)->m_worldMatrixIdx != m_curWorldMatrixIdx) )
					{
						break;
					}
				}

				// prepare thick lines
				if( CAuxGeomCB::e_TriList == primType && CAuxGeomCB::IsThickLine(curRenderFlags) )
				{
					if( e_Mode3D == curRenderFlags.GetMode2D3DFlag() )
					{
						PrepareThickLines3D(itCur, it);
					}
					else
					{
						PrepareThickLines2D(itCur, it);
					}
				}
			}

			const CAuxGeomCB::AuxVertexBuffer& auxVertexBuffer(GetAuxVertexBuffer());
			const CAuxGeomCB::AuxIndexBuffer&  auxIndexBuffer (GetAuxIndexBuffer());

			m_bufman.FillVB(auxVertexBuffer.data(), auxVertexBuffer.size() * sizeof(SAuxVertex));
			m_bufman.FillIB(auxIndexBuffer.data(),  auxIndexBuffer.size()  * sizeof(vtx_idx));


			// process push buffer
			for (CAuxGeomCB::AuxSortedPushBuffer::const_iterator it(m_auxSortedPushBuffer.begin()), itEnd(m_auxSortedPushBuffer.end()); it != itEnd; )
			{
				// mark current push buffer position
				CAuxGeomCB::AuxSortedPushBuffer::const_iterator itCur(it);

				// get current render flags
				const SAuxGeomRenderFlags& curRenderFlags((*itCur)->m_renderFlags);
				m_curTransMatrixIdx = (*itCur)->m_transMatrixIdx;
				m_curWorldMatrixIdx = (*itCur)->m_worldMatrixIdx;

				// get prim type
				CAuxGeomCB::EPrimType primType(CAuxGeomCB::GetPrimType(curRenderFlags));

				// find all entries sharing the same render flags
				while (true)
				{
					++it;
					if ((it == itEnd) || ((*it)->m_renderFlags != curRenderFlags) || ((*it)->m_transMatrixIdx != m_curTransMatrixIdx) ||
						((*it)->m_worldMatrixIdx != m_curWorldMatrixIdx))
					{
						break;
					}
				}

				// set appropriate rendering data
				Prepare(curRenderFlags, mViewProj);

				if( !m_pAuxGeomShader )
				{
					// allow invalid file access for this shader because it shouldn't be used in the final build anyway
					CDebugAllowFileAccess ignoreInvalidFileAccess;
					m_pAuxGeomShader = m_renderer.m_cEF.mfForName("AuxGeom", EF_SYSTEM);
					assert(0 != m_pAuxGeomShader);
				}

				CD3DStereoRenderer& stereoRenderer = gcpRendD3D->GetS3DRend();
				const bool bStereoEnabled = stereoRenderer.IsStereoEnabled();
				const bool bStereoSequentialMode = stereoRenderer.RequiresSequentialSubmission();

				// draw push buffer entries
				switch (primType)
				{
				case CAuxGeomCB::e_PtList:
				case CAuxGeomCB::e_LineList:
				case CAuxGeomCB::e_TriList:
					{
						if (bStereoEnabled)
						{
							stereoRenderer.BeginRenderingTo(LEFT_EYE);
							DrawAuxPrimitives(itCur, it, mViewProj);
							stereoRenderer.EndRenderingTo(LEFT_EYE);

							if (bStereoSequentialMode)
							{
								stereoRenderer.BeginRenderingTo(RIGHT_EYE);
								DrawAuxPrimitives(itCur, it, mViewProj);
								stereoRenderer.EndRenderingTo(RIGHT_EYE);
							}
						}
						else
						{
							DrawAuxPrimitives(itCur, it, mViewProj);
						}
					}
					break;
				case CAuxGeomCB::e_LineListInd:
				case CAuxGeomCB::e_TriListInd:
					{
						if (bStereoEnabled)
						{
							stereoRenderer.BeginRenderingTo(LEFT_EYE);
							DrawAuxIndexedPrimitives(itCur, it, mViewProj);
							stereoRenderer.EndRenderingTo(LEFT_EYE);

							if (bStereoSequentialMode)
							{
								stereoRenderer.BeginRenderingTo(RIGHT_EYE);
								DrawAuxIndexedPrimitives(itCur, it, mViewProj);
								stereoRenderer.EndRenderingTo(RIGHT_EYE);
							}
						}
						else
						{
							DrawAuxIndexedPrimitives(itCur, it, mViewProj);
						}
					}
					break;
				case CAuxGeomCB::e_Obj:
				default:
					{
						if (bStereoEnabled)
						{
							stereoRenderer.BeginRenderingTo(LEFT_EYE);
							DrawAuxObjects(itCur, it, mViewProj);
							stereoRenderer.EndRenderingTo(LEFT_EYE);

							if (bStereoSequentialMode)
							{
								stereoRenderer.BeginRenderingTo(RIGHT_EYE);
								DrawAuxObjects(itCur, it, mViewProj);
								stereoRenderer.EndRenderingTo(RIGHT_EYE);
							}
						}
						else
						{
							DrawAuxObjects(itCur, it, mViewProj);
						}
					}
					break;
				}
			}
		}

		m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_matProj->Pop();
		m_renderer.m_RP.m_TI[m_renderer.m_RP.m_nProcessThreadID].m_matView->Pop();
		m_renderer.EF_DirtyMatrix();

		m_pCurCBRawData = 0;
		m_curTransMatrixIdx = -1;
		m_curWorldMatrixIdx = -1;
	}

	CStandardGraphicsPipeline::SwitchToLegacyPipeline();

	FlushTextMessages(data.m_pData->m_TextMessages, !reset);
}

void CRenderAuxGeomD3D::Flush(SAuxGeomCBRawDataPackaged& data, size_t begin, size_t end, bool reset)
{
	m_renderer.m_pRT->RC_AuxFlush(this, data, begin, end, reset);
}


void CRenderAuxGeomD3D::DrawStringImmediate(IFFont_RenderProxy* pFont, float x, float y, float z, const char* pStr, const bool asciiMultiLine, const STextDrawContext& ctx)
{
	CD3DStereoRenderer* const __restrict rendS3D = &gcpRendD3D->GetS3DRend();

	if( rendS3D->IsStereoEnabled() && !rendS3D->DisplayStereoDone() )
	{
		rendS3D->BeginRenderingTo(LEFT_EYE);
		pFont->RenderCallback(x, y, z, pStr, asciiMultiLine, ctx);
		rendS3D->EndRenderingTo(LEFT_EYE);

		if( rendS3D->RequiresSequentialSubmission() )
		{
			rendS3D->BeginRenderingTo(RIGHT_EYE);
			pFont->RenderCallback(x, y, z, pStr, asciiMultiLine, ctx);
			rendS3D->EndRenderingTo(RIGHT_EYE);
		}
	}
	else
	{
		pFont->RenderCallback(x, y, z, pStr, asciiMultiLine, ctx);
	}
}

void CRenderAuxGeomD3D::FlushTextMessages(CTextMessages& messages, bool reset)
{
	CD3D9Renderer* const renderer = gcpRendD3D;
	renderer->EnableFog(false);
	int vx, vy, vw, vh;
	renderer->GetViewport(&vx, &vy, &vw, &vh);

	while( const CTextMessages::CTextMessageHeader* pEntry = messages.GetNextEntry() )
	{
		const CTextMessages::SText* pText = pEntry->CastTo_Text();

		Vec3 vPos(0, 0, 0);
		int  nDrawFlags = 0;
		const char* szText = 0;
		Vec4  vColor(1, 1, 1, 1);
		Vec2 fSize;
		bool bDraw = true;

		if( !pText )
		{
			return;
		}

		nDrawFlags = pText->m_nDrawFlags;
		szText = pText->GetText();
		vPos = pText->m_vPos;
		vColor = pText->m_Color.toVec4() * 1.0f / 255.0f;
		fSize = pText->m_fFontSize;

		if( (nDrawFlags & eDrawText_LegacyBehavior) == 0 )
		{

			bool b800x600 = (nDrawFlags & eDrawText_800x600) != 0;

			float fMaxPosX = 100.0f;
			float fMaxPosY = 100.0f;

			if( !b800x600 )
			{
				fMaxPosX = (float)vw;
				fMaxPosY = (float)vh;
			}

			if( !(nDrawFlags & eDrawText_2D) )
			{
				float fDist = 1;   //GetDistance(pTextInfo->pos,GetCamera().GetPosition());

				const CCamera& cam = renderer->GetCamera();
				float     ferPlane = cam.GetFarPlane();
				float K = ferPlane / fDist;
				if( fDist > ferPlane * 0.5 )
					vPos = cam.GetPosition()*(1 - K) + K * vPos;

				float sx, sy, sz;
				renderer->ProjectToScreen(vPos.x, vPos.y, vPos.z, &sx, &sy, &sz);

				vPos.x = sx;
				vPos.y = sy;
				vPos.z = sz;
			}
			else
			{
				if( b800x600 )
				{
					// Make 2D coords in range 0-100
					vPos.x *= 100.0f / vw;
					vPos.y *= 100.0f / vh;
				}
			}

			bDraw = vPos.x >= 0 && vPos.x <= fMaxPosX;
			bDraw &= vPos.y >= 0 && vPos.y <= fMaxPosY;
			bDraw &= vPos.z >= 0 && vPos.z <= 1;
			//
			// 			if( nDrawFlags & eDrawText_DepthTest )
			// 			{
			// 				sz = 1.0f - 2.0f * sz;
			// 			}
			// 			else
			// 			{
			// 				sz = 1.0f;
			// 			}

			vPos.x *= (b800x600 ? 8.f : 1.f);
			vPos.y *= (b800x600 ? 6.f : 1.f);
		}

		if( szText && bDraw )
		{
			IFFont* pFont = nullptr;

			if( gEnv->pSystem->GetICryFont() )
				pFont = gEnv->pSystem->GetICryFont()->GetFont("default");

			if( !pFont )
			{
				return;
			}

			const float r = CLAMP(vColor[0], 0.0f, 1.0f);
			const float g = CLAMP(vColor[1], 0.0f, 1.0f);
			const float b = CLAMP(vColor[2], 0.0f, 1.0f);
			const float a = CLAMP(vColor[3], 0.0f, 1.0f);

			STextDrawContext ctx;
			ctx.SetColor(ColorF(r, g, b, a));
			ctx.SetCharWidthScale(1.0f);
			ctx.EnableFrame((nDrawFlags & eDrawText_Framed) != 0);

			if( nDrawFlags & eDrawText_Monospace )
			{
				if( nDrawFlags & eDrawText_FixedSize )
					ctx.SetSizeIn800x600(false);
				ctx.SetSize(Vec2(UIDRAW_TEXTSIZEFACTOR * fSize.x, UIDRAW_TEXTSIZEFACTOR * fSize.y));
				ctx.SetCharWidthScale(0.5f);
				ctx.SetProportional(false);

				if( nDrawFlags & eDrawText_800x600 )
					renderer->ScaleCoordInternal(vPos.x, vPos.y);
			}
			else if( nDrawFlags & eDrawText_FixedSize )
			{
				ctx.SetSizeIn800x600(false);
				ctx.SetSize(Vec2(UIDRAW_TEXTSIZEFACTOR * fSize.x, UIDRAW_TEXTSIZEFACTOR * fSize.y));
				ctx.SetProportional(true);

				if( nDrawFlags & eDrawText_800x600 )
					renderer->ScaleCoordInternal(vPos.x, vPos.y);
			}
			else
			{
				ctx.SetSizeIn800x600(true);
				ctx.SetProportional(false);
				ctx.SetCharWidthScale(0.5f);
				ctx.SetSize(Vec2(UIDRAW_TEXTSIZEFACTOR * fSize.x, UIDRAW_TEXTSIZEFACTOR * fSize.y));
			}

			// align left/right/center
			if( nDrawFlags & (eDrawText_Center | eDrawText_CenterV | eDrawText_Right) )
			{
				Vec2 textSize = pFont->GetTextSize(szText, true, ctx);

				// If we're using virtual 800x600 coordinates, convert the text size from
				// pixels to that before using it as an offset.
				if( ctx.m_sizeIn800x600 )
				{
					textSize.x /= renderer->ScaleCoordXInternal(1.0f);
					textSize.y /= renderer->ScaleCoordYInternal(1.0f);
				}

				if( nDrawFlags & eDrawText_Center ) vPos.x -= textSize.x * 0.5f;
				else if( nDrawFlags & eDrawText_Right ) vPos.x -= textSize.x;

				if( nDrawFlags & eDrawText_CenterV )
					vPos.y -= textSize.y * 0.5f;
			}

			// Pass flags so that overscan borders can be applied if necessary
			ctx.SetFlags(nDrawFlags);

			pFont->DrawString(vPos.x, vPos.y, vPos.z, szText, true, ctx);
		}
	}

	messages.Clear(!reset);
}

void CRenderAuxGeomD3D::SetOrthoMode(bool enable, Matrix44A* pMatrix)
{
	GetRenderAuxGeom()->SetOrthoMode(enable, pMatrix);
}

const Matrix44A& CRenderAuxGeomD3D::GetCurrentView() const
{
	return IsOrthoMode() ? gRenDev->m_IdentityMatrix : m_matrices.m_matView;
}

const Matrix44A& CRenderAuxGeomD3D::GetCurrentViewInv() const
{
	return IsOrthoMode() ? gRenDev->m_IdentityMatrix : m_matrices.m_matViewInv;
}

const Matrix44A& CRenderAuxGeomD3D::GetCurrentProj() const
{
	return IsOrthoMode() ? GetAuxOrthoMatrix(m_curTransMatrixIdx) : m_matrices.m_matProj;
}

const Matrix44A& CRenderAuxGeomD3D::GetCurrentTrans3D() const
{
	return IsOrthoMode() ? GetAuxOrthoMatrix(m_curTransMatrixIdx) : m_matrices.m_matTrans3D;
}

const Matrix44A& CRenderAuxGeomD3D::GetCurrentTrans2D() const
{
	return m_matrices.m_matTrans2D;
}

bool CRenderAuxGeomD3D::IsOrthoMode() const
{
	return m_curTransMatrixIdx != -1;
}

bool CRenderAuxGeomD3D::HasWorldMatrix() const
{
	return m_curWorldMatrixIdx != -1;
}

void CRenderAuxGeomD3D::SMatrices::UpdateMatrices(CD3D9Renderer& renderer)
{
	renderer.GetModelViewMatrix(&m_matView.m00);
	renderer.GetProjectionMatrix(&m_matProj.m00);

	m_matViewInv = m_matView.GetInverted();
	m_matTrans3D = m_matView * m_matProj;

	m_pCurTransMat = 0;
}

void CRenderAuxGeomD3D::FreeMemory()
{
	m_auxGeomCBCol.FreeMemory();

	stl::free_container(m_auxSortedPushBuffer);
}

void CRenderAuxGeomD3D::Process()
{
	m_auxGeomCBCol.Process();
}

CAuxGeomCB* CRenderAuxGeomD3D::GetRenderAuxGeom(void* jobID)
{
	return m_auxGeomCBCol.Get(this, jobID);
}

inline const CAuxGeomCB::AuxVertexBuffer& CRenderAuxGeomD3D::GetAuxVertexBuffer() const
{
	assert(m_pCurCBRawData);
	return m_pCurCBRawData->m_auxVertexBuffer;
}

inline const CAuxGeomCB::AuxIndexBuffer& CRenderAuxGeomD3D::GetAuxIndexBuffer() const
{
	assert(m_pCurCBRawData);
	return m_pCurCBRawData->m_auxIndexBuffer;
}

inline const CAuxGeomCB::AuxDrawObjParamBuffer& CRenderAuxGeomD3D::GetAuxDrawObjParamBuffer() const
{
	assert(m_pCurCBRawData);
	return m_pCurCBRawData->m_auxDrawObjParamBuffer;
}

inline const Matrix44A& CRenderAuxGeomD3D::GetAuxOrthoMatrix(int idx) const
{
	assert(m_pCurCBRawData && idx >= 0 && idx < (int)m_pCurCBRawData->m_auxOrthoMatrices.size());
	return m_pCurCBRawData->m_auxOrthoMatrices[idx];
}

inline const Matrix34A& CRenderAuxGeomD3D::GetAuxWorldMatrix(int idx) const
{
	assert(m_pCurCBRawData && idx >= 0 && idx < (int)m_pCurCBRawData->m_auxWorldMatrices.size());
	return m_pCurCBRawData->m_auxWorldMatrices[idx];
}

#endif // #if defined(ENABLE_RENDER_AUX_GEOM)
