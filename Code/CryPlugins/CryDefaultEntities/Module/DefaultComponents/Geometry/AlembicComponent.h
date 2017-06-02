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
		public:
			CAlembicComponent() {}
			virtual ~CAlembicComponent() {}

			// IEntityComponent
			virtual void Run(Schematyc::ESimulationMode simulationMode) override;
			// ~IEntityComponent

			static void ReflectType(Schematyc::CTypeDesc<CAlembicComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{32B8795F-C9A7-4162-B71F-E514C65093F6}"_cry_guid;
				return id;
			}

			void Enable(bool bEnable);
			void SetLooping(bool bLooping);
			bool IsLooping() const;

			void SetPlaybackTime(float time);
			float GetPlaybackTime() const;

			virtual void SetFilePath(const char* szFilePath);
			const char* GetFilePath() const { return m_filePath.value.c_str(); }

		protected:
			Schematyc::GeomCacheFileName m_filePath;
		};
	}
}