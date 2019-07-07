// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "TrackViewWindow.h"

#include "Controls/SequenceTabWidget.h"
#include "TrackViewComponentsManager.h"
#include "TrackViewBatchRenderDlg.h"
#include "TrackViewSequenceDialog.h"
#include "TrackViewEventsDialog.h"
#include "TrackViewExporter.h"
#include "TrackViewPlugin.h"

#include <EditorFramework/Events.h>
#include <Menu/AbstractMenu.h>
#include <QControls.h>
#include <Timeline.h>

#include <QKeyEvent>
#include <QSplitter>
#include <QVBoxLayout>

std::vector<CTrackViewWindow*> CTrackViewWindow::ms_trackViewWindows;

CTrackViewWindow::CTrackViewWindow(QWidget* pParent)
	: CDockableEditor(pParent)
{
	m_pTrackViewCore = new CTrackViewCore();
	m_pTrackViewCore->Init();

	setAttribute(Qt::WA_DeleteOnClose);

	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		QVBoxLayout* pWindowLayout = new QVBoxLayout();
		pWindowLayout->setContentsMargins(1, 1, 1, 1);

		QMainWindow* pToolBarsFrame = new QMainWindow();
		pToolBarsFrame->addToolBar(Qt::ToolBarArea::TopToolBarArea, pComponentsManager->GetTrackViewPlaybackToolbar());
		pToolBarsFrame->addToolBar(Qt::ToolBarArea::TopToolBarArea, pComponentsManager->GetTrackViewSequenceToolbar());
		pToolBarsFrame->addToolBar(Qt::ToolBarArea::TopToolBarArea, pComponentsManager->GetTrackViewKeysToolbar());

		pWindowLayout->addWidget(pToolBarsFrame);

		QSplitter* pContentLayout = new QSplitter(Qt::Horizontal);
		pContentLayout->setContentsMargins(0, 0, 0, 0);
		pContentLayout->addWidget(pComponentsManager->GetTrackViewSequenceTabWidget());
		pContentLayout->addWidget(pComponentsManager->GetTrackViewPropertyTreeWidget());

		pWindowLayout->addWidget(pContentLayout, 1);

		SetContent(pWindowLayout);
		setObjectName(QStringLiteral("TrackView"));

		pWindowLayout->update();

		const int32 totalSize = pContentLayout->size().width();
		if (totalSize > 240)
		{
			QList<int> sizes;
			sizes << totalSize - 120 << 120;
			pContentLayout->setSizes(sizes);
		}
	}

	stl::push_back_unique(ms_trackViewWindows, this);

	RegisterActions();
	InitMenu();
}

void CTrackViewWindow::RegisterActions()
{
	RegisterAction("general.new", &CTrackViewWindow::OnNew);
	RegisterAction("general.open", &CTrackViewWindow::OnOpen);
	RegisterAction("general.close", &CTrackViewWindow::OnClose);
	RegisterAction("general.undo", &CTrackViewWindow::OnUndo);
	RegisterAction("general.redo", &CTrackViewWindow::OnRedo);
	RegisterAction("general.copy", &CTrackViewWindow::OnCopy);
	RegisterAction("general.cut", &CTrackViewWindow::OnCut);
	RegisterAction("general.paste", &CTrackViewWindow::OnPaste);
	RegisterAction("general.delete", &CTrackViewWindow::OnDelete);
	RegisterAction("general.duplicate", &CTrackViewWindow::OnDuplicate);
	RegisterAction("general.zoom_in", &CTrackViewWindow::OnZoomIn);
	RegisterAction("general.zoom_out", &CTrackViewWindow::OnZoomOut);
	RegisterAction("general.toggle_sync_selection", &CTrackViewWindow::OnToggleSyncSelection);

	RegisterAction("trackview.delete_sequence", &CTrackViewWindow::OnDeleteSequence);
	RegisterAction("trackview.import_from_fbx", &CTrackViewWindow::OnImportSequence);
	RegisterAction("trackview.export_to_fbx", &CTrackViewWindow::OnExportSequence);
	RegisterAction("trackview.new_event", &CTrackViewWindow::OnNewEvent);
	RegisterAction("trackview.show_events", &CTrackViewWindow::OnShowAllEvents);
	RegisterAction("trackview.toggle_show_dopesheet", &CTrackViewWindow::OnToggleShowDopesheet);
	RegisterAction("trackview.toggle_show_curve_editor", &CTrackViewWindow::OnToggleShowCurveEditor);
	RegisterAction("trackview.show_sequence_properties", &CTrackViewWindow::OnSequenceProperties);
	RegisterAction("trackview.toggle_link_timelines", &CTrackViewWindow::OnToggleLinkTimelines);
	RegisterAction("trackview.set_units_ticks", &CTrackViewWindow::OnSetUnitsTicks);
	RegisterAction("trackview.set_units_framecode", &CTrackViewWindow::OnSetUnitsFramecode);
	RegisterAction("trackview.set_units_time", &CTrackViewWindow::OnSetUnitsTime);
	RegisterAction("trackview.set_units_frames", &CTrackViewWindow::OnSetUnitsFrames);
	RegisterAction("trackview.go_to_start", &CTrackViewWindow::OnGoToStart);
	RegisterAction("trackview.go_to_end", &CTrackViewWindow::OnGoToEnd);
	RegisterAction("trackview.pause_play", &CTrackViewWindow::OnPlayPause);
	RegisterAction("trackview.stop", &CTrackViewWindow::OnStop);
	RegisterAction("trackview.record", &CTrackViewWindow::OnRecord);
	RegisterAction("trackview.go_to_next_key", &CTrackViewWindow::OnGoToNextKey);
	RegisterAction("trackview.go_to_prev_key", &CTrackViewWindow::OnGoToPrevKey);
	RegisterAction("trackview.toogle_loop", &CTrackViewWindow::OnToggleLoop);
	RegisterAction("trackview.set_playback_start", &CTrackViewWindow::OnSetPlaybackStart);
	RegisterAction("trackview.set_playback_end", &CTrackViewWindow::OnSetPlaybackEnd);
	RegisterAction("trackview.reset_playback_start_end", &CTrackViewWindow::OnResetPlaybackStartEnd);
	RegisterAction("trackview.render_sequence", &CTrackViewWindow::OnRender);
	RegisterAction("trackview.create_light_animation_set", &CTrackViewWindow::OnLightAnimationSet);
	RegisterAction("trackview.sync_selected_tracks_to_base_position", &CTrackViewWindow::OnSyncSelectedTracksToBasePosition);
	RegisterAction("trackview.sync_selected_tracks_from_base_position", &CTrackViewWindow::OnSyncSelectedTracksFromBasePosition);
	RegisterAction("trackview.no_snap", &CTrackViewWindow::OnNoSnap);
	RegisterAction("trackview.magnet_snap", &CTrackViewWindow::OnMagnetSnap);
	RegisterAction("trackview.frame_snap", &CTrackViewWindow::OnFrameSnap);
	RegisterAction("trackview.grid_snap", &CTrackViewWindow::OnGridSnap);
	RegisterAction("trackview.delete_selected_tracks", &CTrackViewWindow::OnDeleteSelectedTracks);
	RegisterAction("trackview.disable_selected_tracks", &CTrackViewWindow::OnDisableSelectedTracks);
	RegisterAction("trackview.mute_selected_tracks", &CTrackViewWindow::OnMuteSelectedTracks);
	RegisterAction("trackview.enable_selected_tracks", &CTrackViewWindow::OnEnableSelectedTracks);
	RegisterAction("trackview.select_move_keys_tool", &CTrackViewWindow::OnSelectMoveKeysTool);
	RegisterAction("trackview.select_slide_keys_tool", &CTrackViewWindow::OnSelectSlideKeysTool);
	RegisterAction("trackview.select_scale_keys_tool", &CTrackViewWindow::OnSelectScaleKeysTools);
	RegisterAction("trackview.set_tangent_auto", &CTrackViewWindow::OnSetTangentAuto);
	RegisterAction("trackview.set_tangent_in_zero", &CTrackViewWindow::OnSetTangentInZero);
	RegisterAction("trackview.set_tangent_in_step", &CTrackViewWindow::OnSetTangentInStep);
	RegisterAction("trackview.set_tangent_in_linear", &CTrackViewWindow::OnSetTangentInLinear);
	RegisterAction("trackview.set_tangent_out_zero", &CTrackViewWindow::OnSetTangentOutZero);
	RegisterAction("trackview.set_tangent_out_step", &CTrackViewWindow::OnSetTangentOutStep);
	RegisterAction("trackview.set_tangent_out_linear", &CTrackViewWindow::OnSetTangentOutLinear);
	RegisterAction("trackview.break_tangents", &CTrackViewWindow::OnBreakTangents);
	RegisterAction("trackview.unify_tangents", &CTrackViewWindow::OnUnifyTangents);
	RegisterAction("trackview.fit_view_horizontal", &CTrackViewWindow::OnFitViewHorizontal);
	RegisterAction("trackview.fit_view_vertical", &CTrackViewWindow::OnFitViewVertical);
	RegisterAction("trackview.add_track_position", &CTrackViewWindow::OnAddTrackPosition);
	RegisterAction("trackview.add_track_rotation", &CTrackViewWindow::OnAddTrackRotation);
	RegisterAction("trackview.add_track_scale", &CTrackViewWindow::OnAddTrackScale);
	RegisterAction("trackview.add_track_visibility", &CTrackViewWindow::OnAddTrackVisibility);
	RegisterAction("trackview.add_track_animation", &CTrackViewWindow::OnAddTrackAnimation);
	RegisterAction("trackview.add_track_mannequin", &CTrackViewWindow::OnAddTrackMannequin);
	RegisterAction("trackview.add_track_noise", &CTrackViewWindow::OnAddTrackNoise);
	RegisterAction("trackview.add_track_audio_file", &CTrackViewWindow::OnAddTrackAudioFile);
	RegisterAction("trackview.add_track_audio_parameter", &CTrackViewWindow::OnAddTrackAudioParameter);
	RegisterAction("trackview.add_track_audio_switch", &CTrackViewWindow::OnAddTrackAudioSwitch);
	RegisterAction("trackview.add_track_audio_trigger", &CTrackViewWindow::OnAddTrackAudioTrigger);
	RegisterAction("trackview.add_track_drs_signal", &CTrackViewWindow::OnAddTrackDRSSignal);
	RegisterAction("trackview.add_track_event", &CTrackViewWindow::OnAddTrackEvent);
	RegisterAction("trackview.add_track_expression", &CTrackViewWindow::OnAddTrackExpression);
	RegisterAction("trackview.add_track_facial_sequence", &CTrackViewWindow::OnAddTrackFacialSequence);
	RegisterAction("trackview.add_track_look_at", &CTrackViewWindow::OnAddTrackLookAt);
	RegisterAction("trackview.add_track_physicalize", &CTrackViewWindow::OnAddTrackPhysicalize);
	RegisterAction("trackview.add_track_physics_driven", &CTrackViewWindow::OnAddTrackPhysicsDriven);
	RegisterAction("trackview.add_track_procedural_eyes", &CTrackViewWindow::OnAddTrackProceduralEyes);
}

CTrackViewWindow::~CTrackViewWindow()
{
	CTrackViewPlugin::GetAnimationContext()->SetSequence(nullptr, false, false);

	delete m_pTrackViewCore;
	stl::find_and_erase(ms_trackViewWindows, this);
}

void CTrackViewWindow::SetLayout(const QVariantMap& state)
{
	CEditor::SetLayout(state);

	QVariant unitVar = state.value("unit");
	if (unitVar.isValid())
	{
		m_pTrackViewCore->SetDisplayMode((SAnimTime::EDisplayMode)unitVar.toInt());
	}

	auto tabWidget = m_pTrackViewCore->GetComponentsManager()->GetTrackViewSequenceTabWidget();
	assert(tabWidget);

	QVariant linkEnabledVar = state.value("linkEnabled");
	if (linkEnabledVar.isValid())
	{
		tabWidget->EnableTimelineLink(linkEnabledVar.toBool());
	}

	QVariant dopesheetVisibleVar = state.value("dopesheetVisible");
	if (dopesheetVisibleVar.isValid())
	{
		tabWidget->ShowDopeSheet(dopesheetVisibleVar.toBool());
	}

	QVariant curveVisibleVar = state.value("curveVisible");
	if (curveVisibleVar.isValid())
	{
		tabWidget->ShowCurveEditor(curveVisibleVar.toBool());
	}

	QVariant keyTextVar = state.value("keyText");
	if (keyTextVar.isValid())
	{
		tabWidget->ShowKeyText(keyTextVar.toBool());
	}

	QVariant syncSelectionVar = state.value("syncSelection");
	if (syncSelectionVar.isValid())
	{
		tabWidget->SyncSelection(syncSelectionVar.toBool());
	}

	QVariant snapModeVar = state.value("snapMode");
	if (snapModeVar.isValid())
	{
		m_pTrackViewCore->SetSnapMode(static_cast<ETrackViewSnapMode>(snapModeVar.toInt()));
	}
}

QVariantMap CTrackViewWindow::GetLayout() const
{
	QVariantMap state = CEditor::GetLayout();

	state.insert("unit", (int)m_pTrackViewCore->GetCurrentDisplayMode());

	auto tabWidget = m_pTrackViewCore->GetComponentsManager()->GetTrackViewSequenceTabWidget();
	assert(tabWidget);

	state.insert("linkEnabled", tabWidget->IsTimelineLinkEnabled());
	state.insert("dopesheetVisible", tabWidget->IsDopeSheetVisible());
	state.insert("curveVisible", tabWidget->IsCurveEditorVisible());
	state.insert("keyText", tabWidget->IsShowingKeyText());
	state.insert("syncSelection", tabWidget->IsSyncSelectionEnabled());
	state.insert("snapMode", m_pTrackViewCore->GetCurrentSnapMode());
	return state;
}

void CTrackViewWindow::InitMenu()
{
	const CEditor::MenuItems items[] = {
		CEditor::MenuItems::FileMenu,
		CEditor::MenuItems::EditMenu,CEditor::MenuItems::ViewMenu,  CEditor::MenuItems::New,
		CEditor::MenuItems::Open,    CEditor::MenuItems::Close,     CEditor::MenuItems::Undo,
		CEditor::MenuItems::Redo,    CEditor::MenuItems::Copy,      CEditor::MenuItems::Cut,
		CEditor::MenuItems::Paste,   CEditor::MenuItems::Delete,    CEditor::MenuItems::Duplicate };
	AddToMenu(&items[0], CRY_ARRAY_COUNT(items));

	{
		CAbstractMenu* const pFileMenu = GetMenu(CEditor::MenuItems::FileMenu);

		int sec = pFileMenu->GetNextEmptySection();

		auto action = pFileMenu->CreateAction(tr("Delete Sequence..."), sec);
		connect(action, &QAction::triggered, this, &CTrackViewWindow::OnDeleteSequence);

		sec = pFileMenu->GetNextEmptySection();

		action = pFileMenu->CreateAction(tr("Import..."), sec);
		connect(action, &QAction::triggered, this, &CTrackViewWindow::OnImportSequence);

		action = pFileMenu->CreateAction(tr("Export..."), sec);
		connect(action, &QAction::triggered, this, &CTrackViewWindow::OnExportSequence);
	}

	{
		CAbstractMenu* pEditMenu = GetMenu(MenuItems::EditMenu);

		const int sec = pEditMenu->GetNextEmptySection();

		auto action = pEditMenu->CreateAction(tr("New Event"), sec);
		connect(action, &QAction::triggered, this, &CTrackViewWindow::OnNewEvent);

		action = pEditMenu->CreateAction(tr("Show Events"), sec);
		connect(action, &QAction::triggered, this, &CTrackViewWindow::OnShowAllEvents);
	}

	CAbstractMenu* const pPlaybackMenu = GetRootMenu()->CreateMenu(tr("Playback"), 0);
	pPlaybackMenu->signalAboutToShow.Connect([pPlaybackMenu, this]()
	{
		pPlaybackMenu->Clear();
		m_pTrackViewCore->GetComponentsManager()->GetTrackViewPlaybackToolbar()->PopulatePlaybackMenu(pPlaybackMenu);
	});

	CAbstractMenu* const pViewMenu = GetMenu(MenuItems::ViewMenu);
	pViewMenu->signalAboutToShow.Connect([pViewMenu, this]()
	{
		pViewMenu->Clear();

		AddToMenu(CEditor::MenuItems::ZoomIn);
		AddToMenu(CEditor::MenuItems::ZoomOut);

		CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager();

		int sec = pViewMenu->GetNextEmptySection();

		auto action = pViewMenu->CreateAction(tr("Show Dopesheet"), sec);
		action->setCheckable(true);
		action->setChecked(pComponentsManager->GetTrackViewSequenceTabWidget()->IsDopeSheetVisible());
		connect(action, &QAction::triggered, this, &CTrackViewWindow::OnToggleShowDopesheet);

		action = pViewMenu->CreateAction(tr("Show Curve Editor"), sec);
		action->setCheckable(true);
		action->setChecked(pComponentsManager->GetTrackViewSequenceTabWidget()->IsCurveEditorVisible());
		connect(action, &QAction::triggered, this, &CTrackViewWindow::OnToggleShowCurveEditor);

		action = pViewMenu->CreateAction(tr("Sequence Properties"), sec);
		connect(action, &QAction::triggered, this, &CTrackViewWindow::OnSequenceProperties);

		sec = pViewMenu->GetNextEmptySection();

		action = pViewMenu->CreateAction(tr("Link Timelines"), sec);
		action->setCheckable(true);
		action->setChecked(pComponentsManager->GetTrackViewSequenceTabWidget()->IsTimelineLinkEnabled());
		connect(action, &QAction::triggered, this, &CTrackViewWindow::OnToggleLinkTimelines);

		action = pViewMenu->CreateAction(tr("Show Key Text"), sec);
		action->setCheckable(true);
		action->setChecked(pComponentsManager->GetTrackViewSequenceTabWidget()->IsShowingKeyText());
		connect(action, &QAction::triggered, pComponentsManager->GetTrackViewSequenceTabWidget(), &CTrackViewSequenceTabWidget::ShowKeyText);

		action = pViewMenu->CreateAction(tr("Sync Selection"), sec);
		action->setCheckable(true);
		action->setChecked(pComponentsManager->GetTrackViewSequenceTabWidget()->IsSyncSelectionEnabled());
		connect(action, &QAction::triggered, pComponentsManager->GetTrackViewSequenceTabWidget(), &CTrackViewSequenceTabWidget::SyncSelection);

		action = pViewMenu->CreateAction(tr("Invert Scrubber Snapping Behavior"), sec);
		action->setCheckable(true);
		action->setChecked(pComponentsManager->GetTrackViewSequenceTabWidget()->IsScrubberSnappingBehaviorInverted());
		connect(action, &QAction::triggered, pComponentsManager->GetTrackViewSequenceTabWidget(), &CTrackViewSequenceTabWidget::InvertScrubberSnappingBehavior);

		sec = pViewMenu->GetNextEmptySection();
		pViewMenu->SetSectionName(sec, "Units");

		action = pViewMenu->CreateAction(tr("Ticks"), sec);
		action->setCheckable(true);
		action->setChecked(m_pTrackViewCore->GetCurrentDisplayMode() == SAnimTime::EDisplayMode::Ticks);
		action->connect(action, &QAction::triggered, this, &CTrackViewWindow::OnSetUnitsTicks);

		action = pViewMenu->CreateAction(tr("Time"), sec);
		action->setCheckable(true);
		action->setChecked(m_pTrackViewCore->GetCurrentDisplayMode() == SAnimTime::EDisplayMode::Time);
		action->connect(action, &QAction::triggered, this, &CTrackViewWindow::OnSetUnitsTime);

		action = pViewMenu->CreateAction(tr("Timecode"), sec);
		action->setCheckable(true);
		action->setChecked(m_pTrackViewCore->GetCurrentDisplayMode() == SAnimTime::EDisplayMode::Timecode);
		action->connect(action, &QAction::triggered, this, &CTrackViewWindow::OnSetUnitsFramecode);

		action = pViewMenu->CreateAction(tr("Frames"), sec);
		action->setCheckable(true);
		action->setChecked(m_pTrackViewCore->GetCurrentDisplayMode() == SAnimTime::EDisplayMode::Frames);
		action->connect(action, &QAction::triggered, this, &CTrackViewWindow::OnSetUnitsFrames);
	});

	CAbstractMenu* const pToolsMenu = GetRootMenu()->CreateMenu(tr("Tools"), 0);
	pToolsMenu->signalAboutToShow.Connect([pToolsMenu, this]()
	{
		pToolsMenu->Clear();
		auto action = pToolsMenu->CreateAction(tr("Render Sequence"));
		action->connect(action, &QAction::triggered, this, &CTrackViewWindow::OnRender);

		int sec = pToolsMenu->GetNextEmptySection();

		action = pToolsMenu->CreateAction(tr(gEnv->pMovieSystem->FindSequence("_LightAnimationSet") != nullptr ? "Open Light Animation Set" : "Create Light Animation Set"), sec);
		action->connect(action, &QAction::triggered, this, &CTrackViewWindow::OnLightAnimationSet);
	});
}

bool CTrackViewWindow::OnNew()
{
	m_pTrackViewCore->NewSequence();
	return true;
}

bool CTrackViewWindow::OnOpen()
{
	CTrackViewSequenceDialog dialog(CTrackViewSequenceDialog::OpenSequences);
	if (dialog.exec())
	{
		for (const CryGUID& seqGuid : dialog.GetSequenceGuids())
		{
			m_pTrackViewCore->OpenSequence(seqGuid);
		}
	}
	return true;
}

bool CTrackViewWindow::OnClose()
{
	auto pTabWidget = m_pTrackViewCore->GetComponentsManager()->GetTrackViewSequenceTabWidget();
	if (pTabWidget)
	{
		if (m_pTrackViewCore->IsPlaying() || m_pTrackViewCore->IsPaused())
		{
			m_pTrackViewCore->OnStop();
		}

		CTrackViewPlugin::GetAnimationContext()->SetSequence(nullptr, false, false);

		pTabWidget->CloseCurrentTab();
	}

	return true;
}

bool CTrackViewWindow::OnUndo()
{
	auto pTabWidget = m_pTrackViewCore->GetComponentsManager()->GetTrackViewSequenceTabWidget();
	if (pTabWidget)
	{
		auto pTimeline = pTabWidget->GetActiveDopeSheet();
		if (pTimeline)
		{
			pTimeline->SignalUndo(); //Only triggers internal signals
		}
	}

	//uses global undo
	return false;
}

bool CTrackViewWindow::OnRedo()
{
	auto pTabWidget = m_pTrackViewCore->GetComponentsManager()->GetTrackViewSequenceTabWidget();
	if (pTabWidget)
	{
		auto pTimeline = pTabWidget->GetActiveDopeSheet();
		if (pTimeline)
		{
			pTimeline->SignalRedo(); //Only triggers internal signals
		}
	}

	//uses global undo
	return false;
}

bool CTrackViewWindow::OnCopy()
{
	auto pTabWidget = m_pTrackViewCore->GetComponentsManager()->GetTrackViewSequenceTabWidget();
	if (pTabWidget)
	{
		auto pTimeline = pTabWidget->GetActiveDopeSheet();
		if (pTimeline)
		{
			pTimeline->OnMenuCopy();
		}
	}

	return true;
}

bool CTrackViewWindow::OnCut()
{
	auto pTabWidget = m_pTrackViewCore->GetComponentsManager()->GetTrackViewSequenceTabWidget();
	if (pTabWidget)
	{
		auto pTimeline = pTabWidget->GetActiveDopeSheet();
		if (pTimeline)
		{
			pTimeline->OnMenuCut();
		}
	}

	return true;
}

bool CTrackViewWindow::OnPaste()
{
	auto pTabWidget = m_pTrackViewCore->GetComponentsManager()->GetTrackViewSequenceTabWidget();
	if (pTabWidget)
	{
		auto pTimeline = pTabWidget->GetActiveDopeSheet();
		if (pTimeline)
		{
			pTimeline->OnMenuPaste();
		}
	}

	return true;
}

bool CTrackViewWindow::OnDelete()
{
	auto pTabWidget = m_pTrackViewCore->GetComponentsManager()->GetTrackViewSequenceTabWidget();
	if (pTabWidget)
	{
		auto pTimeline = pTabWidget->GetActiveDopeSheet();
		if (pTimeline)
		{
			pTimeline->OnMenuDelete();
		}
	}

	return true;
}

bool CTrackViewWindow::OnDuplicate()
{
	auto pTabWidget = m_pTrackViewCore->GetComponentsManager()->GetTrackViewSequenceTabWidget();
	if (pTabWidget)
	{
		auto pTimeline = pTabWidget->GetActiveDopeSheet();
		if (pTimeline)
		{
			pTimeline->OnMenuDuplicate();
		}
	}

	return true;
}

bool CTrackViewWindow::OnZoomIn()
{
	auto pTabWidget = m_pTrackViewCore->GetComponentsManager()->GetTrackViewSequenceTabWidget();
	if (pTabWidget)
	{
		auto pTimeline = pTabWidget->GetActiveDopeSheet();
		if (pTimeline)
		{
			pTimeline->ZoomStep(1);
		}
	}
	return true;
}

bool CTrackViewWindow::OnZoomOut()
{
	auto pTabWidget = m_pTrackViewCore->GetComponentsManager()->GetTrackViewSequenceTabWidget();
	if (pTabWidget)
	{
		auto pTimeline = pTabWidget->GetActiveDopeSheet();
		if (pTimeline)
		{
			pTimeline->ZoomStep(-1);
		}
	}
	return true;
}

bool CTrackViewWindow::OnExportSequence()
{
	CTrackViewPlugin::GetExporter()->ExportToFile(true);
	return true;
}

bool CTrackViewWindow::OnImportSequence()
{
	CTrackViewPlugin::GetExporter()->ImportFromFile();
	return true;
}

bool CTrackViewWindow::OnUnitsChanged(SAnimTime::EDisplayMode mode)
{
	m_pTrackViewCore->SetDisplayMode(mode);
	m_pTrackViewCore->UpdateProperties();

	return true;
}

bool CTrackViewWindow::OnRender()
{
	CTrackViewBatchRenderDlg dlg;
	dlg.exec();

	return true;
}

bool CTrackViewWindow::OnDeleteSequence()
{
	CTrackViewSequenceDialog dialog(CTrackViewSequenceDialog::DeleteSequences);
	dialog.exec();

	return true;
}

bool CTrackViewWindow::OnSequenceProperties()
{
	if (CTrackViewComponentsManager* pManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pTabWidget = pManager->GetTrackViewSequenceTabWidget())
		{
			if (CTrackViewSequence* pSequence = pTabWidget->GetActiveSequence())
			{
				pTabWidget->ShowSequenceProperties(pSequence);
			}
		}
	}

	return true;
}

bool CTrackViewWindow::OnNewEvent()
{
	if (CTrackViewComponentsManager* pManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pTabWidget = pManager->GetTrackViewSequenceTabWidget())
		{
			if (CTrackViewSequence* pSequence = pTabWidget->GetActiveSequence())
			{
				CTrackViewNewEventDialog dialog(*pSequence);
				if (dialog.exec())
				{
					pSequence->AddTrackEvent(dialog.GetEventName().c_str());
				}
			}
		}
	}

	return true;
}

bool CTrackViewWindow::OnShowAllEvents()
{
	if (CTrackViewComponentsManager* pManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pTabWidget = pManager->GetTrackViewSequenceTabWidget())
		{
			if (CTrackViewSequence* pSequence = pTabWidget->GetActiveSequence())
			{
				CTrackViewEventsDialog dialog(*pSequence);
				dialog.exec();
			}
		}
	}

	return true;
}

bool CTrackViewWindow::OnLightAnimationSet()
{
	CTrackViewSequence* pLightAnimSeq = CTrackViewPlugin::GetSequenceManager()->GetSequenceByName("_LightAnimationSet");
	if (!pLightAnimSeq)
	{
		pLightAnimSeq = CTrackViewPlugin::GetSequenceManager()->CreateSequence("_LightAnimationSet");
		pLightAnimSeq->SetFlags(IAnimSequence::eSeqFlags_LightAnimationSet);
	}

	m_pTrackViewCore->OpenSequence(pLightAnimSeq->GetGUID());
	return true;
}

bool CTrackViewWindow::OnToggleShowDopesheet()
{
	CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager();
	if (pComponentsManager)
	{
		CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget();
		if (pSequenceTabWidget)
		{
			pSequenceTabWidget->ShowDopeSheet(!pSequenceTabWidget->IsDopeSheetVisible());
		}
	}

	return true;
}

bool CTrackViewWindow::OnToggleShowCurveEditor()
{
	CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager();
	if (pComponentsManager)
	{
		CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget();
		if (pSequenceTabWidget)
		{
			pSequenceTabWidget->ShowCurveEditor(!pSequenceTabWidget->IsCurveEditorVisible());
		}
	}

	return true;
}

bool CTrackViewWindow::OnToggleLinkTimelines()
{
	CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager();
	if (pComponentsManager)
	{
		CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget();
		if (pSequenceTabWidget)
		{
			pSequenceTabWidget->EnableTimelineLink(!pSequenceTabWidget->IsTimelineLinkEnabled());
		}
	}

	return true;
}

bool CTrackViewWindow::OnToggleSyncSelection()
{
	CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager();
	if (pComponentsManager)
	{
		CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget();
		if (pSequenceTabWidget)
		{
			pSequenceTabWidget->SyncSelection(!pSequenceTabWidget->IsSyncSelectionEnabled());
		}
	}

	return true;
}

bool CTrackViewWindow::OnSetUnitsTicks()
{
	return OnUnitsChanged(SAnimTime::EDisplayMode::Ticks);
}

bool CTrackViewWindow::OnSetUnitsTime()
{
	return OnUnitsChanged(SAnimTime::EDisplayMode::Time);
}

bool CTrackViewWindow::OnSetUnitsFramecode()
{
	return OnUnitsChanged(SAnimTime::EDisplayMode::Timecode);
}

bool CTrackViewWindow::OnSetUnitsFrames()
{
	return OnUnitsChanged(SAnimTime::EDisplayMode::Frames);
}

bool CTrackViewWindow::OnGoToStart()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewPlaybackControlsToolbar* pPlaybackControlsToolbar = pComponentsManager->GetTrackViewPlaybackToolbar())
		{
			pPlaybackControlsToolbar->OnGoToStart();
		}
	}

	return true;
}

bool CTrackViewWindow::OnGoToEnd()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewPlaybackControlsToolbar* pPlaybackControlsToolbar = pComponentsManager->GetTrackViewPlaybackToolbar())
		{
			pPlaybackControlsToolbar->OnGoToEnd();
		}
	}
	return true;
}

bool CTrackViewWindow::OnPlayPause()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewPlaybackControlsToolbar* pPlaybackControlsToolbar = pComponentsManager->GetTrackViewPlaybackToolbar())
		{
			pPlaybackControlsToolbar->OnPlayPause(!m_pTrackViewCore->IsPlaying() || m_pTrackViewCore->IsPaused());
		}
	}
	return true;
}

bool CTrackViewWindow::OnStop()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewPlaybackControlsToolbar* pPlaybackControlsToolbar = pComponentsManager->GetTrackViewPlaybackToolbar())
		{
			pPlaybackControlsToolbar->OnStop();
		}
	}
	return true;
}

bool CTrackViewWindow::OnRecord()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewPlaybackControlsToolbar* pPlaybackControlsToolbar = pComponentsManager->GetTrackViewPlaybackToolbar())
		{
			pPlaybackControlsToolbar->OnRecord(!m_pTrackViewCore->IsRecording());
		}
	}
	return true;
}

bool CTrackViewWindow::OnGoToNextKey()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewKeysToolbar* pKeysToolbar = pComponentsManager->GetTrackViewKeysToolbar())
		{
			pKeysToolbar->OnGoToNextKey();
		}
	}
	return true;
}

bool CTrackViewWindow::OnGoToPrevKey()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewKeysToolbar* pKeysToolbar = pComponentsManager->GetTrackViewKeysToolbar())
		{
			pKeysToolbar->OnGoToPreviousKey();
		}
	}
	return true;
}

bool CTrackViewWindow::OnToggleLoop()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewPlaybackControlsToolbar* pPlaybackControlsToolbar = pComponentsManager->GetTrackViewPlaybackToolbar())
		{
			pPlaybackControlsToolbar->OnLoop(!m_pTrackViewCore->IsLooping());
		}
	}
	return true;
}

bool CTrackViewWindow::OnSetPlaybackStart()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewPlaybackControlsToolbar* pPlaybackControlsToolbar = pComponentsManager->GetTrackViewPlaybackToolbar())
		{
			pPlaybackControlsToolbar->OnSetStartMarker();
		}
	}
	return true;
}

bool CTrackViewWindow::OnSetPlaybackEnd()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewPlaybackControlsToolbar* pPlaybackControlsToolbar = pComponentsManager->GetTrackViewPlaybackToolbar())
		{
			pPlaybackControlsToolbar->OnSetEndMarker();
		}
	}
	return true;
}

bool CTrackViewWindow::OnResetPlaybackStartEnd()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewPlaybackControlsToolbar* pPlaybackControlsToolbar = pComponentsManager->GetTrackViewPlaybackToolbar())
		{
			pPlaybackControlsToolbar->OnResetMarkers();
		}
	}
	return true;
}

bool CTrackViewWindow::OnSyncSelectedTracksToBasePosition()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewKeysToolbar* pKeysToolbar = pComponentsManager->GetTrackViewKeysToolbar())
		{
			pKeysToolbar->OnSyncSelectedTracksToBase();
		}
	}
	return true;
}

bool CTrackViewWindow::OnSyncSelectedTracksFromBasePosition()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewKeysToolbar* pKeysToolbar = pComponentsManager->GetTrackViewKeysToolbar())
		{
			pKeysToolbar->OnSyncSelectedTracksFromBase();
		}
	}
	return true;
}

bool CTrackViewWindow::OnNoSnap()
{
	m_pTrackViewCore->SetSnapMode(eSnapMode_NoSnapping);
	return true;
}

bool CTrackViewWindow::OnMagnetSnap()
{
	m_pTrackViewCore->SetSnapMode(eSnapMode_KeySnapping);
	return true;
}

bool CTrackViewWindow::OnFrameSnap()
{
	m_pTrackViewCore->SetSnapMode(eSnapMode_TimeSnapping);
	return true;
}

bool CTrackViewWindow::OnGridSnap()
{
	return true;
}

bool CTrackViewWindow::OnDeleteSelectedTracks()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnDeleteNodes();
		}
	}
	return true;
}

bool CTrackViewWindow::OnDisableSelectedTracks()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->DisableSelectedNodesAndTracks(true);
		}
	}
	return true;
}

bool CTrackViewWindow::OnMuteSelectedTracks()
{
	return true;
}

bool CTrackViewWindow::OnEnableSelectedTracks()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->DisableSelectedNodesAndTracks(false);
		}
	}
	return true;
}

bool CTrackViewWindow::OnSelectMoveKeysTool()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewKeysToolbar* pKeysToolbar = pComponentsManager->GetTrackViewKeysToolbar())
		{
			pKeysToolbar->OnMoveKeys();
		}
	}
	return true;
}

bool CTrackViewWindow::OnSelectSlideKeysTool()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewKeysToolbar* pKeysToolbar = pComponentsManager->GetTrackViewKeysToolbar())
		{
			pKeysToolbar->OnSlideKeys();
		}
	}
	return true;
}

bool CTrackViewWindow::OnSelectScaleKeysTools()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewKeysToolbar* pKeysToolbar = pComponentsManager->GetTrackViewKeysToolbar())
		{
			pKeysToolbar->OnScaleKeys();
		}
	}
	return true;
}

bool CTrackViewWindow::OnSetTangentAuto()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			if (CCurveEditor* pCurveEditor = pSequenceTabWidget->GetActiveCurveEditor())
			{
				pCurveEditor->OnSetSelectedKeysTangentAuto();
			}
		}
	}
	return true;
}

bool CTrackViewWindow::OnSetTangentInZero()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			if (CCurveEditor* pCurveEditor = pSequenceTabWidget->GetActiveCurveEditor())
			{
				pCurveEditor->OnSetSelectedKeysInTangentZero();
			}
		}
	}
	return true;
}

bool CTrackViewWindow::OnSetTangentInStep()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			if (CCurveEditor* pCurveEditor = pSequenceTabWidget->GetActiveCurveEditor())
			{
				pCurveEditor->OnSetSelectedKeysInTangentStep();
			}
		}
	}
	return true;
}

bool CTrackViewWindow::OnSetTangentInLinear()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			if (CCurveEditor* pCurveEditor = pSequenceTabWidget->GetActiveCurveEditor())
			{
				pCurveEditor->OnSetSelectedKeysInTangentLinear();
			}
		}
	}
	return true;
}

bool CTrackViewWindow::OnSetTangentOutZero()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			if (CCurveEditor* pCurveEditor = pSequenceTabWidget->GetActiveCurveEditor())
			{
				pCurveEditor->OnSetSelectedKeysOutTangentZero();
			}
		}
	}
	return true;
}

bool CTrackViewWindow::OnSetTangentOutStep()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			if (CCurveEditor* pCurveEditor = pSequenceTabWidget->GetActiveCurveEditor())
			{
				pCurveEditor->OnSetSelectedKeysOutTangentStep();
			}
		}
	}
	return true;
}

bool CTrackViewWindow::OnSetTangentOutLinear()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			if (CCurveEditor* pCurveEditor = pSequenceTabWidget->GetActiveCurveEditor())
			{
				pCurveEditor->OnSetSelectedKeysOutTangentLinear();
			}
		}
	}
	return true;
}

bool CTrackViewWindow::OnBreakTangents()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			if (CCurveEditor* pCurveEditor = pSequenceTabWidget->GetActiveCurveEditor())
			{
				pCurveEditor->OnBreakTangents();
			}
		}
	}
	return true;
}

bool CTrackViewWindow::OnUnifyTangents()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			if (CCurveEditor* pCurveEditor = pSequenceTabWidget->GetActiveCurveEditor())
			{
				pCurveEditor->OnUnifyTangents();
			}
		}
	}
	return true;
}

bool CTrackViewWindow::OnFitViewHorizontal()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			if (CCurveEditor* pCurveEditor = pSequenceTabWidget->GetActiveCurveEditor())
			{
				pCurveEditor->OnFitCurvesHorizontally();
			}
		}
	}
	return true;
}

bool CTrackViewWindow::OnFitViewVertical()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			if (CCurveEditor* pCurveEditor = pSequenceTabWidget->GetActiveCurveEditor())
			{
				pCurveEditor->OnFitCurvesVertically();
			}
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackPosition()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_Position);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackRotation()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_Rotation);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackScale()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_Scale);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackVisibility()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_Visibility);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackAnimation()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_Animation);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackMannequin()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_Mannequin);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackNoise()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrack(eAnimParamType_TransformNoise);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackAudioFile()
{
	//if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	//{
	//	if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
	//	{
	//		pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_AudioFile);
	//	}
	//}
	return true;
}

bool CTrackViewWindow::OnAddTrackAudioParameter()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_AudioParameter);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackAudioSwitch()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_AudioSwitch);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackAudioTrigger()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_AudioTrigger);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackDRSSignal()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_DynamicResponseSignal);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackEvent()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_Event);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackExpression()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_Expression);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackFacialSequence()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_FaceSequence);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackLookAt()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_LookAt);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackPhysicalize()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_Physicalize);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackPhysicsDriven()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_PhysicsDriven);
		}
	}
	return true;
}

bool CTrackViewWindow::OnAddTrackProceduralEyes()
{
	if (CTrackViewComponentsManager* pComponentsManager = m_pTrackViewCore->GetComponentsManager())
	{
		if (CTrackViewSequenceTabWidget* pSequenceTabWidget = pComponentsManager->GetTrackViewSequenceTabWidget())
		{
			pSequenceTabWidget->OnAddTrackToSelectedNode(eAnimParamType_ProceduralEyes);
		}
	}
	return true;
}

void CTrackViewWindow::keyPressEvent(QKeyEvent* e)
{
	if (e->key() == Qt::Key_Space)
	{
		if (!m_pTrackViewCore->IsPlaying())
			m_pTrackViewCore->OnPlayPause(true);
		else
			m_pTrackViewCore->OnPlayPause(m_pTrackViewCore->IsPaused());
	}
}
