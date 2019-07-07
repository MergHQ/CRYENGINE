// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

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

	virtual const char* GetPaneTitle() const override { return "EffectComponents"; }
	void SetEffect(IParticleEffect* pEffect);

private:
	virtual void paintEvent(QPaintEvent* event) override;
	virtual void closeEvent(QCloseEvent*pEvent) override;

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
