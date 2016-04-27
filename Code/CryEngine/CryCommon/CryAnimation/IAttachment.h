// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#ifndef IAttachment_h
#define IAttachment_h

#include <CryAnimation/ICryAnimation.h>
#include <CryParticleSystem/IParticles.h>
#include <CryString/CryName.h>

struct IStatObj;
struct IMaterial;
struct IPhysicalEntity;
struct ICharacterInstance;
struct IAttachmentSkin;
struct IAttachment;
struct IAttachmentObject;
struct IVertexAnimation;
struct IProxy;

//! Flags used by game. DO NOT REORDER.
enum AttachmentTypes
{
	CA_BONE,
	CA_FACE,
	CA_SKIN,
	CA_PROX,
	CA_PROW,
	CA_VCLOTH,
	CA_Invalid,
};

enum AttachmentFlags
{
	//Static Flags
	FLAGS_ATTACH_HIDE_ATTACHMENT          = BIT(0),  //!< Already stored in CDF, so don't change this.
	FLAGS_ATTACH_PHYSICALIZED_RAYS        = BIT(1),  //!< Already stored in CDF, so don't change this.
	FLAGS_ATTACH_PHYSICALIZED_COLLISIONS  = BIT(2),  //!< Already stored in CDF, so don't change this.
	FLAGS_ATTACH_SW_SKINNING              = BIT(3),  //!< Already stored in CDF, so don't change this.
	FLAGS_ATTACH_RENDER_ONLY_EXISTING_LOD = BIT(4),  //!< Already stored in CDF, so don't change this.
	FLAGS_ATTACH_LINEAR_SKINNING          = BIT(5),  //!< Already stored in CDF, so don't change this.
	FLAGS_ATTACH_PHYSICALIZED             = FLAGS_ATTACH_PHYSICALIZED_RAYS | FLAGS_ATTACH_PHYSICALIZED_COLLISIONS,

	// Geometry deformation using direct compute
	FLAGS_ATTACH_DC_DEFORMATION_SKINNING  = BIT(6),  //!< Already stored in CDF, so don't change this.
	FLAGS_ATTACH_DC_DEFORMATION_PREMORPHS = BIT(7),  //!< Already stored in CDF, so don't change this.
	FLAGS_ATTACH_DC_DEFORMATION_TANGENTS  = BIT(8),  //!< Already stored in CDF, so don't change this.

	// Dynamic Flags
	FLAGS_ATTACH_VISIBLE            = BIT(13),    //!< We set this flag if we can render the object.
	FLAGS_ATTACH_PROJECTED          = BIT(14),    //!< We set this flag if we can attacht the object to a triangle.
	FLAGS_ATTACH_WAS_PHYSICALIZED   = BIT(15),    //!< The attachment actually was physicalized.
	FLAGS_ATTACH_HIDE_MAIN_PASS     = BIT(16),
	FLAGS_ATTACH_HIDE_SHADOW_PASS   = BIT(17),
	FLAGS_ATTACH_HIDE_RECURSION     = BIT(18),
	FLAGS_ATTACH_NEAREST_NOFOV      = BIT(19),
	FLAGS_ATTACH_NO_BBOX_INFLUENCE  = BIT(21),
	FLAGS_ATTACH_COMBINEATTACHMENT  = BIT(24),
	FLAGS_ATTACH_ID_MASK            = BIT(25) | BIT(26) | BIT(27) | BIT(28) | BIT(29),
	FLAGS_ATTACH_MERGED_FOR_SHADOWS = BIT(30),    //!< The attachment has been merged with other attachments for shadow rendering.
};

struct SVClothParams
{
	float  thickness;                             //!< The radius of the particles.
	float  collisionDamping;                      //!< Dampens only non-colliding particles.
	float  dragDamping;                           //!< A value around 1 - smaller for less stretch stiffness, bigger for over-relaxation.
	float  stretchStiffness;                      //!< A value around 1 - smaller for less stretch stiffness, bigger for over-relaxation.
	float  shearStiffness;                        //!< A smaller than 1 value affecting the shear constraints.
	float  bendStiffness;                         //!< A smaller than 1 value affecting the bend constraints.
	int    numIterations;                         //!< Number of iterations for the positional PGS solver (contacts & edges).
	float  timeStep;                              //!< The (pseudo)fixed time step for the simulator.
	float  rigidDamping;                          //!< Blending factor between local and world space rotation.
	float  translationBlend;                      //!< Blending factor between local and world space translation.
	float  rotationBlend;                         //!< Blending factor between local and world space rotation.
	float  friction;                              //!< Friction coefficient for particles at contact points.
	float  pullStiffness;                         //!< The strength of pulling pinched vertices towards the anchor position [0..1].
	float  tolerance;                             //!< Collision tolerance expressed by a factor of thickness.
	float* weights;                               //!< Per vertex weights used for blending with animation.
	float  maxBlendWeight;                        //!< Maximum value of pull stiffness for blending with animation.
	float  maxAnimDistance;
	float  windBlend;
	int    collDampingRange;
	float  stiffnessGradient;
	bool   halfStretchIterations;
	bool   hide;
	bool   isMainCharacter;

	string simMeshName;
	string renderMeshName;

	string renderBinding;
	string simBinding;

	SVClothParams()
		: timeStep(0.01f)
		, numIterations(20)
		, thickness(0.01f)
		, pullStiffness(0)
		, stretchStiffness(1)
		, shearStiffness(0)
		, bendStiffness(0)
		, collisionDamping(1)
		, dragDamping(1)
		, rigidDamping(0)
		, translationBlend(1)
		, rotationBlend(1)
		, friction(0)
		, tolerance(1.5f)
		, maxAnimDistance(0)
		, windBlend(0)
		, collDampingRange(3)
		, maxBlendWeight(1)
		, stiffnessGradient(0)
		, halfStretchIterations(false)
		, simMeshName("")
		, renderMeshName("")
		, renderBinding("")
		, simBinding("")
		, hide(false)
		, isMainCharacter(false)
	{
	}
};

struct SimulationParams
{
	enum { MaxCollisionProxies = 10 };

	enum ClampType
	{
		DISABLED                 = 0x00,

		PENDULUM_CONE            = 0x01, //!< For pendula.
		PENDULUM_HINGE_PLANE     = 0x02, //!< For pendula.
		PENDULUM_HALF_CONE       = 0x03, //!< For pendula.
		SPRING_ELLIPSOID         = 0x04, //!< For springs.
		TRANSLATIONAL_PROJECTION = 0x05  //!< Used to project joints out of proxies.
	};

	ClampType          m_nClampType;
	bool               m_useDebugSetup;
	bool               m_useDebugText;
	bool               m_useSimulation;
	bool               m_useRedirect;
	uint8              m_nSimFPS;

	f32                m_fMaxAngle;
	f32                m_fRadius;
	Vec2               m_vSphereScale;
	Vec2               m_vDiskRotation;

	f32                m_fMass;
	f32                m_fGravity;
	f32                m_fDamping;
	f32                m_fStiffness;

	Vec3               m_vPivotOffset;
	Vec3               m_vSimulationAxis;
	Vec3               m_vStiffnessTarget;
	Vec2               m_vCapsule;
	CCryName           m_strProcFunction;

	int32              m_nProjectionType;
	CCryName           m_strDirTransJoint;
	DynArray<CCryName> m_arrProxyNames; //!< Test capsules/sphere joint against these colliders.

	SimulationParams()
	{
		m_nClampType = DISABLED;
		m_useDebugSetup = 0;
		m_useDebugText = 0;
		m_useSimulation = 1;
		m_useRedirect = 0;
		m_nSimFPS = 10;                   //!< Don't set to 0.

		m_fMaxAngle = 45.0f;
		m_vDiskRotation.set(0, 0);

		m_fRadius = 0.5f;
		m_vSphereScale.set(1.0f, 1.0f);

		m_fMass = 1.0f;
		m_fGravity = 9.81f;
		m_fDamping = 1.0f;
		m_fStiffness = 0.0f;

		m_vPivotOffset.zero();
		m_vSimulationAxis = Vec3(0.0f, 0.5f, 0.0f);
		m_vStiffnessTarget.zero();
		m_vCapsule.set(0, 0);
		m_nProjectionType = 0;
	};
};

struct RowSimulationParams
{
	enum ClampMode
	{
		PENDULUM_CONE            = 0x00, //!< For pendula.
		PENDULUM_HINGE_PLANE     = 0x01, //!< For pendula.
		PENDULUM_HALF_CONE       = 0x02, //!< For pendula.
		TRANSLATIONAL_PROJECTION = 0x03  //!< Used to project joints out of proxies.
	};

	ClampMode          m_nClampMode;
	bool               m_useDebugSetup;
	bool               m_useDebugText;
	bool               m_useSimulation;
	uint8              m_nSimFPS;

	f32                m_fConeAngle;
	Vec3               m_vConeRotation;

	f32                m_fMass;
	f32                m_fGravity;
	f32                m_fDamping;
	f32                m_fJointSpring;
	f32                m_fRodLength;
	Vec2               m_vStiffnessTarget;
	Vec2               m_vTurbulence;
	f32                m_fMaxVelocity;

	bool               m_cycle;
	f32                m_fStretch;
	uint32             m_relaxationLoops;

	Vec3               m_vTranslationAxis;
	CCryName           m_strDirTransJoint;

	Vec2               m_vCapsule;
	int32              m_nProjectionType;
	DynArray<CCryName> m_arrProxyNames;  //!< Test capsules/sphere joint against these colliders.

	RowSimulationParams()
	{
		m_nClampMode = PENDULUM_CONE;
		m_useDebugSetup = 0;
		m_useDebugText = 0;
		m_useSimulation = 1;
		m_nSimFPS = 10;

		m_fConeAngle = 45.0f;
		m_vConeRotation = Vec3(0, 0, 0);

		m_fMass = 1.0f;
		m_fGravity = 9.81f;
		m_fDamping = 1.0f;
		m_fJointSpring = 0.0f;
		m_fRodLength = 0.0f;
		m_vStiffnessTarget = Vec2(0.0f, 0.0f);
		m_vTurbulence = Vec2(0.5f, 0.0f);
		m_fMaxVelocity = 8.0f;

		m_cycle = 0;
		m_relaxationLoops = 0;
		m_fStretch = 0.10f;

		m_vCapsule.set(0, 0);
		m_nProjectionType = 0;
		m_vTranslationAxis = Vec3(0.0f, 1.0f, 0.0f);
	};
};

struct IAttachmentManager
{
	// <interfuscator:shuffle>
	virtual ~IAttachmentManager(){}
	virtual uint32              LoadAttachmentList(const char* pathname) = 0;

	virtual IAttachment*        CreateAttachment(const char* szName, uint32 type, const char* szJointName = 0, bool bCallProject = true) = 0;

	virtual int32               RemoveAttachmentByInterface(const IAttachment* pAttachment, uint32 loadingFlags = 0) = 0;
	virtual int32               RemoveAttachmentByName(const char* szName, uint32 loadingFlags = 0) = 0;
	virtual int32               RemoveAttachmentByNameCRC(uint32 nameCrc, uint32 loadingFlags = 0) = 0;

	virtual IAttachment*        GetInterfaceByName(const char* szName) const = 0;
	virtual IAttachment*        GetInterfaceByIndex(uint32 c) const = 0;
	virtual IAttachment*        GetInterfaceByNameCRC(uint32 nameCRC) const = 0;
	virtual IAttachment*        GetInterfaceByPhysId(int id) const = 0;    //!< Takes physical partid as input.

	virtual int32               GetAttachmentCount() const = 0;
	virtual int32               GetIndexByName(const char* szName) const = 0;
	virtual int32               GetIndexByNameCRC(uint32 nameCRC) const = 0;

	virtual uint32              ProjectAllAttachment() = 0;

	virtual void                PhysicalizeAttachment(int idx, IPhysicalEntity* pent = 0, int nLod = 0) = 0;
	virtual void                DephysicalizeAttachment(int idx, IPhysicalEntity* pent = 0) = 0;

	virtual ICharacterInstance* GetSkelInstance() const = 0;

	virtual int32               GetProxyCount() const = 0;
	virtual IProxy*             CreateProxy(const char* szName, const char* szJointName) = 0;
	virtual IProxy*             GetProxyInterfaceByIndex(uint32 c) const = 0;
	virtual IProxy*             GetProxyInterfaceByName(const char* szName) const = 0;
	virtual IProxy*             GetProxyInterfaceByCRC(uint32 nameCRC) const = 0;
	virtual int32               GetProxyIndexByName(const char* szName) const = 0;
	virtual int32               RemoveProxyByInterface(const IProxy* ptr) = 0;
	virtual int32               RemoveProxyByName(const char* szName) = 0;
	virtual int32               RemoveProxyByNameCRC(uint32 nameCRC) = 0;
	virtual void                DrawProxies(uint32 enable) = 0;
	virtual uint32              GetProcFunctionCount() const = 0;
	virtual const char*         GetProcFunctionName(uint32 idx) const = 0;
	// </interfuscator:shuffle>
};

struct IAttachment
{
	// <interfuscator:shuffle>

	virtual void        AddRef() = 0;
	virtual void        Release() = 0;

	virtual const char* GetName() const = 0;
	virtual uint32      GetNameCRC() const = 0;
	virtual uint32      ReName(const char* szSocketName, uint32 crc) = 0;

	virtual uint32      GetType() const = 0;
	bool                IsMerged() const { return (GetFlags() & FLAGS_ATTACH_MERGED_FOR_SHADOWS) != 0; }

	virtual uint32      SetJointName(const char* szJointName) = 0;

	virtual void        SetFlags(uint32 flags) = 0;
	virtual uint32      GetFlags() const = 0;

	/**
	 * Sets model-space location of this attachment in the default pose.
	 * @param rot New attachment location.
	 */
	virtual void SetAttAbsoluteDefault(const QuatT& rot) = 0;

	/**
	 * Retrieves model-space location of this attachment in the default pose.
	 * @return Attachment location.
	 */
	virtual const QuatT& GetAttAbsoluteDefault() const = 0;

	/**
	 * Sets parent-space location of this attachment in the default pose.
	 * @param mat New attachment location.
	 * IMPORTANT: This feature is currently missing. The location set by this method is being overridden by the animation
	 * update. Please compute the location manually and use the SetAttAbsoluteDefault() method instead as a workaround.
	 */
	virtual void SetAttRelativeDefault(const QuatT& mat) = 0;

	/**
	 * Retrieves parent-space location of this attachment in the default pose.
	 * @return Attachment location.
	 */
	virtual const QuatT& GetAttRelativeDefault() const = 0;

	/**
	 * Retrieves current model-space location of this attachment.
	 * @return Attachment location.
	 */
	virtual const QuatT& GetAttModelRelative() const = 0;

	/**
	 * Retrieves current world-space location of this attachment.
	 * @return Attachment location.
	 */
	virtual const QuatTS GetAttWorldAbsolute() const = 0;

	/**
	 * Retrieves the additional transformation which is applied on top of the model-space location
	 * while rendering this attachment. Typically this returns QuatT(IDENTITY), except when simulation is running.
	 * @return Additional transformation
	 */
	virtual const QuatT&       GetAdditionalTransformation() const = 0;

	virtual void               UpdateAttModelRelative() = 0;

	virtual void               HideAttachment(uint32 x) = 0;
	virtual uint32             IsAttachmentHidden() const = 0;

	virtual void               HideInRecursion(uint32 x) = 0;
	virtual uint32             IsAttachmentHiddenInRecursion() const = 0;

	virtual void               HideInShadow(uint32 x) = 0;
	virtual uint32             IsAttachmentHiddenInShadow() const = 0;

	virtual void               AlignJointAttachment() = 0;

	virtual uint32             GetJointID() const = 0;

	virtual IAttachmentObject* GetIAttachmentObject() const = 0;
	virtual IAttachmentSkin*   GetIAttachmentSkin() = 0;

	// These functions modify the attachment object that is bound to an attachment (slot)
	// They are safe to call from the main thread at any time,
	// the actual execution is defered to a time when no animation job is running for
	// the character instance
	virtual void AddBinding(IAttachmentObject* pModel, ISkin* pISkin = 0, uint32 nLoadingFlags = 0) = 0;
	virtual void ClearBinding(uint32 nLoadingFlags = 0) = 0;
	virtual void SwapBinding(IAttachment* pNewAttachment) = 0;

	// TODO: get rid of the default implementation on the following methods:
	virtual SimulationParams&    GetSimulationParams()                                                                   { static SimulationParams ap; return ap; }
	virtual void                 PostUpdateSimulationParams(bool bAttachmentSortingRequired, const char* pJointName = 0) {}
	virtual RowSimulationParams& GetRowParams()                                                                          { static RowSimulationParams ap; return ap; }

	virtual size_t               SizeOfThis() const = 0;
	virtual void                 Serialize(TSerialize ser) = 0; // TODO: should be const
	virtual void                 GetMemoryUsage(ICrySizer* pSizer) const = 0;
	virtual ~IAttachment() {}

	// </interfuscator:shuffle>
};

//! Interface to a skin-attachment.
struct IAttachmentSkin
{
	// <interfuscator:shuffle>
	virtual void              AddRef() = 0;
	virtual void              Release() = 0;
	virtual ISkin*            GetISkin() = 0;
	virtual IVertexAnimation* GetIVertexAnimation() = 0;
	virtual float             GetExtent(EGeomForm eForm) = 0;
	virtual void              GetRandomPos(PosNorm& ran, CRndGen& seed, EGeomForm eForm) const = 0;
	virtual void              GetMemoryUsage(class ICrySizer* pSizer) const = 0;
	virtual void              ComputeGeometricMean(SMeshLodInfo& lodInfo) const = 0;
	virtual ~IAttachmentSkin(){}
	// </interfuscator:shuffle>

#ifdef EDITOR_PCDEBUGCODE
	virtual void TriggerMeshStreaming(uint32 nDesiredRenderLOD, const SRenderingPassInfo& passInfo) = 0;
	virtual void DrawWireframeStatic(const Matrix34& m34, int nLOD, uint32 color) = 0;
#endif
};

//! This interface define a way to allow an object to be bound to a character.
struct IAttachmentObject
{
	enum EType
	{
		eAttachment_Unknown,
		eAttachment_StatObj,
		eAttachment_Skeleton,
		eAttachment_SkinMesh,
		eAttachment_Entity,
		eAttachment_Light,
		eAttachment_Effect,
	};

	// <interfuscator:shuffle>
	virtual ~IAttachmentObject(){}
	virtual EType GetAttachmentType() = 0;

	virtual void  ProcessAttachment(IAttachment* pIAttachment) = 0;
	virtual void  RenderAttachment(SRendParams& rParams, const SRenderingPassInfo& passInfo) {};

	//! \return Handled state.
	virtual bool                PhysicalizeAttachment(IAttachmentManager* pManager, int idx, int nLod, IPhysicalEntity* pent, const Vec3& offset) { return false; }
	virtual bool                UpdatePhysicalizedAttachment(IAttachmentManager* pManager, int idx, IPhysicalEntity* pent, const QuatT& offset)   { return false; }

	virtual AABB                GetAABB() const = 0;
	virtual float               GetRadiusSqr() const = 0;

	virtual IStatObj*           GetIStatObj() const;
	virtual ICharacterInstance* GetICharacterInstance() const;
	virtual IAttachmentSkin*    GetIAttachmentSkin() const { return 0; };
	virtual const char*         GetObjectFilePath() const
	{
		ICharacterInstance* pICharInstance = GetICharacterInstance();
		if (pICharInstance)
			return pICharInstance->GetFilePath();
		IStatObj* pIStaticObject = GetIStatObj();
		if (pIStaticObject)
			return pIStaticObject->GetFilePath();
		IAttachmentSkin* pIAttachmentSkin = GetIAttachmentSkin();
		if (pIAttachmentSkin)
		{
			ISkin* pISkin = pIAttachmentSkin->GetISkin();
			if (pISkin)
				return pISkin->GetModelFilePath();
		}
		return NULL;
	}
	virtual IMaterial* GetBaseMaterial(uint32 nLOD = 0) const = 0;
	virtual void       SetReplacementMaterial(IMaterial* pMaterial, uint32 nLOD = 0) = 0;
	virtual IMaterial* GetReplacementMaterial(uint32 nLOD = 0) const = 0;

	virtual void       OnRemoveAttachment(IAttachmentManager* pManager, int idx) {}
	virtual void       Release() = 0;
	// </interfuscator:shuffle>
};

inline ICharacterInstance* IAttachmentObject::GetICharacterInstance() const
{
	return 0;
}

inline IStatObj* IAttachmentObject::GetIStatObj() const
{
	return 0;
}

struct IAttachmentMerger
{
	virtual ~IAttachmentMerger() {};
	virtual uint GetAllocatedBytes() const = 0;
	virtual bool IsOutOfMemory() const = 0;
	virtual uint GetMergedAttachmentsCount() const = 0;
};

//

struct CCGFAttachment : public IAttachmentObject
{
	virtual EType GetAttachmentType() override                          { return eAttachment_StatObj; };
	virtual void  ProcessAttachment(IAttachment* pIAttachment) override {}
	void          RenderAttachment(SRendParams& rParams, const SRenderingPassInfo& passInfo) override
	{
		rParams.pInstance = this;
		IMaterial* pPrev = rParams.pMaterial;
		rParams.pMaterial = pObj->GetMaterial();
		if (m_pReplacementMaterial)
			rParams.pMaterial = m_pReplacementMaterial;
		pObj->Render(rParams, passInfo);
		rParams.pMaterial = pPrev;
	};

	virtual AABB  GetAABB() const override      { return pObj->GetAABB(); };
	virtual float GetRadiusSqr() const override { return sqr(pObj->GetRadius()); };
	IStatObj*     GetIStatObj() const override;
	void          Release()  override           { delete this; }

	IMaterial*    GetBaseMaterial(uint32 nLOD) const override;
	void          SetReplacementMaterial(IMaterial* pMaterial, uint32 nLOD = 0) override { m_pReplacementMaterial = pMaterial; };
	IMaterial*    GetReplacementMaterial(uint32 nLOD) const override                     { return m_pReplacementMaterial; }

	_smart_ptr<IStatObj>  pObj;
	_smart_ptr<IMaterial> m_pReplacementMaterial;
};

inline IStatObj* CCGFAttachment::GetIStatObj() const
{
	return pObj;
}

inline IMaterial* CCGFAttachment::GetBaseMaterial(uint32 nLOD) const
{
	return m_pReplacementMaterial ? m_pReplacementMaterial.get() : pObj->GetMaterial();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
struct CSKELAttachment : public IAttachmentObject
{
	CSKELAttachment()
	{
	}

	virtual EType GetAttachmentType() override { return eAttachment_Skeleton; };
	void          RenderAttachment(SRendParams& rParams, const SRenderingPassInfo& passInfo) override
	{
		rParams.pInstance = this;
		IMaterial* pPrev = rParams.pMaterial;
		rParams.pMaterial = (IMaterial*)(m_pCharInstance ? m_pCharInstance->GetIMaterial() : 0);
		if (m_pReplacementMaterial)
			rParams.pMaterial = m_pReplacementMaterial;
		m_pCharInstance->Render(rParams, QuatTS(IDENTITY), passInfo);
		rParams.pMaterial = pPrev;
	};
	virtual void        ProcessAttachment(IAttachment* pIAttachment)  override {}
	virtual AABB        GetAABB() const override                               { return m_pCharInstance ? m_pCharInstance->GetAABB() : AABB(AABB::RESET);  };
	virtual float       GetRadiusSqr() const override                          { return m_pCharInstance ? m_pCharInstance->GetRadiusSqr() : 0.0f;    }
	ICharacterInstance* GetICharacterInstance() const override;

	void                Release() override
	{
		if (m_pCharInstance)
			m_pCharInstance->OnDetach();
		delete this;
	}

	IMaterial* GetBaseMaterial(uint32 nLOD) const override;
	void       SetReplacementMaterial(IMaterial* pMaterial, uint32 nLOD) override { m_pReplacementMaterial = pMaterial; }
	IMaterial* GetReplacementMaterial(uint32 nLOD) const override                 { return m_pReplacementMaterial; }

	_smart_ptr<ICharacterInstance> m_pCharInstance;
	_smart_ptr<IMaterial>          m_pReplacementMaterial;
};

inline ICharacterInstance* CSKELAttachment::GetICharacterInstance() const
{
	return m_pCharInstance;
}

inline IMaterial* CSKELAttachment::GetBaseMaterial(uint32 nLOD) const
{
	return m_pReplacementMaterial ? m_pReplacementMaterial.get() : (m_pCharInstance ? m_pCharInstance->GetIMaterial() : NULL);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
struct CSKINAttachment : public IAttachmentObject
{
	CSKINAttachment()
	{
	}

	virtual EType    GetAttachmentType() override                          { return eAttachment_SkinMesh; };
	virtual AABB     GetAABB() const override                              { return AABB(AABB::RESET); };
	virtual float    GetRadiusSqr() const override                         { return 0.0f; }
	IAttachmentSkin* GetIAttachmentSkin() const override                   { return m_pIAttachmentSkin; }
	void             Release() override                                    { delete this; }
	virtual void     ProcessAttachment(IAttachment* pIAttachment) override {};

	IMaterial*       GetBaseMaterial(uint32 nLOD) const override;
	void             SetReplacementMaterial(IMaterial* pMaterial, uint32 nLOD) override
	{
		nLOD = clamp_tpl<uint32>(nLOD, 0, g_nMaxGeomLodLevels - 1);
		m_pReplacementMaterial[nLOD] = pMaterial;
	}
	IMaterial* GetReplacementMaterial(uint32 nLOD) const override
	{
		nLOD = clamp_tpl<uint32>(nLOD, 0, g_nMaxGeomLodLevels - 1);
		return m_pReplacementMaterial[nLOD];
	}

	_smart_ptr<IAttachmentSkin> m_pIAttachmentSkin;
	_smart_ptr<IMaterial>       m_pReplacementMaterial[g_nMaxGeomLodLevels];
};

inline IMaterial* CSKINAttachment::GetBaseMaterial(uint32 nLOD) const
{
	return m_pReplacementMaterial[nLOD] ? m_pReplacementMaterial[nLOD].get() : (m_pIAttachmentSkin ? m_pIAttachmentSkin->GetISkin()->GetIMaterial(nLOD) : NULL);
}

struct CEntityAttachment : public IAttachmentObject
{
public:
	CEntityAttachment() : m_scale(1.0f, 1.0f, 1.0f), m_id(0){}

	virtual EType GetAttachmentType() override { return eAttachment_Entity; };
	void          SetEntityId(EntityId id)     { m_id = id; };
	EntityId      GetEntityId()                { return m_id; }

	virtual void  ProcessAttachment(IAttachment* pIAttachment) override
	{
		const QuatTS quatTS = QuatTS(pIAttachment->GetAttWorldAbsolute());
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_id);
		if (pEntity)
			pEntity->SetPosRotScale(quatTS.t, quatTS.q, m_scale * quatTS.s, ENTITY_XFORM_NO_PROPOGATE);
	}

	virtual AABB GetAABB() const override
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_id);
		AABB aabb(Vec3(0, 0, 0), Vec3(0, 0, 0));

		if (pEntity) pEntity->GetLocalBounds(aabb);
		return aabb;
	};

	virtual float GetRadiusSqr() const override
	{
		IEntity* pEntity = gEnv->pEntitySystem->GetEntity(m_id);
		if (pEntity)
		{
			AABB aabb;
			pEntity->GetLocalBounds(aabb);
			return aabb.GetRadiusSqr();
		}
		else
		{
			return 0.0f;
		}
	};

	void       Release() override { delete this; }

	IMaterial* GetBaseMaterial(uint32 nLOD) const override;
	void       SetReplacementMaterial(IMaterial* pMaterial, uint32 nLOD) override {}
	IMaterial* GetReplacementMaterial(uint32 nLOD) const override                 { return 0; }
	void       SetScale(const Vec3& scale)
	{
		m_scale = scale;
	}

private:
	EntityId m_id;
	Vec3     m_scale;
};

inline IMaterial* CEntityAttachment::GetBaseMaterial(uint32 nLOD) const
{
	return 0;
}

struct CLightAttachment : public IAttachmentObject
{
public:
	CLightAttachment() : m_pLightSource(0) {};
	virtual ~CLightAttachment()
	{
		if (m_pLightSource)
		{
			gEnv->p3DEngine->DeleteRenderNode(m_pLightSource);
			m_pLightSource = NULL;
		}
	};

	virtual EType GetAttachmentType() override { return eAttachment_Light; }

	void          LoadLight(const CDLight& light)
	{
		m_pLightSource = gEnv->p3DEngine->CreateLightSource();
		if (m_pLightSource)
			m_pLightSource->SetLightProperties(light);
	}

	ILightSource* GetLightSource() { return m_pLightSource; }

	virtual void  ProcessAttachment(IAttachment* pIAttachment) override
	{
		if (m_pLightSource)
		{
			CDLight& light = m_pLightSource->GetLightProperties();
			Matrix34 worldMatrix = Matrix34(pIAttachment->GetAttWorldAbsolute());
			Vec3 origin = worldMatrix.GetTranslation();
			light.SetPosition(origin);
			light.SetMatrix(worldMatrix);
			light.m_sName = pIAttachment->GetName();
			m_pLightSource->SetMatrix(worldMatrix);
			f32 r = light.m_fRadius;
			m_pLightSource->SetBBox(AABB(Vec3(origin.x - r, origin.y - r, origin.z - r), Vec3(origin.x + r, origin.y + r, origin.z + r)));
			gEnv->p3DEngine->RegisterEntity(m_pLightSource);
		}
	}

	virtual AABB GetAABB() const override
	{
		f32 r = m_pLightSource->GetLightProperties().m_fRadius;
		return AABB(Vec3(-r, -r, -r), Vec3(+r, +r, +r));
	};

	virtual float GetRadiusSqr() const override { return sqr(m_pLightSource->GetLightProperties().m_fRadius); }

	void          Release() override            { delete this; }

	IMaterial*    GetBaseMaterial(uint32 nLOD) const override;
	void          SetReplacementMaterial(IMaterial* pMaterial, uint32 nLOD) override {}
	IMaterial*    GetReplacementMaterial(uint32 nLOD) const override                 { return 0; }

private:
	ILightSource* m_pLightSource;
};

inline IMaterial* CLightAttachment::GetBaseMaterial(uint32 nLOD) const
{
	return 0;
}

struct CEffectAttachment : public IAttachmentObject
{
public:

	virtual EType GetAttachmentType() override { return eAttachment_Effect; }

	CEffectAttachment(const char* effectName, const Vec3& offset, const Vec3& dir, f32 scale = 1.0f, bool prime = false)
		: m_loc(IParticleEffect::ParticleLoc(offset, dir, scale)), m_bPrime(prime)
	{
		m_pEffect = gEnv->pParticleManager->FindEffect(effectName, "Character Attachment", true);
	}

	CEffectAttachment(IParticleEffect* pParticleEffect, const Vec3& offset, const Vec3& dir, f32 scale = 1.0f, bool prime = false)
		: m_loc(IParticleEffect::ParticleLoc(offset, dir, scale)), m_bPrime(prime)
	{
		m_pEffect = pParticleEffect;
	}

	virtual ~CEffectAttachment()
	{
		if (m_pEmitter)
			m_pEmitter->Activate(false);
	}

	IParticleEmitter* GetEmitter()
	{
		return m_pEmitter;
	}

	virtual void ProcessAttachment(IAttachment* pIAttachment) override
	{
		if (!pIAttachment->IsAttachmentHidden())
		{
			QuatTS loc = pIAttachment->GetAttWorldAbsolute() * m_loc;
			if (!m_pEmitter && m_pEffect != 0)
			{
				SpawnParams sp;
				sp.bPrime = m_bPrime;
				m_pEmitter = m_pEffect->Spawn(loc, sp);
			}
			else if (m_pEmitter)
				m_pEmitter->SetLocation(loc);
		}
		else
		{
			m_pEmitter = 0;
		}
	}

	virtual AABB GetAABB() const override
	{
		if (m_pEmitter)
		{
			AABB bb;
			m_pEmitter->GetLocalBounds(bb);
			return bb;
		}
		else
		{
			return AABB(Vec3(-0.1f), Vec3(0.1f));
		}
	};

	virtual float GetRadiusSqr() const override
	{
		if (m_pEmitter)
		{
			AABB bb;
			m_pEmitter->GetLocalBounds(bb);
			return bb.GetRadiusSqr();
		}
		else
		{
			return 0.1f;
		}
	}

	void       Release() override { delete this; }

	IMaterial* GetBaseMaterial(uint32 nLOD) const override;
	void       SetReplacementMaterial(IMaterial* pMaterial, uint32 nLOD) override {}
	IMaterial* GetReplacementMaterial(uint32 nLOD) const override                 { return 0; }

	void       SetSpawnParams(const SpawnParams& params)
	{
		if (m_pEmitter)
			m_pEmitter->SetSpawnParams(params);
	}

private:
	_smart_ptr<IParticleEmitter> m_pEmitter;
	_smart_ptr<IParticleEffect>  m_pEffect;
	QuatTS                       m_loc;
	bool                         m_bPrime;
};

inline IMaterial* CEffectAttachment::GetBaseMaterial(uint32 nLOD) const
{
	return 0;
}

struct IProxy
{
	virtual const char*  GetName() const = 0;
	virtual uint32       GetNameCRC() const = 0;
	virtual uint32       ReName(const char* szSocketName, uint32 crc) = 0;
	virtual uint32       SetJointName(const char* szJointName = 0) = 0;
	virtual uint32       GetJointID() const = 0;

	virtual const QuatT& GetProxyAbsoluteDefault() const = 0;
	virtual const QuatT& GetProxyRelativeDefault() const = 0;
	virtual const QuatT& GetProxyModelRelative() const = 0;
	virtual void         SetProxyAbsoluteDefault(const QuatT& rot) = 0;

	virtual uint32       ProjectProxy() = 0;

	virtual void         AlignProxyWithJoint() = 0;
	virtual Vec4         GetProxyParams() const = 0;
	virtual void         SetProxyParams(const Vec4& params) = 0;
	virtual int8         GetProxyPurpose() const = 0;
	virtual void         SetProxyPurpose(int8 p) = 0;
	virtual void         SetHideProxy(uint8 nHideProxy) = 0;

	virtual ~IProxy(){}
};

#endif // IAttachment_h
