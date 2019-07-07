// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

namespace UQS
{
	namespace Core
	{

		//===================================================================================
		//
		// CDrawUtil2d
		//
		//===================================================================================

		class CDrawUtil2d
		{
		public:
			static void                      DrawLabel(int row, const ColorF& color, const char* szFormat, ...) PRINTF_PARAMS(3, 4);
			static void                      DrawLabel(float xPos, int row, const ColorF& color, const char* szFormat, ...) PRINTF_PARAMS(4, 5);
			static void                      DrawLabel(float xPos, float yPos, float fontSize, const ColorF& color, const char* szFormat, ...) PRINTF_PARAMS(5, 6);
			static float                     GetRowSize();
			static float                     GetIndentSize();
			static void                      DrawFilledQuad(float x1, float y1, float x2, float y2, const ColorF& color);

		private:
			static void                      DoDrawLabel(float xPos, float yPos, float fontSize, const ColorF& color, const char* szFormat, va_list args);

		private:
			static const float               s_rowSize;
			static const float               s_fontSize;
			static const float               s_indentSize;
			static const SAuxGeomRenderFlags s_flags2d;
		};

	}
}
