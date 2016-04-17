// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

/*************************************************************************
-------------------------------------------------------------------------
$Id$
$DateTime$
Description: Screen Utilities such as safearea rendering.

-------------------------------------------------------------------------
History:
- 22:09:2009: Created by Frank Harrison for console TRC compliance.

*************************************************************************/

#ifndef __ScreenLayoutUtils_H__
#define __ScreenLayoutUtils_H__

class C2DRenderUtils;
class ScreenLayoutManager;

#if ENABLE_HUD_EXTRA_DEBUG
class SafeAreaResizer;
#endif

class SafeAreaRenderer
{
public :
	SafeAreaRenderer( C2DRenderUtils* p2dRenderUtils, ScreenLayoutManager* pScreenLayout );
	~SafeAreaRenderer( );

	void DrawSafeAreas( void );

private :

	void  DrawSafeArea( const char* text, const Vec2& border_pecentage );

	C2DRenderUtils* m_p2dRenderUtils;
	ScreenLayoutManager* m_pScreenLayout;

#if ENABLE_HUD_EXTRA_DEBUG
	SafeAreaResizer* m_pResizer;

	int m_interactiveResize;
#endif
};

#endif // __ScreenLayoutUtils_H__