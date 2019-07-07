// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "TrackViewComponentsManager.h"

CTrackViewComponentsManager::CTrackViewComponentsManager()
	: m_pKeysToolbar(nullptr)
	, m_pSequenceToolbar(nullptr)
	, m_pSequenceTabWidget(nullptr)
	, m_pPropertyTreeWidget(nullptr)
	, m_pPlaybackToolbar(nullptr)
	, m_pTrackViewCore(nullptr)
{
}

void CTrackViewComponentsManager::Init(CTrackViewCore* pTrackViewCore)
{
	m_pTrackViewCore = pTrackViewCore;

	// TODO: Get rid of all the unique named members and generalize this a bit more.

	// Property Tree Widget
	m_pPropertyTreeWidget = CreateComponent<CTrackViewPropertyTreeWidget>(m_pTrackViewCore);

	// Playback Toolbar
	m_pPlaybackToolbar = CreateComponent<CTrackViewPlaybackControlsToolbar>(m_pTrackViewCore);

	// Sequence Toolbar
	m_pSequenceToolbar = CreateComponent<CTrackViewSequenceToolbar>(m_pTrackViewCore);

	// Keys Toolbar
	m_pKeysToolbar = CreateComponent<CTrackViewKeysToolbar>(m_pTrackViewCore);

	// Dopesheet / CurveEditor Widget
	m_pSequenceTabWidget = CreateComponent<CTrackViewSequenceTabWidget>(m_pTrackViewCore);
}

void CTrackViewComponentsManager::BroadcastTrackViewEditorEvent(ETrackViewEditorEvent event)
{
	for (auto it = m_pTrackViewComponents.begin(); it != m_pTrackViewComponents.end(); ++it)
	{
		(*it)->OnTrackViewEditorEvent(event);
	}
}
