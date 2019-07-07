// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "EffectViewWidget.h"

#include "Models/EffectAssetModel.h"

namespace CryParticleEditor
{

CEffectAssetWidget::CEffectAssetWidget(CEffectAssetModel* pEffectAssetModel, QWidget* pParent)
	: QWidget(pParent)
	, m_pEffectAssetModel(pEffectAssetModel)
	, m_updated(false)
{
	CRY_ASSERT(m_pEffectAssetModel);

	QBoxLayout* pMainLayout = new QVBoxLayout();
	pMainLayout->setSpacing(0.0f);
	pMainLayout->setContentsMargins(0.0f, 0.0f, 0.0f, 0.0f);
	setContentsMargins(0.0f, 0.0f, 0.0f, 0.0f);

	m_pGraphView = new CGraphView();
	pMainLayout->addWidget(m_pGraphView);

	setLayout(pMainLayout);

	CRY_ASSERT(pEffectAssetModel);
	m_pEffectAssetModel->signalBeginEffectAssetChange.Connect(this, &CEffectAssetWidget::OnBeginEffectAssetChange);
	m_pEffectAssetModel->signalEndEffectAssetChange.Connect(this, &CEffectAssetWidget::OnEndEffectAssetChange);
	m_pEffectAssetModel->signalEffectEdited.Connect(this, &CEffectAssetWidget::OnEffectEdited);
	OnEndEffectAssetChange(); // Set current effect asset.

	// One-time scene fitting in the view.
	std::shared_ptr<QMetaObject::Connection> pConnection{ new QMetaObject::Connection };
	*pConnection = QObject::connect(m_pGraphView, &CGraphView::SignalItemsReloaded, [this, pConnection](auto& view)
	{
		if (!view.GetModel()->GetNodeItemCount())
		{
			return;
		}

		view.FitSceneInView();
		QObject::disconnect(*pConnection);
	});
}

const pfx2::IParticleEffect* CEffectAssetWidget::GetEffect() const
{
	return m_pEffectAssetModel->GetEffect();
}

pfx2::IParticleEffect* CEffectAssetWidget::GetEffect()
{
	return m_pEffectAssetModel->GetEffect();
}

const char* CEffectAssetWidget::GetName() const
{
	return m_pEffectAssetModel->GetName();
}

void CEffectAssetWidget::OnDeleteSelected()
{
	if (m_pGraphView)
	{
		m_pGraphView->DeleteSelectedItems();
	}
}

void CEffectAssetWidget::CopyComponents()
{
	if (m_pGraphView)
	{
		m_pGraphView->OnCopyEvent();
	}
}

void CEffectAssetWidget::OnPasteComponent()
{
	if (m_pGraphView)
	{
		m_pGraphView->OnPasteEvent();
	}
}

void CEffectAssetWidget::OnNewComponent()
{
	QAction* pAction = qobject_cast<QAction*>(sender());
	if (!pAction)
		return;

	QString templateFile = pAction->data().toString();
	if (templateFile.isEmpty())
		return;

	m_pEffectAssetModel->MakeNewComponent(templateFile.toStdString().c_str());
}

bool CEffectAssetWidget::MakeNewComponent(const char* szTemplateName)
{
	return m_pEffectAssetModel->MakeNewComponent(szTemplateName);
}

void CEffectAssetWidget::paintEvent(QPaintEvent* event)
{
	if (m_updated)
	{
		m_pGraphView->SetModel(nullptr);
		m_pEffectAssetModel->GetModel()->Refresh();
		m_pGraphView->SetModel(m_pEffectAssetModel->GetModel());
		m_updated = false;
	}

	QWidget::paintEvent(event);
}

void CEffectAssetWidget::OnBeginEffectAssetChange()
{
	m_pGraphView->SetModel(nullptr);
}

void CEffectAssetWidget::OnEndEffectAssetChange()
{
	m_pGraphView->SetModel(m_pEffectAssetModel->GetModel());
}

void CEffectAssetWidget::OnEffectEdited(int nComp, int nFeature)
{
	m_updated = true;
}

void CEffectAssetWidget::closeEvent(QCloseEvent*pEvent)
{
	pEvent->accept();

	m_pEffectAssetModel->signalBeginEffectAssetChange.DisconnectObject(this);
	m_pEffectAssetModel->signalEndEffectAssetChange.DisconnectObject(this);
}

}
