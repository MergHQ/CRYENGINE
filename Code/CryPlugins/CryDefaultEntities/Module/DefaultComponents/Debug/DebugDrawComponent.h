#pragma once

#include <CryRenderer/IRenderer.h>
#include <CryRenderer/IShader.h>

#include <CrySchematyc/ResourceTypes.h>
#include <CrySchematyc/MathTypes.h>

namespace Cry
{
	namespace DefaultComponents
	{
		class CDebugDrawComponent
			: public IEntityComponent
		{
		public:
			virtual ~CDebugDrawComponent() {}

			// IEntityComponent
			virtual void Run(Schematyc::ESimulationMode simulationMode) override;

			virtual void ProcessEvent(SEntityEvent& event) override;
			virtual uint64 GetEventMask() const override;
			// ~IEntityComponent

			static void ReflectType(Schematyc::CTypeDesc<CDebugDrawComponent>& desc);

			static CryGUID& IID()
			{
				static CryGUID id = "{9C255A6C-B3E2-4AF2-BDB4-9ADCCC9E07D3}"_cry_guid;
				return id;
			}

			void DrawSphere(const Vec3& pos, float radius, const ColorF& color, float duration);

			void DrawDirection(const Vec3& pos, float radius, const Vec3& dir, const ColorF& color, float duration);

			void DrawLine(const Vec3& posStart, const Vec3& posEnd, const ColorF& color, float duration);

			void DrawPlanarDisc(const Vec3& pos, float innerRadius, float outerRadius, const ColorF& color, float duration);

			void DrawCone(const Vec3& pos, const Vec3& dir, float baseRadius, float height, const ColorF& color, float duration);

			void DrawCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorF& color, float duration);

			void Draw2DText(Schematyc::CSharedString text, float size, const ColorF& color, float duration);

			void DrawText(Schematyc::CSharedString text, float x, float y, float size, const ColorF& color, float duration);

			void DrawText3D(Schematyc::CSharedString text, const Vec3& pos, float size, const ColorF& color, float duration);

			void Draw2DLine(float x1, float y1, float x2, float y2, const ColorF& color, float duration);

			bool IsPersistentTextEnabled() const { return m_bDrawPersistent; }
			void EnablePersistentText(bool bEnable) { m_bDrawPersistent = bEnable; }

			void SetPersistentTextColor(const ColorF& color) { m_persistentTextColor = color; }
			const ColorF& GetPersistentTextColor() const { return m_persistentTextColor; }

			void SetPersistentTextViewDistance(float viewDistance) { m_persistentViewDistance = viewDistance; }
			float GetPersistentTextViewDistance() const { return m_persistentViewDistance; }

			void SetPersistentTextFontSize(float fontSize) { m_persistentFontSize = fontSize; }
			float GetPersistentTextFontSize() const { return m_persistentFontSize; }

			virtual void SetPersistentText(const char* szText);
			const char* GetPersistentText() const { return m_persistentText.c_str(); }

		protected:
			bool m_bDrawPersistent = false;
			ColorF m_persistentTextColor = ColorF(1, 1, 1);
			Schematyc::Range<0, 255> m_persistentViewDistance = 100.f;
			Schematyc::Range<0, 100> m_persistentFontSize = 1.2f;
			
			Schematyc::CSharedString m_persistentText;
		};
	}
}