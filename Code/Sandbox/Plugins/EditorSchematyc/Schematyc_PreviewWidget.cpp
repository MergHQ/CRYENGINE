// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "Schematyc_PreviewWidget.h"

#include <QBoxLayout>
#include <QParentWndWidget.h>
#include <QPushButton>
#include <QSplitter>
#include <QViewport.h>
#include <QViewportConsumer.h>
#include <CryEntitySystem/IEntitySystem.h>
#include <CrySerialization/IArchiveHost.h>
#include <QPropertyTree/QPropertyTree.h>

#include "Objects/DisplayContext.h"
#include "QViewportEvents.h"
#include "IEditor.h"

namespace Schematyc
{
const Vec3 CPreviewWidget::ms_defaultOrbitTarget = Vec3(0.0f, 0.0f, 1.0f);
const float CPreviewWidget::ms_defaultOrbitRadius = 2.0f;

CPreviewWidget::CPreviewWidget(QWidget* pParent)
	: QWidget(pParent)
{
	m_pMainLayout = new QBoxLayout(QBoxLayout::TopToBottom);
	m_pSplitter = new QSplitter(Qt::Horizontal, this);
	m_pSettings = new QPropertyTree(this);
	m_pViewport = new QViewport(gEnv, this);
	m_pViewportAdapter.reset(new CDisplayViewportAdapter(m_pViewport));

	m_pControlLayout = new QBoxLayout(QBoxLayout::LeftToRight);
	m_pShowHideSettingsButton = new QPushButton(">> Show Settings", this);

	m_pSettings->setVisible(false);

	m_viewportSettings.rendering.fps = false;
	m_viewportSettings.grid.showGrid = true;
	m_viewportSettings.grid.spacing = 1.0f;
	m_viewportSettings.camera.showViewportOrientation = false;
	m_viewportSettings.camera.moveSpeed = 0.2f;

	connect(m_pViewport, SIGNAL(SignalRender(const SRenderContext &)), SLOT(OnRender(const SRenderContext &)));
	connect(m_pViewport, SIGNAL(SignalMouse(const SMouseEvent&)), SLOT(OnMouse(const SMouseEvent&)));
	connect(m_pShowHideSettingsButton, SIGNAL(clicked()), this, SLOT(OnShowHideSettingsButtonClicked()));
}

CPreviewWidget::~CPreviewWidget()
{
	SetClass(nullptr);

	SAFE_DELETE(m_pSettings);
}

void CPreviewWidget::InitLayout()
{
	QWidget::setLayout(m_pMainLayout);
	m_pMainLayout->setSpacing(1);
	m_pMainLayout->setMargin(0);
	m_pMainLayout->addWidget(m_pSplitter, 1);
	m_pMainLayout->addLayout(m_pControlLayout, 0);

	m_pSplitter->setStretchFactor(0, 1);
	m_pSplitter->setStretchFactor(1, 0);
	m_pSplitter->addWidget(m_pSettings);
	m_pSplitter->addWidget(m_pViewport);

	QList<int> splitterSizes;
	splitterSizes.push_back(60);
	splitterSizes.push_back(200);
	m_pSplitter->setSizes(splitterSizes);

	m_pSettings->setSizeHint(QSize(250, 400));
	m_pSettings->setExpandLevels(1);
	m_pSettings->setSliderUpdateDelay(5);
	m_pSettings->setValueColumnWidth(0.6f);
	m_pSettings->attach(Serialization::SStruct(*this));

	m_pViewport->SetSettings(m_viewportSettings);
	m_pViewport->SetSceneDimensions(Vec3(100.0f, 100.0f, 100.0f));

	ResetCamera(ms_defaultOrbitTarget, ms_defaultOrbitRadius);
	m_pViewport->SetOrbitMode(true);

	m_pControlLayout->setSpacing(2);
	m_pControlLayout->setMargin(4);
	m_pControlLayout->addWidget(m_pShowHideSettingsButton, 1);

	Update();
}

void CPreviewWidget::Update()
{
	if (m_pViewport)
	{
		m_pViewport->Update();
	}
}

void CPreviewWidget::SetClass(const IScriptClass* pScriptClass)
{
	if (pScriptClass != m_pScriptClass)
	{
		if (m_pObjectPreviewer)
		{
			m_pObjectPreviewer->DestroyObject(m_objectId);

			m_pScriptClass = nullptr;
			m_pObjectPreviewer = nullptr;
			m_objectId.Invalidate();

			m_pSettings->revert(); // Force re-serialization of settings.
		}

		if (pScriptClass)
		{
			IScriptViewPtr pScriptView = GetSchematycFramework().CreateScriptView(pScriptClass->GetGUID());
			const IEnvClass* pEnvClass = pScriptView->GetEnvClass();
			CRY_ASSERT(pEnvClass);
			if (pEnvClass)
			{
				m_pObjectPreviewer = pEnvClass->GetPreviewer();
				if (m_pObjectPreviewer)
				{
					m_pScriptClass = pScriptClass;

					Reset();

					Vec3 orbitTarget = ms_defaultOrbitTarget;
					float orbitRadius = ms_defaultOrbitRadius;

					const Sphere objetcBounds = m_pObjectPreviewer->GetObjectBounds(m_objectId);
					if (objetcBounds.radius > 0.001f)
					{
						orbitTarget = objetcBounds.center;
						orbitRadius = std::max(objetcBounds.radius * 1.25f, orbitRadius);
					}

					ResetCamera(orbitTarget, orbitRadius);
				}
			}
		}
	}
}

void CPreviewWidget::Reset()
{
	if (m_pObjectPreviewer)
	{
		if (m_objectId.IsValid())
		{
			m_pObjectPreviewer->DestroyObject(m_objectId);
		}
		m_objectId = m_pObjectPreviewer->CreateObject(m_pScriptClass->GetGUID());
	}

	m_pSettings->revert(); // Force re-serialization of settings.
}

void CPreviewWidget::LoadSettings(const XmlNodeRef& xml)
{
	Serialization::LoadXmlNode(m_viewportSettings, xml);
}

XmlNodeRef CPreviewWidget::SaveSettings(const char* szName)
{
	return Serialization::SaveXmlNode(m_viewportSettings, szName);
}

void CPreviewWidget::Serialize(Serialization::IArchive& archive)
{
	archive(m_viewportSettings, "viewportSettings", "Viewport Settings");
	if (archive.isInput())
	{
		m_pViewport->SetSettings(m_viewportSettings);
	}

	IObject* pObject = GetSchematycFramework().GetObject(m_objectId);
	if (pObject)
	{
		m_pObjectPreviewer->SerializeProperties(archive);

		std::set<IComponentPreviewer*> componentPreviewers;

		auto visitComponent = [&componentPreviewers](const CComponent& component) -> EVisitStatus
		{
			IComponentPreviewer* pComponentPreviewer = component.GetPreviewer();
			if (pComponentPreviewer)
			{
				componentPreviewers.insert(pComponentPreviewer);
			}
			return EVisitStatus::Continue;
		};
		pObject->VisitComponents(ObjectComponentConstVisitor::FromLambda(visitComponent));

		for (IComponentPreviewer* pComponentPreviewer : componentPreviewers)
		{
			pComponentPreviewer->SerializeProperties(archive); // #SchematycTODO : How do we avoid name collisions? Do we need to fully qualify component names?
		}
	}
}

void CPreviewWidget::OnRender(const SRenderContext& context)
{
	IObject* pObject = GetSchematycFramework().GetObject(m_objectId);
	if (pObject)
	{
		m_pObjectPreviewer->RenderObject(*pObject, *context.renderParams, *context.passInfo);

		auto visitComponent = [pObject, &context](const CComponent& component) -> EVisitStatus
		{
			const IComponentPreviewer* pComponentPreviewer = component.GetPreviewer();
			if (pComponentPreviewer)
			{
				pComponentPreviewer->Render(*pObject, component, *context.renderParams, *context.passInfo);
			}
			return EVisitStatus::Continue;
		};
		pObject->VisitComponents(ObjectComponentConstVisitor::FromLambda(visitComponent));
	}

	DisplayContext dc;
	dc.SetView(m_pViewportAdapter.get());

	m_gizmoManager.Display(dc);
}

// TODO: Either move it somewhere reusable or completely remove IEditor or QViewport style events
void CPreviewWidget::IEditorEventFromQViewportEvent(const SMouseEvent& qEvt, CPoint& p, EMouseEvent& evt, int& flags)
{
	p.x = qEvt.x;
	p.y = qEvt.y;
	flags = 0;

	if (qEvt.control)
	{
		flags |= MK_CONTROL;
	}
	if (qEvt.shift)
	{
		flags |= MK_SHIFT;
	}

	if (qEvt.type == SMouseEvent::MOVE)
	{
		evt = eMouseMove;
	}
	else if (qEvt.type == SMouseEvent::PRESS)
	{
		if (qEvt.button == SMouseEvent::BUTTON_LEFT)
		{
			evt = eMouseLDown;
		}
		else if (qEvt.button == SMouseEvent::BUTTON_RIGHT)
		{
			evt = eMouseRDown;
		}
		else if (qEvt.button == SMouseEvent::BUTTON_MIDDLE)
		{
			evt = eMouseMDown;
		}
	}
	else if (qEvt.type == SMouseEvent::RELEASE)
	{
		if (qEvt.button == SMouseEvent::BUTTON_LEFT)
		{
			evt = eMouseLUp;
		}
		else if (qEvt.button == SMouseEvent::BUTTON_RIGHT)
		{
			evt = eMouseRUp;
		}
		else if (qEvt.button == SMouseEvent::BUTTON_MIDDLE)
		{
			evt = eMouseMUp;
		}
	}
}

void CPreviewWidget::OnMouse(const SMouseEvent& qEvt)
{
	CPoint p;
	EMouseEvent evt;
	int flags;
	IEditorEventFromQViewportEvent(qEvt, p, evt, flags);

	m_gizmoManager.HandleMouseInput(m_pViewportAdapter.get(), evt, p, flags);
}

void CPreviewWidget::OnShowHideSettingsButtonClicked()
{
	const bool bShowSettings = !m_pSettings->isVisible();
	m_pShowHideSettingsButton->setText(bShowSettings ? "<< Hide Settings" : ">> Show Settings");
	m_pSettings->setVisible(bShowSettings);
}

void CPreviewWidget::ResetCamera(const Vec3& orbitTarget, float orbitRadius) const
{
	SViewportState viewportState = m_pViewport->GetState();
	viewportState.cameraTarget.t = Vec3(0.0f, -orbitRadius, orbitRadius * 0.5f);
	viewportState.orbitTarget = orbitTarget;
	viewportState.orbitRadius = orbitRadius;

	m_pViewport->SetState(viewportState);
	m_pViewport->LookAt(orbitTarget, orbitRadius * 2.0f, true);
}
} // Schematyc
