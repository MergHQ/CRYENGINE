// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include <StdAfx.h>
#include "LevelEditorSharedState.h"

#include "Commands/ICommandManager.h"
#include "Commands/QCommandAction.h"
#include "LevelEditor/Tools/EditTool.h"
#include "LevelEditor/Tools/PickObjectTool.h"
#include "LevelEditor/Tools/ObjectMode.h"
#include "EditorFramework/PersonalizationManager.h"
#include "Viewport.h"

#include <IEditor.h>
#include <CrySystem/ISystem.h>
#include <CrySystem/IConsole.h>

Q_DECLARE_METATYPE(CLevelEditorSharedState::CoordSystem)
Q_DECLARE_METATYPE(CLevelEditorSharedState::Axis)
Q_DECLARE_METATYPE(CLevelEditorSharedState::EditMode)

CLevelEditorSharedState::CLevelEditorSharedState()
	: m_showDisplayInfo(false)
	, m_displayInfoLevel(eDisplayInfoLevel_Low)
	, m_editMode(EditMode::Select)
	, m_axisConstraint(Axis::None)
	, m_pEditTool(nullptr)
	, m_pPickTool(nullptr)
{
	unsigned lastEditMode = static_cast<unsigned>(EditMode::LastEditMode);
	for (unsigned i = 0; i < lastEditMode; ++i)
	{
		m_coordSystemForEditMode[i] = CoordSystem::Local;
	}

	m_selectedRegion.min = Vec3(0, 0, 0);
	m_selectedRegion.max = Vec3(0, 0, 0);
}

void CLevelEditorSharedState::OnEditorNotifyEvent(EEditorNotifyEvent eventId)
{
	switch (eventId)
	{
	case eNotify_OnMainFrameInitialized:
		InitActions();
		ResetState();
		break;
	}
}

void CLevelEditorSharedState::InitActions()
{
	signalShowDisplayInfoChanged.Connect([this]()  { GetIEditor()->GetICommandManager()->SetChecked("level.toggle_display_info", IsDisplayInfoEnabled()); });
	signalDisplayInfoLevelChanged.Connect(this, &CLevelEditorSharedState::UpdateDisplayInfoActions);

	signalCoordSystemChanged.Connect(this, &CLevelEditorSharedState::UpdateCoordSystemActions);
	signalAxisConstraintChanged.Connect(this, &CLevelEditorSharedState::UpdateAxisConstraintActions);
	signalEditModeChanged.Connect(this, &CLevelEditorSharedState::UpdateEditModeActions);

	signalCoordSystemChanged.Connect(this, &CLevelEditorSharedState::SaveState);
	signalAxisConstraintChanged.Connect(this, &CLevelEditorSharedState::SaveState);
	signalEditModeChanged.Connect(this, &CLevelEditorSharedState::SaveState);

	UpdateDisplayInfoActions();
	UpdateEditModeActions();
	UpdateCoordSystemActions();
	UpdateAxisConstraintActions();
}

void CLevelEditorSharedState::UpdateDisplayInfoActions()
{
	ICommandManager* pCommandManager = GetIEditor()->GetICommandManager();
	pCommandManager->SetChecked("level.display_info_low", GetDisplayInfoLevel() == eDisplayInfoLevel_Low);
	pCommandManager->SetChecked("level.display_info_medium", GetDisplayInfoLevel() == eDisplayInfoLevel_Med);
	pCommandManager->SetChecked("level.display_info_high", GetDisplayInfoLevel() == eDisplayInfoLevel_High);
}

void CLevelEditorSharedState::UpdateCoordSystemActions()
{
	ICommandManager* pCommandManager = GetIEditor()->GetICommandManager();
	pCommandManager->SetChecked("level.set_view_coordinate_system", GetCoordSystem() == CoordSystem::View);
	pCommandManager->SetChecked("level.set_local_coordinate_system", GetCoordSystem() == CoordSystem::Local);
	pCommandManager->SetChecked("level.set_parent_coordinate_system", GetCoordSystem() == CoordSystem::Parent);
	pCommandManager->SetChecked("level.set_world_coordinate_system", GetCoordSystem() == CoordSystem::World);
}

void CLevelEditorSharedState::UpdateAxisConstraintActions()
{
	ICommandManager* pCommandManager = GetIEditor()->GetICommandManager();
	pCommandManager->SetChecked("tools.enable_x_axis_constraint", GetAxisConstraint() == Axis::X);
	pCommandManager->SetChecked("tools.enable_y_axis_constraint", GetAxisConstraint() == Axis::Y);
	pCommandManager->SetChecked("tools.enable_z_axis_constraint", GetAxisConstraint() == Axis::Z);
	pCommandManager->SetChecked("tools.enable_xy_axis_constraint", GetAxisConstraint() == Axis::XY);
}

void CLevelEditorSharedState::UpdateEditModeActions()
{
	ICommandManager* pCommandManager = GetIEditor()->GetICommandManager();
	pCommandManager->SetChecked("tools.move", GetEditMode() == EditMode::Move);
	pCommandManager->SetChecked("tools.rotate", GetEditMode() == EditMode::Rotate);
	pCommandManager->SetChecked("tools.scale", GetEditMode() == EditMode::Scale);
	pCommandManager->SetChecked("tools.select", GetEditMode() == EditMode::Select);
}

void CLevelEditorSharedState::ResetState()
{
	//If r_DisplayInfo was found in config file, use it, otherwise load from personalization manager
	const bool fromConfig = (GetIEditor()->GetSystem()->GetIConsole()->GetCVar("r_DisplayInfo")->GetFlags() & VF_WASINCONFIG);
	//If user has r_displayInfo in config, use the value.
	if (fromConfig)
	{
		int val = gEnv->pConsole->GetCVar("r_displayInfo")->GetIVal();
		m_showDisplayInfo = (val != 0);
		switch (val)
		{
		case 1:
			m_displayInfoLevel = eDisplayInfoLevel_Med;
			break;
		case 2:
			m_displayInfoLevel = eDisplayInfoLevel_High;
			break;
		case 3:
			m_displayInfoLevel = eDisplayInfoLevel_Low;
			break;

		default:
			//4 & 5 are not supported
			break;
		}
	}
	else
	{
		//Otherwise use setting from personalization manager
		QVariant showDisplayInfo = GET_PERSONALIZATION_PROPERTY(CLevelEditorSharedState, "ShowDisplayInfo");
		if (showDisplayInfo.isValid())
		{
			m_showDisplayInfo = showDisplayInfo.toBool();
		}

		QVariant displayInfoLevel = GET_PERSONALIZATION_PROPERTY(CLevelEditorSharedState, "DisplayInfoLevel");
		if (displayInfoLevel.isValid())
		{
			int val = displayInfoLevel.toInt();
			if (val < eDisplayInfoLevel_Low || val > eDisplayInfoLevel_High)
			{
				val = eDisplayInfoLevel_Low;
			}
			m_displayInfoLevel = static_cast<eDisplayInfoLevel>(val);
		}

		SetDisplayInfoCVar();
	}

	QVariant var;

	var = GET_PERSONALIZATION_PROPERTY(CLevelEditorSharedState, "AxisConstraint");
	if (var.isValid())
		m_axisConstraint = static_cast<CLevelEditorSharedState::Axis>(var.toInt());

	unsigned lastEditMode = static_cast<unsigned>(EditMode::LastEditMode);
	for (unsigned i = 0; i < lastEditMode; ++i)
	{
		var = GET_PERSONALIZATION_PROPERTY(CLevelEditorSharedState, QString("CoordSystem_%1").arg(i));
		if (var.isValid())
			m_coordSystemForEditMode[i] = static_cast<CLevelEditorSharedState::CoordSystem>(var.toInt());
	}

	var = GET_PERSONALIZATION_PROPERTY(CLevelEditorSharedState, "EditMode");
	if (var.isValid())
		m_editMode = static_cast<CLevelEditorSharedState::EditMode>(var.toInt());

	UpdateDisplayInfoActions();
	UpdateEditModeActions();
	UpdateCoordSystemActions();
	UpdateAxisConstraintActions();

	signalShowDisplayInfoChanged();
	signalDisplayInfoLevelChanged();
}

bool CLevelEditorSharedState::IsDisplayInfoEnabled() const
{
	return m_showDisplayInfo;
}

CLevelEditorSharedState::eDisplayInfoLevel CLevelEditorSharedState::GetDisplayInfoLevel() const
{
	return m_displayInfoLevel;
}

void CLevelEditorSharedState::ToggleDisplayInfo()
{
	m_showDisplayInfo = !m_showDisplayInfo;
	SetDisplayInfoCVar();
	signalShowDisplayInfoChanged();
	SaveState();
}

void CLevelEditorSharedState::SetDisplayInfoLevel(eDisplayInfoLevel level)
{
	m_displayInfoLevel = level;
	SetDisplayInfoCVar();
	signalDisplayInfoLevelChanged();
	SaveState();
}

void CLevelEditorSharedState::SetDisplayInfoCVar()
{
	//Translate GUI state to cVar
	int cVar = 0;
	if (m_showDisplayInfo)
	{
		switch (m_displayInfoLevel)
		{
		case eDisplayInfoLevel_Low:
			cVar = 3;
			break;
		case eDisplayInfoLevel_Med:
			cVar = 1;
			break;
		case eDisplayInfoLevel_High:
			cVar = 2;
			break;
		}
	}

	gEnv->pConsole->GetCVar("r_displayInfo")->Set(cVar);
}

void CLevelEditorSharedState::SetCoordSystem(CoordSystem coordSystem)
{
	m_coordSystemForEditMode[static_cast<unsigned>(m_editMode)] = coordSystem;
	signalCoordSystemChanged(coordSystem);
}

CLevelEditorSharedState::CoordSystem CLevelEditorSharedState::GetCoordSystem() const
{
	return m_coordSystemForEditMode[static_cast<unsigned>(m_editMode)];
}

void CLevelEditorSharedState::SetAxisConstraint(Axis axisConstraint)
{
	m_axisConstraint = axisConstraint;
	signalAxisConstraintChanged(m_axisConstraint);
}

void CLevelEditorSharedState::SetEditMode(EditMode editMode)
{
	m_editMode = editMode;

	AABB box(Vec3(0, 0, 0), Vec3(0, 0, 0));
	SetSelectedRegion(box);

	if (GetEditTool() && !GetEditTool()->IsNeedMoveTool())
	{
		SetEditTool(0, true);
	}

	SetCoordSystem(m_coordSystemForEditMode[static_cast<unsigned>(editMode)]);

	signalEditModeChanged(m_editMode);
}

void CLevelEditorSharedState::SetEditTool(CEditTool* tool, bool bStopCurrentTool)
{
	CViewport* pViewport = GetIEditor()->GetActiveView();
	if (pViewport)
	{
		pViewport->SetCurrentCursor(STD_CURSOR_DEFAULT);
	}

	if (tool == nullptr)
	{
		// Replace tool with the object modify edit tool.
		if (m_pEditTool && m_pEditTool->IsKindOf(RUNTIME_CLASS(CObjectMode)))
		{
			// Do not change.
			return;
		}
		else
		{
			tool = new CObjectMode;
		}
	}

	signalPreEditToolChanged();

	m_pEditTool = tool;
	m_pEditTool->Activate();

	// Make sure pick is aborted.
	if (tool != m_pPickTool)
	{
		m_pPickTool = 0;
	}
	signalEditToolChanged();
}

void CLevelEditorSharedState::SetEditTool(const string& sEditToolName, bool bStopCurrentTool)
{
	IClassDesc* pClass = GetIEditor()->GetClassFactory()->FindClass(sEditToolName);
	if (!pClass)
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Editor Tool %s not registered.", (const char*)sEditToolName);
		return;
	}
	if (pClass->SystemClassID() != ESYSTEM_CLASS_EDITTOOL)
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Class name %s is not a valid Edit Tool class.", (const char*)sEditToolName);
		return;
	}
	CRuntimeClass* pRtClass = pClass->GetRuntimeClass();
	if (pRtClass && pRtClass->IsDerivedFrom(RUNTIME_CLASS(CEditTool)))
	{
		CEditTool* pTool = GetEditTool();
		if (pTool)
		{
			// Check if already selected.
			if (pTool->GetRuntimeClass() == pRtClass)
				return;
		}

		CEditTool* pEditTool = (CEditTool*)pRtClass->CreateObject();
		SetEditTool(pEditTool);
		return;
	}
	else
	{
		CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Class name %s is not a valid Edit Tool class.", (const char*)sEditToolName);
		return;
	}
}

void CLevelEditorSharedState::PickObject(IPickObjectCallback* callback, CRuntimeClass* targetClass, bool bMultipick)
{
	m_pPickTool = new CPickObjectTool(callback, targetClass);
	m_pPickTool->SetMultiplePicks(bMultipick);
	SetEditTool(m_pPickTool);
}

void CLevelEditorSharedState::CancelPick()
{
	m_pPickTool = nullptr;
	SetEditTool(m_pPickTool);
}

bool CLevelEditorSharedState::IsPicking()
{
	if (GetEditTool() == m_pPickTool && m_pPickTool != 0)
		return true;
	return false;
}

void CLevelEditorSharedState::SetSelectedRegion(const AABB& box)
{
	m_selectedRegion = box;
}

const AABB& CLevelEditorSharedState::GetSelectedRegion()
{
	return m_selectedRegion;
}

CEditTool* CLevelEditorSharedState::GetEditTool()
{
	return m_pEditTool;
}

void CLevelEditorSharedState::SaveState()
{
	SET_PERSONALIZATION_PROPERTY(CLevelEditorSharedState, "AxisConstraint", static_cast<unsigned>(m_axisConstraint));
	SET_PERSONALIZATION_PROPERTY(CLevelEditorSharedState, "EditMode", static_cast<unsigned>(m_editMode));
	SET_PERSONALIZATION_PROPERTY(CLevelEditorSharedState, "ShowDisplayInfo", m_showDisplayInfo);
	SET_PERSONALIZATION_PROPERTY(CLevelEditorSharedState, "DisplayInfoLevel", m_displayInfoLevel);

	unsigned lastEditMode = static_cast<unsigned>(EditMode::LastEditMode);
	for (unsigned i = 0; i < lastEditMode; ++i)
	{
		SET_PERSONALIZATION_PROPERTY(CLevelEditorSharedState, QString("CoordSystem_%1").arg(i), static_cast<unsigned>(m_coordSystemForEditMode[i]));
	}
}
