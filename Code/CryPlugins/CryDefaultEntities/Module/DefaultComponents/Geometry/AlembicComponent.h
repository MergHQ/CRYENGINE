#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>

namespace Cry
{
	namespace DefaultComponents
	{
		class CAlembicComponent
			: public IEntityComponent
		{
			// IEntityComponent
			virtual void   Initialize() final;

			virtual void   ProcessEvent(SEntityEvent& event) final;
			virtual uint64 GetEventMask() const final;
			// ~IEntityComponent

		public:
			CAlembicComponent() {}
			virtual ~CAlembicComponent() {}

			static void ReflectType(Schematyc::CTypeDesc<CAlembicComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{32B8795F-C9A7-4162-B71F-E514C65093F6}"_cry_guid;
				return id;
			}

			virtual void Enable(bool bEnable);
			virtual void SetLooping(bool bLooping);
			virtual bool IsLooping() const;

			virtual void SetPlaybackTime(float time);
			virtual float GetPlaybackTime() const;

			virtual void SetFilePath(const char* szFilePath);
			const char* GetFilePath() const { return m_filePath.value.c_str(); }

		protected:
			Schematyc::GeomCacheFileName m_filePath;
		};
	}
}