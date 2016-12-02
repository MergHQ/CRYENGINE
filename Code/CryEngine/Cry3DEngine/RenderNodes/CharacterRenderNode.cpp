#include "StdAfx.h"
#include "CharacterRenderNode.h"

#include "ObjMan.h"
#include "3dEngine.h"
#include <CryAnimation/ICryAnimation.h>

//////////////////////////////////////////////////////////////////////////
static bool IsMatrixValid(const Matrix34& mat)
{
	Vec3 vScaleTest = mat.TransformVector(Vec3(0, 0, 1));
	float fDist = mat.GetTranslation().GetDistance(Vec3(0, 0, 0));

	if (vScaleTest.GetLength() > 1000.f || vScaleTest.GetLength() < 0.01f || fDist > 256000 ||
	    !_finite(vScaleTest.x) || !_finite(vScaleTest.y) || !_finite(vScaleTest.z))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////
CCharacterRenderNode::CCharacterRenderNode()
	: m_pCharacterInstance(nullptr)
	, m_pPhysicsEntity(nullptr)
{
	m_boundsWorld = AABB(0.0f);
	m_boundsLocal = AABB(0.0f);
	m_matrix.SetIdentity();
	m_renderOffset.SetIdentity();

	GetInstCount(GetRenderNodeType())++;
}

//////////////////////////////////////////////////////////////////////////
CCharacterRenderNode::~CCharacterRenderNode()
{
	Dephysicalize();
	Get3DEngine()->FreeRenderNodeState(this);

	m_pCharacterInstance = nullptr;
	m_pPhysicsEntity = nullptr;

	if (m_pCameraSpacePos)
	{
		delete m_pCameraSpacePos;
		m_pCameraSpacePos = nullptr;
	}

	Cry3DEngineBase::GetInstCount(GetRenderNodeType())--;
}

//////////////////////////////////////////////////////////////////////////
const char* CCharacterRenderNode::GetName() const
{
	if (m_pCharacterInstance)
		return m_pCharacterInstance->GetFilePath();
	return "Unknown";
}

//////////////////////////////////////////////////////////////////////////
void CCharacterRenderNode::Render(const SRendParams& inputRendParams, const SRenderingPassInfo& passInfo)
{
	FUNCTION_PROFILER_3DENGINE;

	if (!m_pCharacterInstance || m_dwRndFlags & ERF_HIDDEN)
		return;

	// some parameters will be modified
	SRendParams rParms(inputRendParams);

	rParms.nMaterialLayers = m_nMaterialLayers;
	rParms.pMatrix = &m_matrix;
	rParms.nClipVolumeStencilRef = 0;
	rParms.pMaterial = m_pMaterial;
	
	rParms.dwFObjFlags |= FOB_TRANS_MASK | FOB_DYNAMIC_OBJECT;
	rParms.dwFObjFlags |= (GetRndFlags() & ERF_FOB_RENDER_AFTER_POSTPROCESSING) ? FOB_RENDER_AFTER_POSTPROCESSING : 0;

	if (GetRndFlags() & ERF_FOB_NEAREST)
	{
		if (passInfo.IsRecursivePass()) // Nearest objects are not rendered in the recursive passes.
			return;

		rParms.dwFObjFlags |= FOB_NEAREST;

		// Nearest objects recalculate instance matrix every frame
		//m_bPermanentRenderObjectMatrixValid = false;
		QuatTS offset;
		offset.SetIdentity();
		Matrix34 nearestMatrix = m_matrix;
		CalcNearestTransform(nearestMatrix,passInfo);
		rParms.pMatrix = &nearestMatrix;

		m_pCharacterInstance->Render(rParms, offset, passInfo);
	}
	else
	{
		m_pCharacterInstance->Render(rParms, m_renderOffset, passInfo);
	}
}

void CCharacterRenderNode::SetMatrix(const Matrix34& transform)
{
	if (!m_pCharacterInstance)
		return;

	if (!IsMatrixValid(transform))
	{
		Warning("Error: IRenderNode::SetMatrix: Invalid matrix ignored");
		return;
	}

	bool bMoved = !Matrix34::IsEquivalent(m_matrix,transform,0.00001f);
	m_matrix = transform;

	//if (bMoved)
	{
		m_boundsLocal = m_pCharacterInstance->GetAABB();
		m_boundsWorld.SetTransformedAABB(m_matrix, m_boundsLocal);

		Get3DEngine()->UnRegisterEntityAsJob(this);
		Get3DEngine()->RegisterEntity(this);

		m_pCharacterInstance->SetAttachmentLocation_DEPRECATED(QuatTS(transform));
	}
}

//////////////////////////////////////////////////////////////////////////
void CCharacterRenderNode::Physicalize(bool bInstant)
{
	// Not implemented
	assert(0);
}

//////////////////////////////////////////////////////////////////////////
float CCharacterRenderNode::GetMaxViewDist()
{
	if (GetRndFlags() & ERF_FORCE_POST_3D_RENDER)
	{
		// Always want to render models in post 3d render (menus), whatever distance they are
		return FLT_MAX;
	}

	if (GetRndFlags() & ERF_CUSTOM_VIEW_DIST_RATIO)
	{
		float s = max(max((m_boundsWorld.max.x - m_boundsWorld.min.x), (m_boundsWorld.max.y - m_boundsWorld.min.y)), (m_boundsWorld.max.z - m_boundsWorld.min.z));
		return max(GetCVars()->e_ViewDistMin, s * GetCVars()->e_ViewDistRatioCustom * GetViewDistRatioNormilized());
	}

	if (GetMinSpecFromRenderNodeFlags(m_dwRndFlags) == CONFIG_DETAIL_SPEC)
		return max(GetCVars()->e_ViewDistMin, min(GetFloatCVar(e_ViewDistCompMaxSize), m_boundsLocal.GetRadius()) * GetCVars()->e_ViewDistRatioDetail * GetViewDistRatioNormilized());

	return max(GetCVars()->e_ViewDistMin, min(GetFloatCVar(e_ViewDistCompMaxSize), m_boundsLocal.GetRadius()) * GetCVars()->e_ViewDistRatio * GetViewDistRatioNormilized());
}

//////////////////////////////////////////////////////////////////////////
CLodValue CCharacterRenderNode::ComputeLod(int wantedLod, const SRenderingPassInfo& passInfo)
{
	if (m_pCharacterInstance)
	{
		return m_pCharacterInstance->ComputeLod(wantedLod, passInfo);
	}

	return CLodValue(wantedLod);
}

//////////////////////////////////////////////////////////////////////////
bool CCharacterRenderNode::GetLodDistances(const SFrameLodInfo& frameLodInfo, float* distances) const
{
	const float fEntityLodRatio = GetLodRatioNormalized();
	if (fEntityLodRatio > 0.0f)
	{
		float fLodDistance = FLT_MAX;
		if (m_pCharacterInstance)
		{
			fLodDistance = sqrt(m_pCharacterInstance->ComputeGeometricMean().fGeometricMean);
		}
		const float fDistMultiplier = 1.0f / (fEntityLodRatio * frameLodInfo.fTargetSize);

		for (uint i = 0; i < SMeshLodInfo::s_nMaxLodCount; ++i)
		{
			distances[i] = fLodDistance * (i + 1) * fDistMultiplier;
		}
	}
	else
	{
		for (uint i = 0; i < SMeshLodInfo::s_nMaxLodCount; ++i)
		{
			distances[i] = FLT_MAX;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CCharacterRenderNode::OnRenderNodeBecomeVisible(const SRenderingPassInfo& passInfo)
{
	if (!passInfo.IsCachedShadowPass())
	{
		if (m_pCharacterInstance)
		{
			m_pCharacterInstance->SetFlags(m_pCharacterInstance->GetFlags() | CS_FLAG_RENDER_NODE_VISIBLE);
		}

		if (GetOwnerEntity() && (GetRndFlags() & ERF_ENABLE_ENTITY_RENDER_CALLBACK))
		{
			// When render node becomes visible notify our owner render node that it is now visible.
			GetOwnerEntity()->OnRenderNodeVisibilityChange(true);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CCharacterRenderNode::OnRenderNodeBecomeInvisible()
{
	if (m_pCharacterInstance)
	{
		m_pCharacterInstance->SetFlags( m_pCharacterInstance->GetFlags() & ~CS_FLAG_RENDER_NODE_VISIBLE );
		m_pCharacterInstance->KillAllSkeletonEffects();
	}
	if (GetOwnerEntity() && (GetRndFlags() & ERF_ENABLE_ENTITY_RENDER_CALLBACK))
	{
		// When render node becomes invisible notify our owner render node that it is now invisible.
		GetOwnerEntity()->OnRenderNodeVisibilityChange(false);
	}
}

//////////////////////////////////////////////////////////////////////////
void CCharacterRenderNode::SetCameraSpacePos(Vec3* pCameraSpacePos)
{
	if (pCameraSpacePos)
	{
		if (!m_pCameraSpacePos)
			m_pCameraSpacePos = new Vec3;
		*m_pCameraSpacePos = *pCameraSpacePos;
	}
	else
	{
		delete m_pCameraSpacePos;
		m_pCameraSpacePos = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////
void CCharacterRenderNode::SetCharacter(ICharacterInstance* pCharacter)
{
	if (m_pCharacterInstance != pCharacter)
	{
		m_pCharacterInstance = pCharacter;
	}
}

void CCharacterRenderNode::SetCharacterRenderOffset(const QuatTS& renderOffset)
{
	m_renderOffset = renderOffset;
}

//////////////////////////////////////////////////////////////////////////
void CCharacterRenderNode::OffsetPosition(const Vec3& delta)
{
	if (m_pTempData)
		m_pTempData->OffsetPosition(delta);
	m_matrix.SetTranslation(m_matrix.GetTranslation() + delta);
	m_boundsWorld.Move(delta);
}

//////////////////////////////////////////////////////////////////////////
void CCharacterRenderNode::CalcNearestTransform(Matrix34 &transformMatrix, const SRenderingPassInfo& passInfo)
{
	// Camera space
	if (m_pCameraSpacePos)
	{
		// Use camera space relative position
		const Matrix33 cameraRotation = Matrix33(passInfo.GetCamera().GetViewMatrix());
		transformMatrix.SetTranslation(*m_pCameraSpacePos * cameraRotation);
	}
	else
	{
		// We don't have camera space relative position, so calculate it out from world space
		// (This will not have the precision advantages of camera space rendering)
		transformMatrix.AddTranslation(-passInfo.GetCamera().GetPosition());
	}
}
