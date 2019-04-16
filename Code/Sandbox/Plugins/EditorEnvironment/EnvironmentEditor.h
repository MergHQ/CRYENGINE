// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Controller.h"
#include <AssetSystem/AssetEditor.h>

#include <Cry3DEngine/ITimeOfDay.h>

class CEnvironmentEditor : public CAssetEditor
{
	Q_OBJECT
public:
	CEnvironmentEditor();

	ITimeOfDay::IPreset* GetPreset() const;
	string               GetPresetFileName() const;
	static ITimeOfDay*   GetTimeOfDay();

private:
	virtual const char*                           GetEditorName() const override        { return "Environment Editor"; }
	virtual bool                                  AllowsInstantEditing() const override { return true; }

	virtual std::unique_ptr<IAssetEditingSession> CreateEditingSession() override;
	virtual bool                                  OnOpenAsset(CAsset* pAsset) override;
	virtual void                                  OnCloseAsset() override;
	virtual void                                  OnDiscardAssetChanges(CEditableAsset& editAsset) override;

	virtual void                                  CreateDefaultLayout(CDockableContainer* pSender) override;
	void                                          RegisterDockingWidgets();

	virtual bool                                  event(QEvent* event) override;

private:
	ITimeOfDay::IPreset* m_pPreset;
	string               m_presetFileName;
	CController          m_controller;
};
