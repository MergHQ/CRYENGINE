// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "DrawUtil2d.h"

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace uqs
{
	namespace core
	{

		const float CDrawUtil2d::s_rowSize = 11.0f;
		const float CDrawUtil2d::s_fontSize = 1.3f;
		const float CDrawUtil2d::s_indentSize = 8.0f;

		void CDrawUtil2d::DrawLabel(int row, const ColorF& color, const char* fmt, ...)
		{
			va_list args;
			va_start(args, fmt);
			DoDrawLabel(0.0f, (float)row * s_rowSize, color, fmt, args);
			va_end(args);
		}

		void CDrawUtil2d::DrawLabel(float xPos, int row, const ColorF& color, const char* fmt, ...)
		{
			va_list args;
			va_start(args, fmt);
			DoDrawLabel(xPos, (float)row * s_rowSize, color, fmt, args);
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

		void CDrawUtil2d::DoDrawLabel(float xPos, float yPos, const ColorF& color, const char* fmt, va_list args)
		{
			SDrawTextInfo ti;
			ti.scale.set(s_fontSize, s_fontSize);
			ti.flags = eDrawText_IgnoreOverscan | eDrawText_2D | eDrawText_800x600 | eDrawText_FixedSize | eDrawText_Monospace;
			ti.color[0] = color.r;
			ti.color[1] = color.g;
			ti.color[2] = color.b;
			ti.color[3] = color.a;

			stack_string text;
			text.FormatV(fmt, args);

			gEnv->pRenderer->GetIRenderAuxGeom()->RenderTextQueued(Vec3(xPos, yPos, 0.5f), ti, text.c_str());
		}

	}
}
