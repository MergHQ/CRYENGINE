#pragma once

#include "BaseMeshComponent.h"

class CPlugin_CryDefaultEntities;

namespace Cry
{
	namespace DefaultComponents
	{
		class CAnimatedMeshComponent
			: public CBaseMeshComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void   Initialize() final;

			virtual void   ProcessEvent(SEntityEvent& event) final;
			// ~IEntityComponent

		public:
			CAnimatedMeshComponent() {}
			virtual ~CAnimatedMeshComponent() {}

			static void ReflectType(Schematyc::CTypeDesc<CAnimatedMeshComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{5F543092-53EA-46D7-9376-266E778317D7}"_cry_guid;
				return id;
			}

			virtual void PlayAnimation(Schematyc::LowLevelAnimationName name, bool bLoop = false)
			{
				if (ICharacterInstance* pCharacter = m_pEntity->GetCharacter(GetEntitySlotId()))
				{
					CryCharAnimationParams animParams = m_animationParams;
					if (bLoop)
					{
						animParams.m_nFlags |= CA_LOOP_ANIMATION;
					}
					else
					{
						animParams.m_nFlags &= ~CA_LOOP_ANIMATION;
					}

					string animationName = PathUtil::GetFileName(name.value);
					pCharacter->GetISkeletonAnim()->StartAnimation(animationName, animParams);
				}
			}

			virtual void SetPlaybackSpeed(float multiplier) { m_animationParams.m_fPlaybackSpeed = multiplier; }
			virtual void SetPlaybackWeight(float weight) { m_animationParams.m_fPlaybackWeight = weight; }
			virtual void SetLayer(int layer) { m_animationParams.m_nLayerID = layer; }

			virtual void SetCharacterFile(const char* szPath);
			const char* SetCharacterFile() const { return m_filePath.value.c_str(); }

			virtual void SetDefaultAnimationName(const char* szPath);
			const char* GetDefaultAnimationName() const { return m_defaultAnimation.value.c_str(); }
			virtual void SetDefaultAnimationLooped(bool bLooped) { m_bLoopDefaultAnimation = bLooped; }
			bool IsDefaultAnimationLooped() const { return m_bLoopDefaultAnimation; }

			// Loads character and mannequin data from disk
			virtual void LoadFromDisk();
			// Applies the character to the entity
			virtual void ResetObject();

		protected:
			CryCharAnimationParams m_animationParams;

			Schematyc::CharacterFileName m_filePath;

			Schematyc::LowLevelAnimationName m_defaultAnimation;
			bool m_bLoopDefaultAnimation = false;

			_smart_ptr<ICharacterInstance> m_pCachedCharacter = nullptr;
		};
	}
}