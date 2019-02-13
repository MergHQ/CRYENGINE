// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <AssetSystem/AssetEditor.h>
#include <IEditor.h>
#include <QtViewPane.h>

#include <Cry3DEngine/ITimeOfDay.h>

class CEditorEnvironmentWindow : public CAssetEditor
{
	Q_OBJECT
public:

	CEditorEnvironmentWindow();

	virtual bool                                  AllowsInstantEditing() const override { return true; }
	virtual std::unique_ptr<IAssetEditingSession> CreateEditingSession() override;

	ITimeOfDay::IPreset*                          GetPreset();

protected:
	virtual bool OnSaveAsset(CEditableAsset& editAsset) override;
	virtual bool OnOpenAsset(CAsset* pAsset) override;
	virtual void OnCloseAsset() override;
	virtual void OnDiscardAssetChanges(CEditableAsset& editAsset) override;

private:
	const char*  GetEditorName() const override { return "Environment Editor"; }
	void         customEvent(QEvent* event) override;
	QWidget*     CreateToolbar();

	void         RegisterDockingWidgets();
	virtual void CreateDefaultLayout(CDockableContainer* pSender) override;

private:
	ITimeOfDay::IPreset* m_pPreset;
};
