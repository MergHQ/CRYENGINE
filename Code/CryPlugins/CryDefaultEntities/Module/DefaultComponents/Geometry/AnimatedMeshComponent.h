#pragma once

#include "BaseMeshComponent.h"
#include <CrySerialization/StringList.h>
#include <CrySerialization/IArchive.h>
#include <CryAnimation/ICryAnimation.h>
#include <CrySchematyc/Utils/SharedString.h>

class CPlugin_CryDefaultEntities;

struct IAnimationSet;

namespace Cry
{
	namespace DefaultComponents
	{
		struct SAnimationSelector
		{
			string animationString = "";
			bool isUsingResourcePicker = true;
			Serialization::StringList animationNamesList;
			void FillAnimationList(const IAnimationSet* pAnimSet);
			bool operator==(const SAnimationSelector& other) const;
		};

		inline bool Serialize(
			Serialization::IArchive& ar,
			SAnimationSelector& animSel,
			const char* szName,
			const char* szLabel);


		class CAnimatedMeshComponent
			: public CBaseMeshComponent
		{
		protected:
			friend CPlugin_CryDefaultEntities;
			static void Register(Schematyc::CEnvRegistrationScope& componentScope);

			// IEntityComponent
			virtual void   Initialize() final;

			virtual void   ProcessEvent(const SEntityEvent& event) final;
			// ~IEntityComponent

			// IEditorEntityComponent
			virtual bool SetMaterial(int slotId, const char* szMaterial) override;
			// ~IEditorEntityComponent

		public:
			CAnimatedMeshComponent() {}
			virtual ~CAnimatedMeshComponent() {}

			static void ReflectType(Schematyc::CTypeDesc<CAnimatedMeshComponent>& desc)
			{
				desc.SetGUID("{5F543092-53EA-46D7-9376-266E778317D7}"_cry_guid);
				desc.SetEditorCategory("Geometry");
				desc.SetLabel("Animated Mesh");
				desc.SetDescription("A component containing a simple mesh that can be animated");
				desc.SetComponentFlags({ IEntityComponent::EFlags::Transform, IEntityComponent::EFlags::Socket, IEntityComponent::EFlags::Attach });

				desc.AddBase<IEditorEntityComponent>();

				desc.AddMember(&CAnimatedMeshComponent::m_type, 'type', "Type", "Type", "Determines the behavior of the static mesh", EMeshType::RenderAndCollider);

				desc.AddMember(&CAnimatedMeshComponent::m_filePath, 'file', "FilePath", "File", "Determines the animated mesh to load", "");
				desc.AddMember(&CAnimatedMeshComponent::m_materialPath, 'mat', "Material", "Material", "Specifies the override material for the selected object", "");
				desc.AddMember(&CAnimatedMeshComponent::m_renderParameters, 'rend', "Render", "Rendering Settings", "Settings for the rendered representation of the component", SRenderParameters());

				desc.AddMember(&CAnimatedMeshComponent::m_bIsUsingRawAnimationName, 'raw', "IsUsingRawAnimationName", "Use Raw Animation Name", "Specifies whether the default animation is specified as a file or a raw animation name", false);
				desc.AddMember(&CAnimatedMeshComponent::m_defaultAnimation, 'anim', "Animation", "Default Animation", "Specifies the animation we want to play by default", SAnimationSelector());
				desc.AddMember(&CAnimatedMeshComponent::m_bLoopDefaultAnimation, 'loop', "Loop", "Loop Default", "Whether or not to loop the default animation", false);
				desc.AddMember(&CAnimatedMeshComponent::m_defaultAnimationSpeed, 'sped', "AnimSpeed", "Default Animation Speed", "Speed at which to play the default animation", 1.0f);
				
				desc.AddMember(&CAnimatedMeshComponent::m_physics, 'phys', "Physics", "Physics", "Physical properties for the object, only used if a simple physics or character controller is applied to the entity.", SPhysicsParameters());
			}

			virtual void PlayAnimation(Schematyc::AnyLowLevelAnimationName filename, bool bLoop = false)
			{
				if (m_pCachedCharacter)
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

					if (int animId = FindAnimId(filename.value))
					{
						m_pCachedCharacter->GetISkeletonAnim()->StartAnimationById(animId, animParams);
					}
				}
			}

			virtual void PlayAnimationByName(Schematyc::CSharedString name, bool bLoop = false)
			{
				if (m_pCachedCharacter)
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

					m_pCachedCharacter->GetISkeletonAnim()->StartAnimation(name.c_str(), animParams);
				}
			}

			virtual void SetPlaybackSpeed(float multiplier) { m_animationParams.m_fPlaybackSpeed = multiplier; }
			virtual void SetPlaybackWeight(float weight) { m_animationParams.m_fPlaybackWeight = weight; }
			virtual void SetLayer(int layer) { m_animationParams.m_nLayerID = layer; }

			virtual void SetCharacterFile(const char* szPath);
			const char* GetCharacterFile() const { return m_filePath.value.c_str(); }

			virtual void SetIsUsingRawAnimationName(bool bIsUsingRawAnimationName);
			bool IsUsingRawAnimationName() { return m_bIsUsingRawAnimationName; }
			virtual void SetDefaultAnimationName(const char* szPath);
			const char* GetDefaultAnimationName() const { return m_defaultAnimation.animationString.c_str(); }
			virtual void SetDefaultAnimationLooped(bool bLooped) { m_bLoopDefaultAnimation = bLooped; }
			bool IsDefaultAnimationLooped() const { return m_bLoopDefaultAnimation; }

			// Loads character and mannequin data from disk
			virtual void LoadFromDisk();
			// Applies the character to the entity
			virtual void ResetObject();

			// Helper to allow exposing derived function to Schematyc
			virtual void SetMeshType(EMeshType type) { SetType(type); }

		protected:

			virtual int FindAnimId(const char* szFilepath);

			CryCharAnimationParams m_animationParams;

			Schematyc::CharacterFileName m_filePath;
			Schematyc::MaterialFileName m_materialPath;

			bool m_bIsUsingRawAnimationName = false;
			SAnimationSelector m_defaultAnimation;
			bool m_bLoopDefaultAnimation;

			Schematyc::PositiveFloat m_defaultAnimationSpeed = 1.f;

			_smart_ptr<ICharacterInstance> m_pCachedCharacter = nullptr;
		};


		inline bool SAnimationSelector::operator==(const SAnimationSelector& other) const
		{
			return (isUsingResourcePicker == other.isUsingResourcePicker && animationString == other.animationString);
		}

		inline void ReflectType(Schematyc::CTypeDesc<SAnimationSelector>& desc)
		{
			desc.SetGUID("{C7E40D87-39CB-4AFC-94A5-803A405956E0}"_cry_guid);
			desc.SetLabel("Animation file or name");
			desc.SetDescription("Animation file or name");
		}

		inline bool Serialize(
			Serialization::IArchive& ar,
			SAnimationSelector& animSel,
			const char* szName,
			const char* szLabel)
		{
			if (ar.isEdit())
			{
				if (animSel.isUsingResourcePicker)
				{
					return ar(Serialization::AnyAnimationPath(animSel.animationString), szName, szLabel);
				}
				else
				{
					Serialization::StringListValue stringListValue(animSel.animationNamesList, animSel.animationString.c_str());
					bool result = ar(stringListValue, szName, szLabel);
					animSel.animationString = stringListValue.c_str();
					return result;
				}
			}
			else
			{
				return ar(animSel.animationString, szName, szLabel);
			}
		}

		inline void SAnimationSelector::FillAnimationList(const IAnimationSet* pAnimSet)
		{
			animationNamesList.clear();
			if (pAnimSet)
			{
				for (int i = 0; i < pAnimSet->GetAnimationCount(); ++i)
				{
					animationNamesList.push_back(pAnimSet->GetNameByAnimID(i));
				}
			}
		}

		static_assert(Serialization::IsSerializeable<SAnimationSelector>(), "SAnimationSelector is not serializable!");
	}
}
