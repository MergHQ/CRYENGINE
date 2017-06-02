#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>

namespace Cry
{
	namespace DefaultComponents
	{
		class CAnimatedMeshComponent
			: public IEntityComponent
		{
		public:
			CAnimatedMeshComponent() {}
			virtual ~CAnimatedMeshComponent() {}

			// IEntityComponent
			virtual void Initialize() override
			{
				if (m_filePath.value.size() > 0)
				{
					m_pEntity->LoadCharacter(GetOrMakeEntitySlotId(), m_filePath.value);

					if (m_defaultAnimation.value.size() > 0)
					{
						PlayAnimation(m_defaultAnimation, m_bLoopDefaultAnimation);
					}
				}
			}

			virtual void Run(Schematyc::ESimulationMode simulationMode) override;
			// ~IEntityComponent

			static void ReflectType(Schematyc::CTypeDesc<CAnimatedMeshComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{5F543092-53EA-46D7-9376-266E778317D7}"_cry_guid;
				return id;
			}

			void PlayAnimation(Schematyc::LowLevelAnimationName name, bool bLoop = false)
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

			void SetPlaybackSpeed(float multiplier) { m_animationParams.m_fPlaybackSpeed = multiplier; }
			void SetPlaybackWeight(float weight) { m_animationParams.m_fPlaybackWeight = weight; }
			void SetLayer(int layer) { m_animationParams.m_nLayerID = layer; }

			virtual void SetCharacterFile(const char* szPath);
			const char* SetCharacterFile() const { return m_filePath.value.c_str(); }

			virtual void SetDefaultAnimationName(const char* szPath);
			const char* GetDefaultAnimationName() const { return m_defaultAnimation.value.c_str(); }
			void SetDefaultAnimationLooped(bool bLooped) { m_bLoopDefaultAnimation = bLooped; }
			bool IsDefaultAnimationLooped() const { return m_bLoopDefaultAnimation; }

		protected:
			CryCharAnimationParams m_animationParams;

			Schematyc::CharacterFileName m_filePath;

			Schematyc::LowLevelAnimationName m_defaultAnimation;
			bool m_bLoopDefaultAnimation = false;
		};
	}
}