// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "EditorCommonAPI.h"

// CryCommon
#include <CryCore/smartptr.h>
#include <CryMath/Cry_Geo.h>
#include <CrySandbox/CrySignal.h>

// EditorInterface
#include <IEditor.h>

// EditorCommon
#include "LevelEditor/Tools/EditTool.h"

class CPickObjectTool;
struct CRuntimeClass;

//! Callback class passed to PickObject.
struct IPickObjectCallback
{
	virtual ~IPickObjectCallback() {}
	//! Called when object picked.
	virtual void OnPick(CBaseObject* picked) = 0;
	//! Called when pick mode cancelled.
	virtual void OnCancelPick() = 0;
	//! Return true if specified object is pickable.
	virtual bool OnPickFilter(CBaseObject* filterObject) { return true; }
};

class EDITOR_COMMON_API CLevelEditorSharedState : public IAutoEditorNotifyListener
{
public:
	enum class EditMode
	{
		Select,
		SelectArea,
		Move,
		Rotate,
		Scale,
		Tool,
		LastEditMode
	};

	enum class Axis
	{
		None = 0x0,
		X    = 0x1,
		Y    = 0x2,
		Z    = 0x4,
		View = 0x8,
		XY   = X | Y,
		XZ   = X | Z,
		YZ   = Y | Z,
		XYZ  = X | Y | Z
	};

	enum eDisplayInfoLevel
	{
		eDisplayInfoLevel_Low  = 0,
		eDisplayInfoLevel_Med  = 1,
		eDisplayInfoLevel_High = 2
	};

	enum class CoordSystem
	{
		Local = 1,
		Parent,
		World,
		View,
		UserDefined,
		LastCoordSystem // Must always be last member
	};

	CLevelEditorSharedState();

	bool              IsDisplayInfoEnabled() const;
	eDisplayInfoLevel GetDisplayInfoLevel() const;

	void              ToggleDisplayInfo();
	void              SetDisplayInfoLevel(eDisplayInfoLevel level);

	void              SetCoordSystem(CoordSystem coordSystem);
	CoordSystem       GetCoordSystem() const;

	void              SetAxisConstraint(Axis axisConstraint);
	Axis              GetAxisConstraint() const { return m_axisConstraint; }

	void              SetEditMode(EditMode editMode);
	EditMode          GetEditMode() const { return m_editMode; }

	//! Assign current edit tool, destroy previously used edit tool
	virtual void       SetEditTool(CEditTool* tool, bool bStopCurrentTool = true);
	virtual void       SetEditTool(const string& sEditToolName, bool bStopCurrentTool = true);
	virtual CEditTool* GetEditTool();

	void               PickObject(IPickObjectCallback* callback, CRuntimeClass* targetClass = nullptr, bool bMultipick = false);
	void               CancelPick();
	bool               IsPicking();

	void               SetSelectedRegion(const AABB& box);
	const AABB&        GetSelectedRegion();

public:
	CCrySignal<void()>            signalShowDisplayInfoChanged;
	CCrySignal<void()>            signalDisplayInfoLevelChanged;

	CCrySignal<void(Axis)>        signalAxisConstraintChanged;
	CCrySignal<void(CoordSystem)> signalCoordSystemChanged;
	CCrySignal<void(EditMode)>    signalEditModeChanged;

	CCrySignal<void()>            signalPreEditToolChanged;
	CCrySignal<void()>            signalEditToolChanged;

private:
	void OnEditorNotifyEvent(EEditorNotifyEvent eventId);
	void InitActions();
	void ResetState();
	void SetDisplayInfoCVar();
	void SaveState();

	// TODO: We require a better system for linking state to action execution
	void UpdateDisplayInfoActions();
	void UpdateCoordSystemActions();
	void UpdateAxisConstraintActions();
	void UpdateEditModeActions();

	// Wrappers upon r_displayInfo
	bool                  m_showDisplayInfo;
	eDisplayInfoLevel     m_displayInfoLevel;

	Axis                  m_axisConstraint;
	EditMode              m_editMode;

	CoordSystem           m_coordSystemForEditMode[EditMode::LastEditMode];

	_smart_ptr<CEditTool> m_pEditTool;
	CPickObjectTool*      m_pPickTool;
	AABB                  m_selectedRegion;
};
