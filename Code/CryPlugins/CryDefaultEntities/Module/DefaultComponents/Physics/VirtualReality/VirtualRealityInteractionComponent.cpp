#include "StdAfx.h"
#include "VirtualRealityInteractionComponent.h"

#include <Cry3DEngine/IIndexedMesh.h>

namespace Cry
{
	namespace DefaultComponents
	{
		namespace VirtualReality
		{
			void CInteractionComponent::Register(Schematyc::CEnvRegistrationScope& componentScope)
			{
			}

			void CInteractionComponent::Initialize()
			{
				if (IHmdDevice* pDevice = gEnv->pSystem->GetHmdManager()->GetHmdDevice())
				{
					if (pDevice->GetClass() == eHmdClass_Oculus)
					{
						m_hands[static_cast<uint8>(EHand::Left)].controllerId = eHmdController_OculusLeftHand;
						m_hands[static_cast<uint8>(EHand::Right)].controllerId = eHmdController_OculusRightHand;
					}
					else if (pDevice->GetClass() == eHmdClass_OpenVR)
					{
						m_hands[static_cast<uint8>(EHand::Left)].controllerId = eHmdController_OpenVR_1;
						m_hands[static_cast<uint8>(EHand::Right)].controllerId = eHmdController_OpenVR_2;
					}
					else
					{
						CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Unsupported HMD device encountered in interaction component!");
						return;
					}

					LoadHandModel(m_leftHandModelPath.value, EHand::Left);
					LoadHandModel(m_rightHandModelPath.value, EHand::Right);
				}
			}

			void CInteractionComponent::LoadHandModel(const char* szModelPath, EHand handType)
			{
				// Characters not currently supported since we need the physics geometry
				/*if (IsCharacterFile(szModelPath))
				{
					m_hands[static_cast<uint8>(hand)].slotId = m_pEntity->LoadCharacter(-1, szModelPath);
				}*/

				SHand& hand = m_hands[static_cast<uint8>(handType)];
				hand.slotId = m_pEntity->LoadGeometry(-1, szModelPath, nullptr, IEntity::EF_NO_STREAMING);

				if (IStatObj* pHandObject = m_pEntity->GetStatObj(hand.slotId))
				{
					pHandObject->Invalidate();
					if (IIndexedMesh* pIndexedMesh = pHandObject->GetIndexedMesh(true))
					{
						CMesh* pMesh = pIndexedMesh->GetMesh();

						int flags = mesh_multicontact1 | mesh_OBB | mesh_AABB | mesh_approx_box | mesh_approx_capsule;

						hand.pPhysicsGeometry = gEnv->pPhysicalWorld->GetGeomManager()->CreateMesh(
							pMesh->GetStreamPtr<Vec3>(CMesh::POSITIONS),
							pMesh->GetStreamPtr<vtx_idx>(CMesh::INDICES),
							nullptr,
							0,
							pIndexedMesh->GetIndexCount() / 3,
							flags);
					}
				}
			}

			void CInteractionComponent::ProcessEvent(SEntityEvent& event)
			{
				if (event.event == ENTITY_EVENT_UPDATE)
				{
					IHmdDevice* pDevice = gEnv->pSystem->GetHmdManager()->GetHmdDevice();
					if (pDevice == nullptr)
						return;

					SEntityUpdateContext* pUpdateContext = reinterpret_cast<SEntityUpdateContext*>(event.nParam[0]);

					for (SHand& hand : m_hands)
					{
						EKeyId triggerKeyId;
						EKeyId releaseHeldItemKeyId;

						if (pDevice->GetClass() == eHmdClass_Oculus)
						{
							triggerKeyId = static_cast<EKeyId>(hand.controllerId + eKI_Motion_OculusTouch_L1);
							releaseHeldItemKeyId = eKI_Motion_OculusTouch_A;
						}
						else if (pDevice->GetClass() == eHmdClass_OpenVR)
						{
							triggerKeyId = eKI_Motion_OpenVR_Trigger;
							releaseHeldItemKeyId = eKI_Motion_OpenVR_TouchPadBtn;
						}

						Vec3 linearVelocity;
						Vec3 angularVelocity;

						const HmdTrackingState& trackingState = pDevice->GetController()->GetLocalTrackingState(hand.controllerId);
						if (trackingState.CheckStatusFlags(EHmdStatus::eHmdStatus_OrientationTracked))
						{
							Matrix34 localControllerTransform = Matrix34::Create(Vec3(1.f), trackingState.pose.orientation, trackingState.pose.position);
							localControllerTransform.m23 = trackingState.pose.position.z;

							m_pEntity->SetSlotLocalTM(hand.slotId, localControllerTransform);

							const Matrix34& worldTransform = m_pEntity->GetWorldTM();

							linearVelocity = worldTransform.TransformVector(trackingState.pose.linearVelocity);
							angularVelocity = worldTransform.TransformVector(trackingState.pose.angularVelocity);
						}
						else
						{
							linearVelocity = ZERO;
							angularVelocity = ZERO;
						}

						constexpr float triggerHoldThreshold = 0.95f;
						bool isHoldingTrigger = pDevice->GetController()->GetTriggerValue(hand.controllerId, triggerKeyId) > triggerHoldThreshold;

						if (!isHoldingTrigger && hand.m_holdingTrigger)
						{
							hand.m_holdingTrigger = false;
						}

						if (isHoldingTrigger && !hand.m_holdingTrigger && hand.heldEntityId == INVALID_ENTITYID)
						{
							hand.m_holdingTrigger = true;

							if (IEntity* pUsableEntity = FindPickableEntity(hand))
							{
								PickUpObject(hand, pUsableEntity);
							}
						}
						else if (pDevice->GetController()->IsButtonPressed(hand.controllerId, releaseHeldItemKeyId))
						{
							ReleaseObject(hand, linearVelocity, angularVelocity);
						}
						else if (hand.heldEntityId != INVALID_ENTITYID)
						{
							if (IEntity* pHeldEntity = gEnv->pEntitySystem->GetEntity(hand.heldEntityId))
							{
								const Matrix34& handTransform = m_pEntity->GetSlotWorldTM(hand.slotId);
								pHeldEntity->SetWorldTM(handTransform);
							}
						}
					}
				}
			}

			uint64 CInteractionComponent::GetEventMask() const
			{
				return BIT64(ENTITY_EVENT_UPDATE);
			}

			void CInteractionComponent::ReleaseObject(SHand& hand, const Vec3& linearVelocity, const Vec3& angularVelocity)
			{
				if (IEntity* pHeldEntity = gEnv->pEntitySystem->GetEntity(hand.heldEntityId))
				{
					pHeldEntity->EnablePhysics(true);

					if (IPhysicalEntity* pPhysicalEntity = pHeldEntity->GetPhysicalEntity())
					{
						Matrix34 tm = pHeldEntity->GetWorldTM();
						pe_params_pos posParams;
						posParams.pMtx3x4 = &tm;
						pPhysicalEntity->SetParams(&posParams);

						pe_action_awake awake;
						awake.bAwake = 1;
						pPhysicalEntity->Action(&awake);

						pe_action_set_velocity setVel;
						setVel.v = linearVelocity;
						setVel.w = angularVelocity;
						pPhysicalEntity->Action(&setVel);
					}

					DynArray<IInteractableComponent*> usableEntityComponents;
					pHeldEntity->GetAllComponents<IInteractableComponent>(usableEntityComponents);
					for (IInteractableComponent* pUsableEntityComponent : usableEntityComponents)
					{
						pUsableEntityComponent->OnReleased();
					}
				}

				// Re-enable render of the default hand slot
				m_pEntity->SetSlotFlags(hand.slotId, ENTITY_SLOT_RENDER);
				hand.heldEntityId = INVALID_ENTITYID;
			}

			void CInteractionComponent::PickUpObject(SHand& hand, IEntity* pObjectEntity)
			{
				// Disable render of the held item
				m_pEntity->SetSlotFlags(hand.slotId, 0);
				hand.heldEntityId = pObjectEntity->GetId();

				// Temporarily disable physics on the held item
				pObjectEntity->EnablePhysics(false);

				DynArray<IInteractableComponent*> usableEntityComponents;
				pObjectEntity->GetAllComponents<IInteractableComponent>(usableEntityComponents);
				for (IInteractableComponent* pUsableEntityComponent : usableEntityComponents)
				{
					pUsableEntityComponent->OnPickedUp(hand.controllerId);
				}
			}

			IEntity* CInteractionComponent::FindPickableEntity(const SHand& hand)
			{
				Matrix34 handTransformation = m_pEntity->GetSlotWorldTM(hand.slotId);
				IStatObj* pHandGeometry = m_pEntity->GetStatObj(hand.slotId);
				if (pHandGeometry == nullptr)
				{
					return nullptr;
				}

				AABB searchBox = pHandGeometry->GetAABB();
				searchBox = AABB::CreateTransformedAABB(handTransformation, searchBox);

				IPhysicalEntity** ppEntityList = nullptr;
				int entityCount = gEnv->pPhysicalWorld->GetEntitiesInBox(searchBox.min, searchBox.max, ppEntityList, ent_sleeping_rigid | ent_rigid);

				std::array<IPhysicalEntity*, 1> skipEntities = { { m_pEntity->GetPhysicalEntity() } };

				std::vector<IEntity*> usableEntities;
				usableEntities.resize(entityCount);

				// Remove invalid entities for an early out to avoid intersection
				for (int iEntity = 0; iEntity < entityCount;)
				{
					bool bValid = false;

					IEntity* pUsableEntity = gEnv->pEntitySystem->GetEntityFromPhysics(ppEntityList[iEntity]);

					if (pUsableEntity != nullptr)
					{
						if (ppEntityList[iEntity]->GetType() == PE_RIGID)
						{
							pe_status_dynamics dynStatus;
							if ((ppEntityList[iEntity]->GetStatus(&dynStatus) && dynStatus.mass > 0.f))
							{
								usableEntities[iEntity] = pUsableEntity;
							}
						}

						bValid = usableEntities[iEntity] != nullptr;
					}

					if (!bValid || stl::find(skipEntities, ppEntityList[iEntity]))
					{
						// Steal the last element
						if (iEntity + 1 < entityCount)
						{
							ppEntityList[iEntity] = ppEntityList[entityCount - 1];
						}

						entityCount--;
					}
					else
					{
						++iEntity;
					}
				}

				pe_status_nparts numParts;

				geom_world_data gwd[2];
				geom_contact* pContacts;

				intersection_params intersectionParams;
				intersectionParams.bSweepTest = 0;
				intersectionParams.time_interval = 1.f;
				intersectionParams.bKeepPrevContacts = 0;
				intersectionParams.bNoAreaContacts = 1;

				real bestContact = std::numeric_limits<real>::max();
				decltype(usableEntities)::iterator bestEntityIt = usableEntities.end();

				for (int iUsableEntity = 0; iUsableEntity < entityCount; ++iUsableEntity)
				{
					int numEntityParts = ppEntityList[iUsableEntity]->GetStatus(&numParts);
					for (int iUsableEntityPart = 0; iUsableEntityPart < numEntityParts; ++iUsableEntityPart)
					{
						pe_status_pos partStatus;
						partStatus.flags &= ~status_local;
						partStatus.ipart = iUsableEntityPart;
						ppEntityList[iUsableEntity]->GetStatus(&partStatus);

						// Hand coordinates
						gwd[0].offset = handTransformation.GetTranslation();
						gwd[0].v = ZERO;
						gwd[0].R = Matrix33(handTransformation);
						gwd[0].scale = handTransformation.GetUniformScale();

						// Liftable entity coordinates
						gwd[1].offset = partStatus.pos;
						gwd[1].R = Matrix33(partStatus.q);
						gwd[1].scale = partStatus.scale;

						int numContacts = hand.pPhysicsGeometry->Intersect(partStatus.pGeom, gwd, gwd + 1, &intersectionParams, pContacts);

						for (int iContact = 0; iContact < numContacts; ++iContact)
						{
							if (pContacts[iContact].t >= bestContact)
								continue;

							bestContact = pContacts[iContact].t;
							bestEntityIt = usableEntities.begin() + iUsableEntity;
						}
					}
				}

				return bestEntityIt != usableEntities.end() ? *bestEntityIt : nullptr;
			}
		}
	}
}