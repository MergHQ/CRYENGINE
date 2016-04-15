// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <CryMemory/PoolAllocator.h>
#include "GraphicsPipeline/Common/GraphicsPipelineStateSet.h"
#include "GraphicsPipeline/StandardGraphicsPipeline.h"
#include "xxhash.h"

// Special purpose array, base on the DynArray implementation, but embedding first elements into the body of the class itself
// Used for a special optimization case where array contain only few elements, in this case no dynamic allocation is needed.
template<class T, class I = int, class STORE = NArray::SmallDynStorage<>>
struct SSpecialRenderItemArray
{
	enum { eCachedItems = 3 };
public:
	SSpecialRenderItemArray() : m_size(0), m_capacity(0), m_array(0) {};
	~SSpecialRenderItemArray()
	{
		if (m_array)
			CryModuleFree(m_array);
	}
	void push_back(const T& value)
	{
		int index = m_size++;
		if (index >= eCachedItems)
		{
			if (m_size - eCachedItems > m_capacity)
			{
				m_capacity = m_capacity + 16;
				m_array = (T*)CryModuleRealloc(m_array, m_capacity * sizeof(T));
			}
			m_array[index - eCachedItems] = value;
		}
		else
			m_cached[index] = value;
	}
	T& operator[](size_t index)
	{
		if (index >= eCachedItems)
			return m_array[index - eCachedItems];
		else
			return m_cached[index];
	}
	size_t size() const { return m_size; }

private:
	size_t m_size;
	size_t m_capacity;
	T      m_cached[eCachedItems];
	T*     m_array;
};

//////////////////////////////////////////////////////////////////////////
// Class used to allocate/deallocate permanent render objects.
class CPermanentRenderObject : public CRenderObject
{
public:
	enum ERenderPassType
	{
		eRenderPass_General,
		eRenderPass_Shadows,
		eRenderPass_NumTypes
	};
public:
	CPermanentRenderObject()
		: m_pNextPermanent(nullptr)
		, m_compiledReadyMask(0)
		, m_accessLock(0)
		, m_lastCompiledFrame(0)
	{
		m_bPermanent = true; // must be in constructor
	};
	~CPermanentRenderObject();

	//
	void PrepareForUse(CRendElementBase* pRenderElement, ERenderPassType passType)
	{
		if (pRenderElement && pRenderElement->mfGetType() == eDATA_Mesh)
		{
			CRenderMesh* pRenderMesh = static_cast<CRenderMesh*>(static_cast<CREMesh*>(pRenderElement)->m_pRenderMesh);
			if (pRenderMesh != m_pUsedRenderMesh[passType])
			{
				// Cache this render mesh, to prevent it's deletion while we still reference it
				m_pUsedRenderMesh[passType] = pRenderMesh;
			}
		}
		if (m_pCurrMaterial != m_pUsedMaterial)
		{
			// Cache used material
			m_pUsedMaterial = m_pCurrMaterial;
		}
		m_bAllCompiledValid = false;
		m_compiledReadyMask = 0;
		m_lastCompiledFrame = 0;
	}

	static CPermanentRenderObject* AllocateFromPool();
	static void                    FreeToPool(CPermanentRenderObject* pObj);

	static void                    SetStaticPools(CRenderObjectsPools* pools) { s_pPools = pools; }

public:
	struct SPermanentRendItem
	{
		CCompiledRenderObject* m_pCompiledObject; //!< Compiled object with precompiled PSO
		CRendElementBase*      m_pRenderElement;  //!< Mesh subset, or a special rendering object
		uint32                 m_objSort;         //!< Custom object sorting value.
		uint32                 m_sortValue;       //!< Encoded sort value, keeping info about material.
		uint32                 m_nBatchFlags;     //!< see EBatchFlags, batch flags describe on what passes this object will be used (transparent,z-prepass,etc...)
		uint32                 m_nRenderList : 8; //!< Defines in which render list this compiled object belongs to, see ERenderListID
	};
	//! Persistent object record all items added to the object for the general and for shadow pass.
	//! These items are then efficiently expanded adding render items to CRenderView lists.
	SSpecialRenderItemArray<SPermanentRendItem> m_permanentRenderItems[eRenderPass_NumTypes];

	// Next child sub object used for permanent objects
	CPermanentRenderObject* m_pNextPermanent;

	//! Correspond to the m_passReadyMask, object considered compiled when m_compiledReadyMask == m_passReadyMask
	volatile int m_compiledReadyMask;
	int          m_lastCompiledFrame;

	// Make a reference to a render mesh, to prevent it being deleted while permanent render object still exist.
	_smart_ptr<CRenderMesh> m_pUsedRenderMesh[eRenderPass_NumTypes];
	_smart_ptr<IMaterial>   m_pUsedMaterial;

	mutable volatile int    m_accessLock;

private:
	static CRenderObjectsPools* s_pPools;
};

//////////////////////////////////////////////////////////////////////////
// Collection of pools used by CompiledObjects
class CRenderObjectsPools
{
public:
	CRenderObjectsPools();
	~CRenderObjectsPools();
	CConstantBufferPtr AllocatePerInstanceConstantBuffer();
	void               FreePerInstanceConstantBuffer(const CConstantBufferPtr& buffer);

public:

	struct PoolSyncCriticalSection
	{
		void Lock()   { m_cs.Lock(); }
		void Unlock() { m_cs.Unlock(); }
		CryCriticalSectionNonRecursive m_cs;
	};

	// Global pool for compiled objects, overridden class new/delete operators use it.
	stl::TPoolAllocator<CCompiledRenderObject, PoolSyncCriticalSection> m_compiledObjectsPool;

	// Pool used to allocate permanent render objects
	stl::TPoolAllocator<CPermanentRenderObject, PoolSyncCriticalSection> m_permanentRenderObjectsPool;

private:
	// Used to pool per instance constant buffers.
	std::vector<CConstantBufferPtr> m_freeConstantBuffers;
};

// Used by Graphics Pass pipeline
class CCompiledRenderObject
{
public:
	struct SInstancingData
	{
		CConstantBufferPtr m_pConstBuffer;
		int                m_nInstances;
	};
	struct SRootConstants
	{
		float dissolve;
		float dissolveOut;
	};
	struct SDrawParams
	{
		int32 m_nVerticesCount;
		int32 m_nStartIndex;
		int32 m_nNumIndices;

		SDrawParams()
			: m_nVerticesCount(0)
			, m_nStartIndex(0)
			, m_nNumIndices(0)
		{}
	};

public:
	CRenderObject* m_pRO;

	// Optimized access to constants that can be directly bound to the root signature.
	SRootConstants m_rootConstants;

	/////////////////////////////////////////////////////////////////////////////
	// Packed parameters
	uint32 m_StencilRef           : 8; //!< Stencil ref value
	uint32 m_nVertexStreamSetSize : 8; //!< Highest vertex stream index
	uint32 m_bOwnPerInstanceCB    : 1; //!< True if object owns its own per instance constant buffer, and is not sharing it with other compiled object
	uint32 m_bRenderNearest       : 1; //!< True if object needs to be rendered with nearest camera
	uint32 m_bIncomplete          : 1; //!< True if compilation failed
	uint32 m_bHasTessellation     : 1; //!< True if tessellation is enabled
	uint32 m_bSharedWithShadow    : 1; //!< True if objects is shared between shadow and general pass
	/////////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// Members used in submitting in the draw submission, sorted by access order

	// Array of the PSOs for every ERenderableTechnique.
	// Sub array of PSO is defined inside the pass, and indexed by an integer passed inside the SGraphicsPipelinePassContext
	DevicePipelineStatesArray m_pso[MAX_PIPELINE_SCENE_STAGES];

	// From Material
	CDeviceResourceSetPtr m_materialResourceSet;

	// Per instance constant buffer contain transformation matrix and other potentially not often changed data.
	CConstantBufferPtr m_perInstanceCB;

	// Per instance extra data
	CDeviceResourceSetPtr m_perInstanceExtraResources;
	int                   m_TessellationPatchIDOffset; // for tessellation only
	CGpuBuffer*           m_pTessellationAdjacencyBuffer;
	CGpuBuffer*           m_pExtraSkinWeights; // eight weight skinning

	// Streams data.
	CDeviceInputStream*          m_vertexStreamSet;
	CDeviceInputStream*          m_indexStreamSet;

	std::vector<SInstancingData> m_InstancingCBs;

	// DrawCall parameters, store separate values for merged shadow-gen draw calls
	SDrawParams m_drawParams[2];

	// Skinning constant buffers, primary and previous
	CConstantBufferPtr m_skinningCB[2];

	uint32             m_nInstances; //!< Number of instances.

private:
	// These 2 members must be initialized prior to calling Compile
	CRendElementBase* m_pRenderElement;
	SShaderItem       m_shaderItem;

	//////////////////////////////////////////////////////////////////////////
public:
	CCompiledRenderObject()
		: m_StencilRef(0)
		, m_nVertexStreamSetSize(0)
		, m_bOwnPerInstanceCB(false)
		, m_bIncomplete(true)
		, m_bHasTessellation(false)
		, m_bSharedWithShadow(false)
		, m_nInstances(0)
		, m_TessellationPatchIDOffset(-1)
	{
		for (int i = 0; i < MAX_PIPELINE_SCENE_STAGES; ++i)
		{
			std::fill(m_pso[i].begin(), m_pso[i].end(), nullptr);
		}
	}
	~CCompiledRenderObject();

	// Compile(): Returns true if the compilation is fully finished, false if compilation should be retriggered later

	bool Compile(CRenderObject* pRenderObject, float realTime);
	void PrepareForUse(CDeviceGraphicsCommandListRef RESTRICT_REFERENCE commandList, bool bInstanceOnly) const;

	bool DrawVerification(const SGraphicsPipelinePassContext& passContext) const;
	void DrawToCommandList(CDeviceGraphicsCommandListRef RESTRICT_REFERENCE commandList, const CDeviceGraphicsPSOPtr& pPso, uint32 drawParamsIndex) const;

	void Init(const SShaderItem& shaderItem, CRendElementBase* pRE);

public:
	static CCompiledRenderObject* AllocateFromPool()
	{
		return s_pPools->m_compiledObjectsPool.New();
	}
	static void FreeToPool(CCompiledRenderObject* ptr)
	{
		s_pPools->m_compiledObjectsPool.Delete(ptr);
	}
	static void SetStaticPools(CRenderObjectsPools* pools) { s_pPools = pools; }

private:
	void CompilePerInstanceConstantBuffer(CRenderObject* pRenderObject, float realTime);
	void CompilePerInstanceExtraResources(CRenderObject* pRenderObject);
	void CompileInstancingData(CRenderObject* pRenderObject, bool bForce);
	void UpdatePerInstanceCB(void* pData, size_t size);

private:
	static CRenderObjectsPools* s_pPools;
};
