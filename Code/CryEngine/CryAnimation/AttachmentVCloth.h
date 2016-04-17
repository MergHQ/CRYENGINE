// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "AttachmentBase.h"
#include "Vertex/VertexData.h"
#include "Vertex/VertexAnimation.h"
#include "ModelSkin.h"
#include "SkeletonPose.h"

class CCharInstance;
class CClothProxies;
class CAttachmentManager;
struct SVertexAnimationJob;
class CClothPhysics;
struct SBuffers;

typedef Vec3     Vector4;
typedef Matrix33 Matrix3;
typedef Quat     Quaternion;

inline Vec3 SseToVec3(const Vector4& v)
{
	return v;
}

inline Quat SseToQuat(const Quaternion& q)
{
	return q;
}

enum ECollisionPrimitiveType
{
	eCPT_Invalid,
	eCPT_Capsule,
	eCPT_Sphere,
};

struct SPrimitive
{
	ECollisionPrimitiveType type;
	SPrimitive() : type(eCPT_Invalid) {}
	virtual ~SPrimitive() {}
};

struct SSphere : SPrimitive
{
	Vector4 center;
	float   r;
	SSphere()
		: center(ZERO)
		, r(0.0f)
	{
		type = eCPT_Sphere;
	}
};

struct SCapsule : SPrimitive
{
	Vector4 axis, center;
	float   hh, r;
	SCapsule()
		: axis(ZERO)
		, center(ZERO)
		, hh(0.0f)
		, r(0.0f)
	{
		type = eCPT_Capsule;
	}
};

enum EPhysicsLayer
{
	PHYS_LAYER_INVALID,
	PHYS_LAYER_ARTICULATED,
	PHYS_LAYER_CLOTH,
};

struct SPhysicsGeom
{
	SPrimitive*   primitive;
	EPhysicsLayer layer;

	SPhysicsGeom()
		: primitive(nullptr)
		, layer(PHYS_LAYER_INVALID)
	{}
};

struct SBonePhysics
{
	quaternionf q;
	Vec3        t;
	f32         cr, ca;

	SBonePhysics()
	{
		cr = 0;
		ca = 0;
		t = Vec3(ZERO);
		q.SetIdentity();
	}
};

struct STangents
{
	Vector4 t, n;
	STangents()
	{
		t.zero();
		n.zero();
	}
};

struct SBuffers
{
	// temporary buffer for simulated cloth vertices
	DynArray<Vector4>    m_tmpClothVtx;
	std::vector<Vector4> m_normals;

	// output buffers for sw-skinning
	DynArray<Vec3>         m_arrDstPositions;
	DynArray<SPipTangents> m_arrDstTangents; // TODO: static

	std::vector<STangents> m_tangents;
};

struct SSkinMapEntry
{
	int   iMap; // vertex index in the sim mesh
	int   iTri; // triangle index in the sim mesh
	float s, t; // barycentric coordinates in the adjacent triangle
	float h;    // distance from triangle
};

struct STangentData
{
	float t1, t2, r;
};

struct SClothGeometry
{
	enum
	{
		MAX_LODS = 2,
	};

	// the number of vertices in the sim mesh
	int            nVtx;
	// registered physical geometry constructed from sim mesh
	phys_geometry* pPhysGeom;
	// mapping between original mesh and welded one
	vtx_idx*       weldMap;
	// number of non-welded vertices in the sim mesh
	int            nUniqueVtx;
	// blending weights between skinning and sim (coming from vertex colors)
	float*         weights;
	// mapping between render mesh and sim mesh vertices
	SSkinMapEntry* skinMap[MAX_LODS];
	// render mesh indices
	uint           numIndices[MAX_LODS];
	vtx_idx*       pIndices[MAX_LODS];
	// tangent frame uv data
	STangentData*  tangentData[MAX_LODS];
	// maximum number for vertices (for LOD0)
	int            maxVertices;

	// memory pool. keeping it as a list for now to avoid reallocation
	// because the jobs are using pointers inside it
	std::list<SBuffers> pool;
	std::vector<int>    freePoolSlots;
	volatile int        poolLock;

	SClothGeometry()
		: nVtx(0)
		, pPhysGeom(NULL)
		, weldMap(NULL)
		, nUniqueVtx(0)
		, weights(NULL)
		, poolLock(0)
		, maxVertices(0)
	{
		for (int i = 0; i < MAX_LODS; i++)
		{
			skinMap[i] = NULL;
			pIndices[i] = NULL;
			tangentData[i] = NULL;
		}
	}

	void      Cleanup();
	int       GetBuffers();
	void      ReleaseBuffers(int idx);
	SBuffers* GetBufferPtr(int idx);
	void      AllocateBuffer();
};

struct SClothInfo
{
	uint64 key;
	float  distance;
	bool   visible;
	int    frame;

	SClothInfo() {}
	SClothInfo(uint64 k, float d, bool v, int f) : key(k), distance(d), visible(v), frame(f) {}
};

struct SPrimitive;

struct SContact
{
	Vector4 n, pt;
	int     particleIdx;
	float   depth;

	SContact()
	{
		n.zero();
		pt.zero();
		particleIdx = -1;
		depth = 0.f;
	}
};

struct SParticleCold
{
	Vector4   prevPos;
	Vector4   oldPos;
	Vector4   skinnedPos;
	SContact* pContact;
	int       permIdx;
	int       bAttached;

	SParticleCold()
	{
		prevPos.zero();
		pContact = NULL;
		bAttached = 0;
		skinnedPos.zero();
		permIdx = -1;
		oldPos.zero();
	}
};

struct SParticleHot
{
	Vector4 pos;
	float   alpha; // for blending with animation
	int     timer;

	SParticleHot()
	{
		pos.zero();
		timer = 0;
		alpha = 0;
	}
};

struct SLink
{
	int   i1, i2;
	float lenSqr, weight1, weight2;
	bool  skip;

	SLink() : i1(0), i2(0), lenSqr(0.0f), weight1(0.f), weight2(0.f), skip(false) {}
};

struct SCollidable
{
	Matrix3 R, oldR;
	Vector4 offset, oldOffset;
	f32     cr, ca;

	SCollidable()
		: R(IDENTITY)
		, oldR(IDENTITY)
		, offset(ZERO)
		, oldOffset(ZERO)
		, cr(0.0f)
		, ca(0.0f)
	{
	}
};

class CClothSimulator
{
public:

	CClothSimulator()
		: m_pAttachmentManager(nullptr)
		, m_nVtx(0)
		, m_nEdges(0)
		, m_particlesCold(nullptr)
		, m_particlesHot(nullptr)
		, m_links(nullptr)
		, m_gravity(0, 0, -9.8f)
		, m_time(0.0f)
		, m_steps(0)
		, m_maxSteps(3)
		, m_idx0(-1)
		, m_lastqHost(IDENTITY)
		, m_contacts(nullptr)
		, m_nContacts(0)
		, m_bBlendSimMesh(false)
	{
		m_color = ColorB(cry_random(0, 127), cry_random(0, 127), cry_random(0, 127));
	}

	~CClothSimulator()
	{
		SAFE_DELETE_ARRAY(m_particlesHot);
		SAFE_DELETE_ARRAY(m_particlesCold);
		SAFE_DELETE_ARRAY(m_links);
		SAFE_DELETE_ARRAY(m_contacts);
	}

	bool  AddGeometry(phys_geometry* pgeom);
	int   SetParams(const SVClothParams& params, float* weights);
	void  SetSkinnedPositions(const Vector4* points);
	void  GetVertices(Vector4* pWorldCoords);

	int   Awake(int bAwake = 1, int iSource = 0);

	void  StartStep(float time_interval, const QuatT& loacation);
	float GetMaxTimeStep(float time_interval);
	int   Step(float animBlend);

	void  DrawHelperInformation();
	void  SwitchToBlendSimMesh();
	bool  IsBlendSimMeshOn();
	const CAttachmentManager* m_pAttachmentManager;

	SVClothParams             m_config;
private:
	void AddContact(int i, const Vector4& n, const Vector4& pt, int permIdx);
	void PrepareEdge(SLink& link);
	void SolveEdge(const SLink& link, float stretch);
	void DetectCollisions();
	void RigidBodyDamping();

private:

	int                      m_nVtx;          // the number of particles
	int                      m_nEdges;        // the number of links between particles
	SParticleCold*           m_particlesCold; // the simulation particles
	SParticleHot*            m_particlesHot;  // the simulation particles
	SLink*                   m_links;         // the structural links between particles
	Vector4                  m_gravity;       // the gravity vector
	float                    m_time;          // time accumulator over frames (used to determine the number of sub-steps)
	int                      m_steps;
	int                      m_maxSteps;
	int                      m_idx0;            // an attached point chosen as the origin of axes
	Quat                     m_lastqHost;       // the last orientation of the part cloth is attached to
	std::vector<SCollidable> m_permCollidables; // list of contact parts (no collision with the world)
	std::vector<SLink>       m_shearLinks;      // shear and bend links
	std::vector<SLink>       m_bendLinks;
	ColorB                   m_color;

	// contact information list
	SContact* m_contacts;
	int       m_nContacts;

	bool      m_bBlendSimMesh;
};

ILINE void CClothSimulator::AddContact(int i, const Vector4& n, const Vector4& pt, int permIdx)
{
	float depth = (pt - m_particlesHot[i].pos) * n;
	SContact* pContact = NULL;
	if (m_particlesCold[i].pContact)
	{
		if (depth <= m_particlesCold[i].pContact->depth)
			return;
		pContact = m_particlesCold[i].pContact;
	}
	else
	{
		pContact = &m_contacts[m_nContacts++];
		m_particlesCold[i].pContact = pContact;
	}
	m_particlesCold[i].permIdx = permIdx;
	pContact->depth = depth;
	pContact->n = n;
	pContact->pt = pt;
	pContact->particleIdx = i;
}

ILINE void CClothSimulator::PrepareEdge(SLink& link)
{
	int n1 = !(m_particlesCold[link.i1].bAttached);
	int n2 = !(m_particlesCold[link.i2].bAttached);
	link.skip = n1 + n2 == 0;
	if (link.skip)
		return;
	float m1Inv = n1 ? 1.f : 0.f;
	float m2Inv = n2 ? 1.f : 0.f;
	float mu = 1.0f / (m1Inv + m2Inv);
	float stretch = 1.f;
	if (m_config.stiffnessGradient != 0.f)
	{
		stretch = 1.f - (m_particlesHot[link.i1].alpha + m_particlesHot[link.i2].alpha) * 0.5f;
		if (stretch != 1.f)
			stretch = min(1.f, m_config.stiffnessGradient * stretch);
	}
	link.weight1 = m1Inv * mu * stretch;
	link.weight2 = m2Inv * mu * stretch;
}

ILINE void CClothSimulator::SolveEdge(const SLink& link, float stretch)
{
	if (link.skip)
		return;
	Vector4& v1 = m_particlesHot[link.i1].pos;
	Vector4& v2 = m_particlesHot[link.i2].pos;
	Vector4 delta = v1 - v2;
	const float lenSqr = delta.len2();
	delta *= stretch * (lenSqr - link.lenSqr) / (lenSqr + link.lenSqr);
	v1 -= link.weight1 * delta;
	v2 += link.weight2 * delta;
}

class CClothPiece
{
private:
	friend struct VertexCommandClothSkin;

	enum
	{
		HIDE_INTERVAL = 50
	};

public:
	CClothPiece()
	{
		m_pCharInstance = NULL;
		m_animBlend = 0.f;
		m_pVClothAttachment = NULL;
		m_bHidden = false;
		m_numLods = 0;
		m_clothGeom = NULL;
		//	m_bSingleThreaded = false;
		m_lastVisible = false;
		m_hideCounter = 0;
		m_bAlwaysVisible = false;
		m_currentLod = 0;
		m_buffers = NULL;
		m_poolIdx = -1;
		m_initialized = false;
	}

	// initializes the object given a skin and a stat obj
	bool                 Initialize(const CAttachmentVCLOTH* pVClothAttachment);

	void                 Dettach();

	int                  GetNumLods()      { return m_numLods; }
	//	bool IsSingleThreaded() { return m_bSingleThreaded; }
	bool                 IsAlwaysVisible() { return m_bAlwaysVisible; }

	bool                 PrepareCloth(CSkeletonPose& skeletonPose, const Matrix34& worldMat, bool visible, int lod);
	bool                 CompileCommand(SVertexSkinData& skinData, CVertexCommandBuffer& commandBuffer);

	void                 SetBlendWeight(float weight);

	bool                 NeedsDebugDraw() const { return Console::GetInst().ca_DrawCloth & 2 ? true : false; }
	void                 DrawDebug(const SVertexAnimationJob* pVertexAnimation);

	void                 SetClothParams(const SVClothParams& params);
	const SVClothParams& GetClothParams();

private:

	bool    PrepareRenderMesh(int lod);
	Vector4 SkinByTriangle(int i, strided_pointer<Vec3>& pVtx, int lod);

	void    UpdateSimulation(const DualQuat* pTransformations, const uint transformationCount);
	template<bool PREVIOUS_POSITIONS>
	void    SkinSimulationToRenderMesh(int lod, CVertexData& vertexData, const strided_pointer<const Vec3>& pVertexPositionsPrevious);

	void    WaitForJob(bool bPrev);

private:

	CAttachmentVCLOTH* m_pVClothAttachment;
	CClothSimulator    m_simulator;
	SVClothParams      m_clothParams;

	// structure containing all sim mesh related data and working buffers
	SClothGeometry* m_clothGeom;
	SBuffers*       m_buffers;
	int             m_poolIdx;

	// used for blending with animation
	float m_animBlend;

	// used for hiding the cloth (mainly in the editor)
	bool m_bHidden;

	// the number of loaded LODs
	int m_numLods;

	// enables single threaded simulation updated
	//	bool m_bSingleThreaded;

	// the current rotation of the character
	QuatT m_charLocation;

	// for easing in and out
	bool m_lastVisible;
	int  m_hideCounter;

	// flags that the cloth is always simulated
	bool m_bAlwaysVisible;

	int  m_currentLod;

	// used to get the correct time from the character
	CCharInstance* m_pCharInstance;

	bool           m_initialized;
};

ILINE Vector4 CClothPiece::SkinByTriangle(int i, strided_pointer<Vec3>& pVtx, int lod)
{
	DynArray<Vector4>& tmpClothVtx = m_buffers->m_tmpClothVtx;
	std::vector<Vector4>& normals = m_buffers->m_normals;
	const SSkinMapEntry& skinMap = m_clothGeom->skinMap[lod][i];
	int tri = skinMap.iTri;
	if (tri >= 0)
	{
		mesh_data* md = (mesh_data*)m_clothGeom->pPhysGeom->pGeom->GetData();
		const int i2 = skinMap.iMap;
		const int base = tri * 3;
		const int idx = md->pIndices[base + i2];
		const int idx0 = md->pIndices[base + inc_mod3[i2]];
		const int idx1 = md->pIndices[base + dec_mod3[i2]];
		const Vector4 u = tmpClothVtx[idx0] - tmpClothVtx[idx];
		const Vector4 v = tmpClothVtx[idx1] - tmpClothVtx[idx];
		const Vector4 n = (1.f - skinMap.s - skinMap.t) * normals[idx] + skinMap.s * normals[idx0] + skinMap.t * normals[idx1];
		return tmpClothVtx[idx] + skinMap.s * u + skinMap.t * v + skinMap.h * n;
	}
	else
		return tmpClothVtx[skinMap.iMap];
}

ILINE void CClothPiece::SetBlendWeight(float weight)
{
	weight = max(0.f, min(1.f, weight));
	m_animBlend = Console::GetInst().ca_ClothBlending == 0 ? 0.f : weight;
}

class CAttachmentVCLOTH : public IAttachmentSkin, public SAttachmentBase
{
public:

	CAttachmentVCLOTH()
	{
		m_clothCacheKey = -1;
		for (uint32 j = 0; j < 2; ++j) m_pRenderMeshsSW[j] = NULL;
		memset(m_arrSkinningRendererData, 0, sizeof(m_arrSkinningRendererData));
		m_pRenderSkin = 0;
	};

	virtual ~CAttachmentVCLOTH();

	virtual void AddRef() override
	{
		++m_nRefCounter;
	}

	virtual void Release() override
	{
		if (--m_nRefCounter == 0)
			delete this;
	}

	virtual uint32             GetType() const override                               { return CA_VCLOTH; }
	virtual uint32             SetJointName(const char* szJointName) override         { return 0; }

	virtual const char*        GetName() const override                               { return m_strSocketName; };
	virtual uint32             GetNameCRC() const override                            { return m_nSocketCRC32; }
	virtual uint32             ReName(const char* strSocketName, uint32 crc) override { m_strSocketName.clear();  m_strSocketName = strSocketName; m_nSocketCRC32 = crc;  return 1; };

	virtual uint32             GetFlags() const override                              { return m_AttFlags; }
	virtual void               SetFlags(uint32 flags) override                        { m_AttFlags = flags; }

	void                       ReleaseRenderRemapTablePair();
	void                       ReleaseSimRemapTablePair();
	void                       ReleaseSoftwareRenderMeshes();

	virtual uint32             Immediate_AddBinding(IAttachmentObject* pModel, ISkin* pISkinRender = 0, uint32 nLoadingFlags = 0) override;
	virtual void               Immediate_ClearBinding(uint32 nLoadingFlags = 0) override;
	virtual uint32             Immediate_SwapBinding(IAttachment* pNewAttachment) override;

	uint32                     AddSimBinding(const ISkin& pISkinRender, uint32 nLoadingFlags = 0);
	virtual IAttachmentObject* GetIAttachmentObject() const override { return m_pIAttachmentObject; }
	virtual IAttachmentSkin*   GetIAttachmentSkin() override         { return this; }

	virtual void               HideAttachment(uint32 x) override;
	virtual uint32             IsAttachmentHidden() const override             { return m_AttFlags & FLAGS_ATTACH_HIDE_MAIN_PASS; }
	virtual void               HideInRecursion(uint32 x) override;
	virtual uint32             IsAttachmentHiddenInRecursion() const override  { return m_AttFlags & FLAGS_ATTACH_HIDE_RECURSION; }
	virtual void               HideInShadow(uint32 x) override;
	virtual uint32             IsAttachmentHiddenInShadow() const override     { return m_AttFlags & FLAGS_ATTACH_HIDE_SHADOW_PASS; }

	virtual void               SetAttAbsoluteDefault(const QuatT& qt) override {};
	virtual void               SetAttRelativeDefault(const QuatT& qt) override {};
	virtual const QuatT& GetAttAbsoluteDefault() const override          { return g_IdentityQuatT; };
	virtual const QuatT& GetAttRelativeDefault() const override          { return g_IdentityQuatT; };

	virtual const QuatT& GetAttModelRelative() const override            { return g_IdentityQuatT;  };//this is relative to the animated bone
	virtual const QuatTS GetAttWorldAbsolute() const override;
	virtual const QuatT& GetAdditionalTransformation() const override    { return g_IdentityQuatT; }
	virtual void         UpdateAttModelRelative() override;

	virtual uint32       GetJointID() const override     { return -1; };
	virtual void         AlignJointAttachment() override {};

	virtual void         Serialize(TSerialize ser) override;
	virtual size_t       SizeOfThis() const override;
	virtual void         GetMemoryUsage(ICrySizer* pSizer) const override;
	virtual void         TriggerMeshStreaming(uint32 nDesiredRenderLOD, const SRenderingPassInfo& passInfo);

	void                 DrawAttachment(SRendParams& rParams, const SRenderingPassInfo& passInfo, const Matrix34& rWorldMat34, f32 fZoomFactor = 1);
	void                 RecreateDefaultSkeleton(CCharInstance* pInstanceSkel, uint32 nLoadingFlags);
	void                 UpdateRemapTable();

	void                 ComputeClothCacheKey();
	uint64               GetClothCacheKey() const { return m_clothCacheKey; };
	void                 AddClothParams(const SVClothParams& clothParams);
	bool                 InitializeCloth();
	const SVClothParams& GetClothParams();

	// Vertex Transformation
public:
	SSkinningData*          GetVertexTransformationData(const bool bVertexAnimation, uint8 nRenderLOD);
	_smart_ptr<IRenderMesh> CreateVertexAnimationRenderMesh(uint lod, uint id);

#ifdef EDITOR_PCDEBUGCODE
	void DrawWireframeStatic(const Matrix34& m34, int nLOD, uint32 color);
	void SoftwareSkinningDQ_VS_Emulator(CModelMesh* pModelMesh, Matrix34 rRenderMat34, uint8 tang, uint8 binorm, uint8 norm, uint8 wire, const DualQuat* const pSkinningTransformations);
#endif

	virtual IVertexAnimation* GetIVertexAnimation() override { return &m_vertexAnimation; }
	virtual ISkin*            GetISkin() override            { return m_pRenderSkin; };
	virtual float             GetExtent(EGeomForm eForm) override;
	virtual void              GetRandomPos(PosNorm& ran, CRndGen& seed, EGeomForm eForm) const override;
	virtual void              ComputeGeometricMean(SMeshLodInfo& lodInfo) const override;

	int                       GetGuid() const;

	DynArray<JointIdType>   m_arrRemapTable;    // maps skin's bone indices to skeleton's bone indices
	DynArray<JointIdType>   m_arrSimRemapTable; // maps skin's bone indices to skeleton's bone indices
	_smart_ptr<CSkin>       m_pRenderSkin;
	_smart_ptr<CSkin>       m_pSimSkin;
	_smart_ptr<IRenderMesh> m_pRenderMeshsSW[2];
	string                  m_sSoftwareMeshName;
	CVertexData             m_vertexData;
	CVertexAnimation        m_vertexAnimation;
	CClothPiece             m_clothPiece;
	uint64                  m_clothCacheKey;

	// history for skinning data, needed for motion blur
	struct { SSkinningData* pSkinningData; int nFrameID; } m_arrSkinningRendererData[3]; // triple buffered for motion blur

private:
	// functions to keep in sync ref counts on skins and cleanup of remap tables
	void ReleaseRenderSkin();
	void ReleaseSimSkin();
};
