// Copyright 2001-2018 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include "Models/ParticleNodeTreeModel.h"

#include <IEditor.h>
#include <QDrag>

#include <CryCore/Platform/platform.h>
#include <CrySerialization/IArchive.h>
#include <CryParticleSystem/IParticles.h>

#include <AssetSystem/AssetEditor.h>

#include <NodeGraph/ICryGraphEditor.h>
#include <NodeGraph/NodeGraphView.h>

class QContainer;

class QModelIndex;
class QDockWidget;
class QPropertyTree;
class QToolBar;

class CCurveEditorPanel;

namespace pfx2
{
struct IParticleEffect;
}

namespace CryParticleEditor
{

class CEffectAssetModel;
class CEffectAssetTabs;
class CEffectAssetWidget;

class CParticleEditor : public CAssetEditor, public IEditorNotifyListener
{
	Q_OBJECT

public:
	CParticleEditor();

	// CEditor
	virtual void        Initialize() override;
	virtual const char* GetEditorName() const override { return "Particle Editor"; }
	virtual void        SetLayout(const QVariantMap& state) override;
	virtual QVariantMap GetLayout() const override;
	// ~CEditor

	// IEditorNotifyListener
	virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) {}
	//~

	void Serialize(Serialization::IArchive& archive);

protected:
	// CAssetEditor
	virtual bool                                  OnOpenAsset(CAsset* pAsset) override;
	virtual bool                                  OnSaveAsset(CEditableAsset& editAsset) override;
	virtual void                                  OnDiscardAssetChanges(CEditableAsset& editAsset) override;
	virtual bool                                  OnAboutToCloseAsset(string& reason) const override;
	virtual void                                  OnCloseAsset() override;
	virtual std::unique_ptr<IAssetEditingSession> CreateEditingSession() override;
	virtual bool                                  AllowsInstantEditing() const override { return true; }
	// ~CAssetEditor

	// CEditor
	virtual void CreateDefaultLayout(CDockableContainer* pSender) override;
	// ~CEditor

	void AssignToEntity(CBaseObject* pObject, const string& newAssetName);
	bool AssetSaveDialog(string* pOutputName);

	void OnShowEffectOptions();

private:
	void         InitMenu();
	void         RegisterDockingWidgets();

	// CEditor
	bool OnReload();
	bool OnImport();

	virtual bool OnUndo() override;
	virtual bool OnRedo() override;
	// ~CEditor

protected Q_SLOTS:
	void OnLoadFromSelectedEntity();
	void OnApplyToSelectedEntity();

	void OnEffectOptionsChanged();

	void OnNewComponent();

private:
	std::unique_ptr<CEffectAssetModel> m_pEffectAssetModel;

	//
	CInspectorLegacy* m_pInspector;

	QAction*          m_pReloadEffectMenuAction;
	QAction*          m_pShowEffectOptionsMenuAction;
};

}
