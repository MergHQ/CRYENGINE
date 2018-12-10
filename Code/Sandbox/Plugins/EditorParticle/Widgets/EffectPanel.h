// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#include <QWidget>

class QListWidget;
class QTreeWidget;
class QTreeWidgetItem;

namespace CryParticleEditor {

using namespace pfx2;
using IParticleEffect = pfx2::IParticleEffect;
class CEffectAssetModel;
class CComponentTree;
class CFeatureList;

class CEffectPanel : public CDockableWidget
{
	Q_OBJECT

public:
	CEffectPanel(CEffectAssetModel* pModel, QWidget* pParent = nullptr);

	virtual ~CEffectPanel();

	// CDockableWidget
	virtual const char* GetPaneTitle() const override { return "EffectComponents"; }
	virtual void paintEvent(QPaintEvent* event) override;

	// CEffectPanel
	void SetEffect(IParticleEffect* pEffect);

private:
	void onBeginEffectAssetChange();
	void onEndEffectAssetChange();
	void onEffectChanged();
	void onComponentChanged(IParticleComponent* pComp);

private:
	CEffectAssetModel* m_pEffectAssetModel;
	CComponentTree*    m_pComponentTree;
	CFeatureList*      m_pFeatureList;
	int                m_nComponent = -1;
	bool               m_updated = false;
};

}
