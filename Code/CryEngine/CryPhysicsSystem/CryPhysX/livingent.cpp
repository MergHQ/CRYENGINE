#include "StdAfx.h"

#include "geometries.h"
#include "entities.h"
#include "world.h"

#include "CryPhysX.h"

using namespace primitives;
using namespace cpx::Helper;


int PhysXLiving::SetParams(pe_params *_params, int bThreadSafe)
{
	if (_params->type == pe_player_dimensions::type_id && m_type == PE_LIVING) {
		pe_player_dimensions *params = (pe_player_dimensions*)_params;
		if (!is_unused(params->sizeCollider) || !m_parts.size()) {
			PxRigidDynamic *pRD = m_actor->isRigidDynamic();
			pe_geomparams gp;
			gp.pos.z = m_parts.size() ? m_parts[0].shape->getLocalPose().p.z : 0.9f;
			Vec3 size = !is_unused(params->sizeCollider) ? params->sizeCollider : Vec3(0.4f, 0.4f, 0.5f);
			float mass = pRD->getMass() ? pRD->getMass() : 80.0f;
			gp.density = m_parts.size() ? m_parts[0].density : 1;
			RemoveGeometry(100);
			if (!is_unused(params->heightCollider)) gp.pos.z = params->heightCollider;
			capsule caps;
			caps.axis = Vec3(0, 0, 1);
			caps.center.zero();
			caps.r = size.x;
			caps.hh = size.z;
			float dh = max(0.0f, gp.pos.z - caps.hh - caps.r)*0.5f;
			gp.pos.z -= dh; caps.hh += dh;
			phys_geometry *pgeom = g_pPhysWorld->RegisterGeometry(g_pPhysWorld->CreatePrimitive(capsule::type, &caps), NSURFACETYPES - 2);
			pgeom->pGeom->Release();
			gp.flags &= ~geom_colltype_ray;
			AddGeometry(pgeom, &gp, 100); pgeom->nRefCount--;
			pRD->setMass(mass);
			pRD->setCMassLocalPose(T(QuatT(Quat(IDENTITY), Vec3(0, 0, gp.pos.z))));
			pRD->setMassSpaceInertiaTensor(V(Vec3(0)));
		}
		return 1;
	}

	if (_params->type == pe_player_dynamics::type_id && m_type == PE_LIVING) {
		pe_player_dynamics *params = (pe_player_dynamics*)_params;
		PxRigidDynamic *pRD = m_actor->isRigidDynamic();
		if (!is_unused(params->mass))	pRD->setMass(params->mass);
		if (!is_unused(params->kInertia)) { pRD->setLinearDamping(params->kInertia ? params->kInertia : 5.0f); }
		else { pRD->setLinearDamping(5.0); }
		if (!is_unused(params->bActive)) pRD->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, !params->bActive);
		return 1;
	}

	return PhysXEnt::SetParams(_params, bThreadSafe);
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


int PhysXLiving::GetStatus(pe_status* _status) const
{
	if (_status->type == pe_status_living::type_id) {
		pe_status_living *status = (pe_status_living*)_status;

		if (m_actor->getScene()) {
			PxRigidDynamic *pRD = m_actor->isRigidDynamic();

			// set some initial default values - similar to CryPhysics (see livingentity.cpp)
			status->bFlying = 0;
			status->timeFlying = 0;
			status->camOffset = Vec3(0, 0, 0);
			status->vel = V(pRD->getLinearVelocity());
			status->velUnconstrained = Vec3(0);
			status->velRequested = Vec3(0);
			status->velGround = Vec3(0);
			status->groundHeight = 1;
			status->groundSlope = Vec3(0, 0, 1);
			status->groundSurfaceIdx = 0;
			status->groundSurfaceIdxAux = 0;
			status->pGroundCollider = nullptr;
			status->iGroundColliderPart = 0;
			status->timeSinceStanceChange = 0;
			status->bStuck = 0;
			status->pLockStep = nullptr;
			status->bSquashed = false;

			// simple, exemplary implementation of determining bFlying
			status->bFlying = 0;

			int nShapes = pRD->getNbShapes();
			PxShape **shapes = (PxShape**)alloca(nShapes * sizeof(void*));
			pRD->getShapes(shapes, nShapes);
			PxCapsuleGeometry caps;
			if (nShapes > 0 && shapes[0]->getCapsuleGeometry(caps))
			{
				status->bFlying = 1;
				PxOverlapBuffer hit;
				PxTransform M(V(this->getGlobalPose().t), Q(this->getGlobalPose().q));
				const float eps = -0.5f; // shift capsule down for floor collision
				M = PxTransform(0, 0, eps) * M;

				if ((cpx::g_cryPhysX.Scene()->overlap(caps, M, hit)))
				{
					if ((hit.getNbAnyHits() > 0) || (hit.getNbTouches() > 0))
					{
						status->bFlying = 0;
					}
				}
			}
		}
		return 1;
	}

	return PhysXEnt::GetStatus(_status);
}

int PhysXLiving::Action(pe_action* _action, int bThreadSafe)
{
	if (_action->type == pe_action_move::type_id) {
		pe_action_move *action = (pe_action_move*)_action;
		if (!is_unused(action->dir) && m_actor->getScene()) {
			PxRigidDynamic *pRD = m_actor->isRigidDynamic();
			if (action->iJump == 1) pRD->clearForce();
			pRD->addForce(V(action->dir*(pRD->getLinearDamping()*pRD->getMass())));
		}
		return 1;
	}
	return PhysXEnt::Action(_action, bThreadSafe);
}

void PhysXLiving::PostStep(float dt, int immediate)
{
	if (immediate) {
		PxTransform pose = m_actor->getGlobalPose();
		if ((!m_qRot*Q(pose.q)).v.len2() > 0.001f)
			m_actor->setGlobalPose(PxTransform(pose.p, Q(m_qRot)));
	}
	PhysXEnt::PostStep(dt, immediate);
}
