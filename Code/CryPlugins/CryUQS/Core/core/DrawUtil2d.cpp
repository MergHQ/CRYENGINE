// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DrawUtil2d.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		const float CDrawUtil2d::s_rowSize = 11.0f;
		const float CDrawUtil2d::s_fontSize = 1.3f;
		const float CDrawUtil2d::s_indentSize = 8.0f;
		const SAuxGeomRenderFlags CDrawUtil2d::s_flags2d(e_Mode2D | e_AlphaBlended | e_FillModeSolid | e_CullModeNone | e_DepthWriteOff | e_DepthTestOff);

		void CDrawUtil2d::DrawLabel(int row, const ColorF& color, const char* szFormat, ...)
		{
			va_list args;
			va_start(args, szFormat);
			DoDrawLabel(0.0f, (float)row * s_rowSize, s_fontSize, color, szFormat, args);
			va_end(args);
		}

		void CDrawUtil2d::DrawLabel(float xPos, int row, const ColorF& color, const char* szFormat, ...)
		{
			va_list args;
			va_start(args, szFormat);
			DoDrawLabel(xPos, (float)row * s_rowSize, s_fontSize, color, szFormat, args);
			va_end(args);
		}

		void CDrawUtil2d::DrawLabel(float xPos, float yPos, float fontSize, const ColorF& color, const char* szFormat, ...)
		{
			va_list args;
			va_start(args, szFormat);
			DoDrawLabel(xPos, yPos, fontSize, color, szFormat, args);
			va_end(args);
		}

		float CDrawUtil2d::GetRowSize()
		{
			return s_rowSize;
		}

		float CDrawUtil2d::GetIndentSize()
		{
			return s_indentSize;
		}

		void CDrawUtil2d::DrawFilledQuad(float x1, float y1, float x2, float y2, const ColorF& color)
		{
			IRenderer* pRenderer = gEnv->pRenderer;
			if (!pRenderer)
				return;

			IRenderAuxGeom* pRenderAuxGeom = pRenderer->GetIRenderAuxGeom();
			if (!pRenderAuxGeom)
				return;

			pRenderAuxGeom->SetRenderFlags(s_flags2d);

			const Vec3 v0(x1, y1, 0.0f);
			const Vec3 v1(x2, y1, 0.0f);
			const Vec3 v2(x2, y2, 0.0f);
			const Vec3 v3(x1, y2, 0.0f);

			pRenderAuxGeom->DrawQuad(v0, color, v1, color, v2, color, v3, color);
		}

		void CDrawUtil2d::DoDrawLabel(float xPos, float yPos, float fontSize, const ColorF& color, const char* szFormat, va_list args)
		{
			IRenderer* pRenderer = gEnv->pRenderer;
			if (!pRenderer)
				return;

			IRenderAuxGeom* pRenderAuxGeom = pRenderer->GetIRenderAuxGeom();
			if (!pRenderAuxGeom)
				return;

			pRenderAuxGeom->SetRenderFlags(s_flags2d);

			SDrawTextInfo ti;
			ti.scale.set(fontSize, fontSize);
			ti.flags = eDrawText_IgnoreOverscan | eDrawText_2D | eDrawText_800x600 | eDrawText_FixedSize | eDrawText_Monospace;
			ti.color[0] = color.r;
			ti.color[1] = color.g;
			ti.color[2] = color.b;
			ti.color[3] = color.a;

			stack_string text;
			text.FormatV(szFormat, args);

			pRenderAuxGeom->RenderTextQueued(Vec3(xPos, yPos, 0.5f), ti, text.c_str());
		}

	}
}
